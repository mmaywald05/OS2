#include <iostream>
#include <atomic>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <map>

const char* SHARED_MEM_NAME = "/shared_mem_example";
const size_t SHARED_MEM_SIZE = sizeof(std::atomic<bool>) + sizeof(char) + sizeof(std::atomic<bool>); // Spinlock + shared bit
#define NUM_ITERATIONS 1000000


using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<double, std::milli>;
auto t1 = Clock::now();
auto t2 = Clock::now();
double times_raw[NUM_ITERATIONS];


// RUN WITH g++ -std=c++17 -pthread -o spinlock_intraproc spinlock_intraproc.cpp
// ./spinlock_intraproc

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

    // Perform loop for NUM_ITERATIONS
    int i = 0;
    while (i < NUM_ITERATIONS){
        shared_mem->spinlock.acquire();

        if(shared_mem->bit.load(std::memory_order_relaxed)){
            //std::cout << "SUCCESS" <<std::endl;
            /**
             * ZEIT HIER STOPPPEN?
             *  - Halbieren, weil austausch macht round-trip
            **/
            t2 = Clock::now();
            auto result  = t2-t1;
            times_raw[i++] = result.count()/2; // Save time into raw array. Divide by two to approcimate one-way latency
            // Restart Timer.
            t1 = Clock::now();


            shared_mem->bit.store(false, std::memory_order_relaxed); // Set bit to 0
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
        if (shared_mem->terminationBit.load(std::memory_order_relaxed)) {
            //std::cout << "termination signal detected." << std::endl;
            shared_mem->spinlock.release();
            break; // Termination signal detected
        }
        if(!shared_mem->bit.load(std::memory_order_relaxed)) {
            //std::cout << "Thread B setting Bit to 1" << std::endl;
            shared_mem->bit.store(true, std::memory_order_relaxed); // Set bit to 1
        }
        shared_mem->spinlock.release();
    }

    // Clean up
    munmap(shared_mem, SHARED_MEM_SIZE);
    close(shm_fd);
}

void displayData(){
    for(int i = 0; i < NUM_ITERATIONS; i++){
        std::cout << "Iteration: " << i << " duration: " << times_raw[i] << std::endl;
    }
}

int main() {
    // Create threads
    std::thread tA(threadA);
    std::thread tB(threadB);




    // Wait for threads to complete
    tA.join();
    tB.join();

    // Clean up shared memory
    shm_unlink(SHARED_MEM_NAME);

    displayData();
    return 0;
}
