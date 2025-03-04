#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <iomanip>
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <future>
#include <atomic>
#include <numeric>
#include <climits>
#include "chess_rl.h"

// Configuration parameters
const int NUM_GENERATIONS = 100;         // Number of generations to run
const int POPULATION_SIZE = 5;           // Number of agents in population
const int GAMES_PER_MATCHUP = 2;         // Games per matchup (for White/Black balance)
const int ELITES_TO_KEEP = 2;            // Top agents to preserve unchanged
const int TRAINING_EPISODES_PER_GEN = 50; // Self-play episodes per agent per generation
const int MAX_MOVES_PER_GAME = 200;      // Maximum moves per game
const float MUTATION_RATE = 0.05f;       // Rate of mutation for new generations

// Multithreading parameters
const int MAX_THREADS = std::thread::hardware_concurrency(); // Use all available cores

// Result structure without mutex (can be copied)
struct TrainingStatsResult {
	int totalGames = 0;
	int whiteWins = 0;
	int blackWins = 0;
	int draws = 0;
	int truncated = 0;
	int totalMoves = 0;
	int minMoves = INT_MAX;
	int maxMoves = 0;
	float avgMaterialBalance = 0.0f;
};

// Structure to hold training statistics with thread-safe updates
struct TrainingStats {
	int totalGames = 0;
	int whiteWins = 0;
	int blackWins = 0;
	int draws = 0;
	int truncated = 0;
	int totalMoves = 0;
	int minMoves = INT_MAX;
	int maxMoves = 0;
	float avgMaterialBalance = 0.0f;
	std::mutex mutex;

	void updateGameStats(int moves, bool isWhiteWin, bool isBlackWin, bool isDraw,
		bool isTruncated, float materialBalance) {
		std::lock_guard<std::mutex> lock(mutex);
		totalGames++;
		totalMoves += moves;
		minMoves = std::min(minMoves, moves);
		maxMoves = std::max(maxMoves, moves);

		if (isWhiteWin) whiteWins++;
		if (isBlackWin) blackWins++;
		if (isDraw) draws++;
		if (isTruncated) truncated++;

		avgMaterialBalance = ((avgMaterialBalance * (totalGames - 1)) + materialBalance) / totalGames;
	}

	// Method to get copyable results
	TrainingStatsResult getResults() const {
		TrainingStatsResult result;
		result.totalGames = totalGames;
		result.whiteWins = whiteWins;
		result.blackWins = blackWins;
		result.draws = draws;
		result.truncated = truncated;
		result.totalMoves = totalMoves;
		result.minMoves = minMoves;
		result.maxMoves = maxMoves;
		result.avgMaterialBalance = avgMaterialBalance;
		return result;
	}
};

// Tournament results structure
struct TournamentResult {
	std::vector<std::vector<float>> scoreMatrix; // [player][opponent] = score
	std::vector<float> totalScores;              // Sum of scores for each player
	std::vector<size_t> rankings;                // Indices of players ranked best to worst
};

// Structure to store a tournament matchup result
struct MatchupResult {
	size_t agent1;
	size_t agent2;
	float score;
};

