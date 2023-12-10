
#include <iostream>
#include "sockpp/tcp_acceptor.h"
#include "MsgHandler.h"

int main(int argc, char* argv[]) {
    in_port_t port = 1024;
    sockpp::initialize();
    sockpp::tcp_acceptor acc(port);

    if (!acc) {
        std::cerr << "Error creating the acceptor: " << acc.last_error_str() << std::endl;
        return 1;
    }

    std::cout << "Awaiting connections on port " << port << "..." << std::endl;

    // Create MsgHandler object and handle connections
    MsgHandler msgHandler(acc);
    msgHandler.handleConnections();

    return 0;
}
