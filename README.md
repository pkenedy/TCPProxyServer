# TCP Proxy Server

## Overview

This project implements a TCP proxy server that can handle up to 1000 simultaneous connections. The server forwards HTTP requests from clients to target servers and returns the responses back to the clients. It is designed to be simple yet efficient, with a thread pool for concurrent request handling.

## Features

- **Handles 1000 simultaneous connections:** The server can accept and manage multiple incoming connections.
- **Distributes connections over a thread pool:** Uses a thread pool to process connections concurrently.
- **Logs formatted data into a file:** All incoming requests and outgoing responses are logged with timestamps for easy debugging and monitoring.
- **Configurable parameters:** The port, backlog size, and thread pool size can be easily adjusted in the source code.

## Requirements

- Windows operating system
- Microsoft Visual Studio or compatible compiler
- C++11 or later
- `winsock2` library

## Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/paul/tcp_proxy_server.git
   cd tcp_proxy_server
