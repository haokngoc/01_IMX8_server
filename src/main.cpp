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


// Định nghĩa các tên file
const std::string LOG_FILE_NAME = "logfile.txt";
const std::string JSON_FILE_NAME = "received_data.json";

void setup_logger_fromk_json(const std::string& jsonFilePath, std::shared_ptr<spdlog::logger>& logger) {
    // Đọc nội dung của tệp JSON
    std::ifstream file(jsonFilePath);
    if (!file.is_open()) {
        std::cerr << "Error opening JSON file.\n";
        return;
    }
    std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    // Đóng tệp
    file.close();
    json_object *jsonObject = json_tokener_parse(jsonContent.c_str());
    if (jsonObject != nullptr) {
        // Kiểm tra và lấy giá trị mức độ log từ JSON
        json_object *settingsObj = nullptr;
        if (json_object_object_get_ex(jsonObject, "settings", &settingsObj)) {
            json_object *logLevelObj = nullptr;
            if (json_object_object_get_ex(settingsObj, "logging-level", &logLevelObj)) {
                const char* logLevelStr = json_object_get_string(logLevelObj);
                spdlog::level::level_enum logLevel = spdlog::level::from_str(logLevelStr);

                // Đặt mức độ log của logger
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

void receiveAndProcessJson(sockpp::tcp_socket& clientSocket,std::shared_ptr<spdlog::logger>& logger) {
    std::string cmdID;
    const int bufferSize = 1024;
    char buffer[bufferSize];
    // Nhận dữ liệu JSON từ client
    ssize_t bytesRead = clientSocket.read(buffer, bufferSize);
    if (bytesRead < 0) {
        spdlog::error("Error receiving data from client.");
        return;
    }
    Settings config;
    // Phân tích dữ liệu JSON
    struct json_object* root = json_tokener_parse(buffer);
    if (root == nullptr) {
    	 spdlog::error("Error parsing JSON data received from client.");
        return;
    }
    // Hiển thị giá trị JSON nhận được
    spdlog::info("Received JSON data:");
    spdlog::info("{}",json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY));
    config.fromJson(root);
    config.printSetting();
    // Lưu dữ liệu JSON vào tệp tin
    std::ofstream outputFile(JSON_FILE_NAME);
    if (outputFile.is_open()) {
        outputFile << json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY) << std::endl;
        outputFile.close();
        spdlog::info( "Saved JSON data to 'received_data.json");
    } else {
        spdlog::error("Error saving JSON data to file.");
    }
    // Gửi thông báo xác nhận về cho client
	const char* confirmationMsg = "Server recevied data sucessfuly";
	clientSocket.write(confirmationMsg, strlen(confirmationMsg));
    // Giải phóng bộ nhớ
    json_object_put(root);
    // add wifi
//	NMClient *client;
//	GMainLoop *loop;
//	GError *error = NULL;
//	loop = g_main_loop_new(NULL, FALSE);
	// Connect to NetworkManager
//	client = nm_client_new(NULL, &error);
//	if (client == NULL) {
//		g_print("Error creating NMClient: %s", error->message);
//		g_error_free(error);
//	}
	config.add_wifi(config.getWirelessSsid(), config.getWirelessPassPhrase());

	// Bắt đầu vòng lặp chính
//	g_main_loop_run(loop);

	// Giải phóng tài nguyên trước khi thoát chương trình
//	g_main_loop_unref(loop);
//	g_object_unref(client);

    // get ip address
    struct in_addr currentIP = config.getCurrentIP();
    char ipString[INET_ADDRSTRLEN];
	if (inet_ntop(AF_INET, &currentIP, ipString, INET_ADDRSTRLEN) == NULL) {
	   perror("inet_ntop");
	}
    spdlog::info("Current IP address: {}",ipString);
	// Gửi địa chỉ IP về cho client
	clientSocket.write(ipString, strlen(ipString));

    // set ip
	config.setIP(config.getIpAddress());
    // gửi thông báo thay đổi ip thành công
    const char *ipAddress = config.getIpAddress().c_str();
    std::string confirmMessage = "IP address changed successfully to " + std::string(ipAddress);
    const char *confirm_ip = confirmMessage.c_str();
	clientSocket.write(confirm_ip, strlen(confirm_ip));
}
void Read_Json_Configuration(std::shared_ptr<spdlog::logger>& logger) {
    Settings setting;
    std::ifstream file(JSON_FILE_NAME);
    if (!file.is_open()) {
        logger->error("Error opening file.");
        return;
    }

    // Đọc nội dung của tệp JSON vào một chuỗi
    std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    // Đóng tệp
    file.close();
    // Phân tích chuỗi JSON để tạo đối tượng json_object
    json_object *j = json_tokener_parse(jsonContent.c_str());

    // Kiểm tra nếu phân tích thành công
    if (j != nullptr) {
        // In ra nội dung của đối tượng JSON
        spdlog::info("JSON Content: {}", json_object_to_json_string_ext(j, JSON_C_TO_STRING_PRETTY));
        // Giải phóng bộ nhớ của đối tượng JSON khi không cần thiết nữa

    } else {
    	spdlog::error("Error parsing JSON content.");
    }

    setting.fromJson(j);
    setting.printSetting();
    json_object_put(j);
    // add wifi
 //   NMClient *client;
 //   GMainLoop *loop;
//    GError *error = NULL;

//    loop = g_main_loop_new(NULL, FALSE);

    // Connect to NetworkManager
  //  client = nm_client_new(NULL, &error);
//
//    if (client == NULL) {
//    	spdlog::error("Error creating NMClient: {}", error->message);
//        g_error_free(error);
//    }

    setting.add_wifi(setting.getWirelessSsid(), setting.getWirelessPassPhrase());

    // Bắt đầu vòng lặp chính
//    g_main_loop_run(loop);
//    g_main_loop_unref(loop);
//    g_object_unref(client);

    // get ip address
    struct in_addr currentIP = setting.getCurrentIP();
    char ipString[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &currentIP, ipString, INET_ADDRSTRLEN) == NULL) {
        perror("inet_ntop");
    }
    spdlog::info("Current IP address: {}",ipString);
    // set ip
    setting.setIP(setting.getIpAddress());
}


int main(int argc, char* argv[]) {

	auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(LOG_FILE_NAME, 1024*1024 * 100, 3);  // 100 MB size limit, 3 rotated files
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();


	// Tạo logger với cả hai sinks
	auto logger = std::make_shared<spdlog::logger>("DET_logger", spdlog::sinks_init_list{console_sink, file_sink});

	// Đặt mức độ log

	logger->set_level(spdlog::level::debug);
//	setup_logger_fromk_json("received_data.json", logger);

	spdlog::register_logger(logger);
	Read_Json_Configuration(logger);
    in_port_t port = 1024;
    in_port_t port_php = 12345;
    sockpp::initialize();
    sockpp::tcp_acceptor acc(port);
    sockpp::tcp_acceptor acc_php(port_php);
    if (!acc) {
        std::cerr << "Error creating the acceptor: " << acc.last_error_str() << std::endl;
        return 1;
    }

    spdlog::info("Awaiting connections on port {}",port);

    // Tạo và chạy luồng cho cổng 1024
    std::thread thread_1024([&]() {
        Mgard300_Handler mgard300_Handler(acc);
        mgard300_Handler.handle_connections();
    });
    // Tạo và chạy luồng cho cổng 12345
    std::thread thread_12345([&]() {
    	while(true) {
    		sockpp::tcp_socket socket_php = acc_php.accept();
    		spdlog::info("Awaiting connections on port {}",port_php);
    		if (!socket_php) {
				std::cerr << "Error accepting connection from client on port " << port_php << "." << std::endl;
				return;
			}
			// Nhận và xử lý dữ liệu JSON từ client
			receiveAndProcessJson(socket_php,logger);
			setup_logger_fromk_json(JSON_FILE_NAME, logger);

		}
    });
    thread_1024.join();
    thread_12345.join();

    return 0;


}


