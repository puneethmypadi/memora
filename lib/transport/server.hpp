#include <string>
#include <cassert>
namespace Transport
{
    const size_t MAX_BUFFER_SIZE = 4096;
    class Server
    {
    public:
        Server();
        ~Server();

        void start(int port);
        void stop();

    private:
        int port;
        int serverSocket;
        bool running;

        void handleClient(int clientSocket);
        void sendResponse(int clientSocket, const std::string &response);

        int32_t readMessage(int clientSocket, char *buffer, size_t size);

        int32_t writeMessage(int clientSocket, const char *buffer, size_t size);

    };    
}