#include "State.h"
#include "MsgHandler.h"
#include "Singleton.h"
#include <sockpp/tcp_acceptor.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <openssl/md5.h>
#include <iomanip>
struct ThreadData {
    MsgHandler* handler;
    const char* buffer;
    size_t bufferSize;
};

void* sendDataThread(void* data) {
    ThreadData* threadData = static_cast<ThreadData*>(data);

    try {
        threadData->handler->sendDataToClient(threadData->buffer, threadData->bufferSize);
    } catch (const std::exception& e) {
        std::cerr << "Exception in sendDataThread: " << e.what() << std::endl;
    }

    delete threadData;

    return nullptr;
}
void threadFuncionn() {
	Singleton *singleton = Singleton::getInstance();
	singleton->calledSingleton();
}
void computeMD5File(const char* filename, unsigned char* md5sum) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for MD5 calculation." << std::endl;
        return;
    }

    MD5_CTX md5Context;
    MD5_Init(&md5Context);

    char buffer[2*1024*1024];
    while (file.read(buffer, sizeof(buffer))) {
        MD5_Update(&md5Context, buffer, file.gcount());
    }

    MD5_Final(md5sum, &md5Context);

    file.close();
}
void WorkState::handle(MsgHandler& handler) {
    std::cout << "Handling work state..." << std::endl;
    // chờ 3s
    sleep(3);
    const size_t buffer_size = 2*1024*1024;
	char* buffer = new char[buffer_size];
	std::fill_n(buffer, buffer_size, 'C');
	// thêm dữ liệu cho luồng
	ThreadData* threadData = new ThreadData{&handler, buffer, buffer_size};

	// tạo một luồng mới để gửi data đến client
	pthread_t sendThread;
	int ret = pthread_create(&sendThread, nullptr, sendDataThread, threadData);
	if (ret != 0) {
			std::cerr << "Failed to create thread." << std::endl;
			return;
	}
	// Detach luồng
	if (pthread_detach(sendThread) != 0) {
		std::cerr << "Failed to detach thread." << std::endl;
		return;
	}
	// ghi dữ liệu vào file
	std::ofstream output_file("output.bin", std::ios::binary);
    if (!output_file.is_open()) {
        std::cerr << "Failed to open output file." << std::endl;
    }
    output_file.write(buffer,buffer_size);
    output_file.close();
    std::cout << "Data written to file: output.bin" << std::endl;
    unsigned char md5sum[MD5_DIGEST_LENGTH];
    computeMD5File("output.bin", md5sum);
    std::cout << "MD5 checksum: ";
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(md5sum[i]);
    }
    std::cout << std::endl;
//	// tạo 2 luồng để sử dụng Singleton
//	std::thread t1(threadFuncionn);
//	std::thread t2(threadFuncionn);
//	t1.join();
//	t2.join();
}

void SleepState::handle(MsgHandler& handler) {
    std::cout << "Handling sleep state..." << std::endl;

}
