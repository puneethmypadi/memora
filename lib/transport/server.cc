#include<stdint.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<netinet/ip.h>
#include "server.hpp"
#include <iostream>
#include <sstream>

namespace Transport
{
    Server::Server() : port(0) {}

    Server::~Server() {}

    void Server::start(int port)
    {
        this->port = port;
        /**
         * Create a socket -> socket takes 3 arguments: type of socket, type of protocol, and type of communication
         * AF_INET for IPv4, SOCK_STREAM for TCP, and 0 for default protocol (0 for TCP when Stream socket)
         * 
         * Protocol	Arguments
            IPv4+TCP	socket(AF_INET, SOCK_STREAM, 0)
            IPv6+TCP	socket(AF_INET6, SOCK_STREAM, 0)
            IPv4+UDP	socket(AF_INET, SOCK_DGRAM, 0)
            IPv6+UDP	socket(AF_INET6, SOCK_DGRAM, 0)
        */

        this->serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0)
        {
            std::perror("socket");
            exit(EXIT_FAILURE);
        }


        // Set the socket options to allow reuse of the address
        /**
         * Parameter description:
         * - serverSocket: The socket file descriptor.
         * - SOL_SOCKET: The level at which the option is defined (socket level)/IPPROTO_TCP.
         * - SO_REUSEADDR: The option name to set.: https://stackoverflow.com/questions/3229860/what-is-the-meaning-of-so-reuseaddr-setsockopt-option-linux/3233022#3233022
         * - &opt: A pointer to the option value (in this case, an integer).
         * - sizeof(opt): The size of the option value.
         */
        int opt = 1;
        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            std::perror("setsockopt");
            close(serverSocket);
            exit(EXIT_FAILURE);
        }
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        /** 
         * htonl(0) is used to convert the address from host byte order to network byte order.
         * host is LE (little-endian) and network is BE (big-endian).  
         * for 32-bit numbers (IP addresses).
         */
        serverAddr.sin_addr.s_addr = htonl(0);
        /**
         * htons(port) is used to convert the port number from host byte order to network byte order.
         * htons is used for 16-bit numbers (port numbers).
         */
        serverAddr.sin_port = htons(port);

        /**
         * Bind the socket to the address and port
         * - serverSocket: The socket file descriptor.
         * - (sockaddr *)&serverAddr: A pointer to the sockaddr structure containing the address and port.
         * - sizeof(serverAddr): The size of the sockaddr structure.
         */
        if (bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            std::perror("bind");
            close(serverSocket);
            exit(EXIT_FAILURE);
        }

        /**
         * Listen for incoming connections
         * - serverSocket: The socket file descriptor.
         * - 5: The maximum number of pending connections in the queue.
         */
        if (listen(serverSocket, SOMAXCONN) < 0)
        {
            std::perror("listen");
            close(serverSocket);
            exit(EXIT_FAILURE);
        }
        std::cout << "Server started on port " << port << std::endl;
        running = true;
        while (true)
        {
            sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);
            int clientSocket = accept(serverSocket, (sockaddr *)&clientAddr, &clientAddrLen);
            if (clientSocket < 0)
            {
                std::perror("accept");
                continue;
            }
            handleClient(clientSocket);
            close(clientSocket);
        }

        close(serverSocket);
    }
    void Server::stop()
    {
        if (running)
        {
            close(serverSocket);
            running = false;
        }
    }
    void Server::handleClient(int clientSocket)
    {
        char buffer[1024];
        ssize_t bytesRead;
        std::ostringstream oss;
        /**
         * recv() is used to receive data from the client.
         * - clientSocket: The socket file descriptor for the client connection.
         * - buffer: A pointer to the buffer where the received data will be stored.
         * - sizeof(buffer) - 1: The maximum number of bytes to read (leaving space for null-terminator).
         * - 0: Flags (0 means no special flags).
         */
        while((bytesRead= recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
            oss.write(buffer, bytesRead);
        }

        
        if (bytesRead < 0)
        {
            std::perror("recv");
            return;
        }
        // buffer[bytesRead] = '\0'; // Null-terminate the received data
        std::cout << "Received: " << oss.str() << std::endl;

        // Process the request and send a response
        std::string response = "Hello from server!";
        sendResponse(clientSocket, response);
    } 
    void Server::sendResponse(int clientSocket, const std::string &response)
    {
        /**
         * send() is used to send data to the client.
         * - clientSocket: The socket file descriptor for the client connection.
         * - response.c_str(): A pointer to the data to be sent (converted to C-style string).
         * - response.size(): The size of the data to be sent.
         * - 0: Flags (0 means no special flags).
         */
        ssize_t bytesSent = send(clientSocket, response.c_str(), response.size(), 0);
        if (bytesSent < 0)
        {
            std::perror("send");
        }
    }

}