// Function to play a single self-play episode and update stats
void playSelfPlayEpisode(ChessRLAgent& agent, TrainingStats& stats, int episodeNum,
	std::mutex& outputMutex) {
	Board board;
	board.reset();

	std::vector<GameState> gameHistory;
	int moveCount = 0;

	// Play the game
	while (!board.isCheckmate() && !board.isStalemate() &&
		!board.isInsufficientMaterial() && board.halfmoveClock() < 100 &&
		moveCount < MAX_MOVES_PER_GAME) {

		std::vector<Move> legalMoves = board.generateLegalMoves();
		if (legalMoves.empty()) break;

		GameState state;
		state.board = board;
		state.features = BoardFeatureExtractor::extractFeatures(board);

		Move selectedMove = agent.selectMove(board, legalMoves);
		state.chosenMove = selectedMove;

		board.makeMove(selectedMove);
		moveCount++;

		state.reward = agent.calculateReward(board, state.board.sideToMove());
		gameHistory.push_back(state);
	}

	// Determine game outcome
	float finalReward = 0.0f;
	float materialBalance = agent.calculateReward(board, WHITE) * 100.0f;

	bool isWhiteWin = false;
	bool isBlackWin = false;
	bool isDraw = false;
	bool isTruncated = false;

	if (board.isCheckmate()) {
		finalReward = 1.0f;
		isWhiteWin = (board.sideToMove() == BLACK);
		isBlackWin = !isWhiteWin;
	}
	else if (board.isStalemate() || board.isInsufficientMaterial() || board.halfmoveClock() >= 100) {
		finalReward = 0.0f;
		isDraw = true;
	}
	else {
		finalReward = materialBalance / 100.0f;
		isTruncated = true;
	}

	// Update statistics
	stats.updateGameStats(moveCount, isWhiteWin, isBlackWin, isDraw, isTruncated, materialBalance);

	// Log progress periodically
	if (episodeNum % 10 == 0 || episodeNum == TRAINING_EPISODES_PER_GEN) {
		std::lock_guard<std::mutex> lock(outputMutex);
		std::cout << "  Episode " << std::setw(4) << episodeNum << ": ";
		std::cout << std::setw(3) << moveCount << " moves, ";

		if (board.isCheckmate()) {
			std::cout << "Checkmate (" << (board.sideToMove() == BLACK ? "White" : "Black") << " wins)";
		}
		else if (board.isStalemate()) {
			std::cout << "Stalemate";
		}
		else if (board.isInsufficientMaterial()) {
			std::cout << "Insufficient material";
		}
		else if (board.halfmoveClock() >= 100) {
			std::cout << "50-move rule";
		}
		else {
			std::cout << "Truncated (balance: " << std::fixed << std::setprecision(2)
				<< materialBalance << ")";
		}
		std::cout << std::endl;
	}

	// Record transitions and train
	for (size_t i = 0; i < gameHistory.size(); i++) {
		float reward = (i == gameHistory.size() - 1) ? finalReward : 0.0f;
		if (gameHistory[i].board.sideToMove() == BLACK) {
			reward = -reward;
		}
		agent.recordTransition(gameHistory[i].board, gameHistory[i].chosenMove, reward);
	}

	agent.train(std::min(gameHistory.size(), size_t(32)));
	agent.decayExplorationRate();
}

// Function for self-play training of an agent with multithreading
// Return copyable result instead of TrainingStats with mutex
TrainingStatsResult trainAgentViaSelfPlay(ChessRLAgent& agent, int episodes) {
	TrainingStats stats;
	std::mutex outputMutex;

	// Determine batch size for threading
	int numThreads = std::min(MAX_THREADS, episodes);
	int batchSize = episodes / numThreads;
	int remainder = episodes % numThreads;

	std::vector<std::future<void>> futures;

	// Launch worker threads
	for (int t = 0; t < numThreads; t++) {
		int start = t * batchSize + 1;
		int end = (t + 1) * batchSize;
		if (t < remainder) end++;

		futures.push_back(std::async(std::launch::async, [&, start, end]() {
			for (int episode = start; episode <= end; episode++) {
				playSelfPlayEpisode(agent, stats, episode, outputMutex);
			}
			}));
	}

	// Wait for all threads to complete
	for (auto& future : futures) {
		future.wait();
	}

	return stats.getResults();
}

// Function to play a game between two agents (thread-safe)
float playGame(ChessRLAgent& whiteAgent, ChessRLAgent& blackAgent) {
	Board board;
	board.reset();

	int moveCount = 0;

	// Play the game
	while (!board.isCheckmate() && !board.isStalemate() &&
		!board.isInsufficientMaterial() && board.halfmoveClock() < 100 &&
		moveCount < MAX_MOVES_PER_GAME) {

		std::vector<Move> legalMoves = board.generateLegalMoves();
		if (legalMoves.empty()) break;

		Move selectedMove;
		if (board.sideToMove() == WHITE) {
			selectedMove = whiteAgent.selectMove(board, legalMoves);
		}
		else {
			selectedMove = blackAgent.selectMove(board, legalMoves);
		}

		board.makeMove(selectedMove);
		moveCount++;
	}

	// Determine game outcome
	float result = 0.5f; // Default to draw

	if (board.isCheckmate()) {
		result = (board.sideToMove() == BLACK) ? 1.0f : 0.0f; // 1 for white win, 0 for black win
	}
	else if (board.isStalemate() || board.isInsufficientMaterial() || board.halfmoveClock() >= 100) {
		result = 0.5f; // Draw
	}
	else {
		// Evaluate final position based on material balance
		float materialBalance = whiteAgent.calculateReward(board, WHITE) * 100.0f;
		if (materialBalance > 0.5f) {
			result = 0.6f; // Slight advantage to white
		}
		else if (materialBalance < -0.5f) {
			result = 0.4f; // Slight advantage to black
		}
		else {
			result = 0.5f; // Equal position
		}
	}

	return result; // Return the result from white's perspective
}

