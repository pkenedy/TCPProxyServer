# HTTP Proxy Server

This project implements a multithreaded HTTP proxy server using C++. The server accepts incoming HTTP requests from clients, forwards them to the appropriate target server, and sends the response back to the client. The implementation utilizes sockets for network communication and includes logging functionality to track requests and responses.

## Features

- **Multithreaded Processing**: Utilizes a thread pool to handle multiple client connections concurrently.
- **Logging**: Logs incoming requests and outgoing responses with timestamps for debugging and monitoring.
- **Connection Handling**: Uses a queue to manage incoming client connections efficiently.

## Implementation Approach

1. **Socket Initialization**: 
   - The server initializes Winsock and creates a listening socket on a specified port (default is 8081).

2. **Thread Pool**:
   - A fixed number of worker threads are created to handle client connections. These threads wait for incoming connections and process them from a shared queue.

3. **Client Handling**:
   - When a client connects, the server accepts the connection and adds it to a queue.
   - A worker thread picks up the connection from the queue and processes it by reading the incoming request, extracting the target server's host and port, and forwarding the request.

4. **Request Forwarding**:
   - The server creates a new socket to connect to the target server, sends the client's request, and waits for the response.
   - Upon receiving the response, it logs the data and sends it back to the original client.

5. **Logging**:
   - All significant events, such as connection acceptance, request reception, and response forwarding, are logged into a file (`proxy_server.log`) for monitoring purposes.

6. **Graceful Shutdown**:
   - The server can be stopped gracefully, ensuring all worker threads finish processing their current tasks before shutting down.

## Usage

### Compilation

To compile the server, ensure you have a C++ compiler set up (e.g., MSYS2) and run the following command:

```bash
 g++ -o tcp_proxy_server tcp_proxy_server.cpp -lws2_32 -lpthread



run the Proxy Server:

''''bash

./tcp_proxy_server

Testing 

 Proxy: Use a tool like Postman or curl to send requests to http://localhost:8080. The server should process the requests, log them to logs/proxy_log.txt, and return a simple response.