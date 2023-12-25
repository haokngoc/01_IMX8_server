#include "Mgard300_Handler.h"

#include <iostream>
#include <stdexcept>
#include <thread>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iomanip>
#include <queue>
#define MARKER_HEAD 0xAA
#define MARKER_TAIL 0x55
#define DET_STATE_WORK 0x58
#define DET_STATE_SLEEP 0x1b
#define DET_STATE_CLOSE 0x2b
#define CHUNK_SIZE 1024*30
#define BUFFER_SIZE (21 * 1024 * 1024)
#define TIMEOUT_MICRO_SECONDS 100000000000

Mgard300_Handler::Mgard300_Handler(sockpp::tcp_acceptor& acceptor) : acceptor_(acceptor), current_state(nullptr) {
    this->number_connection = 0;
    pthread_mutex_init(&this->connection_mutex, nullptr);
}

Mgard300_Handler::~Mgard300_Handler() {
    delete this->current_state;
    pthread_mutex_destroy(&this->connection_mutex);
}

// --------------------------------------------------------------------------

// tạo hàm để luồng phân tích gói tin thực hiện
void* handler_parse_msg_thread(void* arg) {
	Mgard300_Handler* mgard300_Handler = static_cast<Mgard300_Handler*>(arg);
    pthread_t id = pthread_self();
    while (true) {
        // Gọi đối tượng MsgHandler để phân tích gói tin từ client

        int ret = mgard300_Handler->parse_msg_client(mgard300_Handler->get_current_socket());
        std::cout << "Number of Connections: " << mgard300_Handler->get_number_connection() << " ID thread: " << id << std::endl;
        if(ret == -1) {
        	sleep(3);
        }
        // kiểm tra số lượng kết nối
        if (mgard300_Handler->get_number_connection() == 2) {
            // giảm number_connection
        	mgard300_Handler->decrement_number_connection();
            pthread_exit(nullptr);
        }
    }
}

// tạo hàm để luồng check state thực hiện
void* check_state_thread(void* arg) {
	Mgard300_Handler* mgard300_Handler = static_cast<Mgard300_Handler*>(arg);
	while(true) {
	    int current_state = mgard300_Handler->get_state();
	    switch (current_state) {
	        case DET_STATE_WORK:
	        	mgard300_Handler->transition_to_state(new WorkState());
	            break;
	        case DET_STATE_SLEEP:
	        	mgard300_Handler->transition_to_state(new SleepState());
	            break;
	        case DET_STATE_CLOSE:
	        	mgard300_Handler->transition_to_state(new CloseState());
	        default:
	            break;
	    }
	}
}

// --------------------------------------------------------------------------

void Mgard300_Handler::handle_connections() {
    pthread_t parse_thread;
    pthread_t checkstate_thread;
    std::chrono::microseconds timeoutMicroseconds(TIMEOUT_MICRO_SECONDS); // 5 giây

    while (true) {
        sockpp::inet_address peer;
        this->current_socket = this->acceptor_.accept(&peer);
        //this->current_socket.read_timeout(timeoutMicroseconds);

        this->increment_number_connection();
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

int Mgard300_Handler::parse_msg_client(sockpp::tcp_socket& socket) {
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
                this->set_state(DET_STATE_SLEEP);
                break;
            case DET_STATE_WORK:
                this->set_state(DET_STATE_WORK);
                break;
            case DET_STATE_CLOSE:
            	this->set_state(DET_STATE_CLOSE);
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
        while (remaining_size > 0) {
            // Xác định kích thước của chunk cho lần gửi
            size_t current_chunk_size = std::min(chunk_size, remaining_size);

            // Gửi chunk đến client
            int n = this->current_socket.write_n(data + offset, current_chunk_size);

            if (n < 0) {
                std::cerr << "Error sending data to client!" << std::endl;
                throw std::runtime_error("Error sending data to client");
            }

            // Cập nhật offset và remaining_size cho chunk tiếp theo
            offset += n;
            remaining_size -= n;
            total_sent += n;
            // Kiểm tra kích thước chunk cuối cùng
            if (remaining_size <= 0 && offset != data_size) {
                std::cerr << "Error: Incomplete last chunk sent to client!" << std::endl;
                throw std::runtime_error("Incomplete last chunk sent to client");
            }

            // Hiển thị số byte còn lại
            std::cout << "Remaining bytes: " << remaining_size << " | Total sent: " << total_sent << std::endl;
        }

        std::cout << "Sent all data to Client" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}


// --------------------------------------------------------------------------

sockpp::tcp_socket& Mgard300_Handler::get_current_socket() {
    return this->current_socket;
}

pthread_mutex_t& Mgard300_Handler::get_connection_mutex() {
    return this->connection_mutex;
}

int Mgard300_Handler::get_number_connection() {
	pthread_mutex_lock(&this->get_connection_mutex());
    int result = this->number_connection;
    pthread_mutex_unlock(&this->get_connection_mutex());
    return result;
}

void Mgard300_Handler::increment_number_connection() {
	pthread_mutex_lock(&this->get_connection_mutex());
    this->number_connection++;
    pthread_mutex_unlock(&this->get_connection_mutex());
}

void Mgard300_Handler::decrement_number_connection() {
	pthread_mutex_lock(&this->get_connection_mutex());
    if (this->number_connection > 0) {
        this->number_connection--;
    }
    pthread_mutex_unlock(&this->get_connection_mutex());
}
int Mgard300_Handler::get_state() {
	int state;
	pthread_mutex_lock(&this->get_connection_mutex());
	if (!this->state.empty()) {
		state = this->state.front();
		this->state.pop();
	} else {
		state = -1;
	}
	pthread_mutex_unlock(&this->get_connection_mutex());
	return state;
}
void Mgard300_Handler::set_state(int new_state_) {
	pthread_mutex_lock(&this->get_connection_mutex());
	this->state.push(new_state_);
	pthread_mutex_unlock(&this->get_connection_mutex());
}