// Function to play a matchup between two agents (multithreaded worker)
MatchupResult playMatchup(std::vector<std::unique_ptr<ChessRLAgent>>& agents,
	size_t agent1Idx, size_t agent2Idx) {
	float totalScore = 0.0f;

	// Play multiple games per matchup to reduce variance
	for (int game = 0; game < GAMES_PER_MATCHUP; game++) {
		// Alternate who plays white
		ChessRLAgent& whiteAgent = (game % 2 == 0) ? *agents[agent1Idx] : *agents[agent2Idx];
		ChessRLAgent& blackAgent = (game % 2 == 0) ? *agents[agent2Idx] : *agents[agent1Idx];

		float result = playGame(whiteAgent, blackAgent);

		// Adjust score based on who played white
		if (game % 2 == 0) {
			totalScore += result;
		}
		else {
			totalScore += (1.0f - result);
		}
	}

	return { agent1Idx, agent2Idx, totalScore / GAMES_PER_MATCHUP };
}

// Function to run a tournament between multiple agents with multithreading
TournamentResult runTournament(std::vector<std::unique_ptr<ChessRLAgent>>& agents) {
	const size_t numAgents = agents.size();

	// Initialize score matrix
	std::vector<std::vector<float>> scoreMatrix(numAgents, std::vector<float>(numAgents, 0.0f));

	// Create a list of all matchups
	std::vector<std::pair<size_t, size_t>> matchups;
	for (size_t i = 0; i < numAgents; i++) {
		for (size_t j = 0; j < numAgents; j++) {
			if (i != j) {
				matchups.push_back({ i, j });
			}
		}
	}

	// Futures for each matchup
	std::vector<std::future<MatchupResult>> futures;

	// Launch a thread for each matchup
	for (const auto& matchup : matchups) {
		futures.push_back(std::async(std::launch::async,
			[&agents, matchup]() {
				return playMatchup(agents, matchup.first, matchup.second);
			}
		));
	}

	// Collect results
	for (auto& future : futures) {
		MatchupResult result = future.get();
		scoreMatrix[result.agent1][result.agent2] = result.score;
	}

	// Calculate total scores and rankings
	std::vector<float> totalScores(numAgents, 0.0f);
	for (size_t i = 0; i < numAgents; i++) {
		for (size_t j = 0; j < numAgents; j++) {
			if (i != j) {
				totalScores[i] += scoreMatrix[i][j];
			}
		}
	}

	std::vector<size_t> rankings(numAgents);
	std::iota(rankings.begin(), rankings.end(), 0);
	std::sort(rankings.begin(), rankings.end(),
		[&totalScores](size_t a, size_t b) { return totalScores[a] > totalScores[b]; });

	return { scoreMatrix, totalScores, rankings };
}

// Thread-safe function to create a child agent from two parents
std::unique_ptr<ChessRLAgent> createChildAgent(const ChessRLAgent& parent1, const ChessRLAgent& parent2,
	int childId, float mutationRate) {
	static std::mutex fileMutex;
	std::lock_guard<std::mutex> lock(fileMutex);

	// Create temporary files with unique names
	std::string parent1File = "temp_parent1_" + std::to_string(childId) + ".bin";
	std::string parent2File = "temp_parent2_" + std::to_string(childId) + ".bin";
	std::string childFile = "temp_child_" + std::to_string(childId) + ".bin";

	// Save parents to files
	const_cast<ChessRLAgent&>(parent1).save(parent1File);
	const_cast<ChessRLAgent&>(parent2).save(parent2File);

	// Create a new agent
	auto child = std::make_unique<ChessRLAgent>();

	// Randomly choose which parent to inherit from
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);

	if (dist(gen) < 0.5f) {
		child->load(parent1File);
	}
	else {
		child->load(parent2File);
	}

	// Apply mutation
	if (dist(gen) < mutationRate) {
		child->save(childFile);

		child = std::make_unique<ChessRLAgent>(
			0.1f + dist(gen) * 0.2f,      // Exploration rate
			0.001f + dist(gen) * 0.009f,  // Learning rate
			0.95f + dist(gen) * 0.04f     // Discount factor
		);

		child->load(childFile);
	}

	// Clean up files
	std::remove(parent1File.c_str());
	std::remove(parent2File.c_str());
	std::remove(childFile.c_str());

	return child;
}

