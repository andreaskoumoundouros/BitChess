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

# Include FetchContent for downloading dependencies
include(FetchContent)

# Fetch GoogleTest for unit tests
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.16.0
)
FetchContent_MakeAvailable(googletest)

# Main executable source files
set(SOURCES
    src/bitboard.cpp
    src/movegen.cpp
    src/board.cpp
    src/uci.cpp
    src/engine.cpp
    src/main.cpp
)

# Add the main executable
add_executable(bitchess ${SOURCES})

# Add include directories
target_include_directories(bitchess PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Enable testing
enable_testing()

# Test source files
set(TEST_SOURCES
    test/test_bitboard.cpp
    test/test_movegen.cpp
    test/test_board.cpp
    test/test_main.cpp
)

# Add the test executable
add_executable(bitchess_test ${TEST_SOURCES}
    src/bitboard.cpp
    src/movegen.cpp
    src/board.cpp
    src/engine.cpp
)

# Link against GoogleTest
target_link_libraries(bitchess_test PRIVATE gtest gtest_main gmock)

# Add include directories for tests
target_include_directories(bitchess_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${googletest_SOURCE_DIR}/include
)

# Add tests to CTest
add_test(NAME BitchessTests COMMAND bitchess_test)

# Installation rules
install(TARGETS bitchess DESTINATION bin)
