#include "movegen.h"
#include "board.h"
#include <algorithm>

// Generate sliding piece attacks
Bitboard generateRookAttacks(Square sq, Bitboard occupied) {
    Bitboard attacks = 0ULL;
    
    // North
    for (int r = BitboardUtils::squareRank(sq) + 1; r <= RANK_8; ++r) {
        Square s = BitboardUtils::squareFromRankFile(static_cast<Rank>(r), BitboardUtils::squareFile(sq));
        attacks |= (1ULL << s);
        if (occupied & (1ULL << s)) break;
    }
    
    // South
    for (int r = BitboardUtils::squareRank(sq) - 1; r >= RANK_1; --r) {
        Square s = BitboardUtils::squareFromRankFile(static_cast<Rank>(r), BitboardUtils::squareFile(sq));
        attacks |= (1ULL << s);
        if (occupied & (1ULL << s)) break;
    }
    
    // East
    for (int f = BitboardUtils::squareFile(sq) + 1; f <= FILE_H; ++f) {
        Square s = BitboardUtils::squareFromRankFile(BitboardUtils::squareRank(sq), static_cast<File>(f));
        attacks |= (1ULL << s);
        if (occupied & (1ULL << s)) break;
    }
    
    // West
    for (int f = BitboardUtils::squareFile(sq) - 1; f >= FILE_A; --f) {
        Square s = BitboardUtils::squareFromRankFile(BitboardUtils::squareRank(sq), static_cast<File>(f));
        attacks |= (1ULL << s);
        if (occupied & (1ULL << s)) break;
    }
    
    return attacks;
}

Bitboard generateBishopAttacks(Square sq, Bitboard occupied) {
    Bitboard attacks = 0ULL;
    
    // Northeast
    for (int r = BitboardUtils::squareRank(sq) + 1, f = BitboardUtils::squareFile(sq) + 1; 
         r <= RANK_8 && f <= FILE_H; ++r, ++f) {
        Square s = BitboardUtils::squareFromRankFile(static_cast<Rank>(r), static_cast<File>(f));
        attacks |= (1ULL << s);
        if (occupied & (1ULL << s)) break;
    }
    
    // Southeast
    for (int r = BitboardUtils::squareRank(sq) - 1, f = BitboardUtils::squareFile(sq) + 1; 
         r >= RANK_1 && f <= FILE_H; --r, ++f) {
        Square s = BitboardUtils::squareFromRankFile(static_cast<Rank>(r), static_cast<File>(f));
        attacks |= (1ULL << s);
        if (occupied & (1ULL << s)) break;
    }
    
    // Southwest
    for (int r = BitboardUtils::squareRank(sq) - 1, f = BitboardUtils::squareFile(sq) - 1; 
         r >= RANK_1 && f >= FILE_A; --r, --f) {
        Square s = BitboardUtils::squareFromRankFile(static_cast<Rank>(r), static_cast<File>(f));
        attacks |= (1ULL << s);
        if (occupied & (1ULL << s)) break;
    }
    
    // Northwest
    for (int r = BitboardUtils::squareRank(sq) + 1, f = BitboardUtils::squareFile(sq) - 1; 
         r <= RANK_8 && f >= FILE_A; ++r, --f) {
        Square s = BitboardUtils::squareFromRankFile(static_cast<Rank>(r), static_cast<File>(f));
        attacks |= (1ULL << s);
        if (occupied & (1ULL << s)) break;
    }
    
    return attacks;
}

// Generate attacks for a specific piece type from a square
Bitboard MoveGenerator::getPieceAttacks(PieceType pieceType, Square sq, Color color, Bitboard occupied) {
    switch (pieceType) {
        case PAWN:
            return BitboardConstants::PAWN_ATTACKS[color][sq];
        case KNIGHT:
            return BitboardConstants::KNIGHT_ATTACKS[sq];
        case BISHOP:
            return generateBishopAttacks(sq, occupied);
        case ROOK:
            return generateRookAttacks(sq, occupied);
        case QUEEN:
            return generateBishopAttacks(sq, occupied) | generateRookAttacks(sq, occupied);
        case KING:
            return BitboardConstants::KING_ATTACKS[sq];
        default:
            return 0ULL;
    }
}

