#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <iomanip>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>
#include <numeric>
#include "chess_rl.h"

// Configuration parameters
const int NUM_GENERATIONS = 100;         // Number of generations/iterations to run
const int POPULATION_SIZE = 5;          // Number of different agents in the population
const int GAMES_PER_MATCHUP = 2;        // Number of games per matchup (even number for White/Black balance)
const int ELITES_TO_KEEP = 2;           // Number of top agents to preserve unchanged
const int TRAINING_EPISODES_PER_GEN = 50; // Self-play training episodes per agent per generation
const int MAX_MOVES_PER_GAME = 200;     // Maximum moves per game to prevent infinite games
const float MUTATION_RATE = 0.05f;      // Rate of mutation for new generations

// Tournament results structure
struct TournamentResult {
	std::vector<std::vector<float>> scoreMatrix; // [player][opponent] = score (1=win, 0.5=draw, 0=loss)
	std::vector<float> totalScores;              // Sum of scores for each player
	std::vector<size_t> rankings;                // Indices of players ranked from best to worst
};

// Function to play a game between two agents
float playGame(ChessRLAgent& whiteAgent, ChessRLAgent& blackAgent, bool verbose = false) {
	Board board;
	board.reset(); // Start from the initial position

	int moveCount = 0;

	// Play the game until it's over or max moves reached
	while (!board.isCheckmate() && !board.isStalemate() &&
		!board.isInsufficientMaterial() && board.halfmoveClock() < 100 &&
		moveCount < MAX_MOVES_PER_GAME) {

		// Generate legal moves
		std::vector<Move> legalMoves = board.generateLegalMoves();

		// If no legal moves, the game is over
		if (legalMoves.empty()) break;

		// Select a move using the appropriate agent based on side to move
		Move selectedMove;
		if (board.sideToMove() == WHITE) {
			selectedMove = whiteAgent.selectMove(board, legalMoves);
		}
		else {
			selectedMove = blackAgent.selectMove(board, legalMoves);
		}

		// Make the move
		board.makeMove(selectedMove);
		moveCount++;
	}

	// Determine game outcome
	float result = 0.5f; // Default to draw

	if (board.isCheckmate()) {
		// The side that isn't to move has won
		result = (board.sideToMove() == BLACK) ? 1.0f : 0.0f; // 1 for white win, 0 for black win
		if (verbose) {
			std::cout << "Checkmate! " << (board.sideToMove() == BLACK ? "White" : "Black") << " wins." << std::endl;
		}
	}
	else if (board.isStalemate() || board.isInsufficientMaterial() || board.halfmoveClock() >= 100) {
		result = 0.5f; // Draw
		if (verbose) {
			std::cout << "Draw!" << std::endl;
		}
	}
	else {
		// Game truncated due to move limit
		if (verbose) {
			std::cout << "Game truncated after " << moveCount << " moves." << std::endl;
		}

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

// Function to run a tournament between multiple agents
TournamentResult runTournament(std::vector<std::unique_ptr<ChessRLAgent>>& agents, bool verbose = false) {
	const size_t numAgents = agents.size();

	// Initialize score matrix
	std::vector<std::vector<float>> scoreMatrix(numAgents, std::vector<float>(numAgents, 0.0f));

	// Run all matchups
	for (size_t i = 0; i < numAgents; i++) {
		for (size_t j = 0; j < numAgents; j++) {
			if (i == j) continue; // Skip self-play

			float totalScore = 0.0f;

			// Play multiple games per matchup to reduce variance
			for (int game = 0; game < GAMES_PER_MATCHUP; game++) {
				// Alternate who plays white
				ChessRLAgent& whiteAgent = (game % 2 == 0) ? *agents[i] : *agents[j];
				ChessRLAgent& blackAgent = (game % 2 == 0) ? *agents[j] : *agents[i];

				float result = playGame(whiteAgent, blackAgent, verbose);

				// Adjust score based on who played white
				if (game % 2 == 0) {
					totalScore += result;
				}
				else {
					totalScore += (1.0f - result);
				}
			}

			// Average score for this matchup
			scoreMatrix[i][j] = totalScore / GAMES_PER_MATCHUP;
		}
	}

	// Calculate total scores for each agent
	std::vector<float> totalScores(numAgents, 0.0f);
	for (size_t i = 0; i < numAgents; i++) {
		for (size_t j = 0; j < numAgents; j++) {
			if (i != j) {
				totalScores[i] += scoreMatrix[i][j];
			}
		}
	}

	// Determine rankings
	std::vector<size_t> rankings(numAgents);
	std::iota(rankings.begin(), rankings.end(), 0); // Fill with 0, 1, 2, ...
	std::sort(rankings.begin(), rankings.end(),
		[&totalScores](size_t a, size_t b) { return totalScores[a] > totalScores[b]; });

	return { scoreMatrix, totalScores, rankings };
}

// Structure to hold training statistics
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
};

// Function for self-play training of an agent
TrainingStats trainAgentViaSelfPlay(ChessRLAgent& agent, int episodes) {
	TrainingStats stats;
	float cumulativeMaterialBalance = 0.0f;

	for (int episode = 1; episode <= episodes; episode++) {
		Board board;
		board.reset();

		std::vector<GameState> gameHistory;
		int moveCount = 0;

		// Play the game until it's over or max moves reached
		while (!board.isCheckmate() && !board.isStalemate() &&
			!board.isInsufficientMaterial() && board.halfmoveClock() < 100 &&
			moveCount < MAX_MOVES_PER_GAME) {

			// Generate legal moves
			std::vector<Move> legalMoves = board.generateLegalMoves();

			// If no legal moves, the game is over
			if (legalMoves.empty()) break;

			// Record the current state
			GameState state;
			state.board = board;
			state.features = BoardFeatureExtractor::extractFeatures(board);

			// Select a move using the RL agent
			Move selectedMove = agent.selectMove(board, legalMoves);
			state.chosenMove = selectedMove;

			// Make the move
			board.makeMove(selectedMove);
			moveCount++;

			// Calculate reward
			state.reward = agent.calculateReward(board, state.board.sideToMove());

			// Add to game history
			gameHistory.push_back(state);
		}

		// Update move statistics
		stats.totalGames++;
		stats.totalMoves += moveCount;
		stats.minMoves = std::min(stats.minMoves, moveCount);
		stats.maxMoves = std::max(stats.maxMoves, moveCount);

		// Determine game outcome and update stats
		float finalReward = 0.0f;
		float materialBalance = agent.calculateReward(board, WHITE) * 100.0f;
		cumulativeMaterialBalance += materialBalance;

		if (board.isCheckmate()) {
			// The side that isn't to move has won
			finalReward = 1.0f;
			if (board.sideToMove() == BLACK) {
				stats.whiteWins++;
			}
			else {
				stats.blackWins++;
			}
		}
		else if (board.isStalemate() || board.isInsufficientMaterial() || board.halfmoveClock() >= 100) {
			finalReward = 0.0f;
			stats.draws++;
		}
		else {
			// Game truncated due to move limit
			finalReward = materialBalance / 100.0f; // Small reward based on material
			stats.truncated++;
		}

		// Log progress periodically
		if (episode % 10 == 0 || episode == episodes) {
			std::cout << "  Episode " << std::setw(4) << episode << ": ";
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
				std::cout << "Truncated (material balance: " << std::fixed << std::setprecision(2)
					<< materialBalance << ")";
			}
			std::cout << std::endl;
		}

		// Record transitions for each state in the game
		for (size_t i = 0; i < gameHistory.size(); i++) {
			// The reward for the last state is the final reward
			float reward = (i == gameHistory.size() - 1) ? finalReward : 0.0f;

			// Flip the reward sign for black's moves
			if (gameHistory[i].board.sideToMove() == BLACK) {
				reward = -reward;
			}

			// Record the transition
			agent.recordTransition(gameHistory[i].board, gameHistory[i].chosenMove, reward);
		}

		// Train the agent
		agent.train(std::min(gameHistory.size(), size_t(32)));

		// Decay exploration rate
		agent.decayExplorationRate();
	}

	// Calculate average material balance
	stats.avgMaterialBalance = cumulativeMaterialBalance / stats.totalGames;

	return stats;
}

