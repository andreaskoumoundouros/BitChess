#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "bitboard.h"
#include <vector>

// Forward declaration of Move and Board
struct Move;
class Board;

// MoveGenerator class responsible for generating legal moves
class MoveGenerator {
public:
    // Generate all legal moves for the current position
    static std::vector<Move> generateLegalMoves(const Board& board);
    
    // Generate all pseudo-legal moves (may include moves that leave the king in check)
    static std::vector<Move> generatePseudoLegalMoves(const Board& board);
    
    // Check if the king is in check after a move
    static bool isKingInCheck(const Board& board, Color kingColor);
    
    // Generate attacks for a specific piece type from a square
    static Bitboard getPieceAttacks(PieceType pieceType, Square sq, Color color, Bitboard occupied);
    
    // Generate all squares attacked by a specific color
    static Bitboard getAttackedSquares(const Board& board, Color attackingColor);

private:
    // Generate pawn moves
    static void generatePawnMoves(const Board& board, std::vector<Move>& moves);
    
    // Generate knight moves
    static void generateKnightMoves(const Board& board, std::vector<Move>& moves);
    
    // Generate bishop moves
    static void generateBishopMoves(const Board& board, std::vector<Move>& moves);
    
    // Generate rook moves
    static void generateRookMoves(const Board& board, std::vector<Move>& moves);
    
    // Generate queen moves
    static void generateQueenMoves(const Board& board, std::vector<Move>& moves);
    
    // Generate king moves
    static void generateKingMoves(const Board& board, std::vector<Move>& moves);
    
    // Generate castling moves
    static void generateCastlingMoves(const Board& board, std::vector<Move>& moves);
    
    // Filter out illegal moves that leave the king in check
    static std::vector<Move> filterLegalMoves(const Board& board, const std::vector<Move>& pseudoLegalMoves);
    
    // Get rook attacks using magic bitboards
    static Bitboard getRookAttacks(Square sq, Bitboard occupied);
    
    // Get bishop attacks using magic bitboards
    static Bitboard getBishopAttacks(Square sq, Bitboard occupied);
};

#endif // MOVEGEN_H