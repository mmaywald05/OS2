#include <iostream>
#include <thread>
#include <semaphore.h> // POSIX semaphores
#include <atomic>
#include <fcntl.h>    // For O_CREAT and O_EXCL
#include <sys/stat.h> // For mode constants
#include <unistd.h>   // For unlink

#define NUM_ITERATIONS 1000000


using Clock = std::chrono::high_resolution_clock;
auto t1 = Clock::now();
auto t2 = Clock::now();
double times_raw[NUM_ITERATIONS];

// Shared memory layout
struct SharedMemory {
    std::atomic<bool> bit; // Shared bit for communication
    std::atomic<bool> terminationBit;
};

// Global shared memory and semaphore names
SharedMemory shared_mem;
const char* SEMAPHORE_A_NAME = "/asemaphoreA";
const char* SEMAPHORE_B_NAME = "/asemaphoreB";

// Semaphore pointers
sem_t* semaphoreA = nullptr;
sem_t* semaphoreB = nullptr;

// Function for Thread A
void threadA() {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Wait for Thread A's turn
        sem_wait(semaphoreA);

        /**
        * ZEIT HIER STOPPPEN
        *  - Halbieren, weil austausch macht round-trip
        **/
        t2 = Clock::now();
        auto result  = t2-t1;
        times_raw[i] = result.count()/2; // Save time into raw array. Divide by two to approcimate one-way latency
        // Restart Timer.
        t1 = Clock::now();

        // Set the shared bit to true
        shared_mem.bit.store(true, std::memory_order_relaxed);
        //std::cout << "Thread A: Set bit to true\n";

        // Signal Thread B
        sem_post(semaphoreB);
    }

    shared_mem.terminationBit.store(true, std::memory_order_relaxed);
    //std::cout << "Thread A: Set bit to true\n";

    // Signal Thread B
    sem_post(semaphoreB);

}

// Function for Thread B
void threadB() {
   while (true) {
        // Wait for Thread B's turn
        sem_wait(semaphoreB);

        if(shared_mem.terminationBit.load(std::memory_order_relaxed)){
            std::cout << "Termination Bit detected" << std::endl;
            break;
        }

        // Read the shared bit and reset it to false
        if (shared_mem.bit.load(std::memory_order_relaxed)) {
            //std::cout << "Thread B: Detected bit as true, resetting to false\n";
            shared_mem.bit.store(false, std::memory_order_relaxed);
        }

        // Signal Thread A
        sem_post(semaphoreA);
    }
}

void printTimes(){
    for(int i=0; i <NUM_ITERATIONS; i++){
        std::cout << "Iteration: " << i << " duration: " << times_raw[i] << std::endl;
    }
}

// Main function
int main() {

    // Initialize the shared memory

    shared_mem.bit = false;
    shared_mem.terminationBit=false;

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
    printTimes();

    // Clean up semaphores
    sem_close(semaphoreA);
    sem_close(semaphoreB);
    sem_unlink(SEMAPHORE_A_NAME);
    sem_unlink(SEMAPHORE_B_NAME);

    return 0;
}
