#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <vector>
#include <fstream>

#define SOCKET_PATH "/shared/uds_socket"
#define BUFFER_SIZE 1
#define ITERATIONS 10000
#define OUTPUT_FILE "/shared/client_latency_results.txt"

int times_raw[ITERATIONS];

int main() {
    int client_fd;
    struct sockaddr_un address;
    char buffer[BUFFER_SIZE] = {'P'};
    std::vector<double> latencies; // Store latencies for each iteration

    // Create socket
    if ((client_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        return 1;
    }

    // Connect to server
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1);

    if (connect(client_fd, (struct sockaddr*)&address, sizeof(address)) == -1) {
        perror("Socket connect failed");
        return 1;
    }

    std::cout << "Client: Connected to the server.\n";

    // Perform ping-pong iterations
    for (int i = 0; i < ITERATIONS; ++i) {
        auto start_time = std::chrono::high_resolution_clock::now(); // Start timer

        send(client_fd, buffer, BUFFER_SIZE, 0);
        read(client_fd, buffer, BUFFER_SIZE);

        auto end_time = std::chrono::high_resolution_clock::now(); // End timer
        double latency = std::chrono::duration<double, std::micro>(end_time - start_time).count();


        times_raw[i]= latency;
    }

    std::cout << "Client: Finished iterations. Saving results...\n";

    for(int i = 0; i< ITERATIONS; ++i){
        std::cout << "Time " << i << ": " << times_raw[i] << "ns"<<std::endl;
    }

    // Send kill signal to server
    buffer[0] = 'K';
    send(client_fd, buffer, BUFFER_SIZE, 0);

    close(client_fd);
    return 0;
}
