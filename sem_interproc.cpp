#include <iostream>
#include <thread>
#include <semaphore.h> // POSIX semaphores
#include <atomic>
#include <fcntl.h>    // For O_CREAT and O_EXCL
#include <sys/mman.h> // For shared memory
#include <unistd.h>   // For fork
#include <map>
#include <fstream>
#include <chrono>

// Timing
using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<double, std::nano>; // Nanoseconds
#define NUM_ITERATIONS 1000000

// Shared memory
struct SharedMemory {
    std::atomic<bool> bit; // Shared bit for communication
};

// Semaphore names
const char* SEMAPHORE_A_NAME = "/semaphoreA";
const char* SEMAPHORE_B_NAME = "/semaphoreB";

// Globals
sem_t* semaphoreA = nullptr;
sem_t* semaphoreB = nullptr;

double duration_raw[NUM_ITERATIONS];

// Utility to write results to file
void writeToFile(const std::string& name, const std::map<double, int>& elapsed) {
    std::ofstream outFile(name + ".txt");
    for (const auto& entry : elapsed) {
        outFile << entry.first << ":" << entry.second << "\n";
    }
}

void print_duration(){
    for(int i=0; i<NUM_ITERATIONS; ++i){
        std::cout << i<<": "<< duration_raw[i]<<"ns"<<std::endl;
    }
}

// Thread A (Child Process)
void threadA(SharedMemory* shared_mem) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Wait for semaphore A
        auto start = Clock::now();
        sem_wait(semaphoreA);
        auto end = Clock::now();

        // Record timing
        double duration = std::chrono::duration_cast<Duration>(end - start).count();
        duration_raw[i]=duration;

        // Set bit
        shared_mem->bit.store(true, std::memory_order_relaxed);

        // Signal Thread B
        sem_post(semaphoreB);
    }
}

// Thread B (Parent Process)
void threadB(SharedMemory* shared_mem) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Wait for semaphore B
        sem_wait(semaphoreB);

        // Read and reset bit
        if (shared_mem->bit.load(std::memory_order_relaxed)) {
            shared_mem->bit.store(false, std::memory_order_relaxed);
        }

        // Signal Thread A
        sem_post(semaphoreA);
    }
}

int main() {
    // Create shared memory
    SharedMemory* shared_mem = static_cast<SharedMemory*>(mmap(
            nullptr, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    if (shared_mem == MAP_FAILED) {
        perror("mmap");
        return EXIT_FAILURE;
    }
    shared_mem->bit = false;

    // Create named semaphores
    semaphoreA = sem_open(SEMAPHORE_A_NAME, O_CREAT | O_EXCL, 0666, 1); // A starts
    semaphoreB = sem_open(SEMAPHORE_B_NAME, O_CREAT | O_EXCL, 0666, 0); // B waits
    if (semaphoreA == SEM_FAILED || semaphoreB == SEM_FAILED) {
        perror("sem_open");
        return EXIT_FAILURE;
    }

    // Fork the process
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        // Child process runs Thread A
        threadA(shared_mem);
        exit(EXIT_SUCCESS);
    } else {
        // Parent process runs Thread B
        threadB(shared_mem);

        // Wait for child to finish
        wait(nullptr);

        // Write results and clean up
        print_duration();
        sem_close(semaphoreA);
        sem_close(semaphoreB);
        sem_unlink(SEMAPHORE_A_NAME);
        sem_unlink(SEMAPHORE_B_NAME);
        munmap(shared_mem, sizeof(SharedMemory));
    }

    return 0;
}
