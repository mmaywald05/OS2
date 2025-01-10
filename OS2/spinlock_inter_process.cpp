//
// Created by Moritz Maywald on 07.01.25.
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

const char* SHARED_MEM_NAME = "/shared_mem_example";
const size_t SHARED_MEM_SIZE = sizeof(std::atomic<bool>) + sizeof(char); // Spinlock + shared bit

using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<double, std::milli>;
#define NUM_ITERATIONS 1000000
auto t1 = Clock::now();
auto t2 = Clock::now();
std::map<double ,int> elapsed;

// Spinlock structure for synchronization
struct Spinlock {
    std::atomic<bool> lock = false;

    void acquire() {
        while (lock.exchange(true, std::memory_order_acquire)); // Busy wait until lock is available
    }

    void release() {
        lock.store(false, std::memory_order_release);
    }
};

// Shared memory layout
struct SharedMemory {
    Spinlock spinlock;    // Spinlock for synchronization
    std::atomic<bool> bit; // Shared bit to communicate
};

// Function for process A
void processA() {
    // Open and map shared memory
    int shm_fd = shm_open(SHARED_MEM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }
    ftruncate(shm_fd, SHARED_MEM_SIZE);
    void* addr = mmap(nullptr, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    SharedMemory* shared_mem = static_cast<SharedMemory*>(addr);

    // Initialize shared memory
    shared_mem->bit = false;

    // Perform communication
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        shared_mem->spinlock.acquire();
        /*** TIME-KEEPING ***/
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
        /***/
        //std::cout << "Process A: Set bit to true\n";
        shared_mem->bit.store(true, std::memory_order_relaxed);
        shared_mem->spinlock.release();
        // Wait for Process B to set bit back to false
        while (shared_mem->bit.load(std::memory_order_relaxed));
    }

    // Clean up
    munmap(shared_mem, SHARED_MEM_SIZE);
    close(shm_fd);
}

// Function for process B
void processB() {
    // Open and map shared memory
    int shm_fd = shm_open(SHARED_MEM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }
    void* addr = mmap(nullptr, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    SharedMemory* shared_mem = static_cast<SharedMemory*>(addr);

    // Perform communication
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Wait for Process A to set the bit to true
        while (!shared_mem->bit.load(std::memory_order_relaxed));

        shared_mem->spinlock.acquire();
        //std::cout << "Process B: Detected bit as true, setting bit to false\n";
        shared_mem->bit.store(false, std::memory_order_relaxed);
        shared_mem->spinlock.release();
    }

    // Clean up
    munmap(shared_mem, SHARED_MEM_SIZE);
    close(shm_fd);
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

        // Wait for child process to complete
        wait(nullptr);

        // Clean up shared memory
        shm_unlink(SHARED_MEM_NAME);
    }
    displayMap();

    return 0;
}