// Function to create a child agent from two parents with potential mutation
std::unique_ptr<ChessRLAgent> createChildAgent(const ChessRLAgent& parent1, const ChessRLAgent& parent2,
	int childId, float mutationRate) {
	// Create a temporary file for each parent
	std::string parent1File = "temp_parent1.bin";
	std::string parent2File = "temp_parent2.bin";
	std::string childFile = "temp_child.bin";

	// Save parents to files
	const_cast<ChessRLAgent&>(parent1).save(parent1File);
	const_cast<ChessRLAgent&>(parent2).save(parent2File);

	// Create a new agent
	auto child = std::make_unique<ChessRLAgent>();

	// Randomly choose which parent to inherit from initially
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);

	if (dist(gen) < 0.5f) {
		child->load(parent1File);
	}
	else {
		child->load(parent2File);
	}

	// Apply mutation (done through retraining with some randomness)
	if (dist(gen) < mutationRate) {
		// Save the child
		child->save(childFile);

		// Add some random noise to the child's exploration rate
		child = std::make_unique<ChessRLAgent>(
			0.1f + dist(gen) * 0.2f,  // Randomized exploration rate
			0.001f + dist(gen) * 0.009f, // Randomized learning rate
			0.95f + dist(gen) * 0.04f    // Randomized discount factor
		);

		// Load the weights back
		child->load(childFile);
	}

	// Clean up temporary files
	std::remove(parent1File.c_str());
	std::remove(parent2File.c_str());
	std::remove(childFile.c_str());

	return child;
}

