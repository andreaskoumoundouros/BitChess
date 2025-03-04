#include "gtest/gtest.h"
#include "neural_network.h"
#include <vector>
#include <cmath>

TEST(NeuralNetworkTest, ForwardPass) {
    // Create a simple neural network
    std::vector<int> topology = {2, 3, 1};
    NeuralNetwork network(topology);
    
    // Test with a simple input
    std::vector<float> input = {0.5f, -0.5f};
    float output = network.forward(input);
    
    // Output should be a floating point number
    EXPECT_TRUE(std::isfinite(output));
}

TEST(NeuralNetworkTest, Backpropagation) {
    // Create a simple neural network
    std::vector<int> topology = {2, 3, 1};
    NeuralNetwork network(topology);
    
    // Test inputs and target
    std::vector<float> input = {0.5f, -0.5f};
    float target = 1.0f;
    
    // Run forward pass to get initial output
    float initialOutput = network.forward(input);
    
    // Run backpropagation
    float learningRate = 0.1f;
    network.backpropagate(input, target, learningRate);
    
    // Run forward pass again
    float newOutput = network.forward(input);
    
    // If target > initial output, new output should be greater than initial
    // If target < initial output, new output should be less than initial
    if (target > initialOutput) {
        EXPECT_GT(newOutput, initialOutput);
    } else if (target < initialOutput) {
        EXPECT_LT(newOutput, initialOutput);
    }
}

TEST(NeuralNetworkTest, SaveAndLoad) {
    // Create a neural network
    std::vector<int> topology = {2, 3, 1};
    NeuralNetwork networkOriginal(topology);
    
    // Generate some test data
    std::vector<float> input = {0.5f, -0.5f};
    
    // Get output from original network
    float outputOriginal = networkOriginal.forward(input);
    
    // Save the network
    EXPECT_TRUE(networkOriginal.save("test_network.bin"));
    
    // Create a new network and load the saved one
    NeuralNetwork networkLoaded(topology);
    EXPECT_TRUE(networkLoaded.load("test_network.bin"));
    
    // Get output from loaded network
    float outputLoaded = networkLoaded.forward(input);
    
    // Outputs should be identical
    EXPECT_FLOAT_EQ(outputOriginal, outputLoaded);
}