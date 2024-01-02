#include "DET_State.h"
#include "Mgard300_Handler.h"
#include <sockpp/tcp_acceptor.h>
#include <iostream>
#include <fstream>
#include <openssl/md5.h>
#include <iomanip>
#include "PRB_IMG.h"
#include "Common.h"
#include <pthread.h>
#include <cstdio>
#include "spdlog/sinks/stdout_color_sinks.h"

DET_State::DET_State() {
	this->_logger = spdlog::get("DET_logger");
}
struct ThreadData {
    Mgard300_Handler* handler;
    const char* buffer;
    size_t buffer_size;
};

void* send_data_thread(void* data) {
    ThreadData* thread_data = static_cast<ThreadData*>(data);
    try {
        thread_data->handler->send_data_to_client(thread_data->buffer, thread_data->buffer_size);
    } catch (const std::exception& e) {
#ifdef DEBUG
//        std::cerr << "Exception in send_data_thread: " << e.what() << std::endl;
//        logger1->error("Exception in send_data_thread: {}",e.what());
#endif
    }
    delete[] thread_data->buffer; // Release memory
    delete thread_data;
    return nullptr;
}

#ifdef DEBUG
void compute_md5_file(const char* filename, unsigned char* md5sum) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
//        std::cerr << "Failed to open file for MD5 calculation." << std::endl;
//    	logger1->error("Failed to open file for MD5 calculation.");
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

void WorkState::handle(Mgard300_Handler& handler) {
	handler.getLogger()->info("Handling work state...");
    // Wait for 3 seconds
    sleep(3);

    PRB_IMG* pPRB_IMG = PRB_IMG::getInstance();

    // Generate data
    pPRB_IMG->IMG_acquire();
    char* buffer = new char[BUFFER_SIZE];
    int result = pPRB_IMG->get_IMG(buffer);

    if (result != 1) {
//        std::cerr << "Failed to get data from PRB." << std::endl;
    	handler.getLogger()->warn("Failed to get data from PRB.");
        delete[] buffer; // Release memory
        return;
    }

    // Add data for the thread
    ThreadData* thread_data = new ThreadData{&handler, buffer, BUFFER_SIZE};

    // Create a new thread to send data to the client
    pthread_t send_thread;
    int ret = pthread_create(&send_thread, nullptr, send_data_thread, static_cast<void*>(thread_data));
    if (ret != 0) {
    	handler.getLogger()->warn("Failed to create thread.");
        delete[] thread_data->buffer; // Release memory
        delete thread_data;
        return;
    }

    // Detach the thread
    if (pthread_detach(send_thread) != 0) {
    	handler.getLogger()->warn("Failed to create thread.");
        delete[] thread_data->buffer; // Release memory
        delete thread_data;
        return;
    }

    // Write data to file
    std::ofstream output_file("data.bin", std::ios::binary);
    if (output_file.is_open()) {
        output_file.write(buffer, BUFFER_SIZE);
        output_file.close();
        handler.getLogger()->info("Data written to file: data.bin");

#ifdef DEBUG
        // Compute MD5 checksum
        handler.getLogger()->info("Computing MD5 checksum...");
        unsigned char md5sum[MD5_DIGEST_LENGTH];
        compute_md5_file("data.bin", md5sum);

        handler.getLogger()->info("MD5 checksum: ");
        for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        	handler.getLogger()->info("{:02x}", static_cast<int>(md5sum[i]));
        }
        std::cout << std::endl;
#endif
    } else {
    	handler.getLogger()->warn("Failed to open output file.");
    }
}


void SleepState::handle(Mgard300_Handler &handler) {
	this->_logger->info("Handling sleep state...");

}
void CloseState::handle(Mgard300_Handler &handler) {
	this->_logger->info("Handling close state...");

	handler.close_socket();
}
void TriggerState::handle(Mgard300_Handler &handler) {
	this->_logger->info("Handling Trigger state...");
}
