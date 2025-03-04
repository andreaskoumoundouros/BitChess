#include "neural_network.h"
#include <cmath>

void NeuronLayer::initialize(int inputSize, int outputSize, std::mt19937& rng) {
    std::uniform_real_distribution<float> dist(-0.1f, 0.1f);
    weights.resize(outputSize, std::vector<float>(inputSize));
    biases.resize(outputSize);
    outputs.resize(outputSize);
    
    for (int i = 0; i < outputSize; i++) {
        biases[i] = dist(rng);
        for (int j = 0; j < inputSize; j++) {
            weights[i][j] = dist(rng);
        }
    }
}

NeuralNetwork::NeuralNetwork(const std::vector<int>& topology) {
    // Seed the random number generator
    std::random_device rd;
    rng.seed(rd());
    
    // Create layers based on topology
    for (size_t i = 0; i < topology.size() - 1; i++) {
        NeuronLayer layer;
        layer.initialize(topology[i], topology[i + 1], rng);
        layers.push_back(layer);
    }
}

float NeuralNetwork::forward(const std::vector<float>& inputs) {
    // First layer
    for (size_t i = 0; i < layers[0].outputs.size(); i++) {
        float sum = layers[0].biases[i];
        for (size_t j = 0; j < inputs.size(); j++) {
            sum += inputs[j] * layers[0].weights[i][j];
        }
        layers[0].outputs[i] = tanh(sum); // Using tanh activation
    }
    
    // Hidden layers
    for (size_t l = 1; l < layers.size(); l++) {
        for (size_t i = 0; i < layers[l].outputs.size(); i++) {
            float sum = layers[l].biases[i];
            for (size_t j = 0; j < layers[l-1].outputs.size(); j++) {
                sum += layers[l-1].outputs[j] * layers[l].weights[i][j];
            }
            // Last layer uses no activation (for value output)
            if (l == layers.size() - 1) {
                layers[l].outputs[i] = sum;
            } else {
                layers[l].outputs[i] = tanh(sum);
            }
        }
    }
    
    // Return the final output (value estimate)
    return layers.back().outputs[0];
}

void NeuralNetwork::backpropagate(const std::vector<float>& inputs, float target, float learningRate) {
    // Forward pass
    forward(inputs);
    
    // Calculate output layer error
    float output = layers.back().outputs[0];
    float outputError = target - output;
    
    // Calculate gradients and deltas for each layer, starting from the output layer
    std::vector<std::vector<float>> deltas(layers.size());
    
    // Output layer gradient
    deltas.back().resize(1);
    deltas.back()[0] = outputError;
    
    // Backpropagate error through hidden layers
    for (int l = layers.size() - 2; l >= 0; l--) {
        deltas[l].resize(layers[l].outputs.size());
        
        for (size_t i = 0; i < layers[l].outputs.size(); i++) {
            float error = 0.0f;
            for (size_t j = 0; j < layers[l+1].outputs.size(); j++) {
                error += deltas[l+1][j] * layers[l+1].weights[j][i];
            }
            // Apply derivative of tanh
            float output = layers[l].outputs[i];
            deltas[l][i] = error * (1 - output * output);
        }
    }
    
    // Update weights and biases
    // First layer
    for (size_t i = 0; i < layers[0].outputs.size(); i++) {
        layers[0].biases[i] += learningRate * deltas[0][i];
        for (size_t j = 0; j < inputs.size(); j++) {
            layers[0].weights[i][j] += learningRate * deltas[0][i] * inputs[j];
        }
    }
    
    // Remaining layers
    for (size_t l = 1; l < layers.size(); l++) {
        for (size_t i = 0; i < layers[l].outputs.size(); i++) {
            layers[l].biases[i] += learningRate * deltas[l][i];
            for (size_t j = 0; j < layers[l-1].outputs.size(); j++) {
                layers[l].weights[i][j] += learningRate * deltas[l][i] * layers[l-1].outputs[j];
            }
        }
    }
}

bool NeuralNetwork::save(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    
    // Save number of layers
    size_t numLayers = layers.size();
    file.write(reinterpret_cast<char*>(&numLayers), sizeof(numLayers));
    
    // Save each layer
    for (const auto& layer : layers) {
        // Save dimensions
        size_t outputSize = layer.outputs.size();
        size_t inputSize = layer.weights[0].size();
        file.write(reinterpret_cast<char*>(&outputSize), sizeof(outputSize));
        file.write(reinterpret_cast<char*>(&inputSize), sizeof(inputSize));
        
        // Save biases
        for (float bias : layer.biases) {
            file.write(reinterpret_cast<char*>(&bias), sizeof(bias));
        }
        
        // Save weights
        for (const auto& neuron : layer.weights) {
            for (float weight : neuron) {
                file.write(reinterpret_cast<char*>(&weight), sizeof(weight));
            }
        }
    }
    
    return true;
}

bool NeuralNetwork::load(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    
    // Read number of layers
    size_t numLayers;
    file.read(reinterpret_cast<char*>(&numLayers), sizeof(numLayers));
    
    // Clear existing layers
    layers.clear();
    
    // Read each layer
    for (size_t l = 0; l < numLayers; l++) {
        NeuronLayer layer;
        
        // Read dimensions
        size_t outputSize, inputSize;
        file.read(reinterpret_cast<char*>(&outputSize), sizeof(outputSize));
        file.read(reinterpret_cast<char*>(&inputSize), sizeof(inputSize));
        
        // Initialize structures
        layer.biases.resize(outputSize);
        layer.outputs.resize(outputSize);
        layer.weights.resize(outputSize, std::vector<float>(inputSize));
        
        // Read biases
        for (size_t i = 0; i < outputSize; i++) {
            file.read(reinterpret_cast<char*>(&layer.biases[i]), sizeof(float));
        }
        
        // Read weights
        for (size_t i = 0; i < outputSize; i++) {
            for (size_t j = 0; j < inputSize; j++) {
                file.read(reinterpret_cast<char*>(&layer.weights[i][j]), sizeof(float));
            }
        }
        
        layers.push_back(layer);
    }
    
    return true;
}