#include "bitboard.h"
#include <sstream>
#include <iomanip>
#include <iostream>

// Initialize static constant arrays
namespace BitboardConstants {
    // Rank masks - each bit in the corresponding rank is set
    const std::array<Bitboard, 8> RANK_MASKS = {
        0x00000000000000FFULL, // RANK_1
        0x000000000000FF00ULL, // RANK_2
        0x0000000000FF0000ULL, // RANK_3
        0x00000000FF000000ULL, // RANK_4
        0x000000FF00000000ULL, // RANK_5
        0x0000FF0000000000ULL, // RANK_6
        0x00FF000000000000ULL, // RANK_7
        0xFF00000000000000ULL  // RANK_8
    };

    // File masks - each bit in the corresponding file is set
    const std::array<Bitboard, 8> FILE_MASKS = {
        0x0101010101010101ULL, // FILE_A
        0x0202020202020202ULL, // FILE_B
        0x0404040404040404ULL, // FILE_C
        0x0808080808080808ULL, // FILE_D
        0x1010101010101010ULL, // FILE_E
        0x2020202020202020ULL, // FILE_F
        0x4040404040404040ULL, // FILE_G
        0x8080808080808080ULL  // FILE_H
    };

    // Diagonal masks (from bottom-left to top-right)
    const std::array<Bitboard, 15> DIAGONAL_MASKS = {
        0x0000000000000080ULL, // A8
        0x0000000000008040ULL, // A7-B8
        0x0000000000804020ULL, // A6-C8
        0x0000000080402010ULL, // A5-D8
        0x0000008040201008ULL, // A4-E8
        0x0000804020100804ULL, // A3-F8
        0x0080402010080402ULL, // A2-G8
        0x8040201008040201ULL, // A1-H8
        0x4020100804020100ULL, // B1-H7
        0x2010080402010000ULL, // C1-H6
        0x1008040201000000ULL, // D1-H5
        0x0804020100000000ULL, // E1-H4
        0x0402010000000000ULL, // F1-H3
        0x0201000000000000ULL, // G1-H2
        0x0100000000000000ULL  // H1
    };

    // Anti-diagonal masks (from bottom-right to top-left)
    const std::array<Bitboard, 15> ANTI_DIAGONAL_MASKS = {
        0x0000000000000001ULL, // H8
        0x0000000000000102ULL, // G8-H7
        0x0000000000010204ULL, // F8-H6
        0x0000000001020408ULL, // E8-H5
        0x0000000102040810ULL, // D8-H4
        0x0000010204081020ULL, // C8-H3
        0x0001020408102040ULL, // B8-H2
        0x0102040810204080ULL, // A8-H1
        0x0204081020408000ULL, // A7-G1
        0x0408102040800000ULL, // A6-F1
        0x0810204080000000ULL, // A5-E1
        0x1020408000000000ULL, // A4-D1
        0x2040800000000000ULL, // A3-C1
        0x4080000000000000ULL, // A2-B1
        0x8000000000000000ULL  // A1
    };

    // Pre-computed attack patterns for pieces
    std::array<Bitboard, 64> KNIGHT_ATTACKS;
    std::array<Bitboard, 64> KING_ATTACKS;
    std::array<std::array<Bitboard, 64>, 2> PAWN_ATTACKS;
}

namespace BitboardUtils {
    // Print a bitboard as an 8x8 grid
    std::string prettyPrint(Bitboard bb) {
        std::stringstream ss;
        ss << "  +---+---+---+---+---+---+---+---+" << std::endl;
        
        for (int rank = 7; rank >= 0; --rank) {
            ss << (rank + 1) << " |";
            
            for (int file = 0; file < 8; ++file) {
                Square sq = squareFromRankFile(static_cast<Rank>(rank), static_cast<File>(file));
                ss << " " << (testBit(bb, sq) ? "X" : " ") << " |";
            }
            
            ss << std::endl << "  +---+---+---+---+---+---+---+---+" << std::endl;
        }
        
        ss << "    a   b   c   d   e   f   g   h" << std::endl;
        return ss.str();
    }
    
    // Count the number of set bits (population count)
    int popCount(Bitboard bb) {
        int count = 0;
        while (bb) {
            bb &= bb - 1; // Clear the least significant bit set
            count++;
        }
        return count;
    }
    
    // Get index of least significant bit
    Square lsb(Bitboard bb) {
        if (bb == 0) return NO_SQUARE;
        
        // Use compiler intrinsics if available (GCC, Clang)
        #if defined(__GNUC__) || defined(__clang__)
            return static_cast<Square>(__builtin_ctzll(bb));
        #else
            // Fallback implementation
            unsigned long index;
            Bitboard isolated = bb & -bb; // Isolate the least significant bit
            
            // De Bruijn sequence for finding the index of the least significant bit
            static const int INDEX64[64] = {
                0,  1,  48,  2, 57, 49, 28,  3,
                61, 58, 50, 42, 38, 29, 17,  4,
                62, 55, 59, 36, 53, 51, 43, 22,
                45, 39, 33, 30, 24, 18, 12,  5,
                63, 47, 56, 27, 60, 41, 37, 16,
                54, 35, 52, 21, 44, 32, 23, 11,
                46, 26, 40, 15, 34, 20, 31, 10,
                25, 14, 19,  9, 13,  8,  7,  6
            };
            
            // Use magic number for De Bruijn multiplication
            index = ((isolated * 0x03f79d71b4cb0a89ULL) >> 58);
            return static_cast<Square>(INDEX64[index]);
        #endif
    }
    
