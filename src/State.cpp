#include "State.h"
#include "MsgHandler.h"
#include <sockpp/tcp_acceptor.h>
#include <iostream>
#include <chrono>
#include <thread> // thư viện sử dụng std::this_thread::sleep_for(std::chrono::seconds(5));

void WorkState::handle(MsgHandler& handler) {
    std::cout << "Handling work state..." << std::endl;

    const size_t buffer_size = 2 * 1024 * 1024; // 2MB
	std::vector<char> buffer(buffer_size, 'A');
	//chuyển sang trạng thái SleepState và chờ 5s
	handler.transitionToState(new SleepState());
	std::this_thread::sleep_for(std::chrono::seconds(5));
	// gửi đến client
	handler.sendDataToClient(buffer);
}

void SleepState::handle(MsgHandler& handler) {
    std::cout << "Handling sleep state..." << std::endl;
    // Additional logic for handling sleep state
}
