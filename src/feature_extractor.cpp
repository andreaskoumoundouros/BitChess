#include "feature_extractor.h"

std::vector<float> BoardFeatureExtractor::extractFeatures(const Board& board) {
    std::vector<float> features;
    features.reserve(64 * 12 + 8); // 64 squares * 12 piece types + 8 game state features
    
    // Piece placement features (12 piece types × 64 squares = 768 binary values)
    // We'll use a more compact representation with one-hot encoding for each square
    for (int sq = 0; sq < 64; sq++) {
        Color pieceColor;
        PieceType pieceType = board.pieceAt(static_cast<Square>(sq), pieceColor);
        
        // Add 12 binary values for each square (6 piece types × 2 colors)
        for (int pc = 0; pc < 6; pc++) {
            for (int c = 0; c < 2; c++) {
                if (pieceType == pc && pieceColor == c) {
                    features.push_back(1.0f);
                } else {
                    features.push_back(0.0f);
                }
            }
        }
    }
    
    // Side to move (1 value)
    features.push_back(board.sideToMove() == WHITE ? 1.0f : -1.0f);
    
    // Castling rights (4 values)
    features.push_back((board.castlingRights() & WHITE_OO) ? 1.0f : 0.0f);
    features.push_back((board.castlingRights() & WHITE_OOO) ? 1.0f : 0.0f);
    features.push_back((board.castlingRights() & BLACK_OO) ? 1.0f : 0.0f);
    features.push_back((board.castlingRights() & BLACK_OOO) ? 1.0f : 0.0f);
    
    // En passant possibility (1 value)
    features.push_back(board.enPassantSquare() != NO_SQUARE ? 1.0f : 0.0f);
    
    // Halfmove clock (for 50-move rule) - normalized
    features.push_back(static_cast<float>(board.halfmoveClock()) / 100.0f);
    
    // Check status (2 values)
    features.push_back(board.isInCheck(WHITE) ? 1.0f : 0.0f);
    features.push_back(board.isInCheck(BLACK) ? 1.0f : 0.0f);
    
    return features;
}

size_t BoardFeatureExtractor::getFeatureSize() {
    // Calculate the total number of features
    return 64 * 12 + 9; // 64 squares * 12 piece types + 9 game state features
}