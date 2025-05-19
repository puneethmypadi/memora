#include <stdio.h>
#include <cstdint>
#include "lib/transport/server.hpp"

int main() {
    Transport::Server server;
    server.start(3030);
    // server.stop();
    return 0;
}