#include "Mgard300_Handler.h"
#include <iostream>
#include <stdexcept>
#include <thread>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iomanip>
#include <queue>
#include "Common.h"

Mgard300_Handler::Mgard300_Handler(sockpp::tcp_acceptor& acceptor) : acceptor_(acceptor), current_state(nullptr),check_exist_connection(false) {
    this->number_connection = 0;
    this->_logger = spdlog::get("DET_logger");
}

Mgard300_Handler::~Mgard300_Handler() {
    delete this->current_state;
}

// -------------------------------------------------------------------------

// Create a function for the packet analysis thread to execute
void handler_parse_msg_thread(Mgard300_Handler* mgard300_Handler) {
    while (true) {
    	pthread_t id = pthread_self();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        // Call the MsgHandler object to parse packets from the client
        int ret = mgard300_Handler->parse_msg_client(mgard300_Handler->get_current_socket());
        //std::cout << "Number of Connections: " << mgard300_Handler->get_number_connection() << " ID thread: " << id << std::endl;
        mgard300_Handler->getLogger()->info("Number of Connections: {} ID thread: {}", mgard300_Handler->get_number_connection(), id);
        if (ret == -1) {
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
        // Check the number of connections
        if (mgard300_Handler->get_number_connection() == 2) {
        	mgard300_Handler->set_check_exist_connection(true);
            mgard300_Handler->decrement_number_connection();
            return;
        }
    }
}

// Create a function for the state check thread to execute
void execute_cmd_thread(Mgard300_Handler* mgard300_Handler) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        int q_execute_cmd = mgard300_Handler->get_state();
        switch (q_execute_cmd) {
            case DET_STATE_WORK:
                mgard300_Handler->transition_to_state(new WorkState());
                break;
            case DET_STATE_SLEEP:
                mgard300_Handler->transition_to_state(new SleepState());
                break;
            case DET_STATE_CLOSE:
                mgard300_Handler->transition_to_state(new CloseState());
                break;
            case DET_STATE_TRIGGER:
                mgard300_Handler->transition_to_state(new TriggerState());
                break;
            default:
                break;
        }
    }
}

int Mgard300_Handler::send_msg(int cmd, unsigned char param0, unsigned char param1) {
    unsigned char buf[5];
    buf[0] = MARKER_HEAD;
    buf[1] = static_cast<unsigned char>(cmd);
    buf[2] = param0;
    buf[3] = param1;
    buf[4] = MARKER_TAIL;
    int n = this->get_current_socket().write_n(buf, 5);
    if (n < 0) {
        this->_logger->warn("Error sending data to client!");
        return -1;
    } else {
        this->_logger->info("Sent 5 bytes to client");
    }
    return n;
}

// --------------------------------------------------------------------------
void Mgard300_Handler::close_all_threads() {
    for (auto& kv : tm_) {
        if (kv.second.joinable()) {
            kv.second.std::thread::~thread();
            std::cout << "Thread " << kv.first << " detached and closed immediately:" << std::endl;
        }
    }
    tm_.clear();
}
void Mgard300_Handler::start_thread_parse_thread() {
	std::thread parse_thread = std::thread(handler_parse_msg_thread, this);
	parse_thread.detach();
	this->tm_["parse_thread"] = std::move(parse_thread);
	std::cout << "Thread " << "parse_thread" << " created" << std::endl;
}
void Mgard300_Handler::start_thread_checkstate_thread() {
	std::thread checkstate_thread = std::thread(execute_cmd_thread, this);
	checkstate_thread.detach();
	this->tm_["checkstate_thread"] = std::move(checkstate_thread);
	std::cout << "Thread " << "checkstate_thread" << " created" << std::endl;
}
void Mgard300_Handler::handle_connections() {
    while (true) {
    	// close all old thread
    	this->close_all_threads();
        sockpp::inet_address peer;
        this->current_socket = this->acceptor_.accept(&peer);
        this->increment_number_connection();
        if (!this->current_socket) {
            this->_logger->warn("Error accepting incoming connection: {}", acceptor_.last_error_str());
            continue;
        }
        this->_logger->info("Received a connection request from {}", peer.to_string());
        this->start_thread_parse_thread();
        this->start_thread_checkstate_thread();

        if (!this->get_current_socket()) {
            break;
        }
    }
}

// --------------------------------------------------------------------------

