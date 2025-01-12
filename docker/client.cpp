#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <fstream>

#define SOCKET_PATH "/shared/uds_socket"
#define BUFFER_SIZE 1
#define NUM_ITERATIONS 1000000
#define WARM_UP_ITERATIONS 10000


double durations_raw[NUM_ITERATIONS];
using Clock = std::chrono::high_resolution_clock;
auto t1 = Clock::now();
auto t2 = Clock::now();


void printTimes(){
    for(int i=0; i <NUM_ITERATIONS; i++){
        std::cout << "Iteration: " << i << " duration: " << durations_raw[i] << std::endl;
    }
}

// Function to write the map to a file
void saveData(const std::string& name, const double* elapsed) {
    // Construct the filename
    std::string filename = name + ".txt";
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
    for (int i = 0; i < NUM_ITERATIONS+WARM_UP_ITERATIONS; ++i) {
        t1 = Clock::now();
        send(client_fd, buffer, BUFFER_SIZE, 0);
        read(client_fd, buffer, BUFFER_SIZE);
        t2 = Clock::now();
        auto duration= t2-t1;
        if(i>=WARM_UP_ITERATIONS) {
            durations_raw[i-WARM_UP_ITERATIONS] =  duration.count();
        }
    }

    std::cout << "Client: Finished iterations. Saving results...\n";
    // Send kill signal to server
    buffer[0] = 'K';
    send(client_fd, buffer, BUFFER_SIZE, 0);



    //printTimes();
    saveData("docker_client", durations_raw);
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::cout << i<<":"<<durations_raw[i] << std::endl;
    }
    /* Intentional infinite loop.
     * Allows copying the data created in a seperate terminal before container is destroyed. */
    while(true){

    }
    //close(client_fd);
    return 0;
}
