#include "board.h"
#include <gtest/gtest.h>
#include <string>

class BoardTest : public ::testing::Test {
protected:
    void SetUp() override {
        BitboardUtils::initBitboards();
        board.reset();
    }
    
    Board board;
};

TEST_F(BoardTest, DefaultPosition) {
    // Check initial board setup
    EXPECT_EQ(board.sideToMove(), WHITE);
    EXPECT_EQ(board.castlingRights(), ANY_CASTLING);
    EXPECT_EQ(board.enPassantSquare(), NO_SQUARE);
    EXPECT_EQ(board.halfmoveClock(), 0);
    EXPECT_EQ(board.fullmoveNumber(), 1);
    
    // Check piece placement
    Color color;
    EXPECT_EQ(board.pieceAt(E1, color), KING);
    EXPECT_EQ(color, WHITE);
    
    EXPECT_EQ(board.pieceAt(D8, color), QUEEN);
    EXPECT_EQ(color, BLACK);
    
    EXPECT_EQ(board.pieceAt(A2, color), PAWN);
    EXPECT_EQ(color, WHITE);
}

TEST_F(BoardTest, FenParsingStartPosition) {
    const std::string startPosFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    EXPECT_TRUE(board.setFromFen(startPosFen));
    
    // Check if FEN output matches input
    EXPECT_EQ(board.getFen(), startPosFen);
}

TEST_F(BoardTest, MakeMove) {
    // Test e4 (e2-e4)
    Move e4Move(E2, E4);
    EXPECT_TRUE(board.makeMove(e4Move));
    
    // Check if pawn moved
    Color color;
    EXPECT_EQ(board.pieceAt(E4, color), PAWN);
    EXPECT_EQ(color, WHITE);
    EXPECT_EQ(board.pieceAt(E2, color), NO_PIECE_TYPE);
    
    // Check if side to move changed
    EXPECT_EQ(board.sideToMove(), BLACK);
    
    // Check en passant square
    EXPECT_EQ(board.enPassantSquare(), E3);
    
    // Move counter should not increment for pawn move
    EXPECT_EQ(board.halfmoveClock(), 0);
    
    // Test e5 (e7-e5)
    Move e5Move(E7, E5);
    EXPECT_TRUE(board.makeMove(e5Move));
    
    // Check move counters
    EXPECT_EQ(board.halfmoveClock(), 0);
    EXPECT_EQ(board.fullmoveNumber(), 2);
    
    // Test Nf3 (g1-f3)
    Move nf3Move(G1, F3);
    EXPECT_TRUE(board.makeMove(nf3Move));
    
    // Halfmove clock should increment for non-pawn move
    EXPECT_EQ(board.halfmoveClock(), 1);
}

TEST_F(BoardTest, Capture) {
    // Setup a position for capture
    EXPECT_TRUE(board.setFromFen("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2"));
    
    // Capture with e4xd5
    Move captureMove(E4, D5);
    EXPECT_TRUE(board.makeMove(captureMove));
    
    // Check if pawn was captured
    Color color;
    EXPECT_EQ(board.pieceAt(D5, color), PAWN);
    EXPECT_EQ(color, WHITE);
    
    // Halfmove clock should reset on capture
    EXPECT_EQ(board.halfmoveClock(), 0);
}

TEST_F(BoardTest, Castling) {
    // Setup a position for kingside castling
    EXPECT_TRUE(board.setFromFen("rnbqk2r/pppp1ppp/5n2/2b1p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4"));
    
    // White kingside castling
    Move castleMove(E1, G1);
    EXPECT_TRUE(board.makeMove(castleMove));
    
    // Check if king and rook moved correctly
    Color color;
    EXPECT_EQ(board.pieceAt(G1, color), KING);
    EXPECT_EQ(color, WHITE);
    EXPECT_EQ(board.pieceAt(F1, color), ROOK);
    EXPECT_EQ(color, WHITE);
    
    // Castling rights should be updated
    EXPECT_EQ(board.castlingRights() & WHITE_OO, 0);
    EXPECT_EQ(board.castlingRights() & WHITE_OOO, 0);
}

