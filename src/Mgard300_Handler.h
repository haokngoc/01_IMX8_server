#pragma once

#include <sockpp/tcp_acceptor.h>
#include <vector>
#include "DET_State.h"

class Mgard300_Handler {
public:
	// Constructor: Khởi tạo đối tượng MsgHandler với một TCP acceptor
	Mgard300_Handler(sockpp::tcp_acceptor& acceptor);
    ~Mgard300_Handler();
    void handle_connections(); // xử lý các kết nối đến từ client
    void send_data_to_client(const char* data, size_t data_size); // gửi dữ liệu đến client đã kết nối
    void transition_to_state(DET_State* new_state); // chuyển trạng thái
    sockpp::tcp_socket& get_current_socket();
    int parse_msg_client(sockpp::tcp_socket& socket); // phân tích gói tin từ client
    pthread_mutex_t& get_connection_mutex();
    int get_number_connection();
    void increment_number_connection();
    void decrement_number_connection();
    void set_state(int new_state);
    int get_state();

private:
    sockpp::tcp_acceptor& acceptor_;
    ssize_t n_read_bytes;
    unsigned char buf[5];
    DET_State* current_state;
    int state;
    sockpp::tcp_socket current_socket;
    int number_connection;
    pthread_mutex_t connection_mutex;
};
