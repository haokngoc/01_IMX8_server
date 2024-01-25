#include <iostream>
#include "sockpp/tcp_acceptor.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/async.h"
#include "spdlog/cfg/env.h"
#include "Mgard300_Handler.h"
#include <fstream>
#include <json-c/json.h>
#include <thread>
#include "spdlog/sinks/rotating_file_sink.h"
#include "Settings.h"
#include <string>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <cstdlib>
#include <cstring>
#include <iomanip>

const std::string LOG_FILE_NAME = "logfile.txt";
const std::string JSON_FILE_NAME = "received_data.json";

void setup_logger_fromk_json(const std::string& jsonFilePath, std::shared_ptr<spdlog::logger>& logger) {
    // Read the contents of the JSON file
    std::ifstream file(jsonFilePath);
    if (!file.is_open()) {
        std::cerr << "Error opening JSON file.\n";
        return;
    }
    std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    // close file
    file.close();
    json_object *jsonObject = json_tokener_parse(jsonContent.c_str());
    if (jsonObject != nullptr) {
        // Check and get log level value from JSON
        json_object *settingsObj = nullptr;
        if (json_object_object_get_ex(jsonObject, "settings", &settingsObj)) {
            json_object *logLevelObj = nullptr;
            if (json_object_object_get_ex(settingsObj, "logging-level", &logLevelObj)) {
                const char* logLevelStr = json_object_get_string(logLevelObj);
                spdlog::level::level_enum logLevel = spdlog::level::from_str(logLevelStr);

                // Set the logger's log level
                if (logLevel != spdlog::level::off) {
                    logger->set_level(logLevel);
                    spdlog::info("Set log level to: {}", logLevelStr);
                } else {
                    spdlog::warn("Invalid log level specified in JSON: {}", logLevelStr);
                }
            } else {
                spdlog::warn("Logging level not found in JSON. Defaulting to debug.");
            }
        } else {
            spdlog::warn("Settings not found in JSON.");
        }
        json_object_put(jsonObject);
    } else {
        spdlog::error("Error parsing JSON content.");
    }
}


int main(int argc, char* argv[]) {

	auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(LOG_FILE_NAME, 1024*1024 * 100, 3);  // 100 MB size limit, 3 rotated files
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();


	// Create logger with 2 sinks
	auto logger = std::make_shared<spdlog::logger>("DET_logger", spdlog::sinks_init_list{console_sink, file_sink});
	Settings settings;
	// Set the logger's log level
	logger->set_level(spdlog::level::debug);
//	setup_logger_fromk_json("received_data.json", logger);

	spdlog::register_logger(logger);
//	Read_Json_Configuration();
	settings.Read_Json_Configuration();
    in_port_t port = 1024;
    in_port_t port_php = 12345;
    sockpp::initialize();
    sockpp::tcp_acceptor acc(port);
    sockpp::tcp_acceptor acc_php(port_php);
    if (!acc) {
    	logger->error("Error creating the acceptor: {}",acc.last_error_str());
        return 1;
    }

    logger->info("Awaiting connections on port {}",port);

    // Create and run for port 1024
    std::thread thread_1024([&]() {
        Mgard300_Handler mgard300_Handler(acc);
        mgard300_Handler.handle_connections();
    });
    // Create and run for port 12345
    std::thread thread_12345([&]() {
    	while(true) {
    		sockpp::tcp_socket socket_php = acc_php.accept();
    		logger->info("Awaiting connections on port {}",port_php);
    		if (!socket_php) {
    			logger->error("Error accepting connection from client on port {}",port_php);
				return;
			}
    		const size_t bufferSize = 1024;
    		char buffer[bufferSize];

    		ssize_t bytesRead = socket_php.read(buffer, bufferSize);
    		logger->info("Buffer: {}",buffer);
    		logger->info("bytesRead: {}",bytesRead);
    		if(strcmp(buffer, "cmd_scan_wifi") ==  0) {
    			settings.handle_scan_wf(socket_php);
    			memset(buffer,0,bufferSize);
    		}
    		else {
    			// Receive and process JSON data from the client
				settings.receive_processJson(socket_php);
	//			setup_logger_fromk_json(JSON_FILE_NAME, logger);
    		}
		}
    });
    thread_1024.join();
    thread_12345.join();

    return 0;


}


