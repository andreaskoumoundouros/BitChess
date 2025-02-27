# BitChess RL

A chess engine designed to use reinforcement learning, built with a bitboard representation for efficient board state modeling.

## Features

- Efficient bitboard representation for fast move generation
- UCI protocol compatibility for integration with chess GUIs and other engines
- RL-ready architecture for future training implementation
- Currently uses random move selection (placeholder for future RL model)

## Building the Project

### Prerequisites

- C++17 compatible compiler
- CMake 3.15 or higher
- GoogleTest for running tests (automatically downloaded by CMake)

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/andreaskoumoundouros/bitchess-rl.git
cd bitchess-rl

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make

# Run tests
make test

# Run the engine
./bitchess
```

## UCI Commands

The engine supports the following Universal Chess Interface (UCI) commands:

- `uci`: Enter UCI mode
- `isready`: Check if engine is ready
- `position [fen | startpos] moves ...`: Set up position
- `go`: Start calculating moves
- `quit`: Exit the program

## Project Structure

- `src/`: Core engine code
  - `bitboard.*`: Bitboard data structures and operations
  - `movegen.*`: Move generation algorithms
  - `board.*`: Chess board representation
  - `uci.*`: UCI protocol implementation
  - `engine.*`: Engine core (will use RL in the future)
  - `main.cpp`: Entry point
- `test/`: Test files
  - Unit tests for all core components

## Future Enhancements

- Reinforcement learning model integration
- Opening book support
- Endgame tablebase integration
- Iterative deepening search
- Evaluation function tuning

## License

MIT License