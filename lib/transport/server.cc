#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
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
        char buffer[4 + MAX_BUFFER_SIZE] = {};
        errno = 0;
        // Read the length of the message
        auto err = readMessage(clientSocket, &buffer[0], 4);
        if (err)
        {
            std::perror(errno == 0? "EOF": "readMessage");
            return;
        }
        errno = 0;
        uint32_t len = 0;
        memcpy(&len, buffer, 4);  // assume little endian
        if (len > MAX_BUFFER_SIZE) {
            std::perror("Message too long");
            close(clientSocket);
            return;         
        }
        err = readMessage(clientSocket, &buffer[4], len);
        if(err)
        {
            std::perror("Read error");
        }
        buffer[4+len] = '\0'; // Null-terminate the string
        std::cout << "Received: " << &buffer[4] << std::endl;

        // Process the request and send a response
        std::string response = "Hello from server!";
        sendResponse(clientSocket, response);
    }
    void Server::sendResponse(int clientSocket, const std::string &response)
    {
        // Send the length of the response
        uint32_t len = response.size();
        std::cout << "Sending: " << response << std::endl;
        writeMessage(clientSocket, (const char *)&len, 4);
        // Send the response
        writeMessage(clientSocket, response.c_str(), len);  
    }

    int32_t Server::readMessage(int clientSocket, char *buffer, size_t size)
    {
        while (size > 0)
        {

            /**
             * recv() is used to receive data from the client.
             * - clientSocket: The socket file descriptor for the client connection.
             * - buffer: A pointer to the buffer where the received data will be stored.
             * - sizeof(buffer) - 1: The maximum number of bytes to read (leaving space for null-terminator).
             * - 0: Flags (0 means no special flags).
             */
            auto rv = recv(clientSocket, buffer, size, 0);
            if (rv <= 0)
            {
                /**
                 * If the read call is interrupted by a signal, errno will be set to EINTR and the read will return -1.
                 * In this case, we should check errno and if it is EINTR, reset it to 0, we should continue reading.
                 */
                if(errno == EINTR) {
                    errno = 0; // Reset errno to 0
                    continue; // Interrupted by a signal, try again
                }
                return -1;
            }
            assert((size_t)rv <= size);

            size -= (size_t)rv;
            buffer += rv;    
        }
        return 0;
    }

    int32_t Server::writeMessage(int clientSocket, const char *buffer, size_t size)
    {
        while (size > 0)
        {
            /**
             * send() is used to send data to the client.
             * - clientSocket: The socket file descriptor for the client connection.
             * - response.c_str(): A pointer to the data to be sent (converted to C-style string).
             * - response.size(): The size of the data to be sent.
             * - 0: Flags (0 means no special flags).
             */
            auto rv = send(clientSocket, buffer, size, 0);
            if (rv <= 0)
            {
                return -1;
            }
            assert((size_t)rv <= size);
            size -= rv;
            buffer += rv;
        }
        return 0;
    }

}