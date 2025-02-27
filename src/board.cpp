#include "board.h"
#include "movegen.h"
#include <sstream>
#include <cctype>
#include <unordered_map>

// Convert move to UCI string
std::string Move::toUci() const {
    if (!isValid()) {
        return "0000";
    }
    
    std::string uci;
    
    // Add source square
    uci += 'a' + BitboardUtils::squareFile(from);
    uci += '1' + BitboardUtils::squareRank(from);
    
    // Add destination square
    uci += 'a' + BitboardUtils::squareFile(to);
    uci += '1' + BitboardUtils::squareRank(to);
    
    // Add promotion piece if applicable
    if (promotion != NO_PIECE_TYPE) {
        switch (promotion) {
            case QUEEN:  uci += 'q'; break;
            case ROOK:   uci += 'r'; break;
            case BISHOP: uci += 'b'; break;
            case KNIGHT: uci += 'n'; break;
            default: break;
        }
    }
    
    return uci;
}

// Create move from UCI string
Move Move::fromUci(const std::string& uci) {
    if (uci.length() < 4 || uci.length() > 5) {
        return Move(); // Invalid UCI string
    }
    
    // Parse source square
    File fromFile = static_cast<File>(uci[0] - 'a');
    Rank fromRank = static_cast<Rank>(uci[1] - '1');
    
    // Parse destination square
    File toFile = static_cast<File>(uci[2] - 'a');
    Rank toRank = static_cast<Rank>(uci[3] - '1');
    
    // Check if file and rank are valid
    if (fromFile < FILE_A || fromFile > FILE_H || fromRank < RANK_1 || fromRank > RANK_8 ||
        toFile < FILE_A || toFile > FILE_H || toRank < RANK_1 || toRank > RANK_8) {
        return Move(); // Invalid coordinates
    }
    
    // Create squares
    Square from = BitboardUtils::squareFromRankFile(fromRank, fromFile);
    Square to = BitboardUtils::squareFromRankFile(toRank, toFile);
    
    // Parse promotion piece if present
    PieceType promotion = NO_PIECE_TYPE;
    if (uci.length() == 5) {
        switch (uci[4]) {
            case 'q': promotion = QUEEN; break;
            case 'r': promotion = ROOK; break;
            case 'b': promotion = BISHOP; break;
            case 'n': promotion = KNIGHT; break;
            default: return Move(); // Invalid promotion piece
        }
    }
    
    return Move(from, to, promotion);
}

// Constructor for Board
Board::Board() : _chess960(false) {
    reset();
}

// Reset board to starting position
void Board::reset() {
    // Clear all bitboards
    for (int color = WHITE; color <= BLACK; ++color) {
        for (int piece = PAWN; piece <= KING; ++piece) {
            _pieces[color][piece] = 0ULL;
        }
        _allPieces[color] = 0ULL;
    }
    _occupiedSquares = 0ULL;
    
    // Set up pawns
    _pieces[WHITE][PAWN] = 0x000000000000FF00ULL; // Rank 2
    _pieces[BLACK][PAWN] = 0x00FF000000000000ULL; // Rank 7
    
    // Set up knights
    _pieces[WHITE][KNIGHT] = 0x0000000000000042ULL; // B1, G1
    _pieces[BLACK][KNIGHT] = 0x4200000000000000ULL; // B8, G8
    
    // Set up bishops
    _pieces[WHITE][BISHOP] = 0x0000000000000024ULL; // C1, F1
    _pieces[BLACK][BISHOP] = 0x2400000000000000ULL; // C8, F8
    
    // Set up rooks
    _pieces[WHITE][ROOK] = 0x0000000000000081ULL; // A1, H1
    _pieces[BLACK][ROOK] = 0x8100000000000000ULL; // A8, H8
    
    // Set up queens
    _pieces[WHITE][QUEEN] = 0x0000000000000008ULL; // D1
    _pieces[BLACK][QUEEN] = 0x0800000000000000ULL; // D8
    
    // Set up kings
    _pieces[WHITE][KING] = 0x0000000000000010ULL; // E1
    _pieces[BLACK][KING] = 0x1000000000000000ULL; // E8
    
    // Update combined bitboards
    updateCombinedBitboards();
    
    // Set up game state
    _sideToMove = WHITE;
    _castlingRights = ANY_CASTLING;
    _enPassantSquare = NO_SQUARE;
    _halfmoveClock = 0;
    _fullmoveNumber = 1;
}