// Function to create a child in a container (to avoid returning unique_ptr from async)
void createChildAgentIntoVector(
	std::vector<std::unique_ptr<ChessRLAgent>>& container,
	std::mutex& containerMutex,
	const ChessRLAgent& parent1,
	const ChessRLAgent& parent2,
	int childId,
	float mutationRate) {

	auto child = createChildAgent(parent1, parent2, childId, mutationRate);

	// Add to container with thread safety
	std::lock_guard<std::mutex> lock(containerMutex);
	container.push_back(std::move(child));
}

int main() {
	auto startTime = std::chrono::high_resolution_clock::now();

	std::cout << "Starting Chess RL Tournament Training with " << MAX_THREADS << " threads" << std::endl;

	// Initialize population
	std::vector<std::unique_ptr<ChessRLAgent>> population;

	// Create initial population
	for (int i = 0; i < POPULATION_SIZE; i++) {
		std::cout << "Initializing agent " << i + 1 << "/" << POPULATION_SIZE << std::endl;
		auto agent = std::make_unique<ChessRLAgent>();

		// Try to load existing model for first agent
		if (i == 0 && agent->load("chess_rl_model.bin")) {
			std::cout << "Loaded existing model for agent 1" << std::endl;
		}
		else {
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_real_distribution<float> dist(0.0f, 1.0f);

			float epsilon = 0.05f + dist(gen) * 0.15f;
			float alpha = 0.0005f + dist(gen) * 0.0015f;
			float gamma = 0.95f + dist(gen) * 0.04f;

			agent = std::make_unique<ChessRLAgent>(epsilon, alpha, gamma);
			std::cout << "Created new agent " << i + 1 << std::endl;
		}

		population.push_back(std::move(agent));
	}

	// Track progress across generations
	std::vector<float> bestScores;
	std::vector<size_t> bestAgents;

	// Evolution loop
	for (int generation = 1; generation <= NUM_GENERATIONS; generation++) {
		auto genStartTime = std::chrono::high_resolution_clock::now();

		std::cout << "\n=== Generation " << generation << "/" << NUM_GENERATIONS << " ===" << std::endl;

		// Train all agents in parallel
		std::vector<std::future<TrainingStatsResult>> trainingFutures;

		for (int i = 0; i < POPULATION_SIZE; i++) {
			std::cout << "Training agent " << i + 1 << " via self-play..." << std::endl;

			trainingFutures.push_back(std::async(std::launch::async,
				[&population, i]() {
					return trainAgentViaSelfPlay(*population[i], TRAINING_EPISODES_PER_GEN);
				}
			));
		}

		// Collect training results
		for (int i = 0; i < POPULATION_SIZE; i++) {
			TrainingStatsResult stats = trainingFutures[i].get();

			// Print training summary
			std::cout << "Agent " << i + 1 << " training: "
				<< stats.whiteWins << "W/" << stats.blackWins << "B/"
				<< stats.draws << "D/" << stats.truncated << "T, "
				<< "Avg moves: " << (stats.totalGames > 0 ? static_cast<float>(stats.totalMoves) / stats.totalGames : 0)
				<< std::endl;
		}

		// Run tournament
		std::cout << "Running tournament..." << std::endl;
		TournamentResult results = runTournament(population);

		// Print tournament summary
		std::cout << "Tournament rankings:" << std::endl;
		for (size_t i = 0; i < results.rankings.size(); i++) {
			size_t agentIdx = results.rankings[i];
			float winRate = results.totalScores[agentIdx] / (POPULATION_SIZE - 1) * 100;
			std::cout << i + 1 << ". Agent " << agentIdx + 1
				<< " (Score: " << std::fixed << std::setprecision(2) << results.totalScores[agentIdx]
				<< ", Win rate: " << winRate << "%)" << std::endl;
		}

		// Save best agent
		size_t bestAgentIdx = results.rankings[0];
		float bestScore = results.totalScores[bestAgentIdx];

		bestScores.push_back(bestScore);
		bestAgents.push_back(bestAgentIdx);

		// Save model files
		std::string genModelFile = "chess_rl_model_gen" + std::to_string(generation) + ".bin";
		population[bestAgentIdx]->save(genModelFile);

		if (generation % 5 == 0 || generation == NUM_GENERATIONS) {
			population[bestAgentIdx]->save("chess_rl_model.bin");
		}

		// Skip evolution on final generation
		if (generation == NUM_GENERATIONS) break;

		// Create next generation
		std::vector<std::unique_ptr<ChessRLAgent>> nextGeneration;

		// Keep elite agents
		for (int i = 0; i < ELITES_TO_KEEP && i < POPULATION_SIZE; i++) {
			size_t eliteIdx = results.rankings[i];
			std::cout << "Keeping elite agent " << eliteIdx + 1 << std::endl;

			// Copy elite agents
			auto eliteCopy = std::make_unique<ChessRLAgent>();
			std::string tempFile = "temp_elite_" + std::to_string(i) + ".bin";
			population[eliteIdx]->save(tempFile);
			eliteCopy->load(tempFile);
			std::remove(tempFile.c_str());

			nextGeneration.push_back(std::move(eliteCopy));
		}

		// Create children using a different approach to avoid "function returns function" error
		std::vector<std::unique_ptr<ChessRLAgent>> childAgents;
		std::mutex childAgentsMutex;
		std::vector<std::future<void>> childFutures;
		int childCounter = 0;

		while (nextGeneration.size() + childFutures.size() < POPULATION_SIZE) {
			// Select parents from top half
			std::uniform_int_distribution<size_t> parentDist(0, POPULATION_SIZE / 2);
			size_t parent1Rank = parentDist(std::mt19937(std::random_device()()));
			size_t parent2Rank = parentDist(std::mt19937(std::random_device()()));

			// Ensure we don't use the same parent twice
			while (parent2Rank == parent1Rank) {
				parent2Rank = parentDist(std::mt19937(std::random_device()()));
			}

			size_t parent1Idx = results.rankings[parent1Rank];
			size_t parent2Idx = results.rankings[parent2Rank];

			// Launch task to create child and add to container
			childFutures.push_back(std::async(std::launch::async,
				[&childAgents, &childAgentsMutex, &population, parent1Idx, parent2Idx, childCounter]() {
					createChildAgentIntoVector(
						childAgents,
						childAgentsMutex,
						*population[parent1Idx],
						*population[parent2Idx],
						childCounter,
						MUTATION_RATE
					);
				}
			));

			childCounter++;
		}

		// Wait for all child creation tasks to complete
		for (auto& future : childFutures) {
			future.wait();
		}

		// Add all children to next generation
		for (auto& child : childAgents) {
			nextGeneration.push_back(std::move(child));
		}

		// Replace population
		population = std::move(nextGeneration);

		// Print timing
		auto genEndTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> genElapsed = genEndTime - genStartTime;
		std::cout << "Generation " << generation << " completed in "
			<< std::fixed << std::setprecision(2) << genElapsed.count()
			<< " seconds" << std::endl;
	}

	// Print final summary
	std::cout << "\n=== Training Complete ===" << std::endl;

	// Print total time
	auto endTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = endTime - startTime;
	std::cout << "Total training time: " << std::fixed << std::setprecision(2)
		<< elapsed.count() << " seconds" << std::endl;

	// Calculate improvement
	if (bestScores.size() >= 2) {
		float firstScore = bestScores[0];
		float lastScore = bestScores.back();
		float improvement = ((lastScore - firstScore) / firstScore) * 100;

		std::cout << "Overall improvement: " << std::fixed << std::setprecision(2)
			<< improvement << "%" << std::endl;
	}

	std::cout << "Best model saved as chess_rl_model.bin" << std::endl;

	return 0;
}