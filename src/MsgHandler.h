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
    sockpp::tcp_socket& getCurrentSocket();
    void setCurrentSocket(const sockpp::tcp_socket& socket);
    int parseMsgClient(sockpp::tcp_socket& socket);
    pthread_mutex_t& getConnectionMutex();
    int getNumberConnection();
	void incrementNumberConnection();
	void decrementNumberConnection();
	void setThread(pthread_t threadId);
private:
    sockpp::tcp_acceptor& acceptor_;
    ssize_t n_read_bytes;
    unsigned char buf[5];
    State* currentState;
    sockpp::tcp_socket currentSocket;
    int numberConnection;
    pthread_mutex_t connectionMutex;
};
