#include "engine.h"
#include <algorithm>
#include <random>
#include <chrono>

Engine::Engine() : _rng(std::chrono::system_clock::now().time_since_epoch().count()) {
    // Set default move selection strategy to random
    _moveSelectionStrategy = std::bind(&Engine::randomMove, std::placeholders::_1, std::placeholders::_2);
    
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

// Future implementation for RL-based move selection
/*
Move Engine::modelBasedMove(const std::vector<Move>& legalMoves, const Board& board) {
    // This function will use a reinforcement learning model to select the best move
    // For now, it would return a random move as a placeholder
    return randomMove(legalMoves, board);
}
*/