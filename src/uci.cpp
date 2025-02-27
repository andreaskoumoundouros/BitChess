#include "uci.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

UCI::UCI() : _quit(false) {
    // Initialize board to starting position
    _board.reset();
    
    // Set the engine's position to match
    _engine.setPosition(_board);
}

void UCI::startLoop() {
    std::string line;
    
    while (!_quit && std::getline(std::cin, line)) {
        processCommand(line);
    }
}

void UCI::processCommand(const std::string& command) {
    std::istringstream is(command);
    std::string token;
    
    is >> token;
    
    if (token == "uci") {
        handleUci();
    } else if (token == "setoption") {
        handleSetOption(is);
    } else if (token == "isready") {
        handleIsReady();
    } else if (token == "position") {
        handlePosition(is);
    } else if (token == "go") {
        handleGo(is);
    } else if (token == "stop") {
        handleStop();
    } else if (token == "quit") {
        handleQuit();
    } else if (token == "ucinewgame") {
        handleUciNewGame();
    } else if (token == "printboard") {
        handlePrintBoard();
    }
}

void UCI::handleUci() {
    sendResponse("id name BitChess RL");
    sendResponse("id author AndreasKoumoundouros");
    
    // Send UCI options here if needed
    // sendResponse("option name Hash type spin default 64 min 1 max 1024");
    sendResponse("option name UCI_Chess960 type check default false");
    
    sendResponse("uciok");
}

void UCI::handleSetOption(std::istringstream& is) {
    std::string name, id;

    is >> name >> id;

    // Validating setoption command
    if (name != "name") {
        return;
    }

    std::cout << "Handling: " << id << "\n";

    if (id == "UCI_Chess960") {
        // Handle Chess960 mode
        std::string value, data;
        is >> value >> data;
        if (value != "value") {
            return;
        }

        if (data == "false") {
           _board._chess960 = false;
        }
        else if (data == "true")
        {
            _board._chess960 = true;
        }
    }
}

void UCI::handleIsReady() {
    sendResponse("readyok");
}

void UCI::handlePosition(std::istringstream& is) {
    std::string token;
    is >> token;
    
    if (token == "startpos") {
        _board.reset();
        
        // Check for additional moves
        is >> token;
        if (token == "moves") {
            std::string moves;
            std::getline(is, moves);
            applyMoves(moves);
        }
    } else if (token == "fen") {
        std::string fen;
        
        // Read the FEN string (which can contain spaces)
        for (int i = 0; i < 6; ++i) {
            std::string fenPart;
            is >> fenPart;
            fen += fenPart + (i < 5 ? " " : "");
        }
        
        _board.setFromFen(fen);
        
        // Check for additional moves
        is >> token;
        if (token == "moves") {
            std::string moves;
            std::getline(is, moves);
            applyMoves(moves);
        }
    }
    
    // Update the engine's position
    _engine.setPosition(_board);
}

void UCI::handleGo(std::istringstream& is) {
    // For now, just make a move immediately using the random strategy
    // In the future, this could parse time controls and use them for decision making
    
    // Make a move
    Move move = _engine.makeMove();
    
    // Send the move to the GUI
    if (move.isValid()) {
        sendResponse("bestmove " + move.toUci());
    } else {
        // If no valid move, just send a null move
        sendResponse("bestmove 0000");
    }
}

void UCI::handleStop() {
    // This would stop a search in progress
    // For now, we don't have a search function, so this does nothing
}

void UCI::handleQuit() {
    _quit = true;
}

void UCI::handleUciNewGame() {
    // Reset the board and engine for a new game
    _board.reset();
    _engine.setPosition(_board);
}

void UCI::handlePrintBoard() {
    sendResponse(_board.toString());
}

void UCI::applyMoves(const std::string& moveStr) {
    std::istringstream iss(moveStr);
    std::string moveToken;
    
    while (iss >> moveToken) {
        Move move = Move::fromUci(moveToken);
        if (move.isValid()) {
            _board.makeMove(move);
        }
    }
}

std::vector<std::string> UCI::splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

void UCI::sendResponse(const std::string& response) {
    std::cout << response << std::endl;
}