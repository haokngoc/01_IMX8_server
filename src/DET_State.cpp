#include "DET_State.h"
#include "Mgard300_Handler.h"
#include <sockpp/tcp_acceptor.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <openssl/md5.h>
#include <iomanip>
#include "PRB_IMG.h"
#include "Common.h"


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
#ifdef DEBUG
		std::cerr << "Exception in send_data_thread: " << e.what() << std::endl;
#endif
	}

	delete thread_data;

	return nullptr;
}


#ifdef DEBUG
void compute_md5_file(const char *filename, unsigned char *md5sum) {
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "Failed to open file for MD5 calculation." << std::endl;
		return;
	}

	MD5_CTX md5_context;
	MD5_Init(&md5_context);

	char buffer[BUFFER_CAL_MD5];
	while (file.read(buffer, sizeof(buffer))) {
		MD5_Update(&md5_context, buffer, file.gcount());
	}

	MD5_Final(md5sum, &md5_context);

	file.close();
}
#endif
void WorkState::handle(Mgard300_Handler &handler) {
    std::cout << "Handling work state..." << std::endl;
    // chờ 3s
    sleep(3);
    PRB_IMG *pPRB_IMG = PRB_IMG::getInstance();
    // generate data
    pPRB_IMG->IMG_acquire();
    char *buffer = new char[BUFFER_SIZE];
    int result = pPRB_IMG->get_IMG(buffer);

    if (result != 1) {
        std::cerr << "Failed to get data from PRB." << std::endl;
        delete[] buffer; // Giải phóng bộ nhớ
        return;
    }
    // thêm dữ liệu cho luồng
    ThreadData *thread_data = new ThreadData{&handler, buffer, BUFFER_SIZE};

    // tạo một luồng mới để gửi data đến client
    pthread_t send_thread;
    int ret = pthread_create(&send_thread, nullptr, send_data_thread, thread_data);
    if (ret != 0) {
        std::cerr << "Failed to create thread." << std::endl;
        delete[] thread_data->buffer; // Giải phóng bộ nhớ
        delete thread_data;
        return;
    }
    // Detach luồng
    if (pthread_detach(send_thread) != 0) {
        std::cerr << "Failed to detach thread." << std::endl;
        delete[] thread_data->buffer; // Giải phóng bộ nhớ
        delete thread_data;
        return;
    }
    // ghi dữ liệu vào file
    std::ofstream output_file("data.bin", std::ios::binary);
    if (!output_file.is_open()) {
        std::cerr << "Failed to open output file." << std::endl;
    }
    output_file.write(buffer, BUFFER_SIZE);
    output_file.close();
    std::cout << "Data written to file: output.bin" << std::endl;
#ifdef DEBUG

    std::cout << "Computing MD5 checksum..." << std::endl;
    unsigned char md5sum[MD5_DIGEST_LENGTH];
    compute_md5_file("data.bin", md5sum);


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
	handler.close_socket();
}
void TriggerState::handle(Mgard300_Handler &handler) {
	std::cout << "Handling Trigger state..." << std::endl;
}
