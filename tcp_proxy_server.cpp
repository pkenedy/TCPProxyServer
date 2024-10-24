#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <cstring>
#include <iomanip>
#include <chrono>

#pragma comment(lib, "Ws2_32.lib")

#define PORT 8081
#define BACKLOG 1000
#define BUFFER_SIZE 8192
#define THREAD_POOL_SIZE 5  // Number of threads in the pool

std::mutex log_mutex;
std::ofstream log_file("proxy_server.log");
std::mutex connection_mutex;
std::condition_variable connection_cv;
std::queue<SOCKET> connection_queue;
bool stop_server = false;

// Log function that includes timestamps and connection details
void log_to_file(const std::string& log_message, const std::string& log_level = "INFO") {
    std::lock_guard<std::mutex> guard(log_mutex);
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    log_file << "[" << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S") << "] "
             << "[" << log_level << "] " << log_message << std::endl;
    log_file.flush();  // Ensure data is written to the file immediately
}

// Function to handle each client connection
void handle_client(SOCKET client_socket) {
    char buffer[BUFFER_SIZE];
    int read_size;

    std::cout << "Waiting to receive data from client..." << std::endl; // Debug output
    read_size = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    
    std::cout << "Received data size: " << read_size << std::endl; // Debug output
    if (read_size <= 0) {
        std::cerr << "Failed to receive data from client or connection closed." << std::endl; // Debug output
        closesocket(client_socket);
        return;
    }

    buffer[read_size] = '\0';  // Null-terminate the buffer

    // Log the incoming HTTP request
    log_to_file("Received request:\n" + std::string(buffer));
    std::cout << "Received request logged." << std::endl; // Debug output

    // Extract the target host and port from the request (assuming HTTP/1.1 format)
    std::string request(buffer);
    std::string target_host;
    int target_port = 80; // Default HTTP port

    // Find the target host from the request line
    size_t pos = request.find("Host: ");
    if (pos != std::string::npos) {
        size_t start = pos + 6; // Start after "Host: "
        size_t end = request.find("\r\n", start);
        target_host = request.substr(start, end - start);

        // Check if the host contains a port
        size_t port_pos = target_host.find(":");
        if (port_pos != std::string::npos) {
            target_port = std::stoi(target_host.substr(port_pos + 1));
            target_host = target_host.substr(0, port_pos);
        }
    }

    // Create a socket to connect to the target server
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(target_port);
    
    // Resolve host to IP address
    struct addrinfo hints = {0}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(target_host.c_str(), nullptr, &hints, &result) != 0 || !result) {
        log_to_file("Failed to resolve host: " + target_host, "ERROR");
        std::cerr << "Failed to resolve host: " << target_host << std::endl; // Debug output
        closesocket(client_socket);
        closesocket(server_socket);
        return;
    }

    server_addr.sin_addr = ((struct sockaddr_in*)result->ai_addr)->sin_addr;
    freeaddrinfo(result);

    // Connect to the target server
    if (connect(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        log_to_file("Failed to connect to the target server: " + target_host, "ERROR");
        std::cerr << "Failed to connect to the target server: " << target_host << std::endl; // Debug output
        closesocket(client_socket);
        closesocket(server_socket);
        return;
    }

    // Forward the HTTP request to the target server
    send(server_socket, buffer, read_size, 0);

    // Receive the response from the target server
    while ((read_size = recv(server_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[read_size] = '\0';  // Null-terminate the buffer

        // Log the response from the server
        log_to_file("Forwarded response:\n" + std::string(buffer));
        std::cout << "Forwarded response logged." << std::endl; // Debug output

        // Send the response back to the client
        send(client_socket, buffer, read_size, 0);
    }

    // Clean up sockets
    closesocket(server_socket);
    closesocket(client_socket);
}

// Worker function for handling connections
void worker_thread() {
    while (true) {
        SOCKET client_socket;
        
        {
            std::unique_lock<std::mutex> lock(connection_mutex);
            connection_cv.wait(lock, [] { return !connection_queue.empty() || stop_server; });
            
            if (stop_server && connection_queue.empty()) return; // Exit if stopping and queue is empty

            client_socket = connection_queue.front();
            connection_queue.pop();
        }

        handle_client(client_socket);  // Handle the client connection
    }
}

// Main function to start the server
int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        WSACleanup();
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed." << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, BACKLOG) == SOCKET_ERROR) {
        std::cerr << "Listen failed." << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Starting HTTP Proxy Server on port " << PORT << " with " << THREAD_POOL_SIZE << " threads." << std::endl;

    // Start worker threads
    std::vector<std::thread> thread_pool;
    for (int i = 0; i < THREAD_POOL_SIZE; ++i) {
        thread_pool.emplace_back(worker_thread);
    }

    while (!stop_server) {
        SOCKET client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed." << std::endl;
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(connection_mutex);
            connection_queue.push(client_socket);
        }

        connection_cv.notify_one();  // Notify a worker thread that a new connection is available
    }

    // Stop server and join worker threads
    stop_server = true;
    connection_cv.notify_all();  // Notify all threads to exit

    for (auto& thread : thread_pool) {
        thread.join();
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
