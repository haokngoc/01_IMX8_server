#include "Singleton.h"
#include <sockpp/tcp_acceptor.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <openssl/md5.h>
#include <iomanip>

#include "DET_State.h"
#include "Mgard300_Handler.h"

#define CHUNK_SIZE 1024
#define BUFFER_SIZE (2 * 1024 * 1024)

struct ThreadData {
	Mgard300_Handler *handler;
	const char *buffer;
	size_t buffer_size;
};

void* send_data_thread(void *data) {
	ThreadData *thread_data = static_cast<ThreadData*>(data);

	try {
		thread_data->handler->send_data_to_client(thread_data->buffer,
				thread_data->buffer_size);
	} catch (const std::exception &e) {
		std::cerr << "Exception in send_data_thread: " << e.what() << std::endl;
	}

	delete thread_data;

	return nullptr;
}

void thread_function() {
	Singleton *singleton = Singleton::getInstance();
	singleton->calledSingleton();
}

void compute_md5_file(const char *filename, unsigned char *md5sum) {
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "Failed to open file for MD5 calculation." << std::endl;
		return;
	}

	MD5_CTX md5_context;
	MD5_Init(&md5_context);

	char buffer[BUFFER_SIZE];
	while (file.read(buffer, sizeof(buffer))) {
		MD5_Update(&md5_context, buffer, file.gcount());
	}

	MD5_Final(md5sum, &md5_context);

	file.close();
}

void WorkState::handle(Mgard300_Handler &handler) {
	std::cout << "Handling work state..." << std::endl;
	// chờ 3s
	sleep(3);
	char *buffer = new char[BUFFER_SIZE];
	std::fill_n(buffer, BUFFER_SIZE, 'C');
	// thêm dữ liệu cho luồng
	ThreadData *thread_data = new ThreadData { &handler, buffer, BUFFER_SIZE };

	// tạo một luồng mới để gửi data đến client
	pthread_t send_thread;
	int ret = pthread_create(&send_thread, nullptr, send_data_thread,
			thread_data);
	if (ret != 0) {
		std::cerr << "Failed to create thread." << std::endl;
		return;
	}
	// Detach luồng
	if (pthread_detach(send_thread) != 0) {
		std::cerr << "Failed to detach thread." << std::endl;
		return;
	}
	// ghi dữ liệu vào file
	std::ofstream output_file("output.bin", std::ios::binary);
	if (!output_file.is_open()) {
		std::cerr << "Failed to open output file." << std::endl;
	}
	output_file.write(buffer, BUFFER_SIZE);
	output_file.close();
	std::cout << "Data written to file: output.bin" << std::endl;
#ifdef DEBUG
	// Thêm thông báo debug cho tính toán MD5 checksum
	std::cout << "Computing MD5 checksum..." << std::endl;
#endif
	unsigned char md5sum[MD5_DIGEST_LENGTH];
	compute_md5_file("output.bin", md5sum);

#ifdef DEBUG
	// In ra giá trị MD5 checksum trong chế độ debug
	std::cout << "MD5 checksum: ";
	for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
		std::cout << std::hex << std::setw(2) << std::setfill('0')
				<< static_cast<int>(md5sum[i]);
	}
	std::cout << std::endl;
#endif
}

void SleepState::handle(Mgard300_Handler &handler) {
	std::cout << "Handling sleep state..." << std::endl;
}
void CloseState::handle(Mgard300_Handler &handler) {
	std::cout << "Handling close state..." << std::endl;
	sockpp::tcp_socket &current_socket = handler.get_current_socket();

	// đóng kết nối socket
	current_socket.close();
	std::cout << "socket closed connection" << std::endl;
}