int main() {
	std::cout << "Starting Chess Reinforcement Learning Tournament Training..." << std::endl;

	// Initialize population of agents
	std::vector<std::unique_ptr<ChessRLAgent>> population;

	// Create initial population
	for (int i = 0; i < POPULATION_SIZE; i++) {
		std::cout << "Initializing agent " << i + 1 << "/" << POPULATION_SIZE << std::endl;
		auto agent = std::make_unique<ChessRLAgent>();

		// Try to load from an existing file for the first agent
		if (i == 0 && agent->load("chess_rl_model.bin")) {
			std::cout << "Loaded existing model for agent 1." << std::endl;
		}
		else {
			std::cout << "Starting with a new model for agent " << i + 1 << "." << std::endl;
			// Vary the hyperparameters slightly for diversity
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_real_distribution<float> dist(0.0f, 1.0f);

			float epsilon = 0.05f + dist(gen) * 0.15f;  // Exploration rate between 0.05 and 0.2
			float alpha = 0.0005f + dist(gen) * 0.0015f; // Learning rate between 0.0005 and 0.002
			float gamma = 0.95f + dist(gen) * 0.04f;     // Discount factor between 0.95 and 0.99

			agent = std::make_unique<ChessRLAgent>(epsilon, alpha, gamma);
		}

		population.push_back(std::move(agent));
	}

	// Variables to track progress across generations
	std::vector<float> bestScoreHistory;
	std::vector<float> avgScoreHistory;
	std::vector<int> bestAgentHistory;

	// Evolution loop - each generation consists of tournament + reproduction
	for (int generation = 1; generation <= NUM_GENERATIONS; generation++) {
		std::cout << "\n============================================================" << std::endl;
		std::cout << "=== Generation " << generation << "/" << NUM_GENERATIONS << " ===" << std::endl;
		std::cout << "============================================================" << std::endl;

		// First, train each agent via self-play
		std::vector<TrainingStats> trainingStatsForGen;

		for (int i = 0; i < POPULATION_SIZE; i++) {
			std::cout << "Training agent " << i + 1 << " via self-play..." << std::endl;
			TrainingStats stats = trainAgentViaSelfPlay(*population[i], TRAINING_EPISODES_PER_GEN);
			trainingStatsForGen.push_back(stats);

			// Print training statistics
			std::cout << "  Training Summary for Agent " << i + 1 << ":" << std::endl;
			std::cout << "  - Games played: " << stats.totalGames << std::endl;
			std::cout << "  - Game outcomes: "
				<< stats.whiteWins << "W/" << stats.blackWins << "B/"
				<< stats.draws << "D/" << stats.truncated << "T" << std::endl;

			float avgMoves = stats.totalGames > 0 ? static_cast<float>(stats.totalMoves) / stats.totalGames : 0;
			std::cout << "  - Moves per game: " << std::fixed << std::setprecision(1)
				<< avgMoves << " avg, " << stats.minMoves << " min, " << stats.maxMoves << " max" << std::endl;

			std::cout << "  - Average material balance: " << std::fixed << std::setprecision(2)
				<< stats.avgMaterialBalance << std::endl;
		}

		// Run a tournament to rank the agents
		std::cout << "Running tournament..." << std::endl;
		TournamentResult results = runTournament(population);

		// Display detailed tournament results
		std::cout << "==== Tournament Results for Generation " << generation << " ====" << std::endl;
		std::cout << std::fixed << std::setprecision(2);

		// Score matrix header
		std::cout << "\nScore Matrix (row vs column):" << std::endl;
		std::cout << "       ";
		for (size_t j = 0; j < results.scoreMatrix.size(); j++) {
			std::cout << "Agent" << std::setw(2) << j + 1 << " ";
		}
		std::cout << " Total" << std::endl;

		// Score matrix data
		for (size_t i = 0; i < results.scoreMatrix.size(); i++) {
			std::cout << "Agent" << std::setw(2) << i + 1 << " ";
			for (size_t j = 0; j < results.scoreMatrix[i].size(); j++) {
				if (i == j) {
					std::cout << "  --- ";
				}
				else {
					std::cout << " " << std::setw(5) << results.scoreMatrix[i][j] << " ";
				}
			}
			std::cout << " " << std::setw(5) << results.totalScores[i] << std::endl;
		}

		// Rankings
		std::cout << "\nRankings:" << std::endl;
		for (size_t i = 0; i < results.rankings.size(); i++) {
			size_t agentIdx = results.rankings[i];
			std::cout << i + 1 << ". Agent " << std::setw(2) << agentIdx + 1
				<< " (Score: " << std::setw(5) << results.totalScores[agentIdx]
				<< ", Win rate: " << std::setw(5) << results.totalScores[agentIdx] / (results.rankings.size() - 1) * 100 << "%)"
					<< std::endl;
		}

		// Save statistics for tracking progress
		size_t bestAgentIdx = results.rankings[0];
		float bestScore = results.totalScores[bestAgentIdx];

		// Calculate average score
		float totalScore = 0.0f;
		for (float score : results.totalScores) {
			totalScore += score;
		}
		float avgScore = totalScore / results.totalScores.size();

		// Store history
		bestScoreHistory.push_back(bestScore);
		avgScoreHistory.push_back(avgScore);
		bestAgentHistory.push_back(bestAgentIdx);

		// Track if the best agent has changed or score has improved
		bool newBestAgent = generation > 1 && bestAgentIdx != bestAgentHistory[generation - 2];
		bool scoreImproved = generation > 1 && bestScore > bestScoreHistory[generation - 2];

		// Save the best agent of this generation
		std::string bestModelFile = "chess_rl_model_gen" + std::to_string(generation) + ".bin";
		population[bestAgentIdx]->save(bestModelFile);
		std::cout << "Best agent saved as " << bestModelFile << std::endl;

		// Print progress summary
		std::cout << "\nGeneration " << generation << " Summary:" << std::endl;
		std::cout << "- Best agent: Agent " << bestAgentIdx + 1
			<< (newBestAgent ? " (NEW BEST)" : "") << std::endl;
		std::cout << "- Best score: " << std::fixed << std::setprecision(2) << bestScore
			<< (scoreImproved ? " (IMPROVED)" : "") << std::endl;
		std::cout << "- Average score: " << avgScore << std::endl;

		// Every 5 generations, also save as the main model and print trend analysis
		if (generation % 5 == 0 || generation == NUM_GENERATIONS) {
			population[bestAgentIdx]->save("chess_rl_model.bin");
			std::cout << "Best agent also saved as chess_rl_model.bin" << std::endl;

			// Print trend analysis if we have enough data
			if (generation >= 5) {
				std::cout << "\nTrend Analysis (last 5 generations):" << std::endl;

				float avgBestScore = 0.0f;
				float avgOfAvgScores = 0.0f;

				for (int i = std::max(0, (int)bestScoreHistory.size() - 5); i < bestScoreHistory.size(); i++) {
					avgBestScore += bestScoreHistory[i];
					avgOfAvgScores += avgScoreHistory[i];
				}

				avgBestScore /= std::min(5, (int)bestScoreHistory.size());
				avgOfAvgScores /= std::min(5, (int)avgScoreHistory.size());

				std::cout << "- Average best score: " << avgBestScore << std::endl;
				std::cout << "- Average of average scores: " << avgOfAvgScores << std::endl;

				// Analyze if we're still improving
				if (bestScoreHistory.size() >= 10) {
					float recentAvg = 0.0f;
					float previousAvg = 0.0f;

					for (int i = bestScoreHistory.size() - 5; i < bestScoreHistory.size(); i++) {
						recentAvg += bestScoreHistory[i];
					}
					recentAvg /= 5;

					for (int i = bestScoreHistory.size() - 10; i < bestScoreHistory.size() - 5; i++) {
						previousAvg += bestScoreHistory[i];
					}
					previousAvg /= 5;

					float improvementRate = (recentAvg - previousAvg) / previousAvg * 100;
					std::cout << "- Improvement rate: " << std::fixed << std::setprecision(2)
						<< improvementRate << "% over last 5 generations" << std::endl;

					if (improvementRate <= 0.5f) {
						std::cout << "WARNING: Training may be plateauing. Consider adjusting parameters." << std::endl;
					}
				}
			}
		}

		// Skip evolution on the final generation
		if (generation == NUM_GENERATIONS) break;

		// Create the next generation
		std::vector<std::unique_ptr<ChessRLAgent>> nextGeneration;

		// Keep the elite agents unchanged
		for (int i = 0; i < ELITES_TO_KEEP && i < POPULATION_SIZE; i++) {
			size_t eliteIdx = results.rankings[i];
			std::cout << "Keeping elite agent " << eliteIdx + 1 << std::endl;

			// Create a new agent and copy the elite's weights
			auto eliteCopy = std::make_unique<ChessRLAgent>();

			// Save the elite to a temporary file
			std::string tempFile = "temp_elite.bin";
			population[eliteIdx]->save(tempFile);

			// Load the elite's weights into the new agent
			eliteCopy->load(tempFile);

			// Clean up
			std::remove(tempFile.c_str());

			nextGeneration.push_back(std::move(eliteCopy));
		}

		// Fill the rest of the population with children of the top agents
		while (nextGeneration.size() < POPULATION_SIZE) {
			// Select two parents from the top half of the population
			std::uniform_int_distribution<size_t> parentDist(0, POPULATION_SIZE / 2);
			size_t parent1Rank = parentDist(std::mt19937(std::random_device()()));
			size_t parent2Rank = parentDist(std::mt19937(std::random_device()()));

			// Ensure we don't use the same parent twice
			while (parent2Rank == parent1Rank) {
				parent2Rank = parentDist(std::mt19937(std::random_device()()));
			}

			size_t parent1Idx = results.rankings[parent1Rank];
			size_t parent2Idx = results.rankings[parent2Rank];

			std::cout << "Creating child from agents " << parent1Idx + 1 << " and " << parent2Idx + 1 << std::endl;

			// Create a child
			auto child = createChildAgent(*population[parent1Idx], *population[parent2Idx],
				nextGeneration.size(), MUTATION_RATE);

			nextGeneration.push_back(std::move(child));
		}

		// Replace the current population with the next generation
		population = std::move(nextGeneration);
	}

	// Print final training statistics
	std::cout << "\n============================================================" << std::endl;
	std::cout << "=== Training Complete ===" << std::endl;
	std::cout << "============================================================" << std::endl;

	std::cout << "The best agent has been saved as chess_rl_model.bin" << std::endl;

	// Print best score progression
	std::cout << "\nBest Score Progression:" << std::endl;
	std::cout << "Generation | Best Agent | Best Score | Avg Score" << std::endl;
	std::cout << "-----------------------------------------------" << std::endl;
	for (size_t i = 0; i < bestScoreHistory.size(); i++) {
		std::cout << std::setw(10) << i + 1 << " | ";
		std::cout << std::setw(10) << bestAgentHistory[i] + 1 << " | ";
		std::cout << std::setw(10) << std::fixed << std::setprecision(2) << bestScoreHistory[i] << " | ";
		std::cout << std::setw(9) << avgScoreHistory[i] << std::endl;
	}

	// Calculate overall improvement
	if (bestScoreHistory.size() >= 2) {
		float firstScore = bestScoreHistory[0];
		float lastScore = bestScoreHistory.back();
		float improvement = ((lastScore - firstScore) / firstScore) * 100;

		std::cout << "\nOverall improvement: " << std::fixed << std::setprecision(2)
			<< improvement << "%" << std::endl;
	}

	std::cout << "\nTraining complete. Use the chess_rl_model.bin in your chess engine." << std::endl;

	return 0;
}