TEST_F(BoardTest, EnPassant) {
    // Setup a position for en passant
    EXPECT_TRUE(board.setFromFen("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3"));
    
    // En passant capture e5xf6
    Move epMove(E5, F6);
    EXPECT_TRUE(board.makeMove(epMove));
    
    // Check if pawn was captured and moved correctly
    Color color;
    EXPECT_EQ(board.pieceAt(F6, color), PAWN);
    EXPECT_EQ(color, WHITE);
    EXPECT_EQ(board.pieceAt(F5, color), NO_PIECE_TYPE);
}

TEST_F(BoardTest, Promotion) {
    // Setup a position for promotion
    EXPECT_TRUE(board.setFromFen("rnbqkbnr/pPpppppp/8/8/8/8/PPPPPPPp/RNBQKBNR w KQkq - 0 1"));
    
    // White promotion b7-b8=Q
    Move promMove(B7, B8, QUEEN);
    EXPECT_TRUE(board.makeMove(promMove));
    
    // Check if pawn was promoted
    Color color;
    EXPECT_EQ(board.pieceAt(B8, color), QUEEN);
    EXPECT_EQ(color, WHITE);
    
    // Black promotion h2-h1=N
    Move promMoveBlack(H2, H1, KNIGHT);
    EXPECT_TRUE(board.makeMove(promMoveBlack));
    
    // Check if pawn was promoted
    EXPECT_EQ(board.pieceAt(H1, color), KNIGHT);
    EXPECT_EQ(color, BLACK);
}

TEST_F(BoardTest, IllegalMoves) {
    // Try to move opponent's piece
    Move illegalMove(E7, E5);
    EXPECT_FALSE(board.makeMove(illegalMove));
    
    // Try to move into check
	EXPECT_TRUE(board.setFromFen("rnbqkbnr/ppp1pppp/8/3p4/4P3/3K4/PPPP1PPP/RNBQ1BNR b kq - 0 2"));
    Move moveIntoCheck(D3, E3);
    EXPECT_FALSE(board.makeMove(moveIntoCheck));
}

TEST_F(BoardTest, CheckmateDetection) {
    // Fool's mate
    EXPECT_TRUE(board.setFromFen("rnbqkbnr/pppp1ppp/8/4p3/6P1/5P2/PPPPP2P/RNBQKBNR b KQkq - 0 2"));
    Move qh4(D8, H4);
    EXPECT_TRUE(board.makeMove(qh4));
    EXPECT_TRUE(board.isCheckmate());
    EXPECT_FALSE(board.isStalemate());
}

TEST_F(BoardTest, StalemateDetection) {
    // Stalemate position
    EXPECT_TRUE(board.setFromFen("7k/8/6Q1/8/8/8/8/7K w - - 0 1"));
    Move queenMove(G6, F7);
    EXPECT_TRUE(board.makeMove(queenMove));
    EXPECT_FALSE(board.isCheckmate());
    EXPECT_TRUE(board.isStalemate());
}

TEST_F(BoardTest, InsufficientMaterial) {
    // K vs K
    EXPECT_TRUE(board.setFromFen("8/8/8/4k3/8/8/8/4K3 w - - 0 1"));
    EXPECT_TRUE(board.isInsufficientMaterial());
    
    // K+B vs K
    EXPECT_TRUE(board.setFromFen("8/8/8/4k3/8/8/3B4/4K3 w - - 0 1"));
    EXPECT_TRUE(board.isInsufficientMaterial());
    
    // K+B vs K+B (same colored bishops)
    EXPECT_TRUE(board.setFromFen("8/8/8/4k3/8/2b5/3B4/4K3 w - - 0 1"));
    EXPECT_TRUE(board.isInsufficientMaterial());
    
    // K+B vs K+B (different colored bishops)
    EXPECT_TRUE(board.setFromFen("8/8/8/4k3/8/1b6/3B4/4K3 w - - 0 1"));
    EXPECT_FALSE(board.isInsufficientMaterial());
    
    // K+N vs K+N
    EXPECT_TRUE(board.setFromFen("8/8/8/4k3/8/1n6/3N4/4K3 w - - 0 1"));
    EXPECT_FALSE(board.isInsufficientMaterial());
}