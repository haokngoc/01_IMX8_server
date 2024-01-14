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
#include <thread>

DET_State::DET_State() {
	this->_logger = spdlog::get("DET_logger");
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
void perform_work(Mgard300_Handler& handler) {
    handler.getLogger()->info("Handling work state...");

    // Wait for 10 seconds
    for (int i = 1; i <= 50; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        handler.getLogger()->info("Working state {}", i);
    }

    PRB_IMG* pPRB_IMG = PRB_IMG::getInstance();

    // Generate data
    pPRB_IMG->IMG_acquire();
    char* buffer = new char[BUFFER_SIZE];
    int result = pPRB_IMG->get_IMG(buffer);

    if (result != 1) {
        handler.getLogger()->warn("Failed to get data from PRB.");
        delete[] buffer; // Release memory
        return;
    }

    // Send data to client
    handler.send_data_to_client(buffer, BUFFER_SIZE);

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
    // Release memory
    delete[] buffer;
}
void WorkState::close_thread() {
	for (auto& kv : tm_) {
	        if (kv.second.joinable()) {
	            kv.second.std::thread::~thread();
	            std::cout << "Thread " << kv.first << " detached and closed immediately:" << std::endl;
	        }
	    }
	// Xóa tất cả các luồng cũ từ map
	tm_.clear();
}
void WorkState::handle(Mgard300_Handler& handler) {
	this->close_thread();
	std::thread work_thread = std::thread(perform_work, std::ref(handler));
	work_thread.detach();
	this->tm_["work_thread"] = std::move(work_thread);
	std::cout << "Thread " << "work_thread" << " created" << std::endl;
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
