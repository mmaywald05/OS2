#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

#define SOCKET_PATH "/shared/uds_socket"
#define BUFFER_SIZE 1

int main() {
    int server_fd, client_fd;
    struct sockaddr_un address;
    char buffer[BUFFER_SIZE];

    // Create socket
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        return 1;
    }

    // Bind socket to path
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1);

    unlink(SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == -1) {
        perror("Socket bind failed");
        return 1;
    }

    // Listen for connections
    if (listen(server_fd, 1) == -1) {
        perror("Socket listen failed");
        return 1;
    }

    std::cout << "Server: Waiting for a connection...\n";

    // Accept a client connection
    if ((client_fd = accept(server_fd, nullptr, nullptr)) == -1) {
        perror("Socket accept failed");
        return 1;
    }

    std::cout << "Server: Connection established.\n";

    while (true) {
        int bytes_read = read(client_fd, buffer, BUFFER_SIZE);

        if (bytes_read > 0) {
            if (buffer[0] == 'P') {
                send(client_fd, buffer, BUFFER_SIZE, 0);
            } else if (buffer[0] == 'K') {
                std::cout << "Server: Terminating on client request.\n";
                break;
            }
        }
    }

    //close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);
    return 0;
}
