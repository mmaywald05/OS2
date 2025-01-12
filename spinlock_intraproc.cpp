#include <iostream>
#include <atomic>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <cstdlib>
#include <map>
#include <fstream>

const char* SHARED_MEM_NAME = "/shared_mem_example";
const size_t SHARED_MEM_SIZE = sizeof(std::atomic<bool>) + sizeof(char) + sizeof(std::atomic<bool>); // Spinlock + shared bit
#define NUM_ITERATIONS 1000000
#define WARM_UP_ITERATIONS 10000


using Clock = std::chrono::high_resolution_clock;

auto t1 = Clock::now();
auto t2 = Clock::now();
double durations_raw[NUM_ITERATIONS];

// RUN WITH g++ -std=c++17 -pthread -o spinlock_intraproc spinlock_intraproc.cpp
// ./spinlock_intraproc

/*
 * Spinlock structure for synchronization, very primitive
 * and, according to the internet "The worst possble implementationof a spinlock imaginable."
 */
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
    std::atomic<bool> terminationBit;
};

// Function for Thread A
void threadA() {
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
    shared_mem->bit = 1; // Start with bit set to 1
    shared_mem->terminationBit = 0;

    t1 = Clock::now();
    t2 = Clock::now();

    // Perform loop
    int i = 0;
    while (i < NUM_ITERATIONS+WARM_UP_ITERATIONS){
        shared_mem->spinlock.acquire();

        if(shared_mem->bit.load(std::memory_order_acquire)){
            t2 = Clock::now();
            auto result  = t2-t1;
            i+=1;
            if(i>=WARM_UP_ITERATIONS){
                durations_raw[i-WARM_UP_ITERATIONS] = result.count(); // Save time into raw array
            }
            // Restart Timer.
            t1 = Clock::now();


            shared_mem->bit.store(false, std::memory_order_release); // Set bit to 0
            //std::cout << "Thread A setting bit to 0 "<< std::endl;

        }
        /* else{
            std::cout<< "X" << std::endl;
        }
        */
        shared_mem->spinlock.release();
    }
    //std::cout << "Thread A setting termination bit " << std::endl;
    // Signal Thread B to terminate
    shared_mem->spinlock.acquire();
    shared_mem->terminationBit.store(true, std::memory_order_relaxed);; // set termination bit
    shared_mem->bit.store(true, std::memory_order_relaxed); // Set bit to 1
    shared_mem->spinlock.release();

    // Clean up
    munmap(shared_mem, SHARED_MEM_SIZE);
    close(shm_fd);
}

// Function for Thread B
void threadB() {
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
    // Infinite loop until termination signal
    while (true) {
        shared_mem->spinlock.acquire();
        if (shared_mem->terminationBit.load(std::memory_order_acquire)) {
            //std::cout << "termination signal detected." << std::endl;
            shared_mem->spinlock.release();
            break; // Termination signal detected
        }
        if(!shared_mem->bit.load(std::memory_order_relaxed)) {
            //std::cout << "Thread B setting Bit to 1" << std::endl;
            shared_mem->bit.store(true, std::memory_order_release); // Set bit to 1
        }
        shared_mem->spinlock.release();
    }

    // Clean up
    munmap(shared_mem, SHARED_MEM_SIZE);
    close(shm_fd);
}

void displayData(){
    for(int i = 0; i < NUM_ITERATIONS; i++){
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

int main() {
    // Create threads
    std::thread tA(threadA);
    std::thread tB(threadB);
    // Wait for threads
    tA.join();
    tB.join();
    // Clean up
    shm_unlink(SHARED_MEM_NAME);

    displayData();
    saveData("spinlock_intraproc", durations_raw);
    return 0;
}