int Mgard300_Handler::parse_msg_client(sockpp::tcp_socket& socket) {
    try {
        // Read data from socket
        this->n_read_bytes = socket.read(this->buf, sizeof(this->buf));
        this->_logger->info("Received: {} bytes from client", n_read_bytes);
        // Check whether the correct number of bytes has been read

        if (this->n_read_bytes < sizeof(this->buf)) {
            throw std::runtime_error("Not enough bytes read");
            return -1;
        }

#ifdef DEBUG
        // Check for mismatched markers
        if (buf[0] != MARKER_HEAD || buf[4] != MARKER_TAIL) {
            throw std::runtime_error("Invalid markers");
        }
#endif
        switch (this->buf[1]) {
            case DET_STATE_SLEEP:
                this->set_state(DET_STATE_SLEEP);
                break;
            case DET_STATE_WORK:
                this->set_state(DET_STATE_WORK);
                break;
            case DET_STATE_CLOSE:
                this->set_state(DET_STATE_CLOSE);
                break;
            case DET_STATE_TRIGGER:
                this->set_state(DET_STATE_TRIGGER);
                break;
            default:
                break;
        }
        // send back to the client
        this->send_msg(this->buf[1], this->buf[2], this->buf[3]);

    } catch (const std::exception& e) {
#ifdef DEBUG
        this->_logger->error("Exception: {}", e.what());
#endif
        return -1;
    }

    return 0;
}

// --------------------------------------------------------------------------

void Mgard300_Handler::transition_to_state(DET_State* new_state) {
    if (this->current_state != nullptr) {
        delete this->current_state;
    }
    this->current_state = new_state;
    this->current_state->handle(*this);
}

// --------------------------------------------------------------------------

void Mgard300_Handler::send_data_to_client(const char* data, size_t data_size) {
    try {
        const size_t chunk_size = CHUNK_SIZE;
        size_t remaining_size = data_size;
        size_t offset = 0;
        size_t total_sent = 0;
        this->set_is_client_closed(true);
        while (remaining_size > 0 && this->get_is_client_closed()) {
            // Determines the size of the chunk for the send
            size_t current_chunk_size = std::min(chunk_size, remaining_size);

            // Send chunk to client
            int n = this->current_socket.write_n(data + offset, current_chunk_size);

            offset += n;
            remaining_size -= n;
            total_sent += n;

            if (n < 0) {
                this->_logger->error("Error sending data to client!");
                this->set_is_client_closed(false);
                break;
            }

#ifdef DEBUG
            if (remaining_size <= 0 && offset != data_size) {
                this->_logger->warn("Error: Incomplete last chunk sent to client!");
                throw std::runtime_error("Incomplete last chunk sent to client");
            }
            // Displays the remaining bytes
            this->_logger->info("Remaining bytes: {} | Total sent: {}", remaining_size, total_sent);
#endif
        }
        if (!this->get_is_client_closed()) {
            this->_logger->info("Client disconnected. Handling reconnect...");
            this->set_is_client_closed(true);
            this->handle_connections();
        }
        this->_logger->info("Sent all data to Client");
    } catch (const std::exception& e) {
#ifdef DEBUG
        this->_logger->warn("Exception: {}", e.what());
#endif
    }
}

// --------------------------------------------------------------------------

sockpp::tcp_socket& Mgard300_Handler::get_current_socket() {
    std::lock_guard<std::mutex> lock(this->connection_mutex);
    return this->current_socket;
}

int Mgard300_Handler::get_number_connection() {
    std::lock_guard<std::mutex> lock(this->connection_mutex);
    return this->number_connection;
}

void Mgard300_Handler::increment_number_connection() {
    std::lock_guard<std::mutex> lock(this->connection_mutex);
    this->number_connection++;
}

void Mgard300_Handler::decrement_number_connection() {
    std::lock_guard<std::mutex> lock(this->connection_mutex);
    if (this->number_connection > 0) {
        this->number_connection--;
    }
}

int Mgard300_Handler::get_state() {
    std::lock_guard<std::mutex> lock(this->connection_mutex);
    int q_execute_cmd;
    if (!this->q_execute_cmd.empty()) {
        q_execute_cmd = this->q_execute_cmd.front();
        this->q_execute_cmd.pop();
    } else {
        q_execute_cmd = -1;
    }
    return q_execute_cmd;
}

void Mgard300_Handler::set_state(int new_state_) {
    std::lock_guard<std::mutex> lock(this->connection_mutex);
    this->q_execute_cmd.push(new_state_);
}

bool Mgard300_Handler::get_is_client_closed() {
    std::lock_guard<std::mutex> lock(this->connection_mutex);
    return this->is_client_closed;
}

void Mgard300_Handler::set_is_client_closed(bool isClientClosed) {
    std::lock_guard<std::mutex> lock(this->connection_mutex);
    this->is_client_closed = isClientClosed;
}

void Mgard300_Handler::close_socket() {
    std::lock_guard<std::mutex> lock(this->connection_mutex);
    if (this->current_socket.is_open()) {
        this->current_socket.close();
        std::cout << "Socket closed." << std::endl;
    }
}

const std::shared_ptr<spdlog::logger>& Mgard300_Handler::getLogger() {
    return _logger;
}
void Mgard300_Handler::set_check_exist_connection(bool check_exit) {
	std::lock_guard<std::mutex> lock(this->connection_mutex);
	this->check_exist_connection = check_exit;
}
bool Mgard300_Handler::get_check_exist_connection() {
	std::lock_guard<std::mutex> lock(this->connection_mutex);
	return this->check_exist_connection;
}
