//
// Created by Moritz Maywald on 07.01.25.
//
#include <iostream>
#include <thread>
#include "zmq.hpp" // ZeroMQ C++ bindings

#define NUM_ITERATIONS 1000000


using Clock = std::chrono::high_resolution_clock;
auto t1 = Clock::now();
auto t2 = Clock::now();
double times_raw[NUM_ITERATIONS];

// Function for Thread A (Sender)
void threadA(zmq::context_t& context) {
    // Create a PAIR socket for sending
    zmq::socket_t socket(context, zmq::socket_type::pair);
    socket.bind("inproc://thread_comm"); // Use in-process communication

    for (int i = 0; i < NUM_ITERATIONS;i++) {
        // Create and send a message
        t2 = Clock::now();
        auto duration  = t2-t1;
        times_raw[i] = duration.count()/2; // Save time into raw array. Divide by two to approcimate one-way latency
        // Restart Timer.
        t1 = Clock::now();

        std::string m = "Message " + std::to_string(i);
        socket.send(zmq::buffer(m), zmq::send_flags::none);
        //std::cout << "Thread A: Sent -> " << m << "\n";

        zmq::message_t message;
        auto result = socket.recv(message, zmq::recv_flags::none);

    }
    std::string message = "kill";
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
        if(received_message == "kill"){
            std::cout << "Kill message received" << std::endl;
            break;
        }
        std::string accept = "accept " + received_message;
        //std::cout << "Thread B: Received -> " << received_message << "\n";
        socket.send(zmq::buffer(accept), zmq::send_flags::none);
    }
}
void printTimes(){
    for(int i=0; i <NUM_ITERATIONS; i++){
        std::cout << "Iteration: " << i << " duration: " << times_raw[i] << std::endl;
    }
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

    return 0;
}
