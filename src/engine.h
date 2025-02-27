#ifndef ENGINE_H
#define ENGINE_H

#include "board.h"
#include <string>
#include <random>
#include <functional>

// Function type for move selection strategy
using MoveSelectionFunc = std::function<Move(const std::vector<Move>&, const Board&)>;

// Engine class that handles move selection and UCI protocol
class Engine {
public:
    // Constructor
    Engine();
    
    // Set the current board position
    void setPosition(const Board& board);
    
    // Get the current board position
    const Board& getPosition() const;
    
    // Select and make a move using the current strategy
    Move makeMove();
    
    // Set a custom move selection strategy
    void setMoveSelectionStrategy(MoveSelectionFunc strategy);
    
    // Default move selection strategies
    static Move randomMove(const std::vector<Move>& legalMoves, const Board& board);
    
    // Future: Add strategy that uses RL model
    // static Move modelBasedMove(const std::vector<Move>& legalMoves, const Board& board);
    
private:
    // Current board position
    Board _board;
    
    // Current move selection strategy
    MoveSelectionFunc _moveSelectionStrategy;
    
    // Random number generator for random move selection
    std::mt19937 _rng;
};

#endif // ENGINE_H