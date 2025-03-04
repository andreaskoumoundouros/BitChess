#ifndef UCI_H
#define UCI_H

#include "engine.h"
#include <string>
#include <vector>
#include <sstream>

// Include chess_rl.h only when RL is enabled
#ifdef ENABLE_RL
#include "chess_rl.h"
#endif

// UCI protocol handler
class UCI {
public:
    // Constructor
    UCI();
    
    // Start the UCI loop
    void startLoop();
    
    // Process a UCI command
    void processCommand(const std::string& command);

private:
    // The chess engine
    Engine _engine;
    
    // Board representation
    Board _board;
    
    // Flag to signal when to quit the UCI loop
    bool _quit;
    
    // Handle the 'uci' command
    void handleUci();

    void handleSetOption(std::istringstream& is);

    void handlePrintBoard();
    
    // Handle the 'isready' command
    void handleIsReady();
    
    // Handle the 'position' command
    void handlePosition(std::istringstream& is);
    
    // Handle the 'go' command
    void handleGo(std::istringstream& is);
    
    // Handle the 'stop' command
    void handleStop();
    
    // Handle the 'quit' command
    void handleQuit();
    
    // Handle the 'ucinewgame' command
    void handleUciNewGame();
    
    // Parse moves from a string (for the 'position' command)
    void applyMoves(const std::string& moveStr);
    
    // Split a string into tokens
    std::vector<std::string> splitString(const std::string& str, char delimiter = ' ');
    
    // Send a response to the GUI
    void sendResponse(const std::string& response);
};

#endif // UCI_H