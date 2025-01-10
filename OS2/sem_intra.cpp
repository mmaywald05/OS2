#include <iostream>
#include <thread>
#include <semaphore.h> // POSIX semaphores
#include <atomic>
#include <fcntl.h>    // For O_CREAT and O_EXCL
#include <map>
#include <fstream>

using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<double, std::milli>;
#define NUM_ITERATIONS 1000000
auto t1 = Clock::now();
auto t2 = Clock::now();
std::map<double ,int> elapsed;
// Shared memory layout
struct SharedMemory {
    std::atomic<bool> bit; // Shared bit for communication
};

// Global shared memory and semaphore names
SharedMemory shared_mem;
const char* SEMAPHORE_A_NAME = "/semaphoreA";
const char* SEMAPHORE_B_NAME = "/semaphoreB";

// Semaphore pointers
sem_t* semaphoreA = nullptr;
sem_t* semaphoreB = nullptr;

// Function for Thread A
void threadA() {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Wait for Thread A's turn
        sem_wait(semaphoreA);
        t2 = Clock::now();

        // save elapsed time as t2-t1
        auto result= t2-t1;

        if(elapsed.find(result.count()) != elapsed.end()) {
            elapsed[result.count()]++;
        }else{
            elapsed[result.count()]=1;
        }

        // start timer for new iteration
        t1 = Clock::now();

        // Set the shared bit to true
        shared_mem.bit.store(true, std::memory_order_relaxed);
        //std::cout << "Thread A: Set bit to true\n";

        // Signal Thread B
        sem_post(semaphoreB);
    }
}

// Function for Thread B
void threadB() {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Wait for Thread B's turn
        sem_wait(semaphoreB);

        // Read the shared bit and reset it to false
        if (shared_mem.bit.load(std::memory_order_relaxed)) {
            //std::cout << "Thread B: Detected bit as true, resetting to false\n";
            shared_mem.bit.store(false, std::memory_order_relaxed);
        }

        // Signal Thread A
        sem_post(semaphoreA);
    }
}

void displayMap() {
    std::cout << "Index counts:\n";
    for (const auto& pair : elapsed) {
        std::cout << "Time " << pair.first << ": " << pair.second << " occurrences\n";
    }
}



// Function to write the map to a file
void writeToFile(const std::string& name, const std::map<double, int>& elapsed) {
    // Construct the filename
    std::string filename = "OS2_Analysis/" +  name + ".txt";

    // Open the file in write mode
    std::ofstream outFile(filename);
    if (!outFile) {
        std::cerr << "Error: Could not open file " << filename << " for writing.\n";
        return;
    }

    // Write each entry in the map to the file
    for (const auto& entry : elapsed) {
        outFile << entry.first << ":" << entry.second << "\n";
    }

    // Close the file
    outFile.close();

    std::cout << "Data successfully written to " << filename << "\n";
}


// Main function
int main() {
    // Initialize the shared memory
    shared_mem.bit = false;

    // Create and initialize named semaphores
    semaphoreA = sem_open(SEMAPHORE_A_NAME, O_CREAT | O_EXCL, 0666, 1); // Thread A starts first
    semaphoreB = sem_open(SEMAPHORE_B_NAME, O_CREAT | O_EXCL, 0666, 0); // Thread B waits initially

    if (semaphoreA == SEM_FAILED || semaphoreB == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // Create threads
    std::thread tA(threadA);
    std::thread tB(threadB);

    // Wait for threads to finish
    tA.join();
    tB.join();

    // Clean up semaphores
    sem_close(semaphoreA);
    sem_close(semaphoreB);
    sem_unlink(SEMAPHORE_A_NAME);
    sem_unlink(SEMAPHORE_B_NAME);

    displayMap();
    writeToFile("sem_intra",elapsed);
    return 0;
}