// Update all combined bitboards
void Board::updateCombinedBitboards() {
    // Update combined bitboards for each color
    for (int color = WHITE; color <= BLACK; ++color) {
        _allPieces[color] = 0ULL;
        for (int piece = PAWN; piece <= KING; ++piece) {
            _allPieces[color] |= _pieces[color][piece];
        }
    }
    
    // Update occupied squares
    _occupiedSquares = _allPieces[WHITE] | _allPieces[BLACK];
}

// Get the piece at a given square
PieceType Board::pieceAt(Square sq, Color& color) const {
    Bitboard bb = 1ULL << sq;
    
    // Check if square is occupied by white piece
    if (_allPieces[WHITE] & bb) {
        color = WHITE;
        
        for (int piece = PAWN; piece <= KING; ++piece) {
            if (_pieces[WHITE][piece] & bb) {
                return static_cast<PieceType>(piece);
            }
        }
    }
    // Check if square is occupied by black piece
    else if (_allPieces[BLACK] & bb) {
        color = BLACK;
        
        for (int piece = PAWN; piece <= KING; ++piece) {
            if (_pieces[BLACK][piece] & bb) {
                return static_cast<PieceType>(piece);
            }
        }
    }
    
    // Square is empty
    color = NO_COLOR;
    return NO_PIECE_TYPE;
}

