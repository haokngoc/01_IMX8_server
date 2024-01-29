#pragma once

#include <sockpp/tcp_acceptor.h>
#include <vector>
#include "DET_State.h"
#include <queue>
#include "spdlog/spdlog.h"
#include "spdlog/async.h" //support for async logging
#include "spdlog/sinks/basic_file_sink.h"
class Mgard300_Handler {
public:

	Mgard300_Handler(sockpp::tcp_acceptor& acceptor);
    ~Mgard300_Handler();
    void handle_connections();
    void send_data_to_client(const char* data, size_t data_size);
    void transition_to_state(DET_State* new_state);
    sockpp::tcp_socket& get_current_socket();
    int parse_msg_client(sockpp::tcp_socket& socket);
    pthread_mutex_t& get_connection_mutex();
    int get_number_connection();
    void increment_number_connection();
    void decrement_number_connection();
    int get_state();
    void set_state(int new_state_);
    int send_msg(int cmd, unsigned char param0, unsigned char param1);
	bool get_is_client_closed();
	void set_is_client_closed(bool isClientClosed);
	void close_socket();
	const std::shared_ptr<spdlog::logger>& getLogger();
	void close_all_threads();
	void start_thread_parse_thread();
	void start_thread_checkstate_thread();
	void set_check_exist_connection(bool check_exit);
	bool get_check_exist_connection();
	void set_is_working(bool check_);
	bool get_is_working();
private:
    sockpp::tcp_acceptor& acceptor_;
    ssize_t n_read_bytes;
    unsigned char buf[5];
    DET_State* current_state;
    std::queue<int> q_execute_cmd;
    sockpp::tcp_socket current_socket;
    int number_connection;
    std::mutex connection_mutex;
    bool is_client_closed;
	std::shared_ptr<spdlog::logger> _logger;
	typedef std::unordered_map<std::string, std::thread> ThreadMap;
	ThreadMap tm_;
	bool close_threads;
	bool check_exist_connection;
	bool is_working;
};
