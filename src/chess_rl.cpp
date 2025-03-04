#include "chess_rl.h"
#include <limits>
#include <algorithm>

ChessRLAgent::ChessRLAgent(float epsilon, float alpha, float gamma) 
    : explorationRate(epsilon), learningRate(alpha), discountFactor(gamma), maxReplayBufferSize(10000) {
    
    // Initialize network with a topology appropriate for chess
    // Input: board features
    // Hidden layers: 2 layers with 256 and 128 neurons
    // Output: 1 value (position evaluation)
    std::vector<int> topology = {static_cast<int>(BoardFeatureExtractor::getFeatureSize()), 256, 128, 1};
    valueNetwork = std::make_unique<NeuralNetwork>(topology);
    
    // Seed the random number generator
    std::random_device rd;
    rng.seed(rd());
}

Move ChessRLAgent::selectMove(const Board& board, const std::vector<Move>& legalMoves) {
    if (legalMoves.empty()) {
        return Move(); // Return invalid move if no legal moves
    }
    
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    // With probability epsilon, choose a random move (exploration)
    if (dist(rng) < explorationRate) {
        std::uniform_int_distribution<int> moveDist(0, legalMoves.size() - 1);
        return legalMoves[moveDist(rng)];
    }
    
    // Otherwise, choose the best move according to the value network (exploitation)
    float bestValue = -std::numeric_limits<float>::infinity();
    Move bestMove = legalMoves[0]; // Default to first move
    
    for (const Move& move : legalMoves) {
        // Create a copy of the board and make the move
        Board boardCopy = board;
        boardCopy.makeMove(move);
        
        // Extract features and evaluate the resulting position
        std::vector<float> features = BoardFeatureExtractor::extractFeatures(boardCopy);
        float value = valueNetwork->forward(features);
        
        // Negate the value if it's black's turn (minimize for black)
        if (board.sideToMove() == BLACK) {
            value = -value;
        }
        
        // Update best move if this one is better
        if (value > bestValue) {
            bestValue = value;
            bestMove = move;
        }
    }
    
    return bestMove;
}

void ChessRLAgent::recordTransition(const Board& board, const Move& move, float reward) {
    GameState state;
    state.board = board;
    state.features = BoardFeatureExtractor::extractFeatures(board);
    state.chosenMove = move;
    state.reward = reward;
    
    replayBuffer.push_back(state);
    
    // Limit the replay buffer size
    if (replayBuffer.size() > maxReplayBufferSize) {
        replayBuffer.erase(replayBuffer.begin());
    }
}

float ChessRLAgent::calculateReward(const Board& board, Color agentColor) {
    // Check for checkmate
    if (board.isCheckmate()) {
        // Big reward for winning, big penalty for losing
        return (board.sideToMove() != agentColor) ? 1.0f : -1.0f;
    }
    
    // Check for draw conditions
    if (board.isStalemate() || board.isInsufficientMaterial() || board.halfmoveClock() >= 100) {
        return 0.0f; // Neutral reward for draws
    }
    
    // Small reward based on material balance
    float materialBalance = 0.0f;
    
    // Piece values (pawn=1, knight/bishop=3, rook=5, queen=9)
    const float pieceValues[6] = {1.0f, 3.0f, 3.0f, 5.0f, 9.0f, 0.0f};
    
    for (int pc = 0; pc < 6; pc++) { // Skip KING (index 5)
        // Count white pieces
        int whitePieces = __builtin_popcountll(board._pieces[WHITE][pc]);
        // Count black pieces
        int blackPieces = __builtin_popcountll(board._pieces[BLACK][pc]);
        
        materialBalance += pieceValues[pc] * (whitePieces - blackPieces);
    }
    
    // Normalize and adjust for agent color
    materialBalance = materialBalance / 32.0f; // Normalize by approximate max value
    if (agentColor == BLACK) {
        materialBalance = -materialBalance;
    }
    
    return 0.01f * materialBalance; // Small incremental reward
}

void ChessRLAgent::train(size_t batchSize) {
    if (replayBuffer.size() < batchSize) return;
    
    std::uniform_int_distribution<size_t> dist(0, replayBuffer.size() - 1);
    
    for (size_t i = 0; i < batchSize; i++) {
        // Sample a random transition from the replay buffer
        size_t idx = dist(rng);
        const GameState& state = replayBuffer[idx];
        
        // Skip this sample if it's the last state in a game
        if (idx == replayBuffer.size() - 1) continue;
        
        // Get the next state
        const GameState& nextState = replayBuffer[idx + 1];
        
        // Calculate target value using TD learning
        float targetValue;
        
        // If this is the terminal state, the target is just the reward
        // For simplicity, we assume the last state in the buffer is terminal
        if (idx == replayBuffer.size() - 2) {
            targetValue = state.reward;
        } else {
            // Target = reward + gamma * V(next_state)
            float nextStateValue = valueNetwork->forward(nextState.features);
            targetValue = state.reward + discountFactor * nextStateValue;
        }
        
        // Update the network
        valueNetwork->backpropagate(state.features, targetValue, learningRate);
    }
}

void ChessRLAgent::decayExplorationRate(float decayFactor) {
    explorationRate *= decayFactor;
    if (explorationRate < 0.01f) {
        explorationRate = 0.01f; // Set a minimum exploration rate
    }
}

void ChessRLAgent::save(const std::string& filename) {
    valueNetwork->save(filename);
}

bool ChessRLAgent::load(const std::string& filename) {
    return valueNetwork->load(filename);
}

Move modelBasedMove(const std::vector<Move>& legalMoves, const Board& board) {
    static ChessRLAgent agent; // Create a static agent so it persists between calls
    static bool agentLoaded = false;
        
    // Try to load the agent if not already loaded
    if (!agentLoaded) {
        agentLoaded = agent.load("chess_rl_model.bin");
        // If loading failed, we'll use the initial random weights
    }
        
    // Select a move using the RL agent
    return agent.selectMove(board, legalMoves);
}