// Make a move on the board
bool Board::makeMove(const Move& move) {
    if (!move.isValid()) {
        return false;
    }
    
    // Get moving piece
    Color movingColor;
    PieceType movingPiece = pieceAt(move.from, movingColor);
    
    // Make sure the piece belongs to the side to move
    if (movingColor != _sideToMove) {
        return false;
    }
    
    // Get captured piece
    Color capturedColor;
    PieceType capturedPiece = pieceAt(move.to, capturedColor);
    
    // Check if move is a capture
    bool isCapture = (capturedColor != NO_COLOR);
    
    // Handle special move: Castling
    if (movingPiece == KING) {
        // Kingside castling
        if (move.from == E1 && move.to == G1 && _sideToMove == WHITE) {
            if (!(_castlingRights & WHITE_OO) || 
                (_occupiedSquares & ((1ULL << F1) | (1ULL << G1))) ||
                isInCheck(WHITE) || isSquareAttacked(F1, BLACK)) {
                return false;
            }
            
            // Move king and rook
            _pieces[WHITE][KING] &= ~(1ULL << E1);
            _pieces[WHITE][KING] |= (1ULL << G1);
            _pieces[WHITE][ROOK] &= ~(1ULL << H1);
            _pieces[WHITE][ROOK] |= (1ULL << F1);
            
            // Update castle rights
            _castlingRights &= ~(WHITE_OO | WHITE_OOO);
        }
        // Queenside castling for white
        else if (move.from == E1 && move.to == C1 && _sideToMove == WHITE) {
            if (!(_castlingRights & WHITE_OOO) || 
                (_occupiedSquares & ((1ULL << B1) | (1ULL << C1) | (1ULL << D1))) ||
                isInCheck(WHITE) || isSquareAttacked(D1, BLACK)) {
                return false;
            }
            
            // Move king and rook
            _pieces[WHITE][KING] &= ~(1ULL << E1);
            _pieces[WHITE][KING] |= (1ULL << C1);
            _pieces[WHITE][ROOK] &= ~(1ULL << A1);
            _pieces[WHITE][ROOK] |= (1ULL << D1);
            
            // Update castle rights
            _castlingRights &= ~(WHITE_OO | WHITE_OOO);
        }
        // Kingside castling for black
        else if (move.from == E8 && move.to == G8 && _sideToMove == BLACK) {
            if (!(_castlingRights & BLACK_OO) || 
                (_occupiedSquares & ((1ULL << F8) | (1ULL << G8))) ||
                isInCheck(BLACK) || isSquareAttacked(F8, WHITE)) {
                return false;
            }
            
            // Move king and rook
            _pieces[BLACK][KING] &= ~(1ULL << E8);
            _pieces[BLACK][KING] |= (1ULL << G8);
            _pieces[BLACK][ROOK] &= ~(1ULL << H8);
            _pieces[BLACK][ROOK] |= (1ULL << F8);
            
            // Update castle rights
            _castlingRights &= ~(BLACK_OO | BLACK_OOO);
        }
        // Queenside castling for black
        else if (move.from == E8 && move.to == C8 && _sideToMove == BLACK) {
            if (!(_castlingRights & BLACK_OOO) || 
                (_occupiedSquares & ((1ULL << B8) | (1ULL << C8) | (1ULL << D8))) ||
                isInCheck(BLACK) || isSquareAttacked(D8, WHITE)) {
                return false;
            }
            
            // Move king and rook
            _pieces[BLACK][KING] &= ~(1ULL << E8);
            _pieces[BLACK][KING] |= (1ULL << C8);
            _pieces[BLACK][ROOK] &= ~(1ULL << A8);
            _pieces[BLACK][ROOK] |= (1ULL << D8);
            
            // Update castle rights
            _castlingRights &= ~(BLACK_OO | BLACK_OOO);
        }
        // Regular king move
        else {
            // Update castling rights when king moves
            if (_sideToMove == WHITE) {
                _castlingRights &= ~(WHITE_OO | WHITE_OOO);
            } else {
                _castlingRights &= ~(BLACK_OO | BLACK_OOO);
            }
            
            // Move piece
            _pieces[movingColor][movingPiece] &= ~(1ULL << move.from);
            if (isCapture) {
                _pieces[capturedColor][capturedPiece] &= ~(1ULL << move.to);
            }
            _pieces[movingColor][movingPiece] |= (1ULL << move.to);
        }
    }
    // Handle en passant capture
    else if (movingPiece == PAWN && move.to == _enPassantSquare) {
        Square capturedPawnSquare = (_sideToMove == WHITE) ? 
            static_cast<Square>(_enPassantSquare - 8) : 
            static_cast<Square>(_enPassantSquare + 8);
        
        // Remove captured pawn
        _pieces[1 - _sideToMove][PAWN] &= ~(1ULL << capturedPawnSquare);
        
        // Move pawn
        _pieces[movingColor][movingPiece] &= ~(1ULL << move.from);
        _pieces[movingColor][movingPiece] |= (1ULL << move.to);
        
        _enPassantSquare = NO_SQUARE;
    }
    // Handle pawn promotion
    else if (movingPiece == PAWN && move.promotion != NO_PIECE_TYPE) {
        // Remove pawn from source
        _pieces[movingColor][PAWN] &= ~(1ULL << move.from);
        
        // Remove captured piece if any
        if (isCapture) {
            _pieces[capturedColor][capturedPiece] &= ~(1ULL << move.to);
        }
        
        // Add promoted piece
        _pieces[movingColor][move.promotion] |= (1ULL << move.to);
    }
    // Handle pawn double push
    else if (movingPiece == PAWN && 
             abs(static_cast<int>(BitboardUtils::squareRank(move.to)) - 
                 static_cast<int>(BitboardUtils::squareRank(move.from))) == 2) {
        
        // Move pawn
        _pieces[movingColor][movingPiece] &= ~(1ULL << move.from);
        _pieces[movingColor][movingPiece] |= (1ULL << move.to);
        
        // Set en passant square
        _enPassantSquare = (_sideToMove == WHITE) ? 
            static_cast<Square>(move.from + 8) : 
            static_cast<Square>(move.from - 8);
    }
    // Regular piece move
    else {
        // Update castling rights when rook moves
        if (movingPiece == ROOK) {
            if (_sideToMove == WHITE) {
                if (move.from == A1) _castlingRights &= ~WHITE_OOO;
                if (move.from == H1) _castlingRights &= ~WHITE_OO;
            } else {
                if (move.from == A8) _castlingRights &= ~BLACK_OOO;
                if (move.from == H8) _castlingRights &= ~BLACK_OO;
            }
        }
        
        // Check for rook captures
        if (isCapture && capturedPiece == ROOK) {
            if (capturedColor == WHITE) {
                if (move.to == A1) _castlingRights &= ~WHITE_OOO;
                if (move.to == H1) _castlingRights &= ~WHITE_OO;
            } else {
                if (move.to == A8) _castlingRights &= ~BLACK_OOO;
                if (move.to == H8) _castlingRights &= ~BLACK_OO;
            }
        }
        
        // Move piece
        _pieces[movingColor][movingPiece] &= ~(1ULL << move.from);
        if (isCapture) {
            _pieces[capturedColor][capturedPiece] &= ~(1ULL << move.to);
        }
        _pieces[movingColor][movingPiece] |= (1ULL << move.to);
        
        // Reset en passant square
        _enPassantSquare = NO_SQUARE;
    }
    
    // Update bitboards
    updateCombinedBitboards();
    
    // Verify king not in check
    if (isInCheck(_sideToMove)) {
        // Illegal move (would need move undo)
        return false;
    }
    
    // Update game state
    _sideToMove = (_sideToMove == WHITE) ? BLACK : WHITE;
    
    // Update halfmove clock
    if (movingPiece == PAWN || isCapture) {
        _halfmoveClock = 0;
    } else {
        _halfmoveClock++;
    }
    
    // Update fullmove number
    if (_sideToMove == WHITE) {
        _fullmoveNumber++;
    }
    
    return true;
}

