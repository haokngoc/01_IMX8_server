#include "State.h"
#include "MsgHandler.h"
#include <sockpp/tcp_acceptor.h>
#include <iostream>
#include <chrono>
#include <thread> // thư viện sử dụng std::this_thread::sleep_for(std::chrono::seconds(5));

struct ThreadData {
    MsgHandler* handler;
    std::vector<char> buffer;
};

void* sendDataThread(void* data) {
    ThreadData* threadData = static_cast<ThreadData*>(data);

    try {
        threadData->handler->sendDataToClient(threadData->buffer);
    } catch (const std::exception& e) {
        std::cerr << "Exception in sendDataThread: " << e.what() << std::endl;
    }

    delete threadData;

    return nullptr;
}

void WorkState::handle(MsgHandler& handler) {
    std::cout << "Handling work state..." << std::endl;
    // chờ 3s
    std::this_thread::sleep_for(std::chrono::seconds(3));
    const size_t buffer_size = 2 * 1024 * 1024; // 2MB
	std::vector<char> buffer(buffer_size, 'A');
	// thêm dữ liệu cho luồng
	ThreadData* threadData = new ThreadData{&handler, buffer};

	// tạo một luồng mới để gửi data đến client
	pthread_t sendThread;
	if (pthread_create(&sendThread, nullptr, sendDataThread, threadData) != 0) {
			std::cerr << "Failed to create thread." << std::endl;
			return;
	}
	// Detach luồng
	if (pthread_detach(sendThread) != 0) {
		std::cerr << "Failed to detach thread." << std::endl;
		return;
	}
}

void SleepState::handle(MsgHandler& handler) {
    std::cout << "Handling sleep state..." << std::endl;

}
