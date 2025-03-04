#pragma once

#include <vector>
#include "board.h"

// Feature extraction from chess board
class BoardFeatureExtractor {
public:
    // Convert a board position to input features for the neural network
    static std::vector<float> extractFeatures(const Board& board);
    
    // Get the size of the feature vector for a given board
    static size_t getFeatureSize();
};