// Generate all legal moves
std::vector<Move> Board::generateLegalMoves() const {
    return MoveGenerator::generateLegalMoves(*this);
}

// Check if the current position is a checkmate
bool Board::isCheckmate() const {
    return isInCheck(_sideToMove) && generateLegalMoves().empty();
}

// Check if the current position is a stalemate
bool Board::isStalemate() const {
    return !isInCheck(_sideToMove) && generateLegalMoves().empty();
}

// Check if a square is attacked by a given color
bool Board::isSquareAttacked(Square sq, Color attackingColor) const {
    // TODO: That first 1 - attackingColor is kinda sus...
    return MoveGenerator::getPieceAttacks(PAWN, sq, Color(1 - attackingColor), _occupiedSquares) & _pieces[attackingColor][PAWN] ||
           MoveGenerator::getPieceAttacks(KNIGHT, sq, NO_COLOR, _occupiedSquares) & _pieces[attackingColor][KNIGHT] ||
           MoveGenerator::getPieceAttacks(BISHOP, sq, NO_COLOR, _occupiedSquares) & _pieces[attackingColor][BISHOP] ||
           MoveGenerator::getPieceAttacks(ROOK, sq, NO_COLOR, _occupiedSquares) & _pieces[attackingColor][ROOK] ||
           MoveGenerator::getPieceAttacks(QUEEN, sq, NO_COLOR, _occupiedSquares) & _pieces[attackingColor][QUEEN] ||
           MoveGenerator::getPieceAttacks(KING, sq, NO_COLOR, _occupiedSquares) & _pieces[attackingColor][KING];
}

// Check if the king of a given color is in check
bool Board::isInCheck(Color color) const {
    Square kingSquare = findKing(color);
    return kingSquare != NO_SQUARE && isSquareAttacked(kingSquare, Color(1 - color));
}

// Find the king square for a given color
Square Board::findKing(Color color) const {
    return BitboardUtils::lsb(_pieces[color][KING]);
}

