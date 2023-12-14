// MsgHandler.cpp
#include "ParseRespMsg.h"

#include <iostream>
#include <iomanip>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <stdlib.h>
#include "checksum.h"

#define Battery_Voltage 0x01
#define Battery_Current 0x02
#define Absolute_State_Of_Charge 0x03
#define Remaining_Capacity 0x04
#define Full_Charge_Capacity 0x05
#define Charging_Current 0x06
#define Battery_Status(Fault) 0x07
#define Cycle_Count 0x08
#define Serial_Number 0x09

ParseRespMsg::ParseRespMsg(sockpp::tcp_acceptor& acceptor) : acceptor_(acceptor){
}

ParseRespMsg::~ParseRespMsg() {}

void* threadHandler(void* arg) {
	ParseRespMsg* parseRespMsg = static_cast<ParseRespMsg*>(arg);
    while (true) {
        // Gọi đối tượng MsgHandler để xử lý gói tin từ client
        int ret = parseRespMsg->recvMsgClient(parseRespMsg->getCurrentSocket());
        if (ret == -1) {
            // chờ 5s và quay lại vòng lặp nhận tiếp
            sleep(3);
        }
    }
//		pthread_exit(nullptr);
}

void ParseRespMsg::handleConnections() {
	pthread_t thread;
    while (true) {
        sockpp::inet_address peer;
        currentSocket = acceptor_.accept(&peer);
        if (!currentSocket) {
            std::cerr << "Error accepting incoming connection: " << acceptor_.last_error_str() << std::endl;
            continue;
        }
        std::cout << "Received a connection request from " << peer << std::endl;

		int result = pthread_create(&thread, nullptr, threadHandler, this);
		if (result != 0) {
			std::cerr << "Error creating thread: " << strerror(result) << std::endl;
		}
		pthread_detach(thread);
    }
}
sockpp::tcp_socket& ParseRespMsg::getCurrentSocket(){
	return currentSocket;
}
int ParseRespMsg::recvMsgClient(sockpp::tcp_socket& socket) {
    try {
        // Đọc dữ liệu từ socket
        n_read_bytes = socket.read(buf, sizeof(buf));

        // Kiểm tra xem đã đọc đúng số byte hay không
        if (n_read_bytes < sizeof(buf)) {
            throw std::runtime_error("Not enough bytes read");
            return -1;
        } else {
        	std::cout << "Recv " << n_read_bytes << " bytes" << std::endl;
        	str = new char[n_read_bytes+1];

			std::memcpy(str, buf, n_read_bytes);
			str[n_read_bytes] = '\0';
			std::cout << "Data received: ";
			for (ssize_t i = 0; i < n_read_bytes; i++) {
				std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)str[i] << " ";
			}
			std::cout << std::endl;
			id = new char[2];
			id[0] = static_cast<char>(buf[1]);
			id[1] = '\0';
        }

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}

void ParseRespMsg::parseMsg(char * str, ssize_t number_of_bytes, unsigned short int value, char * id) {
	switch (id) {
		case Battery_Voltage:
			std::cout << "id: Battery_Voltage";
			break;
		case Battery_Current:
			std::cout << "id: Battery_Current";
			break;
		case Absolute_State_Of_Charge:
			std::cout << "id: Absolute_State_Of_Charge";
			break;
		case Remaining_Capacity:
			std::cout << "id: Remaining_Capacity";
			break;
		case Full_Charge_Capacity:
			std::cout << "id: Full_Charge_Capacity";
			break;
		case Charging_Current:
			std::cout << "id: Charging_Current";
			break;
		case Cycle_Count:
			std::cout << "id: Cycle_Count";
			break;
		case Battery_Status(Fault):
		std::cout << "id: Battery_Status(Fault)";
			break;
		case Serial_Number:
			std::cout << "id: Serial_Number";
			break;
		default:
			break;
	}
}
