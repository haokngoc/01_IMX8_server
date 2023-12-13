// MsgHandler.h
#pragma once

#include "State.h"
#include <sockpp/tcp_acceptor.h>
#include <vector>

class MsgHandler {
public:
    MsgHandler(sockpp::tcp_acceptor& acceptor);
    ~MsgHandler(); // Hàm hủy để giải phóng bộ nhớ
    void handleConnections();
    void sendDataToClient(const std::vector<char>& data);
    void transitionToState(State* newState);
private:
    sockpp::tcp_acceptor& acceptor_;
    ssize_t n_read_bytes;
    unsigned char buf[5];
    State* currentState;
    sockpp::tcp_socket currentSocket;
    void parseMsgClient(sockpp::tcp_socket& socket);
};