// Set the board from a FEN string
bool Board::setFromFen(const std::string& fen) {
    // Clear the board
    for (int color = WHITE; color <= BLACK; ++color) {
        for (int piece = PAWN; piece <= KING; ++piece) {
            _pieces[color][piece] = 0ULL;
        }
        _allPieces[color] = 0ULL;
    }
    _occupiedSquares = 0ULL;
    
    std::istringstream ss(fen);
    std::string board, activeColor, castling, enPassant, halfmove, fullmove;
    
    // Parse fields
    ss >> board >> activeColor >> castling >> enPassant >> halfmove >> fullmove;
    
    // Validate FEN
    if (board.empty() || activeColor.empty() || castling.empty() || 
        enPassant.empty() || halfmove.empty() || fullmove.empty()) {
        return false;
    }
    
    // Parse board setup
    int rank = 7;
    int file = 0;
    
    for (char c : board) {
        if (c == '/') {
            rank--;
            file = 0;
        } else if (isdigit(c)) {
            file += c - '0';
        } else {
            Square sq = BitboardUtils::squareFromRankFile(static_cast<Rank>(rank), static_cast<File>(file));
            
            Color color = isupper(c) ? WHITE : BLACK;
            PieceType pieceType;
            
            switch (tolower(c)) {
                case 'p': pieceType = PAWN; break;
                case 'n': pieceType = KNIGHT; break;
                case 'b': pieceType = BISHOP; break;
                case 'r': pieceType = ROOK; break;
                case 'q': pieceType = QUEEN; break;
                case 'k': pieceType = KING; break;
                default: return false; // Invalid piece
            }
            
            _pieces[color][pieceType] |= (1ULL << sq);
            file++;
        }
    }
    
    // Parse side to move
    _sideToMove = (activeColor == "w") ? WHITE : BLACK;
    
    // Parse castling rights
    _castlingRights = 0;
    if (castling != "-") {
        if (_chess960) {
            // In Chess960, castling rights are denoted by the files of the rooks
            for (char c : castling) {
                if (c >= 'A' && c <= 'H') {
                    // Uppercase letters for white's rooks
                    int rookFile = c - 'A';
                    //_castlingRights |= (rookFile < _kingFile[WHITE] ? WHITE_OOO : WHITE_OO);
                    _castlingRights |= NO_CASTLING;
                }
                else if (c >= 'a' && c <= 'h') {
                    // Lowercase letters for black's rooks
                    int rookFile = c - 'a';
                    //_castlingRights |= (rookFile < _kingFile[BLACK] ? BLACK_OOO : BLACK_OO);
                    _castlingRights |= NO_CASTLING;
                }
                else {
                    return false; // Invalid castling right
                }
            }
        }
        else {
            // Standard chess notation
            for (char c : castling) {
                switch (c) {
                case 'K': _castlingRights |= WHITE_OO; break;
                case 'Q': _castlingRights |= WHITE_OOO; break;
                case 'k': _castlingRights |= BLACK_OO; break;
                case 'q': _castlingRights |= BLACK_OOO; break;
                default: return false; // Invalid castling right
                }
            }
        }
    }
    
    // Parse en passant square
    if (enPassant == "-") {
        _enPassantSquare = NO_SQUARE;
    } else {
        File file = static_cast<File>(enPassant[0] - 'a');
        Rank rank = static_cast<Rank>(enPassant[1] - '1');
        _enPassantSquare = BitboardUtils::squareFromRankFile(rank, file);
    }
    
    // Parse halfmove clock and fullmove number
    try {
        _halfmoveClock = std::stoi(halfmove);
        _fullmoveNumber = std::stoi(fullmove);
    } catch (...) {
        return false; // Invalid numbers
    }
    
    // Update combined bitboards
    updateCombinedBitboards();
    
    return true;
}

// Get FEN string representing current position
std::string Board::getFen() const {
    std::ostringstream fen;
    
    // Board setup
    for (int rank = 7; rank >= 0; --rank) {
        int emptyCount = 0;
        
        for (int file = 0; file < 8; ++file) {
            Square sq = BitboardUtils::squareFromRankFile(static_cast<Rank>(rank), static_cast<File>(file));
            Color pieceColor;
            PieceType pieceType = pieceAt(sq, pieceColor);
            
            if (pieceType == NO_PIECE_TYPE) {
                emptyCount++;
            } else {
                if (emptyCount > 0) {
                    fen << emptyCount;
                    emptyCount = 0;
                }
                
                char pieceChar;
                switch (pieceType) {
                    case PAWN: pieceChar = 'p'; break;
                    case KNIGHT: pieceChar = 'n'; break;
                    case BISHOP: pieceChar = 'b'; break;
                    case ROOK: pieceChar = 'r'; break;
                    case QUEEN: pieceChar = 'q'; break;
                    case KING: pieceChar = 'k'; break;
                    default: pieceChar = '?'; break;
                }
                
                if (pieceColor == WHITE) {
                    pieceChar = toupper(pieceChar);
                }
                
                fen << pieceChar;
            }
        }
        
        if (emptyCount > 0) {
            fen << emptyCount;
        }
        
        if (rank > 0) {
            fen << '/';
        }
    }
    
    // Side to move
    fen << (_sideToMove == WHITE ? " w " : " b ");
    
    // Castling rights
    if (_castlingRights == 0) {
        fen << '-';
    } else {
        if (_castlingRights & WHITE_OO) fen << 'K';
        if (_castlingRights & WHITE_OOO) fen << 'Q';
        if (_castlingRights & BLACK_OO) fen << 'k';
        if (_castlingRights & BLACK_OOO) fen << 'q';
    }
    
    // En passant, halfmove clock, fullmove number
    fen << ' ';
    if (_enPassantSquare == NO_SQUARE) {
        fen << '-';
    } else {
        fen << char('a' + BitboardUtils::squareFile(_enPassantSquare));
        fen << char('1' + BitboardUtils::squareRank(_enPassantSquare));
    }
    
    fen << ' ' << _halfmoveClock << ' ' << _fullmoveNumber;
    
    return fen.str();
}

