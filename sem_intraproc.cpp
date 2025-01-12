#include <iostream>
#include <thread>
#include <semaphore.h> // POSIX semaphores
#include <atomic>
#include <fcntl.h>    // For O_CREAT and O_EXCL
#include <sys/stat.h> // For mode constants
#include <unistd.h>   // For unlink
#include <fstream>
#define NUM_ITERATIONS 1000000
#define WARM_UP_ITERATIONS 10000


using Clock = std::chrono::high_resolution_clock;
auto t1 = Clock::now();
auto t2 = Clock::now();
double durations_raw[NUM_ITERATIONS];

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
    t1 = Clock ::now();
    for (int i = 0; i < NUM_ITERATIONS+WARM_UP_ITERATIONS; ++i) {
        // Wait for Thread A's turn
        sem_wait(semaphoreA);

        t2=Clock::now();
        auto result  = t2-t1;
        if(i>=WARM_UP_ITERATIONS) {
            durations_raw[i-WARM_UP_ITERATIONS] = result.count(); // Save time into raw array
        }
        t1=Clock::now();

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

void displayData(){
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
        outFile << i << ":" <<(long) elapsed[i] << "\n";
    }
    // Close the file
    outFile.close();
    std::cout << "data successfully written to " << filename << "\n";
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


    // Clean up semaphores
    sem_close(semaphoreA);
    sem_close(semaphoreB);
    sem_unlink(SEMAPHORE_A_NAME);
    sem_unlink(SEMAPHORE_B_NAME);


    displayData();
    saveData("sem_intraproc", durations_raw);
    return 0;
}