    // Get index of most significant bit
    Square msb(Bitboard bb) {
        if (bb == 0) return NO_SQUARE;
        
        // Use compiler intrinsics if available (GCC, Clang)
        #if defined(__GNUC__) || defined(__clang__)
            return static_cast<Square>(63 - __builtin_clzll(bb));
        #else
            // Fallback implementation
            static const int INDEX64[64] = {
                0, 47,  1, 56, 48, 27,  2, 60,
                57, 49, 41, 37, 28, 16,  3, 61,
                54, 58, 35, 52, 50, 42, 21, 44,
                38, 32, 29, 23, 17, 11,  4, 62,
                46, 55, 26, 59, 40, 36, 15, 53,
                34, 51, 20, 43, 31, 22, 10, 45,
                25, 39, 14, 33, 19, 30,  9, 24,
                13, 18,  8, 12,  7,  6,  5, 63
            };
            
            bb |= bb >> 1;
            bb |= bb >> 2;
            bb |= bb >> 4;
            bb |= bb >> 8;
            bb |= bb >> 16;
            bb |= bb >> 32;
            
            return static_cast<Square>(INDEX64[(bb * 0x03f79d71b4cb0a89ULL) >> 58]);
        #endif
    }
    
    // Generate knight attack bitboard for a given square
    Bitboard generateKnightAttacks(Square sq) {
        Bitboard attacks = 0;
        Bitboard bb = 1ULL << sq;
        
        attacks |= (bb << 17) & ~BitboardConstants::FILE_MASKS[FILE_A];     // NNE
        attacks |= (bb << 10) & ~(BitboardConstants::FILE_MASKS[FILE_A] | BitboardConstants::FILE_MASKS[FILE_B]); // ENE
        attacks |= (bb >>  6) & ~(BitboardConstants::FILE_MASKS[FILE_A] | BitboardConstants::FILE_MASKS[FILE_B]); // ESE
        attacks |= (bb >> 15) & ~BitboardConstants::FILE_MASKS[FILE_A];     // SSE
        attacks |= (bb >> 17) & ~BitboardConstants::FILE_MASKS[FILE_H];     // SSW
        attacks |= (bb >> 10) & ~(BitboardConstants::FILE_MASKS[FILE_G] | BitboardConstants::FILE_MASKS[FILE_H]); // WSW
        attacks |= (bb <<  6) & ~(BitboardConstants::FILE_MASKS[FILE_G] | BitboardConstants::FILE_MASKS[FILE_H]); // WNW
        attacks |= (bb << 15) & ~BitboardConstants::FILE_MASKS[FILE_H];     // NNW
        
        return attacks;
    }
    
    // Generate king attack bitboard for a given square
    Bitboard generateKingAttacks(Square sq) {
        Bitboard attacks = 0;
        Bitboard bb = 1ULL << sq;
        
        attacks |= (bb << 8);  // N
        attacks |= (bb << 9) & ~BitboardConstants::FILE_MASKS[FILE_A];  // NE
        attacks |= (bb << 1) & ~BitboardConstants::FILE_MASKS[FILE_A];  // E
        attacks |= (bb >> 7) & ~BitboardConstants::FILE_MASKS[FILE_A];  // SE
        attacks |= (bb >> 8);  // S
        attacks |= (bb >> 9) & ~BitboardConstants::FILE_MASKS[FILE_H];  // SW
        attacks |= (bb >> 1) & ~BitboardConstants::FILE_MASKS[FILE_H];  // W
        attacks |= (bb << 7) & ~BitboardConstants::FILE_MASKS[FILE_H];  // NW
        
        return attacks;
    }
    
    // Generate pawn attack bitboard for a given square and color
    Bitboard generatePawnAttacks(Square sq, Color color) {
        Bitboard attacks = 0;
        Bitboard bb = 1ULL << sq;
        
        if (color == WHITE) {
            attacks |= (bb << 9) & ~BitboardConstants::FILE_MASKS[FILE_A];  // NE capture
            attacks |= (bb << 7) & ~BitboardConstants::FILE_MASKS[FILE_H];  // NW capture
        } else {
            attacks |= (bb >> 7) & ~BitboardConstants::FILE_MASKS[FILE_A];  // SE capture
            attacks |= (bb >> 9) & ~BitboardConstants::FILE_MASKS[FILE_H];  // SW capture
        }
        
        return attacks;
    }
    
    // Initialize all attack tables and masks
    void initBitboards() {
        // Initialize knight attacks
        for (int sqInt = A1; sqInt < NO_SQUARE; ++sqInt) {
            Square sq = static_cast<Square>(sqInt);
            BitboardConstants::KNIGHT_ATTACKS[sq] = generateKnightAttacks(sq);
        }
        
        // Initialize king attacks
        for (int sqInt = A1; sqInt < NO_SQUARE; ++sqInt) {
            Square sq = static_cast<Square>(sqInt);
            BitboardConstants::KING_ATTACKS[sq] = generateKingAttacks(sq);
        }
        
        // Initialize pawn attacks for both colors
        for (int sqInt = A1; sqInt < NO_SQUARE; ++sqInt) {
            Square sq = static_cast<Square>(sqInt);
            BitboardConstants::PAWN_ATTACKS[WHITE][sq] = generatePawnAttacks(sq, WHITE);
            BitboardConstants::PAWN_ATTACKS[BLACK][sq] = generatePawnAttacks(sq, BLACK);
        }
    }
}