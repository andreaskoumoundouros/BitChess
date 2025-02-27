#include "bitboard.h"
#include <gtest/gtest.h>

class BitboardTest : public ::testing::Test {
protected:
    void SetUp() override {
        BitboardUtils::initBitboards();
    }
};

TEST_F(BitboardTest, PopulationCount) {
    // Test empty bitboard
    EXPECT_EQ(BitboardUtils::popCount(0ULL), 0);
    
    // Test single bit set
    EXPECT_EQ(BitboardUtils::popCount(1ULL), 1);
    
    // Test multiple bits set
    EXPECT_EQ(BitboardUtils::popCount(0x101010101010101ULL), 8); // A-file
    EXPECT_EQ(BitboardUtils::popCount(0xFFULL), 8); // First rank
    EXPECT_EQ(BitboardUtils::popCount(0xFFFFFFFFFFFFFFFFULL), 64); // Full board
}

TEST_F(BitboardTest, LeastSignificantBit) {
    // Test with no bits set
    EXPECT_EQ(BitboardUtils::lsb(0ULL), NO_SQUARE);
    
    // Test with single bits set
    EXPECT_EQ(BitboardUtils::lsb(1ULL), A1);
    EXPECT_EQ(BitboardUtils::lsb(1ULL << 10), C2);
    EXPECT_EQ(BitboardUtils::lsb(1ULL << 63), H8);
    
    // Test with multiple bits set
    EXPECT_EQ(BitboardUtils::lsb(0x100000001ULL), A1); // A1 and E5
}

TEST_F(BitboardTest, MostSignificantBit) {
    // Test with no bits set
    EXPECT_EQ(BitboardUtils::msb(0ULL), NO_SQUARE);
    
    // Test with single bits set
    EXPECT_EQ(BitboardUtils::msb(1ULL), A1);
    EXPECT_EQ(BitboardUtils::msb(1ULL << 10), C2);
    EXPECT_EQ(BitboardUtils::msb(1ULL << 63), H8);
    
    // Test with multiple bits set
    EXPECT_EQ(BitboardUtils::msb(0x1000000001ULL), E5); // A1 and E5
    //EXPECT_EQ(BitboardUtils::msb(0xFFFFFFFFFFFFFFFFULL), E5); // A1 and E5
}

TEST_F(BitboardTest, PopLeastSignificantBit) {
    // Test popping the least significant bit
    Bitboard bb = 0x100000001ULL; // A1 and E5
    Bitboard original = bb;
    
    BitboardUtils::popLsb(bb);
    EXPECT_EQ(bb, 0x100000000ULL); // Only E5 should remain
    
    BitboardUtils::popLsb(bb);
    EXPECT_EQ(bb, 0ULL); // Should be empty now
}

TEST_F(BitboardTest, KnightAttacks) {
    // Test knight attacks from corner
    EXPECT_EQ(BitboardUtils::popCount(BitboardConstants::KNIGHT_ATTACKS[A1]), 2);
    EXPECT_TRUE(BitboardUtils::testBit(BitboardConstants::KNIGHT_ATTACKS[A1], B3));
    EXPECT_TRUE(BitboardUtils::testBit(BitboardConstants::KNIGHT_ATTACKS[A1], C2));
    
    // Test knight attacks from center
    EXPECT_EQ(BitboardUtils::popCount(BitboardConstants::KNIGHT_ATTACKS[D4]), 8);
}

TEST_F(BitboardTest, KingAttacks) {
    // Test king attacks from corner
    EXPECT_EQ(BitboardUtils::popCount(BitboardConstants::KING_ATTACKS[A1]), 3);
    EXPECT_TRUE(BitboardUtils::testBit(BitboardConstants::KING_ATTACKS[A1], A2));
    EXPECT_TRUE(BitboardUtils::testBit(BitboardConstants::KING_ATTACKS[A1], B1));
    EXPECT_TRUE(BitboardUtils::testBit(BitboardConstants::KING_ATTACKS[A1], B2));
    
    // Test king attacks from center
    EXPECT_EQ(BitboardUtils::popCount(BitboardConstants::KING_ATTACKS[D4]), 8);
}

TEST_F(BitboardTest, PawnAttacks) {
    // Test white pawn attacks
    EXPECT_EQ(BitboardUtils::popCount(BitboardConstants::PAWN_ATTACKS[WHITE][A2]), 1);
    EXPECT_TRUE(BitboardUtils::testBit(BitboardConstants::PAWN_ATTACKS[WHITE][A2], B3));
    
    EXPECT_EQ(BitboardUtils::popCount(BitboardConstants::PAWN_ATTACKS[WHITE][D4]), 2);
    EXPECT_TRUE(BitboardUtils::testBit(BitboardConstants::PAWN_ATTACKS[WHITE][D4], C5));
    EXPECT_TRUE(BitboardUtils::testBit(BitboardConstants::PAWN_ATTACKS[WHITE][D4], E5));
    
    // Test black pawn attacks
    EXPECT_EQ(BitboardUtils::popCount(BitboardConstants::PAWN_ATTACKS[BLACK][A7]), 1);
    EXPECT_TRUE(BitboardUtils::testBit(BitboardConstants::PAWN_ATTACKS[BLACK][A7], B6));
    
    EXPECT_EQ(BitboardUtils::popCount(BitboardConstants::PAWN_ATTACKS[BLACK][D4]), 2);
    EXPECT_TRUE(BitboardUtils::testBit(BitboardConstants::PAWN_ATTACKS[BLACK][D4], C3));
    EXPECT_TRUE(BitboardUtils::testBit(BitboardConstants::PAWN_ATTACKS[BLACK][D4], E3));
}

TEST_F(BitboardTest, SquareRankFile) {
    // Test square to rank/file conversions
    EXPECT_EQ(BitboardUtils::squareRank(A1), RANK_1);
    EXPECT_EQ(BitboardUtils::squareFile(A1), FILE_A);
    
    EXPECT_EQ(BitboardUtils::squareRank(E4), RANK_4);
    EXPECT_EQ(BitboardUtils::squareFile(E4), FILE_E);
    
    EXPECT_EQ(BitboardUtils::squareRank(H8), RANK_8);
    EXPECT_EQ(BitboardUtils::squareFile(H8), FILE_H);
    
    // Test rank/file to square conversions
    EXPECT_EQ(BitboardUtils::squareFromRankFile(RANK_1, FILE_A), A1);
    EXPECT_EQ(BitboardUtils::squareFromRankFile(RANK_4, FILE_E), E4);
    EXPECT_EQ(BitboardUtils::squareFromRankFile(RANK_8, FILE_H), H8);
}

TEST_F(BitboardTest, PrettyPrint) {
    // Test pretty printing an empty board
    std::string emptyBoard = BitboardUtils::prettyPrint(0ULL);
    EXPECT_NE(emptyBoard.find("+---+---+---+---+---+---+---+---+"), std::string::npos);
    
    // Test pretty printing a board with a single piece
    std::string singlePiece = BitboardUtils::prettyPrint(1ULL << E4);
    EXPECT_NE(singlePiece.find(" X "), std::string::npos);
}