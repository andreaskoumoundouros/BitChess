#include "engine.h"
#include <algorithm>
#include <random>
#include <chrono>

Engine::Engine() : _rng(std::chrono::system_clock::now().time_since_epoch().count()) {
	// Set default move selection strategy to random
	//_moveSelectionStrategy = std::bind(&Engine::randomMove, std::placeholders::_1, std::placeholders::_2);
#ifdef ENABLE_RL
	_moveSelectionStrategy = std::bind(&modelBasedMove, std::placeholders::_1, std::placeholders::_2);
#else
	_moveSelectionStrategy = std::bind(&Engine::weightedRandomMove, std::placeholders::_1, std::placeholders::_2);
#endif

	// Initialize board to starting position
	_board.reset();
}

void Engine::setPosition(const Board& board) {
	_board = board;
}

const Board& Engine::getPosition() const {
	return _board;
}

Move Engine::makeMove() {
	// Generate legal moves
	std::vector<Move> legalMoves = _board.generateLegalMoves();

	// Check if there are legal moves
	if (legalMoves.empty()) {
		return Move(); // Return invalid move if no legal moves
	}

	// Select a move using the current strategy
	Move selectedMove = _moveSelectionStrategy(legalMoves, _board);

	// Make the selected move on the board
	_board.makeMove(selectedMove);

	return selectedMove;
}

void Engine::setMoveSelectionStrategy(MoveSelectionFunc strategy) {
	_moveSelectionStrategy = strategy;
}

Move Engine::randomMove(const std::vector<Move>& legalMoves, const Board& board) {
	if (legalMoves.empty()) {
		return Move(); // Return invalid move if no legal moves
	}

	// Create a copy of the random number generator to ensure thread safety
	std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());

	// Generate a random index
	std::uniform_int_distribution<size_t> dist(0, legalMoves.size() - 1);
	size_t randomIndex = dist(rng);

	// Return the randomly selected move
	return legalMoves[randomIndex];
}

Move Engine::weightedRandomMove(const std::vector<Move>& legalMoves, const Board& board) {
	if (legalMoves.empty()) {
		return Move(); // Return invalid move if no legal moves
	}

	// Calculate weights for each move
	std::vector<std::pair<Move, int>> weightedMoves;
	for (const Move& move : legalMoves) {
		int weight = evaluateMoveWeight(move, board);
		weightedMoves.push_back({ move, weight });
	}

	// Sort moves by weight in descending order
	std::sort(weightedMoves.begin(), weightedMoves.end(),
		[](const auto& a, const auto& b) { return a.second > b.second; });

	// Create a distribution based on weights
	std::vector<int> weights;
	for (const auto& [move, weight] : weightedMoves) {
		weights.push_back(weight);
	}

	std::discrete_distribution<size_t> dist(weights.begin(), weights.end());

	// Create a copy of the random number generator to ensure thread safety
	std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());

	// Select a move based on the weights
	size_t selectedIndex = dist(rng);

	// Return the selected move
	return weightedMoves[selectedIndex].first;
}

int Engine::evaluateMoveWeight(const Move& move, const Board& board) {
	// Base weight for any move
	int weight = 10;

	// Get the moving piece and potential captured piece before making the move
	Color movingColor;
	PieceType movingPiece = board.pieceAt(move.from, movingColor);

	Color capturedColor;
	PieceType capturedPiece = board.pieceAt(move.to, capturedColor);

	// Check if move is a capture
	if (capturedPiece != NO_PIECE_TYPE) {
		// Weight based on captured piece value
		switch (capturedPiece) {
		case PAWN:   weight += 10; break;
		case KNIGHT: weight += 30; break;
		case BISHOP: weight += 30; break;
		case ROOK:   weight += 50; break;
		case QUEEN:  weight += 90; break;
		default:     break;
		}
	}

	// Check if move is a promotion
	if (movingPiece == PAWN && move.promotion != NO_PIECE_TYPE) {
		switch (move.promotion) {
		case QUEEN:  weight += 80; break;
		case ROOK:   weight += 40; break;
		case BISHOP: weight += 20; break;
		case KNIGHT: weight += 20; break;
		default:     break;
		}
	}
	else
	{
		switch (movingPiece)
		{
			case KNIGHT:
				weight += 25;
				break;
			case BISHOP:
				weight += 20;
				break;
			case ROOK:
				weight += 20;
				break;
			case QUEEN:
				weight += 15;
				break;
			case PAWN:
			{

				// Prioritize pawn moves in early game (first ~5 turns)
				if (board.fullmoveNumber() <= 5) {
					// Higher weight for early pawn moves, especially center pawns
					weight += 50 - (board.fullmoveNumber() * 10); // Decreases from +50 to +10 over 5 moves

					// Additional weight for center pawns (files D and E)
					File fromFile = BitboardUtils::squareFile(move.from);
					if (fromFile == FILE_D || fromFile == FILE_E) {
						weight += 20;
					}
				}
				break;
			}
			case NO_PIECE_TYPE:
				break;
			default:
				break;
		}
	}

	// Create a temporary board to simulate the move
	Board tempBoard = board;

	// Make the move on the temporary board
	if (tempBoard.makeMove(move)) {
		// Check if move puts opponent in check
		if (tempBoard.isInCheck(Color(1 - movingColor))) {
			weight += 40;

			// Check if move is checkmate
			if (tempBoard.isCheckmate()) {
				weight += 1000;  // Extremely high weight for checkmate
			}
		}
	}

	return weight;
}
