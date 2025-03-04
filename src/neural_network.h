#pragma once

#include <vector>
#include <random>
#include <string>
#include <fstream>

// Define neural network structures
struct NeuronLayer {
    std::vector<std::vector<float>> weights;
    std::vector<float> biases;
    std::vector<float> outputs;
    
    // Initialize a layer with random weights
    void initialize(int inputSize, int outputSize, std::mt19937& rng);
};

class NeuralNetwork {
private:
    std::vector<NeuronLayer> layers;
    std::mt19937 rng;

public:
    NeuralNetwork(const std::vector<int>& topology);
    
    // Forward pass through the network
    float forward(const std::vector<float>& inputs);
    
    // Update weights using backpropagation
    void backpropagate(const std::vector<float>& inputs, float target, float learningRate);
    
    // Save network to file
    bool save(const std::string& filename);
    
    // Load network from file
    bool load(const std::string& filename);
};