// Generate all squares attacked by a specific color
Bitboard MoveGenerator::getAttackedSquares(const Board& board, Color attackingColor) {
    Bitboard attacks = 0ULL;
    Bitboard occupied = board._occupiedSquares;
    
    // Pawn attacks
    Bitboard pawns = board._pieces[attackingColor][PAWN];
    while (pawns) {
        Square sq = BitboardUtils::lsb(pawns);
        attacks |= BitboardConstants::PAWN_ATTACKS[attackingColor][sq];
        pawns &= pawns - 1;
    }
    
    // Knight attacks
    Bitboard knights = board._pieces[attackingColor][KNIGHT];
    while (knights) {
        Square sq = BitboardUtils::lsb(knights);
        attacks |= BitboardConstants::KNIGHT_ATTACKS[sq];
        knights &= knights - 1;
    }
    
    // Bishop attacks
    Bitboard bishops = board._pieces[attackingColor][BISHOP];
    while (bishops) {
        Square sq = BitboardUtils::lsb(bishops);
        attacks |= generateBishopAttacks(sq, occupied);
        bishops &= bishops - 1;
    }
    
    // Rook attacks
    Bitboard rooks = board._pieces[attackingColor][ROOK];
    while (rooks) {
        Square sq = BitboardUtils::lsb(rooks);
        attacks |= generateRookAttacks(sq, occupied);
        rooks &= rooks - 1;
    }
    
    // Queen attacks
    Bitboard queens = board._pieces[attackingColor][QUEEN];
    while (queens) {
        Square sq = BitboardUtils::lsb(queens);
        attacks |= generateBishopAttacks(sq, occupied) | generateRookAttacks(sq, occupied);
        queens &= queens - 1;
    }
    
    // King attacks
    Bitboard king = board._pieces[attackingColor][KING];
    if (king) {
        Square sq = BitboardUtils::lsb(king);
        attacks |= BitboardConstants::KING_ATTACKS[sq];
    }
    
    return attacks;
}

