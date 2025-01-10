//
// Created by Moritz Maywald on 09.01.25.
//

#include <iostream>
#include <atomic>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <map>
#include <fstream>
#include <zmq.hpp>


#define NUM_ITERATIONS 1000

using Clock = std::chrono::high_resolution_clock;
auto t1 = Clock::now();
auto t2 = Clock::now();
double times_raw[NUM_ITERATIONS];


void displayLoadingBar(int n, int N, int barWidth = 50) {
    // Calculate the fraction of progress
    double progress = static_cast<double>(n) / N;

    // Calculate the number of characters to fill
    int filledWidth = static_cast<int>(progress * barWidth);

    // Display the loading bar
    std::cout << "\r["; // Start of the bar
    for (int i = 0; i < barWidth; ++i) {
        if (i < filledWidth)
            std::cout << "="; // Filled part
        else
            std::cout << " "; // Empty part
    }
    std::cout << "] " << std::fixed << std::setprecision(2) << (progress * 100) << "%"; // Percentage
    std::cout.flush(); // Ensure the bar is displayed immediately
}
void printTimes(){
    for(int i=0; i <NUM_ITERATIONS; i++){
        std::cout << "Iteration: " << i << " duration: " << times_raw[i] << std::endl;
    }
}

void processA(zmq::context_t& context){
    // Create a PAIR socket for sending
    zmq::socket_t socket(context, zmq::socket_type::pair);
    socket.connect("ipc:///tmp/thread_comm"); // Connect to ipc:// for inter-process communication
    //std::cout<< "Client started..."<<std::endl;
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        // Create and send a message
        t2 = Clock::now();
        displayLoadingBar(i,NUM_ITERATIONS);
        auto duration = t2 - t1;
        times_raw[i] = duration.count() / 2; // Save time into raw array. Divide by two to approximate one-way latency
        // Restart Timer.
        t1 = Clock::now();

        std::string m = "Message " + std::to_string(i);
        socket.send(zmq::buffer(m), zmq::send_flags::none);
        zmq::message_t message;
        auto recv_raw = socket.recv(message, zmq::recv_flags::none);
        std::string received(static_cast<char*>(message.data()), message.size());
        //std::cout << "ProcessA received " << received << std::endl;
    }

    std::string message = "kill";
    socket.send(zmq::buffer(message), zmq::send_flags::none);
    // Wait for acknowledgment from ProcessB

    zmq::message_t ackMessage;
    auto ack_raw =socket.recv(ackMessage, zmq::recv_flags::none);
    std::string ack(static_cast<char*>(ackMessage.data()), ackMessage.size());
    if (ack == "ack") {
        std::cout << "ProcessA received acknowledgment, finishing." << std::endl;
    }
    socket.close();
    std::cout<<"process A finishes" << std::endl;
}

void processB(zmq::context_t& context){
    // Create a PAIR socket for receiving
    zmq::socket_t socket(context, zmq::socket_type::pair);
    socket.bind("ipc:///tmp/thread_comm"); // Bind to ipc:// for inter-process communication
    //std::cout<< "Server started..."<<std::endl;
    while (true){
        // Receive a message
        zmq::message_t message;
        auto result = socket.recv(message, zmq::recv_flags::none);
        std::string received_message(static_cast<char*>(message.data()), message.size());
        //std::cout <<  "Process B received message " << received_message << std::endl;
        if(received_message == "kill"){
            std::cout << "Kill message received, acknowledging" << std::endl;
            std::string ack = "ack";
            socket.send(zmq::buffer(ack), zmq::send_flags::none);
            break;
        }
        std::string accept = "accept " + received_message;
        socket.send(zmq::buffer(accept), zmq::send_flags::none);
    }

    std::cout<<"process B finishes" << std::endl;
}


// Main function
int main() {
    zmq::context_t context(1);
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    }
    if (pid == 0) {
        // Child process (Process B)
        processB(context);
    } else {
        // Parent process (Process A)
        processA(context);
        // Wait for child process to complete
        wait(nullptr);
        printTimes();
    }
    return 0;
}