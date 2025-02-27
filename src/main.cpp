#include "bitboard.h"
#include "uci.h"
#include <iostream>

int main() {
    // Initialize bitboards
    BitboardUtils::initBitboards();
    
    // Create UCI interface
    UCI uci;
    
    // Start the UCI loop
    uci.startLoop();
    
    return 0;
}