// Generate all pseudo-legal moves (may include moves that leave the king in check)
std::vector<Move> MoveGenerator::generatePseudoLegalMoves(const Board& board) {
    std::vector<Move> moves;
    Color us = board._sideToMove;
    Bitboard ourPieces = board._allPieces[us];
    Bitboard occupied = board._occupiedSquares;
    
    // Generate pawn moves
    Bitboard pawns = board._pieces[us][PAWN];
    while (pawns) {
        Square from = BitboardUtils::lsb(pawns);
        
        // Regular pawn moves (single and double push)
        int pushDir = (us == WHITE) ? 8 : -8;
        Square to1 = static_cast<Square>(from + pushDir);
        
        if (!(occupied & (1ULL << to1))) {
            // Check for promotion
            if ((us == WHITE && BitboardUtils::squareRank(from) == RANK_7) ||
                (us == BLACK && BitboardUtils::squareRank(from) == RANK_2)) {
                moves.emplace_back(from, to1, QUEEN);
                moves.emplace_back(from, to1, ROOK);
                moves.emplace_back(from, to1, BISHOP);
                moves.emplace_back(from, to1, KNIGHT);
            } else {
                moves.emplace_back(from, to1);
            }
            
            // Double push from starting rank
            if ((us == WHITE && BitboardUtils::squareRank(from) == RANK_2) ||
                (us == BLACK && BitboardUtils::squareRank(from) == RANK_7)) {
                Square to2 = static_cast<Square>(to1 + pushDir);
                if (!(occupied & (1ULL << to2))) {
                    moves.emplace_back(from, to2);
                }
            }
        }
        
        // Pawn captures
        Bitboard captures = BitboardConstants::PAWN_ATTACKS[us][from] & board._allPieces[1-us];
        while (captures) {
            Square to = BitboardUtils::lsb(captures);
            
            // Check for promotion
            if ((us == WHITE && BitboardUtils::squareRank(to) == RANK_8) ||
                (us == BLACK && BitboardUtils::squareRank(to) == RANK_1)) {
                moves.emplace_back(from, to, QUEEN);
                moves.emplace_back(from, to, ROOK);
                moves.emplace_back(from, to, BISHOP);
                moves.emplace_back(from, to, KNIGHT);
            } else {
                moves.emplace_back(from, to);
            }
            
            captures &= captures - 1;
        }
        
        // En passant captures
        if (board._enPassantSquare != NO_SQUARE) {
            Bitboard epCaptures = BitboardConstants::PAWN_ATTACKS[us][from] & (1ULL << board._enPassantSquare);
            if (epCaptures) {
                moves.emplace_back(from, board._enPassantSquare);
            }
        }
        
        pawns &= pawns - 1;
    }
    
    // Generate knight moves
    Bitboard knights = board._pieces[us][KNIGHT];
    while (knights) {
        Square from = BitboardUtils::lsb(knights);
        Bitboard targets = BitboardConstants::KNIGHT_ATTACKS[from] & ~ourPieces;
        
        while (targets) {
            Square to = BitboardUtils::lsb(targets);
            moves.emplace_back(from, to);
            targets &= targets - 1;
        }
        
        knights &= knights - 1;
    }
    
    // Generate bishop moves
    Bitboard bishops = board._pieces[us][BISHOP];
    while (bishops) {
        Square from = BitboardUtils::lsb(bishops);
        Bitboard targets = generateBishopAttacks(from, occupied) & ~ourPieces;
        
        while (targets) {
            Square to = BitboardUtils::lsb(targets);
            moves.emplace_back(from, to);
            targets &= targets - 1;
        }
        
        bishops &= bishops - 1;
    }
    
    // Generate rook moves
    Bitboard rooks = board._pieces[us][ROOK];
    while (rooks) {
        Square from = BitboardUtils::lsb(rooks);
        Bitboard targets = generateRookAttacks(from, occupied) & ~ourPieces;
        
        while (targets) {
            Square to = BitboardUtils::lsb(targets);
            moves.emplace_back(from, to);
            targets &= targets - 1;
        }
        
        rooks &= rooks - 1;
    }
    
    // Generate queen moves
    Bitboard queens = board._pieces[us][QUEEN];
    while (queens) {
        Square from = BitboardUtils::lsb(queens);
        Bitboard targets = (generateBishopAttacks(from, occupied) | generateRookAttacks(from, occupied)) & ~ourPieces;
        
        while (targets) {
            Square to = BitboardUtils::lsb(targets);
            moves.emplace_back(from, to);
            targets &= targets - 1;
        }
        
        queens &= queens - 1;
    }
    
    // Generate king moves
    Bitboard king = board._pieces[us][KING];
    if (king) {
        Square from = BitboardUtils::lsb(king);
        Bitboard targets = BitboardConstants::KING_ATTACKS[from] & ~ourPieces;
        
        while (targets) {
            Square to = BitboardUtils::lsb(targets);
            moves.emplace_back(from, to);
            targets &= targets - 1;
        }
        
        // Castling
        if (us == WHITE) {
            // Kingside castling
            if ((board._castlingRights & WHITE_OO) &&
                !(occupied & ((1ULL << F1) | (1ULL << G1))) &&
                !board.isSquareAttacked(E1, BLACK) &&
                !board.isSquareAttacked(F1, BLACK)) {
                moves.emplace_back(E1, G1);
            }
            
            // Queenside castling
            if ((board._castlingRights & WHITE_OOO) &&
                !(occupied & ((1ULL << B1) | (1ULL << C1) | (1ULL << D1))) &&
                !board.isSquareAttacked(E1, BLACK) &&
                !board.isSquareAttacked(D1, BLACK)) {
                moves.emplace_back(E1, C1);
            }
        } else {
            // Kingside castling
            if ((board._castlingRights & BLACK_OO) &&
                !(occupied & ((1ULL << F8) | (1ULL << G8))) &&
                !board.isSquareAttacked(E8, WHITE) &&
                !board.isSquareAttacked(F8, WHITE)) {
                moves.emplace_back(E8, G8);
            }
            
            // Queenside castling
            if ((board._castlingRights & BLACK_OOO) &&
                !(occupied & ((1ULL << B8) | (1ULL << C8) | (1ULL << D8))) &&
                !board.isSquareAttacked(E8, WHITE) &&
                !board.isSquareAttacked(D8, WHITE)) {
                moves.emplace_back(E8, C8);
            }
        }
    }
    
    return moves;
}

// Filter out illegal moves that leave the king in check
std::vector<Move> MoveGenerator::filterLegalMoves(const Board& board, const std::vector<Move>& pseudoLegalMoves) {
    std::vector<Move> legalMoves;
    
    for (const Move& move : pseudoLegalMoves) {
        // Make a copy of the board
        Board boardCopy = board;
        
        // Try the move
        if (boardCopy.makeMove(move)) {
            // If the king is not in check after the move, it's legal
            legalMoves.push_back(move);
        }
    }
    
    return legalMoves;
}

// Generate all legal moves for the current position
std::vector<Move> MoveGenerator::generateLegalMoves(const Board& board) {
    std::vector<Move> pseudoLegalMoves = generatePseudoLegalMoves(board);
    return filterLegalMoves(board, pseudoLegalMoves);
}

// Check if the king is in check
bool MoveGenerator::isKingInCheck(const Board& board, Color kingColor) {
    Square kingSquare = board.findKing(kingColor);
    return kingSquare != NO_SQUARE && board.isSquareAttacked(kingSquare, Color(1 - kingColor));
}