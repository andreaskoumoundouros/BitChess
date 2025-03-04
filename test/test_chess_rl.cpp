#include "gtest/gtest.h"
#include "chess_rl.h"
#include "board.h"
#include <vector>

TEST(ChessRLTest, FeatureExtractor) {
    // Create a board in the initial position
    Board board;
    board.reset();
    
    // Extract features
    std::vector<float> features = BoardFeatureExtractor::extractFeatures(board);
    
    // Check if the feature vector has the correct size
    EXPECT_EQ(features.size(), BoardFeatureExtractor::getFeatureSize());
    
    // Check if features are valid (all values should be finite)
    for (float feature : features) {
        EXPECT_TRUE(std::isfinite(feature));
    }
}

TEST(ChessRLTest, AgentMoveSelection) {
    // Create a board in the initial position
    Board board;
    board.reset();
    
    // Generate legal moves
    std::vector<Move> legalMoves = board.generateLegalMoves();
    
    // Create an agent
    ChessRLAgent agent;
    
    // Select a move
    Move selectedMove = agent.selectMove(board, legalMoves);
    
    // Check if the selected move is valid
    EXPECT_TRUE(selectedMove.isValid());
    
    // Check if the selected move is in the legal moves list
    bool found = false;
    for (const Move& move : legalMoves) {
        if (move == selectedMove) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST(ChessRLTest, RewardCalculation) {
    // Create a board in the initial position
    Board board;
    board.reset();
    
    // Create an agent
    ChessRLAgent agent;
    
    // Calculate reward for white
    float rewardWhite = agent.calculateReward(board, WHITE);
    
    // Calculate reward for black
    float rewardBlack = agent.calculateReward(board, BLACK);
    
    // In the initial position, the material is balanced, so the rewards should be near zero
    EXPECT_NEAR(rewardWhite, 0.0f, 0.01f);
    EXPECT_NEAR(rewardBlack, 0.0f, 0.01f);
    
    // Now let's modify the board to create an imbalance
    // Remove a white pawn
    board._pieces[WHITE][PAWN] &= ~(1ULL << A2);
    // board._pieces[WHITE][PAWN] = 0x0000000000007F00ULL;
    board.updateCombinedBitboards();
    
    // Calculate rewards again
    rewardWhite = agent.calculateReward(board, WHITE);
    rewardBlack = agent.calculateReward(board, BLACK);
    
    // White has less material, so white's reward should be negative and black's should be positive
    EXPECT_LT(rewardWhite, 0.0f);
    EXPECT_GT(rewardBlack, 0.0f);
}

TEST(ChessRLTest, TrainingBasics) {
    // Create an agent
    ChessRLAgent agent;
    
    // Create a board
    Board board;
    board.reset();
    
    // Record a transition
    std::vector<Move> legalMoves = board.generateLegalMoves();
    Move move = legalMoves[0]; // Just pick the first legal move
    float reward = 0.1f;
    agent.recordTransition(board, move, reward);
    
    // Train the agent (with a single sample)
    // This should not crash even though we only have one sample
    agent.train(1);
    
    // Test exploration rate decay
    float initialExplorationRate = 0.1f; // Default value
    agent.decayExplorationRate(0.5f);
    
    // We don't have direct access to the exploration rate, so we can't test its value directly
    // But we can ensure that the operation doesn't crash
    SUCCEED();
}