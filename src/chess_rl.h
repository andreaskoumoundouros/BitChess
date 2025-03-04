#pragma once

#include <vector>
#include <random>
#include <memory>
#include <string>
#include <unordered_map>
#include "board.h"
#include "neural_network.h"
#include "feature_extractor.h"

#ifdef _MSC_VER
#  include <intrin.h>
#  define __builtin_popcountll _mm_popcnt_u64
#endif

// Structure to store game state information for training
struct GameState {
    Board board;
    std::vector<float> features;
    Move chosenMove;
    float reward;
};

// Reinforcement Learning Agent
class ChessRLAgent {
private:
    std::unique_ptr<NeuralNetwork> valueNetwork;
    float explorationRate;
    float learningRate;
    float discountFactor;
    std::mt19937 rng;
    
    std::vector<GameState> replayBuffer;
    size_t maxReplayBufferSize;

public:
    ChessRLAgent(float epsilon = 0.1, float alpha = 0.001, float gamma = 0.99);
    
    // Select a move using epsilon-greedy strategy
    Move selectMove(const Board& board, const std::vector<Move>& legalMoves);
    
    // Record a state-action-reward transition
    void recordTransition(const Board& board, const Move& move, float reward);
    
    // Calculate reward based on game outcome
    float calculateReward(const Board& board, Color agentColor);
    
    // Train the network using experience replay
    void train(size_t batchSize = 32);
    
    // Decrease exploration rate over time
    void decayExplorationRate(float decayFactor = 0.995f);
    
    // Save the agent to a file
    void save(const std::string& filename);
    
    // Load the agent from a file
    bool load(const std::string& filename);
};

// Declaration of the modelBasedMove function
Move modelBasedMove(const std::vector<Move>& legalMoves, const Board& board);
