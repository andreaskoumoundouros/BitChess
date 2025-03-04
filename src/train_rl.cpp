#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include "chess_rl.h"

// Number of training episodes
const int NUM_EPISODES = 10000;

// Maximum moves per game to prevent infinite games
const int MAX_MOVES_PER_GAME = 200;

// Function to play a self-play game and train the agent
void trainEpisode(ChessRLAgent& agent, int episodeNum) {
    Board board;
    board.reset(); // Start from the initial position
    
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
    
    // Determine game outcome
    float finalReward = 0.0f;
    if (board.isCheckmate()) {
        // The side that isn't to move has won
        finalReward = 1.0f;
        std::cout << "Episode " << episodeNum << ": Checkmate! ";
        std::cout << (board.sideToMove() == BLACK ? "White" : "Black") << " wins." << std::endl;
    } else if (board.isStalemate() || board.isInsufficientMaterial() || board.halfmoveClock() >= 100) {
        finalReward = 0.0f;
        std::cout << "Episode " << episodeNum << ": Draw!" << std::endl;
    } else {
        // Game truncated due to move limit
        std::cout << "Episode " << episodeNum << ": Game truncated after " << moveCount << " moves." << std::endl;
        
        // Evaluate final position
        finalReward = agent.calculateReward(board, WHITE) / 100.0f; // Small reward based on material
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
    
    // Save the model every 100 episodes
    if (episodeNum % 100 == 0) {
        agent.save("chess_rl_model.bin");
        std::cout << "Model saved at episode " << episodeNum << std::endl;
    }
}


int main() {
    std::cout << "Starting Chess Reinforcement Learning Training..." << std::endl;
    
    // Create the RL agent
    ChessRLAgent agent;
    
    // Load previous model if it exists
    if (agent.load("chess_rl_model.bin")) {
        std::cout << "Loaded existing model." << std::endl;
    } else {
        std::cout << "Starting with a new model." << std::endl;
    }
    
    // Training loop
    for (int episode = 1; episode <= NUM_EPISODES; episode++) {
        std::cout << "Episode " << episode << "/" << NUM_EPISODES << std::endl;
        trainEpisode(agent, episode);
    }
    
    // Save the final model
    agent.save("chess_rl_model_final.bin");
    std::cout << "Training complete. Final model saved." << std::endl;
    
    return 0;
}