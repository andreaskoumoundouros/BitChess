#ifndef BOARD_H
#define BOARD_H

#include "bitboard.h"
#include <string>
#include <vector>

// A struct to represent a chess move
struct Move {
    Square from;
    Square to;
    PieceType promotion;
    
    Move() : from(NO_SQUARE), to(NO_SQUARE), promotion(NO_PIECE_TYPE) {}
    Move(Square f, Square t, PieceType p = NO_PIECE_TYPE) : from(f), to(t), promotion(p) {}
    
    // Compare moves for equality
    bool operator==(const Move& other) const {
        return from == other.from && to == other.to && promotion == other.promotion;
    }
    
    // Convert move to UCI string
    std::string toUci() const;
    
    // Create move from UCI string
    static Move fromUci(const std::string& uci);
    
    // Check if move is valid
    bool isValid() const {
        return from != NO_SQUARE && to != NO_SQUARE;
    }
};

// Castling rights
enum CastlingRight {
    WHITE_OO = 1,         // White kingside
    WHITE_OOO = 2,        // White queenside
    BLACK_OO = 4,         // Black kingside
    BLACK_OOO = 8,        // Black queenside
    NO_CASTLING = 0,
    ANY_CASTLING = 15
};

// A class to represent the chess board
class Board {
public:
    // Constructor
    Board();
    
    // Setup the board from a FEN string
    bool setFromFen(const std::string& fen);
    
    // Get FEN string representing current position
    std::string getFen() const;
    
    // Reset board to starting position
    void reset();

    void updateCombinedBitboards();

    // Make a move on the board
    bool makeMove(const Move& move);
    
    // Generate all legal moves
    std::vector<Move> generateLegalMoves() const;
    
    // Check if the current position is a checkmate
    bool isCheckmate() const;
    
    // Check if the current position is a stalemate
    bool isStalemate() const;
    
    // Check if the game is drawn by insufficient material
    bool isInsufficientMaterial() const;
    
    // Check if a square is attacked by a given color
    bool isSquareAttacked(Square sq, Color attackingColor) const;
    
    // Get the piece at a given square
    PieceType pieceAt(Square sq, Color& color) const;
    
    // Get the side to move
    Color sideToMove() const { return _sideToMove; }
    
    // Get castling rights
    int castlingRights() const { return _castlingRights; }
    
    // Get en passant square
    Square enPassantSquare() const { return _enPassantSquare; }
    
    // Get halfmove clock (for 50-move rule)
    int halfmoveClock() const { return _halfmoveClock; }
    
    // Get fullmove number
    int fullmoveNumber() const { return _fullmoveNumber; }
    
    // Check if the king of a given color is in check
    bool isInCheck(Color color) const;
    
    // Print the board
    std::string toString() const;
    
    // Bitboards for each piece type and color
    std::array<std::array<Bitboard, 6>, 2> _pieces; // [color][piece_type]
    
    // Combined bitboards for all pieces of each color
    std::array<Bitboard, 2> _allPieces; // [color]
    
    // Combined bitboard for all pieces
    Bitboard _occupiedSquares;
    
    // Side to move
    Color _sideToMove;
    
    // Castling rights
    int _castlingRights;
    
    // En passant square
    Square _enPassantSquare;
    
    // Halfmove clock (for 50-move rule)
    int _halfmoveClock;
    
    // Fullmove number
    int _fullmoveNumber;

    // Chess960 mode
    bool _chess960;
    
    // Helper method to find the king square for a given color
    Square findKing(Color color) const;
private:
    
    // Helper methods for legal move generation
    void addPawnMoves(std::vector<Move>& moves, Square from) const;
    void addKnightMoves(std::vector<Move>& moves, Square from) const;
    void addBishopMoves(std::vector<Move>& moves, Square from) const;
    void addRookMoves(std::vector<Move>& moves, Square from) const;
    void addQueenMoves(std::vector<Move>& moves, Square from) const;
    void addKingMoves(std::vector<Move>& moves, Square from) const;
    
    // Helper method to update board state after a move
    void updateBoardState(const Move& move, PieceType movingPiece, PieceType capturedPiece, Color capturedColor);
    
};

#endif // BOARD_H