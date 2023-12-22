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

MsgHandler::MsgHandler(sockpp::tcp_acceptor& acceptor) : acceptor_(acceptor), currentState(nullptr) {
	this->numberConnection = 0;
	pthread_mutex_init(&connectionMutex, nullptr);
}

MsgHandler::~MsgHandler() {
    delete currentState;
    pthread_mutex_destroy(&connectionMutex);
}

void* threadHandler(void* arg) {
    MsgHandler* msgHandler = static_cast<MsgHandler*>(arg);
    pthread_t id = pthread_self();
    while (true) {
        // Gọi đối tượng MsgHandler để xử lý gói tin từ client
        int ret = msgHandler->parseMsgClient(msgHandler->getCurrentSocket());
        if (ret == -1) {
            // chờ 5s và quay lại vòng lặp nhận tiếp
            sleep(3);
        }
        pthread_mutex_lock(&msgHandler->getConnectionMutex());
		std::cout << "Number of Connections: " << msgHandler->getNumberConnection() << " ID thread: " << id << std::endl;

		// kiểm tra số lượng kết nối
		if(msgHandler->getNumberConnection() == 2) {
			//giảm numberConnection
			msgHandler->decrementNumberConnection();
			pthread_mutex_unlock(&msgHandler->getConnectionMutex());
			pthread_exit(nullptr);
		}
		pthread_mutex_unlock(&msgHandler->getConnectionMutex());

    }
//		pthread_exit(nullptr);
}
void* threadCheckState(void* arg) {
	MsgHandler* msgHandler = static_cast<MsgHandler*>(arg);
	int currentState = msgHandler->getSate();
	switch (currentState) {
		case DET_STATE_WORK:
			msgHandler->transitionToState(new WorkState());
			break;
		case DET_STATE_SLEEP:
			msgHandler->transitionToState(new SleepState());
			break;
		default:
			break;
	}
}
void MsgHandler::handleConnections() {
	pthread_t thread;
	pthread_t stateThread;
//	int res = pthread_create(&stateThread, nullptr, threadCheckState, this);
    while (true) {
        sockpp::inet_address peer;
        currentSocket = acceptor_.accept(&peer);

        pthread_mutex_lock(&this->getConnectionMutex());
        this->incrementNumberConnection();
        pthread_mutex_unlock(&this->getConnectionMutex());

        if (!currentSocket) {
            std::cerr << "Error accepting incoming connection: " << acceptor_.last_error_str() << std::endl;
            continue;
        }
        std::cout << "Received a connection request from " << peer << std::endl;

		int result = pthread_create(&thread, nullptr, threadHandler, this);
		pthread_create(&stateThread, nullptr, threadCheckState, this);
		if (result != 0) {
			std::cerr << "Error creating thread: " << strerror(result) << std::endl;
		}
		pthread_detach(thread);
    }
}

int MsgHandler::parseMsgClient(sockpp::tcp_socket& socket) {
    try {
        // Đọc dữ liệu từ socket
        n_read_bytes = socket.read(buf, sizeof(buf));

        // Kiểm tra xem đã đọc đúng số byte hay không
        if (n_read_bytes < sizeof(buf)) {
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
            	pthread_mutex_lock(&this->getConnectionMutex());
            	this->setState(DET_STATE_SLEEP);
            	pthread_mutex_unlock(&this->getConnectionMutex());
                break;
            case DET_STATE_WORK:
            	pthread_mutex_lock(&this->getConnectionMutex());
            	this->setState(DET_STATE_WORK);
            	pthread_mutex_unlock(&this->getConnectionMutex());
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
void MsgHandler::transitionToState(State* newState) {
    if (currentState != nullptr) {
        delete currentState;
    }
    currentState = newState;
    currentState->handle(*this);
}

void MsgHandler::sendDataToClient(const char* data, size_t dataSize) {
    try {
        const size_t chunkSize = 1024;
        size_t remainingSize = dataSize;
        size_t offset = 0;

        while (remainingSize > 0) {
            // Xác định kích thước của chunk cho lần gửi
            size_t currentChunkSize = std::min(chunkSize, remainingSize);

            // Gửi chunk đến client
            int n = currentSocket.write_n(data + offset, currentChunkSize);

            std::cout << n << " bytes sent in this chunk." << std::endl;

            if (n < 0) {
                std::cerr << "Error sending data to client!" << std::endl;
                break; // Thoát khỏi vòng lặp nếu có lỗi
            }

            // Cập nhật offset và remainingSize cho chunk tiếp theo
            offset += n;
            remainingSize -= n;
        }

        std::cout << "Sent all data to Client" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

sockpp::tcp_socket& MsgHandler::getCurrentSocket(){
	return currentSocket;
}
pthread_mutex_t& MsgHandler::getConnectionMutex() {
    return connectionMutex;
}
int MsgHandler::getNumberConnection() {
    return numberConnection;
}
void MsgHandler::incrementNumberConnection() {
    numberConnection++;
}
void MsgHandler::decrementNumberConnection() {
    if (numberConnection > 0) {
        numberConnection--;
    }
}
void MsgHandler::setState(int newSate_) {
	this->sate = newSate_;
}
int MsgHandler::getSate() const {
	return sate;
}
