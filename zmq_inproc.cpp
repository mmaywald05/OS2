//
// Created by Moritz Maywald on 07.01.25.
//
#include <iostream>
#include <thread>
#include "zmq.hpp" // ZeroMQ C++ bindings
#include <fstream>
#define NUM_ITERATIONS 1000000
#define WARM_UP_ITERATIONS 10000


using Clock = std::chrono::high_resolution_clock;
auto t1 = Clock::now();
auto t2 = Clock::now();
double durations_raw[NUM_ITERATIONS];

// Display loading bar (placeholder)
void displayLoadingBar(int n, int N) {
    std::cout << "\rProgress: [" << std::string(n * 50 / N, '=') << "> " << n * 100 / N << "%]" << std::flush;
}
// Function for Thread A (Sender)
void threadA(zmq::context_t& context) {
    // Create a PAIR socket for sending
    zmq::socket_t socket(context, zmq::socket_type::pair);
    socket.bind("inproc://thread_comm"); // Use in-process communication

    for (int i = 0; i < NUM_ITERATIONS+WARM_UP_ITERATIONS;i++) {
        // Create and send a message
        t2 = Clock::now();
        displayLoadingBar(i, NUM_ITERATIONS + WARM_UP_ITERATIONS);
        auto duration  = t2-t1;
        if(i > WARM_UP_ITERATIONS) {
            durations_raw[i-WARM_UP_ITERATIONS] =
                    duration.count() / 2; // Save time into raw array. Divide by two to approcimate one-way latency
        }
        // Restart Timer.
        t1 = Clock::now();

        std::string m = "M";
        socket.send(zmq::buffer(m), zmq::send_flags::none);
        //std::cout << "Thread A: Sent -> " << m << "\n";

        zmq::message_t message;
        auto result = socket.recv(message, zmq::recv_flags::none);

    }
    std::string message = "K";
    socket.send(zmq::buffer(message), zmq::send_flags::none);
}

// Function for Thread B (Receiver)
void threadB(zmq::context_t& context) {
    // Create a PAIR socket for receiving
    zmq::socket_t socket(context, zmq::socket_type::pair);
    socket.connect("inproc://thread_comm"); // Connect to the in-process address

    while (true){
        // Receive a message
        zmq::message_t message;
        auto result = socket.recv(message, zmq::recv_flags::none);
        std::string received_message(static_cast<char*>(message.data()), message.size());
        if(received_message == "K"){
            std::cout << "Kill message received" << std::endl;
            break;
        }
        std::string accept = "A";
        //std::cout << "Thread B: Received -> " << received_message << "\n";
        socket.send(zmq::buffer(accept), zmq::send_flags::none);
    }
}
void printTimes(){
    for(int i=0; i <NUM_ITERATIONS; i++){
        std::cout << "Iteration: " << i << " duration: " << durations_raw[i] << std::endl;
    }
}

// Function to write the map to a file
void saveData(const std::string& name, const double* elapsed) {
    // Construct the filename
    std::string filename = "OS2_Analysis/data/" +  name + ".txt";
    // Open the file in write mode
    std::ofstream outFile(filename);
    if (!outFile) {
        std::cerr << "Error: Could not open file " << filename << " for writing.\n";
        return;
    }
    // Write each entry in the map to the file
    for(int i = 0; i< NUM_ITERATIONS; i++){
        outFile << i << ":" << (long) elapsed[i] << "\n";
    }
    // Close the file
    outFile.close();
    std::cout << "data successfully written to " << filename << "\n";
}

// Main function
int main() {
    // Create a ZeroMQ context
    zmq::context_t context(1);

    // Create threads
    std::thread tA(threadA, std::ref(context));
    std::thread tB(threadB, std::ref(context));

    // Wait for threads to finish
    tA.join();
    tB.join();
    printTimes();
    saveData("zmq_intraproc", durations_raw);

    return 0;
}
