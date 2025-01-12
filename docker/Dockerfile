FROM ubuntu:latest

# Install build tools
RUN apt-get update && apt-get install -y build-essential

# Set the working directory
WORKDIR /app

# Copy the C++ files into the container
COPY server.cpp client.cpp ./

# Compile the C++ programs
RUN g++ -o server server.cpp
RUN g++ -o client client.cpp

# Default command (not used when explicitly running server or client)
CMD ["bash"]
