# How to Run Each Part

## 1. Spinlocks

### Intraprocess

```bash
# Compile the program
g++ -std=c++17 -pthread -o out/spinlock_intraproc spinlock_intraproc.cpp

# Run the program
./out/spinlock_intraproc
```
Es gibt noch eine optimierte Version des Spinlocks mit nidrigeren Zeiten:
```bash
# Compile the program
g++ -std=c++17 -pthread -o out/better_spinlock better_spinlock.cpp

# Run the programx^
./out/better_spinlock
```
---

## 2. Semaphore

### Intraprocess
```bash
# Compile the program
g++ -std=c++17 -o out/sem_intraproc sem_intraproc.cpp -pthread

# Run the program
./out/sem_intraproc
```



---

## 3. ZeroMQ

### 3.1 Intraprocess (zmq:inproc)
Use the provided shell script to run the intraprocess ZeroMQ communication:
```bash
# Make the script executable
chmod +x run_zmq_intraproc.sh

# Run the script
./run_zmq_intraproc.sh
```

### 3.2 Interprocess (zmq:ipc)
Use the provided shell script to run the interprocess ZeroMQ communication:
```bash
# Make the script executable
chmod +x run_zmq_interproc.sh

# Run the script
./run_zmq_interproc.sh
```

---

## 4. Docker

Run the Docker-based setup using the provided shell script:
The Client proess enters an infinite loop at the end. This way, the container persists, and a new terminal can be opened to 
extract the timing data from the container memory.
```bash
# Navigate to the /docker directory
cd /docker

# Make the script executable
chmod +x run_docker.sh

# Run the script
./run_docker.sh
```
It can then be copied out with, where the target directory needs to be changed.

After copying, the running docker terminals can be force-quit with strg+c, as all data is collected and saved.
```bash
docker cp uds-client:/app/docker_client.txt /Users/maywald/C_projects/OS2/OS2_Analysis/data/docker.txt
```
---

