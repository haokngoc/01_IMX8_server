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
    sockpp::tcp_socket& getCurrentSocket();
    int parseMsgClient(sockpp::tcp_socket& socket);
    pthread_mutex_t& getConnectionMutex();
    int getNumberConnection();
	void incrementNumberConnection();
	void decrementNumberConnection();
	void setState(int newState_);
	int getSate() const;

private:
    sockpp::tcp_acceptor& acceptor_;
    ssize_t n_read_bytes;
    unsigned char buf[5];
    State* currentState;
    int sate;
    sockpp::tcp_socket currentSocket;
    int numberConnection;
    pthread_mutex_t connectionMutex;
};
