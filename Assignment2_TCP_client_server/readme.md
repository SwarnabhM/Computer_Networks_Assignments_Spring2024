# Concurrent TCP Socket Communication

This assignment focuses on demonstrating the usage of TCP sockets and how concurrent TCP clients can be handled using fork() calls in a Unix-based environment.

## Overview

Reliable file transfer is achieved using TCP sockets and concurrency is achieved by utilizing the fork() system call, which creates child processes to handle each client connection independently.

## Key Concepts

- **TCP Sockets**: Transmission Control Protocol (TCP) is a standard communication protocol used for transmitting data over networks. Sockets provide the interface for communication between two processes.

- **Concurrency**: Concurrency refers to the ability of a system to handle multiple tasks or processes simultaneously. In the context of TCP communication, concurrency allows a server to handle multiple client connections concurrently.

- **fork() System Call**: The fork() system call is used to create a new process (child process) that is an exact copy of the calling process (parent process). In the context of concurrent TCP servers, fork() is commonly used to handle multiple client connections.

## Usage

To run the assignment:
1. Compile the server and client programs.
2. Start the TCP server.
3. Run multiple instances of the TCP clients to simulate concurrent client connections.
4. Observe how the server handles multiple client connections simultaneously using fork().
5. To check how concurrency is being handled try using sleep() calls with random values in the child processes.

## Files Included

- **server.c**: Source code for the TCP server.
- **client.c**: Source code for the TCP client.
- **README.md**: This README file providing an overview of the assignment.
- **Test Files**: Contains text files for testing correctness of the code.

## Requirements

- C Compiler (e.g., gcc)
- Unix-like Operating System (e.g., Linux, macOS)
