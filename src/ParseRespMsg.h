// MsgHandler.h
#pragma once

#include <sockpp/tcp_acceptor.h>
#include <vector>

class ParseRespMsg {
public:
	ParseRespMsg(sockpp::tcp_acceptor& acceptor);
    ~ParseRespMsg(); // Hàm hủy để giải phóng bộ nhớ
    void handleConnections();
    sockpp::tcp_socket& getCurrentSocket();
    int recvMsgClient(sockpp::tcp_socket& socket);
    void parseMsg(char *str, ssize_t number_of_bytes, unsigned short value, char *id);

    char* getid() const;
private:
    sockpp::tcp_acceptor& acceptor_;
    ssize_t n_read_bytes;
    unsigned char buf[7];
    sockpp::tcp_socket currentSocket;
    char *str;
    unsigned short value;
    char *id;
};
