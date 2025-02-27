#ifndef BITBOARD_H
#define BITBOARD_H

#include <cstdint>
#include <string>
#include <array>

// Type definition for a bitboard (64-bit unsigned integer)
using Bitboard = uint64_t;

// Chess piece types
enum PieceType {
    PAWN = 0,
    KNIGHT = 1,
    BISHOP = 2,
    ROOK = 3,
    QUEEN = 4,
    KING = 5,
    NO_PIECE_TYPE = 6
};

// Chess colors
enum Color {
    WHITE = 0,
    BLACK = 1,
    NO_COLOR = 2
};

// Chess squares (little-endian rank-file mapping)
enum Square {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    NO_SQUARE
};

// Rank enumeration
enum Rank {
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, NO_RANK
};

// File enumeration
enum File {
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, NO_FILE
};

// Direction offsets for piece movement
enum Direction {
    NORTH = 8,
    EAST = 1,
    SOUTH = -8,
    WEST = -1,
    NORTH_EAST = 9,
    SOUTH_EAST = -7,
    SOUTH_WEST = -9,
    NORTH_WEST = 7
};

// Constants for bitboard operations
namespace BitboardConstants {
    // Pre-computed masks for ranks and files
    extern const std::array<Bitboard, 8> RANK_MASKS;
    extern const std::array<Bitboard, 8> FILE_MASKS;
    
    // Pre-computed masks for diagonals and anti-diagonals
    extern const std::array<Bitboard, 15> DIAGONAL_MASKS;
    extern const std::array<Bitboard, 15> ANTI_DIAGONAL_MASKS;

    // Pre-computed attack patterns
    extern std::array<Bitboard, 64> KNIGHT_ATTACKS;
    extern std::array<Bitboard, 64> KING_ATTACKS;
    extern std::array<std::array<Bitboard, 64>, 2> PAWN_ATTACKS; // Indexed by [color][square]
}

// Bitboard utility functions
namespace BitboardUtils {
    // Print a bitboard as an 8x8 grid
    std::string prettyPrint(Bitboard bb);

    // Get rank and file from a square
    inline Rank squareRank(Square sq) { return static_cast<Rank>(sq / 8); }
    inline File squareFile(Square sq) { return static_cast<File>(sq % 8); }

    // Get a square from rank and file
    inline Square squareFromRankFile(Rank r, File f) { return static_cast<Square>(r * 8 + f); }

    // Bit manipulation utilities
    inline Bitboard setBit(Bitboard bb, Square sq) { return bb | (1ULL << sq); }
    inline Bitboard clearBit(Bitboard bb, Square sq) { return bb & ~(1ULL << sq); }
    inline bool testBit(Bitboard bb, Square sq) { return (bb & (1ULL << sq)) != 0; }
    
    // Count number of set bits (population count)
    int popCount(Bitboard bb);
    
    // Get index of least significant bit
    Square lsb(Bitboard bb);
    
    // Get index of most significant bit
    Square msb(Bitboard bb);
    
    // Clear least significant bit and return new bitboard
    inline Bitboard popLsb(Bitboard& bb) {
        Bitboard result = bb;
        bb &= (bb - 1); // Clear the least significant bit
        return result;
    }
    
    // Initialize attack tables and masks
    void initBitboards();
}

#endif // BITBOARD_H