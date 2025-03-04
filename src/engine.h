#ifndef ENGINE_H
#define ENGINE_H

#include "board.h"
#include <string>
#include <random>
#include <functional>

// Include chess_rl.h only when RL is enabled
#ifdef ENABLE_RL
#include "chess_rl.h"
#endif

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

	// Add after the randomMove declaration:
	static Move weightedRandomMove(const std::vector<Move>& legalMoves, const Board& board);
    
    
private:
	
    // Add this helper function to evaluate move weights
	static int evaluateMoveWeight(const Move& move, const Board& board);

    // Current board position
    Board _board;
    
    // Current move selection strategy
    MoveSelectionFunc _moveSelectionStrategy;
    
    // Random number generator for random move selection
    std::mt19937 _rng;

};

#endif // ENGINE_H