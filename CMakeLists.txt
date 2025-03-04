cmake_minimum_required(VERSION 3.15)
project(BitChessRL VERSION 0.1.0 LANGUAGES CXX)

if(MSVC)
    # Or use static linking for all targets
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add optimization flags for Release build
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Use OpenMP for parallelization in neural network
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# Include FetchContent for downloading dependencies
include(FetchContent)

# Fetch GoogleTest for unit tests
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.16.0
)

# Define core sources without main.cpp
set(CORE_SOURCES
    src/bitboard.cpp
    src/movegen.cpp
    src/board.cpp
    src/uci.cpp
    src/engine.cpp
)

# Main executable source files
set(SOURCES
    ${CORE_SOURCES}
    src/main.cpp
)

# Reinforcement learning source files
set(RL_SOURCES
    src/chess_rl.cpp
    src/neural_network.cpp
    src/feature_extractor.cpp
)

# Training executable source files
set(TRAIN_SOURCES
    src/train_rl.cpp
)

# Make dependencies available
FetchContent_MakeAvailable(googletest)

# Add the main executable
add_executable(bitchess ${SOURCES})

# Add the RL training executable (without main.cpp)
add_executable(bitchess_train ${CORE_SOURCES} ${RL_SOURCES} ${TRAIN_SOURCES})
target_compile_definitions(bitchess_train PRIVATE ENABLE_RL=1)

# Add the RL-enabled main executable
add_executable(bitchess_rl ${SOURCES} ${RL_SOURCES})
target_compile_definitions(bitchess_rl PRIVATE ENABLE_RL=1)

# Add include directories
target_include_directories(bitchess PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(bitchess_train PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(bitchess_rl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Link OpenMP if found
if(OpenMP_CXX_FOUND)
    target_link_libraries(bitchess_train PRIVATE OpenMP::OpenMP_CXX)
    target_link_libraries(bitchess_rl PRIVATE OpenMP::OpenMP_CXX)
endif()

# Define linking with math library for Unix-like systems
if(UNIX AND NOT APPLE)
    target_link_libraries(bitchess_train PRIVATE m)
    target_link_libraries(bitchess_rl PRIVATE m)
endif()

# Enable testing
enable_testing()

# Test source files
set(TEST_SOURCES
    test/test_bitboard.cpp
    test/test_movegen.cpp
    test/test_board.cpp
    test/test_main.cpp
)

# RL test sources
set(RL_TEST_SOURCES
    test/test_neural_network.cpp
    test/test_chess_rl.cpp
)

# Add the test executable
add_executable(bitchess_test ${TEST_SOURCES}
    src/bitboard.cpp
    src/movegen.cpp
    src/board.cpp
    src/engine.cpp
)

# Add the RL test executable
add_executable(bitchess_rl_test ${TEST_SOURCES} ${RL_TEST_SOURCES}
    src/bitboard.cpp
    src/movegen.cpp
    src/board.cpp
    src/engine.cpp
    src/chess_rl.cpp
    src/neural_network.cpp
    src/feature_extractor.cpp
)
target_compile_definitions(bitchess_rl_test PRIVATE ENABLE_RL=1)

# Link against GoogleTest
target_link_libraries(bitchess_test PRIVATE gtest gtest_main gmock)
target_link_libraries(bitchess_rl_test PRIVATE gtest gtest_main gmock)

# Add include directories for tests
target_include_directories(bitchess_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${googletest_SOURCE_DIR}/include
)
target_include_directories(bitchess_rl_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${googletest_SOURCE_DIR}/include
)

# Add tests to CTest
add_test(NAME BitchessTests COMMAND bitchess_test)
add_test(NAME BitchessRLTests COMMAND bitchess_rl_test)

# Installation rules
install(TARGETS bitchess bitchess_rl DESTINATION bin)
install(TARGETS bitchess_train DESTINATION bin)