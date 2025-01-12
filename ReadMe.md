# How to Run Each Part

## 1. Spinlocks

### Intraprocess

```bash
# Compile the program
g++ -std=c++17 -pthread -o out/spinlock_intraproc spinlock_intraproc.cpp

# Run the program
./out/spinlock_intraproc
```
Es gibt noch eine optimierte Version des Spinlocks mit niedrigeren Zeiten:
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
The shell scripts verify the location of the ZMQ header and lib file. On my system, they are installed in the default homebrew directory.
These paths are linked in the compile command. The shell scripts verify the locations, run the compile command and the executable.


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
Docker Daemon needs to be running.

Run the Docker-based setup using the provided shell script:
The Client proess enters an infinite loop at the end. This way, the container persists, and a new terminal can be opened to 
extract the timing data from the container memory.
```bash
# Navigate to the /docker directory
cd /docker

# Build images 
docker build -t uds-server -f Dockerfile.server .
docker build -t uds-client -f Dockerfile.client .

# Create shared volume
docker volume create uds_shared_volume

# Make the run cript executable
chmod +x run_docker.sh

# Run the script
./run_docker.sh
```
The run script starts server and client containers in seperate terminals (terminal will open two extra terminals). They should start interacting on their own. 
After some time, the client will output processing times and then enter an infinite loop.

At this point, the data is saved in the Container. 
Now, in a fourth terminal, the data can be copied from the docker volume to the machine. 
The target path needs to be adjusted for this.
```bash
docker cp uds-client:/app/docker_client.txt /Users/maywald/C_projects/OS2/OS2_Analysis/data/docker.txt
```
After copying, the running docker terminals can be force-quit with strg+c, as all data is collected and saved.

---
In /OS2_Analysis, the data is saved and analysed using a python script.
The python script can be executed (easiest in Python IDE, e.g. PyCharm) and will read all data files, preprocess the 
contents, display a histogram and calculate and output the confidence interval to the console.