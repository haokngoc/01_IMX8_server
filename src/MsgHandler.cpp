#include "MsgHandler.h"
#include <iostream>
#include <stdexcept>
#include <thread>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iomanip>

#define MARKER_HEAD 0xAA
#define MARKER_TAIL 0x55
#define DET_STATE_WORK 0x58
#define DET_STATE_SLEEP 0x1b
#define CHUNK_SIZE 1024
#define BUFFER_SIZE (2 * 1024 * 1024)

MsgHandler::MsgHandler(sockpp::tcp_acceptor& acceptor) : acceptor_(acceptor), current_state(nullptr) {
    this->number_connection = 0;
    pthread_mutex_init(&this->connection_mutex, nullptr);
}

MsgHandler::~MsgHandler() {
    delete this->current_state;
    pthread_mutex_destroy(&this->connection_mutex);
}

// --------------------------------------------------------------------------

// tạo hàm để luồng phân tích gói tin thực hiện
void* handler_parse_msg_thread(void* arg) {
    MsgHandler* msg_handler = static_cast<MsgHandler*>(arg);
    pthread_t id = pthread_self();
    while (true) {
        // Gọi đối tượng MsgHandler để phân tích gói tin từ client
        int ret = msg_handler->parse_msg_client(msg_handler->get_current_socket());
        if (ret == -1) {
            // chờ 3s và quay lại vòng lặp nhận tiếp
            sleep(3);
        }
        pthread_mutex_lock(&msg_handler->get_connection_mutex());
        std::cout << "Number of Connections: " << msg_handler->get_number_connection() << " ID thread: " << id << std::endl;

        // kiểm tra số lượng kết nối
        if (msg_handler->get_number_connection() == 2) {
            // giảm number_connection
            msg_handler->decrement_number_connection();
            pthread_mutex_unlock(&msg_handler->get_connection_mutex());
            pthread_exit(nullptr);
        }
        pthread_mutex_unlock(&msg_handler->get_connection_mutex());
    }
}

// tạo hàm để luồng check state thực hiện
void* check_state_thread(void* arg) {
    MsgHandler* msg_handler = static_cast<MsgHandler*>(arg);
    int current_state = msg_handler->get_state();
    switch (current_state) {
        case DET_STATE_WORK:
            msg_handler->transition_to_state(new WorkState());
            break;
        case DET_STATE_SLEEP:
            msg_handler->transition_to_state(new SleepState());
            break;
        default:
            break;
    }
}

// --------------------------------------------------------------------------

void MsgHandler::handle_connections() {
    pthread_t parse_thread;
    pthread_t checkstate_thread;
    while (true) {
        sockpp::inet_address peer;
        this->current_socket = acceptor_.accept(&peer);

        pthread_mutex_lock(&this->get_connection_mutex());
        this->increment_number_connection();
        pthread_mutex_unlock(&this->get_connection_mutex());

        if (!this->current_socket) {
            std::cerr << "Error accepting incoming connection: " << acceptor_.last_error_str() << std::endl;
            continue;
        }
        std::cout << "Received a connection request from " << peer << std::endl;

        int result = pthread_create(&parse_thread, nullptr, handler_parse_msg_thread, this);
        pthread_create(&checkstate_thread, nullptr, check_state_thread, this);
        if (result != 0) {
            std::cerr << "Error creating thread: " << strerror(result) << std::endl;
        }
        pthread_detach(parse_thread);
    }
}

// --------------------------------------------------------------------------

int MsgHandler::parse_msg_client(sockpp::tcp_socket& socket) {
    try {
        // Đọc dữ liệu từ socket
        this->n_read_bytes = socket.read(buf, sizeof(buf));

        // Kiểm tra xem đã đọc đúng số byte hay không
        if (this->n_read_bytes < sizeof(buf)) {
            throw std::runtime_error("Not enough bytes read");
            return -1;
        }

        // Kiểm tra marker không khớp
        if (buf[0] != MARKER_HEAD || buf[4] != MARKER_TAIL) {
            throw std::runtime_error("Invalid markers");
        }
        // Xử lý dữ liệu
        switch (buf[1]) {
            case DET_STATE_SLEEP:
                pthread_mutex_lock(&this->get_connection_mutex());
                this->set_state(DET_STATE_SLEEP);
                pthread_mutex_unlock(&this->get_connection_mutex());
                break;
            case DET_STATE_WORK:
                pthread_mutex_lock(&this->get_connection_mutex());
                this->set_state(DET_STATE_WORK);
                pthread_mutex_unlock(&this->get_connection_mutex());
                break;
            default:
                break;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}

// --------------------------------------------------------------------------

void MsgHandler::transition_to_state(State* new_state) {
    if (this->current_state != nullptr) {
        delete this->current_state;
    }
    this->current_state = new_state;
    this->current_state->handle(*this);
}

// --------------------------------------------------------------------------

void MsgHandler::send_data_to_client(const char* data, size_t data_size) {
    try {
        const size_t chunk_size = CHUNK_SIZE;
        size_t remaining_size = data_size;
        size_t offset = 0;

        while (remaining_size > 0) {
            // Xác định kích thước của chunk cho lần gửi
            size_t current_chunk_size = std::min(chunk_size, remaining_size);

            // Gửi chunk đến client
            int n = this->current_socket.write_n(data + offset, current_chunk_size);

            std::cout << n << " bytes sent in this chunk." << std::endl;

            if (n < 0) {
                std::cerr << "Error sending data to client!" << std::endl;
                break; // Thoát khỏi vòng lặp nếu có lỗi
            }

            // Cập nhật offset và remaining_size cho chunk tiếp theo
            offset += n;
            remaining_size -= n;
        }

        std::cout << "Sent all data to Client" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

// --------------------------------------------------------------------------

sockpp::tcp_socket& MsgHandler::get_current_socket() {
    return this->current_socket;
}

pthread_mutex_t& MsgHandler::get_connection_mutex() {
    return this->connection_mutex;
}

int MsgHandler::get_number_connection() {
    return this->number_connection;
}

void MsgHandler::increment_number_connection() {
    this->number_connection++;
}

void MsgHandler::decrement_number_connection() {
    if (this->number_connection > 0) {
        this->number_connection--;
    }
}

void MsgHandler::set_state(int new_state_) {
    this->state = new_state_;
}

int MsgHandler::get_state() const {
    return this->state;
}
