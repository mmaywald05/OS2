#!/bin/bash

# Script to automate Docker UDS setup and execution with synchronized cleanup

# Set variables
IMAGE_NAME="uds-example"
SERVER_CONTAINER="uds-server"
CLIENT_CONTAINER="uds-client"
SHARED_VOLUME="uds_shared_volume"
DOCKERFILE_PATH="."

# Function to build Docker image
build_image() {
  echo "Building Docker image..."
  docker build -t $IMAGE_NAME $DOCKERFILE_PATH
  if [ $? -ne 0 ]; then
    echo "Failed to build Docker image. Exiting."
    exit 1
  fi
}

# Function to create Docker volume
create_volume() {
  echo "Creating shared volume..."
  docker volume create $SHARED_VOLUME
  if [ $? -ne 0 ]; then
    echo "Failed to create Docker volume. Exiting."
    exit 1
  fi
}

# Function to start server container in a new terminal
start_server() {
  echo "Starting server container in a new terminal..."
  osascript <<EOF
tell application "Terminal"
    do script "docker run --name $SERVER_CONTAINER --volume $SHARED_VOLUME:/shared $IMAGE_NAME ./server"
end tell
EOF
}

# Function to start client container in a new terminal
start_client() {
  echo "Starting client container in a new terminal..."
  osascript <<EOF
tell application "Terminal"
    do script "docker run --name $CLIENT_CONTAINER --volume $SHARED_VOLUME:/shared $IMAGE_NAME ./client"
end tell
EOF
}

# Function to wait for containers to finish
wait_for_containers() {
  echo "Waiting for containers to complete..."
  docker wait $SERVER_CONTAINER
  docker wait $CLIENT_CONTAINER

}

# Function to cleanup Docker resources
cleanup() {
  echo "Cleaning up..."
  docker rm -f $SERVER_CONTAINER $CLIENT_CONTAINER >/dev/null 2>&1
  docker volume rm $SHARED_VOLUME
  if [ $? -ne 0 ]; then
    echo "Failed to remove Docker volume. You may need to remove it manually."
  fi
}

# Main script execution
echo "Starting UDS Docker automation script..."

# Build the Docker image
build_image

# Create the shared volume
create_volume

# Start the server container
start_server

# Wait a moment for the server to initialize
sleep 2

# Start the client container
start_client

# Wait for server and client to finish
wait_for_containers

# Cleanup
cleanup

echo "Script completed successfully!"
