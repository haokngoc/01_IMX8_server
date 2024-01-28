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

void WorkState::handle(Mgard300_Handler& handler) {
	this->work_thread = std::thread([this, &handler]() {
		handler.getLogger()->info("Handling work state...");
		std::this_thread::sleep_for(std::chrono::seconds(2));
		// Wait for 10 seconds
		for (int i = 1; i <= 5; ++i) {
			if(handler.get_check_exist_connection()) {
				handler.getLogger()->info("Exiting thread due to check_close_threads");

				handler.set_check_exist_connection(false);
				return;
			}
			std::this_thread::sleep_for(std::chrono::seconds(1));
			handler.getLogger()->info("Working state {}", i);
		}

		PRB_IMG* pPRB_IMG = PRB_IMG::getInstance();
		int ret = 0;
		// Generate data
#ifdef IMX8_SOM
	    ret = pPRB_IMG->intialize_stream();
	    ret = pPRB_IMG->open_stream();
	  //  pPRB_IMG->trigger_event();
	    ret = pPRB_IMG->close_stream();
#else if
	    ret = pPRB_IMG->IMG_acquire();
#endif
	    ret = pPRB_IMG->IMG_acquire();
		char* buffer = new char[BUFFER_SIZE];
		if (ret==0)
			pPRB_IMG->get_IMG(buffer);

		if (ret != 0) {
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
		delete[] buffer;
    });

	work_thread.detach();
}



void SleepState::handle(Mgard300_Handler &handler) {
	handler.getLogger()->info("Handling sleep state...");

}
void CloseState::handle(Mgard300_Handler &handler) {
	handler.getLogger()->info("Handling close state...");

	handler.close_socket();
}
void TriggerState::handle(Mgard300_Handler &handler) {
	handler.getLogger()->info("Handling Trigger state...");
}
