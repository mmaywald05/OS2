#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include "zmq.hpp"
#include <fstream>
#define NUM_ITERATIONS 1000000 // Reduce for testing
#define WARM_UP_ITERATIONS 10000
double durations_raw[NUM_ITERATIONS]; // Timing array for testing
using Clock = std::chrono::high_resolution_clock;
auto t1 = Clock::now();
auto t2 = Clock::now();

// Display loading bar (placeholder)
void displayLoadingBar(int n, int N) {
    std::cout << "\rProgress: [" << std::string(n * 50 / N, '=') << "> " << n * 100 / N << "%]" << std::flush;
}

void processA() {
    zmq::context_t context(1); // Create a new context for this process
    zmq::socket_t socket(context, zmq::socket_type::pair);
    socket.connect("ipc:///tmp/thread_comm");
    t1=Clock::now();
    for (int i = 0; i < NUM_ITERATIONS+WARM_UP_ITERATIONS; i++) {
        t2 = Clock::now();
        displayLoadingBar(i, NUM_ITERATIONS + WARM_UP_ITERATIONS);
        auto duration = t2-t1;
        if(i >= WARM_UP_ITERATIONS) {
            durations_raw[i-WARM_UP_ITERATIONS] = duration.count();
        }
        t1 = Clock::now();
        std::string m = "Message " + std::to_string(i);
        socket.send(zmq::buffer(m), zmq::send_flags::none);

        zmq::message_t message;
        socket.recv(message, zmq::recv_flags::none);
        std::string received(static_cast<char*>(message.data()), message.size());
    }

    std::string kill_message = "kill";
    socket.send(zmq::buffer(kill_message), zmq::send_flags::none);

    zmq::message_t ackMessage;
    socket.recv(ackMessage, zmq::recv_flags::none);
    std::string ack(static_cast<char*>(ackMessage.data()), ackMessage.size());

    if (ack == "ack") {
        std::cout << "\nProcessA received acknowledgment, finishing." << std::endl;
    }

    socket.close();
    context.close();
}

void processB() {
    zmq::context_t context(1); // Create a new context for this process
    zmq::socket_t socket(context, zmq::socket_type::pair);
    socket.bind("ipc:///tmp/thread_comm");

    while (true) {
        zmq::message_t message;
        socket.recv(message, zmq::recv_flags::none);
        std::string received_message(static_cast<char*>(message.data()), message.size());

        if (received_message == "kill") {
            std::cout << "Kill message received, acknowledging." << std::endl;
            std::string ack = "ack";
            socket.send(zmq::buffer(ack), zmq::send_flags::none);
            break;
        }

        std::string accept = "accept " + received_message;
        socket.send(zmq::buffer(accept), zmq::send_flags::none);
    }

    socket.close();
    context.close();
    std::cout << "Process B finishes." << std::endl;
}

void printTimes(){
    for(int i=0; i <NUM_ITERATIONS; i++){
        std::cout << i << ": " << durations_raw[i] <<"ns"<< std::endl;
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
        outFile << i << ":" << (long)elapsed[i] << "\n";
    }
    // Close the file
    outFile.close();
    std::cout << "data successfully written to " << filename << "\n";
}

int main() {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    }
    if (pid == 0) {
        // Child process (Process B)
        processB();
    } else {
        // Parent process (Process A)
        processA();
        wait(nullptr); // Wait for child process to complete
        printTimes();
        saveData("zmq_interproc", durations_raw);
        std::cout << "Processes finished." << std::endl;
    }
    return 0;
}