// Check if the game is drawn by insufficient material
bool Board::isInsufficientMaterial() const {
    // Kings only
    if (BitboardUtils::popCount(_occupiedSquares) == 2) {
        return true;
    }
    
    // King + minor piece vs. King
    if (BitboardUtils::popCount(_occupiedSquares) == 3) {
        if (BitboardUtils::popCount(_allPieces[WHITE]) == 1 || 
            BitboardUtils::popCount(_allPieces[BLACK]) == 1) {
            
            Bitboard minorPieces = _pieces[WHITE][KNIGHT] | _pieces[WHITE][BISHOP] |
                                   _pieces[BLACK][KNIGHT] | _pieces[BLACK][BISHOP];
            return BitboardUtils::popCount(minorPieces) == 1;
        }
    }
    
    // King + bishop vs. King + bishop (same color)
    if (BitboardUtils::popCount(_occupiedSquares) == 4) {
        if (BitboardUtils::popCount(_pieces[WHITE][BISHOP]) == 1 && 
            BitboardUtils::popCount(_pieces[BLACK][BISHOP]) == 1 &&
            BitboardUtils::popCount(_allPieces[WHITE]) == 2 &&
            BitboardUtils::popCount(_allPieces[BLACK]) == 2) {
            
            Square whiteBishopSq = BitboardUtils::lsb(_pieces[WHITE][BISHOP]);
            Square blackBishopSq = BitboardUtils::lsb(_pieces[BLACK][BISHOP]);
            
            bool whiteSquareIsLight = (BitboardUtils::squareRank(whiteBishopSq) + BitboardUtils::squareFile(whiteBishopSq)) % 2 == 0;
            bool blackSquareIsLight = (BitboardUtils::squareRank(blackBishopSq) + BitboardUtils::squareFile(blackBishopSq)) % 2 == 0;
            
            return whiteSquareIsLight == blackSquareIsLight;
        }
    }
    
    return false;
}

// Print the board
std::string Board::toString() const {
    std::ostringstream ss;
    ss << "  +---+---+---+---+---+---+---+---+" << std::endl;
    
    for (int rank = 7; rank >= 0; --rank) {
        ss << (rank + 1) << " |";
        
        for (int file = 0; file < 8; ++file) {
            Square sq = BitboardUtils::squareFromRankFile(static_cast<Rank>(rank), static_cast<File>(file));
            Color pieceColor;
            PieceType pieceType = pieceAt(sq, pieceColor);
            
            ss << " ";
            
            if (pieceType == NO_PIECE_TYPE) {
                ss << " ";
            } else {
                char pieceChar;
                switch (pieceType) {
                    case PAWN: pieceChar = 'P'; break;
                    case KNIGHT: pieceChar = 'N'; break;
                    case BISHOP: pieceChar = 'B'; break;
                    case ROOK: pieceChar = 'R'; break;
                    case QUEEN: pieceChar = 'Q'; break;
                    case KING: pieceChar = 'K'; break;
                    default: pieceChar = '?'; break;
                }
                
                if (pieceColor == BLACK) {
                    pieceChar = tolower(pieceChar);
                }
                
                ss << pieceChar;
            }
            
            ss << " |";
        }
        
        ss << std::endl << "  +---+---+---+---+---+---+---+---+" << std::endl;
    }
    
    ss << "    a   b   c   d   e   f   g   h" << std::endl;
    
    // Additional info
    ss << "Side to move: " << (_sideToMove == WHITE ? "White" : "Black") << std::endl;
    ss << "Castling: " << ((_castlingRights & WHITE_OO) ? "K" : "") 
                        << ((_castlingRights & WHITE_OOO) ? "Q" : "")
                        << ((_castlingRights & BLACK_OO) ? "k" : "")
                        << ((_castlingRights & BLACK_OOO) ? "q" : "")
                        << ((_castlingRights == 0) ? "-" : "") << std::endl;
    
    if (_enPassantSquare != NO_SQUARE) {
        ss << "En passant: " << char('a' + BitboardUtils::squareFile(_enPassantSquare))
                             << char('1' + BitboardUtils::squareRank(_enPassantSquare)) << std::endl;
    }
    
    return ss.str();
}