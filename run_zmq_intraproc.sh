#!/bin/bash

# Step 1: Verify the location of the header file
HEADER_PATH="/opt/homebrew/include/zmq.h"
if [ -f "$HEADER_PATH" ]; then
    echo "Header file found: $HEADER_PATH"
else
    echo "Error: Header file not found in /opt/homebrew/include."
    exit 1
fi

# Step 2: Verify the location of the library
LIBRARY_PATH="/opt/homebrew/lib/libzmq.dylib"
if [ -f "$LIBRARY_PATH" ]; then
    echo "Library file found: $LIBRARY_PATH"
else
    echo "Error: Library file not found in /opt/homebrew/lib."
    exit 1
fi

# Step 3: Compile the program
echo "Compiling zmq_threads.cpp..."
g++ -std=c++17 -Wall -pthread -I/opt/homebrew/include -L/opt/homebrew/lib zmq_inproc.cpp -o out/zmq_inproc -lzmq

if [ $? -eq 0 ]; then
    echo "Compilation successful."
else
    echo "Error: Compilation failed."
    exit 1
fi

# Step 4: Run the program
echo "Running zmq_inproc..."
./out/zmq_inproc

if [ $? -eq 0 ]; then
    echo "Program executed successfully."
else
    echo "Error: Program execution failed."
    exit 1
fi
