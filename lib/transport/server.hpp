#include <string>
namespace Transport
{
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
    };
}