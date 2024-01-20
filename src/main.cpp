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
#include "get_connection.hpp"
#include <string>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <cstdlib>
#include "ip_handler.h"

#define Battery_Voltage 0x01
#define Battery_Current 0x02
#define Absolute_State_Of_Charge 0x03
#define Remaining_Capacity 0x04
#define Full_Charge_Capacity 0x05
#define Charging_Current 0x06
#define Battery_Status(Fault) 0x07
#define Cycle_Count 0x08
#define Serial_Number 0x09

void setup_logger_fromk_json(const std::string& jsonFilePath, std::shared_ptr<spdlog::logger>& logger) {
    // Đọc nội dung của tệp JSON
    std::ifstream file(jsonFilePath);
    if (!file.is_open()) {
        std::cerr << "Error opening JSON file.\n";
        return;
    }

    // Đọc nội dung của tệp JSON vào một chuỗi
    std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    // Đóng tệp
    file.close();

    // Phân tích chuỗi JSON để tạo đối tượng json_object
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

        // Giải phóng bộ nhớ của đối tượng JSON khi không cần thiết nữa
        json_object_put(jsonObject);
    } else {
        spdlog::error("Error parsing JSON content.");
    }
}
void added_cb(GObject *client, GAsyncResult *result, gpointer user_data)
{
    NMRemoteConnection *remote;
    GError *error = NULL;

    /* NM responded to our request; either handle the resulting error or
     * print out the object path of the connection we just added.
     */
    remote = nm_client_add_connection_finish(NM_CLIENT(client), result, &error);

    if (error) {
        g_print("Error adding connection: %s", error->message);
        g_error_free(error);
    } else {
//        g_print("Added: %s\n", nm_connection_get_path(NM_CONNECTION(remote)));
        g_object_unref(remote);
    }

    /* Tell the mainloop we're done and we can quit now */
    g_main_loop_quit((GMainLoop *)user_data);
}
void add_wifi(NMClient *client, GMainLoop *loop, std::string& id, std::string& pass)
{
    NMConnection *connection;

    const char *uuid;
    const char *password;
    const char* cString = id.c_str();
    std::cout << "ssid: " << id << std::endl;
    std::cout << "pass: " << pass << std::endl;
    /* Create a new connection object */
    uuid = nm_utils_uuid_generate();
    GString *ssid = g_string_new(cString);
    password = pass.c_str();
    connection = get_client_nmconnection(cString, uuid, ssid, password);

    /* Ask the settings service to add the new connection; we'll quit the
     * mainloop and exit when the callback is called.
     */
    nm_client_add_connection_async(client, connection, TRUE, NULL, added_cb, loop);
    g_object_unref(connection);
}


void receiveAndProcessJson(sockpp::tcp_socket& clientSocket) {
	NMClient *client;
	GMainLoop *loop;
	GError *error = NULL;
	loop = g_main_loop_new(NULL, FALSE);
	// Connect to NetworkManager
	client = nm_client_new(NULL, &error);

    std::string cmdID;
    const int bufferSize = 1024;
    char buffer[bufferSize];

    // Nhận dữ liệu JSON từ client
    ssize_t bytesRead = clientSocket.read(buffer, bufferSize);
    if (bytesRead < 0) {
        std::cerr << "Error receiving data from client." << std::endl;
        return;
    }

    Settings config;

    // Phân tích dữ liệu JSON
    struct json_object* root = json_tokener_parse(buffer);
    if (root == nullptr) {
        std::cerr << "Error parsing JSON data received from client." << std::endl;
        return;
    }

    // Hiển thị giá trị JSON nhận được
    std::cout << "Received JSON data:" << std::endl;
    std::cout << json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY) << std::endl;
    config.fromJson(root);
    config.printSetting();
    // Lưu dữ liệu JSON vào tệp tin
    std::ofstream outputFile("received_data.json");
    if (outputFile.is_open()) {
        outputFile << json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY) << std::endl;
        outputFile.close();
        std::cout << "Saved JSON data to 'received_data.json'" << std::endl;
    } else {
        std::cerr << "Error saving JSON data to file." << std::endl;
    }
    // Gửi thông báo xác nhận về cho client
	const char* confirmationMsg = "Server recevied data sucessfuly";
	clientSocket.write(confirmationMsg, strlen(confirmationMsg));
    // Giải phóng bộ nhớ
    json_object_put(root);
    add_wifi(client, loop, config.getWirelessSsid(),config.getWirelessPassPhrase());

    // Wait for the connection to be added
    g_main_loop_run(loop);
    // Clean up
    g_object_unref(client);

    // get ip address
    IPHandler ip_handeler;
    struct in_addr currentIP = ip_handeler.getCurrentIP();
    char ipString[INET_ADDRSTRLEN];
	if (inet_ntop(AF_INET, &currentIP, ipString, INET_ADDRSTRLEN) == NULL) {
	   perror("inet_ntop");
	}

	printf("Current IP address: %s\n", ipString);
	// Gửi địa chỉ IP về cho client
	clientSocket.write(ipString, strlen(ipString));

    // set ip
    ip_handeler.setIP(config.getIpAddress());
    // gửi thông báo thay đổi ip thành công
    const char *ipAddress = config.getIpAddress().c_str();
    std::string confirmMessage = "IP address changed successfully to " + std::string(ipAddress);
    const char *confirm_ip = confirmMessage.c_str();
	clientSocket.write(confirm_ip, strlen(confirm_ip));
}
void Read_Json_Configuration() {
	Settings setting;
	std::ifstream file("received_data.json");
	if (!file.is_open()) {
		std::cerr << "Error opening file.\n";
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
		std::cout << json_object_to_json_string_ext(j, JSON_C_TO_STRING_PRETTY) << std::endl;
		// Giải phóng bộ nhớ của đối tượng JSON khi không cần thiết nữa

	} else {
		std::cerr << "Error parsing JSON content.\n";
	}
	setting.fromJson(j);
	setting.printSetting();
	json_object_put(j);
	// add wifi
	NMClient *client;
	GMainLoop *loop;
	GError *error = NULL;
	loop = g_main_loop_new(NULL, FALSE);
	// Connect to NetworkManager
	client = nm_client_new(NULL, &error);
	add_wifi(client, loop, setting.getWirelessSsid(),setting.getWirelessPassPhrase());
	// Wait for the connection to be added
	g_main_loop_run(loop);
	// Clean up
	g_object_unref(client);

	// get ip address
	IPHandler ip_handler;
	ip_handler.getCurrentIP();

	struct in_addr currentIP = ip_handler.getCurrentIP();

	// Convert the IP address to a human-readable string
	char ipString[INET_ADDRSTRLEN];
	if (inet_ntop(AF_INET, &currentIP, ipString, INET_ADDRSTRLEN) == NULL) {
		perror("inet_ntop");
	}
	printf("Current IP address: %s\n", ipString);
	// set ip
	ip_handler.setIP(setting.getIpAddress());
}

int main(int argc, char* argv[]) {
	Read_Json_Configuration();
	auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logfile.txt", 1024*1024 * 100, 3);  // 100 MB size limit, 3 rotated files
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();


	// Tạo logger với cả hai sinks
	auto logger = std::make_shared<spdlog::logger>("DET_logger", spdlog::sinks_init_list{console_sink, file_sink});

	// Đặt mức độ log
	setup_logger_fromk_json("received_data.json", logger);

	spdlog::register_logger(logger);

    in_port_t port = 1024;
    in_port_t port_php = 12345;
    sockpp::initialize();
    sockpp::tcp_acceptor acc(port);
    sockpp::tcp_acceptor acc_php(port_php);
    if (!acc) {
        std::cerr << "Error creating the acceptor: " << acc.last_error_str() << std::endl;
        return 1;
    }

    std::cout << "Awaiting connections on port " << port << "..." << std::endl;

    // Tạo và chạy luồng cho cổng 1024
    std::thread thread_1024([&]() {
        Mgard300_Handler mgard300_Handler(acc);
        mgard300_Handler.handle_connections();
    });
    // Tạo và chạy luồng cho cổng 12345
    std::thread thread_12345([&]() {
    	while(true) {
    		sockpp::tcp_socket socket_php = acc_php.accept();
    		std::cout << "Awaiting connections on port " << port_php << "..." << std::endl;
    		if (!socket_php) {
				std::cerr << "Error accepting connection from client on port " << port_php << "." << std::endl;
				return;
			}
			// Nhận và xử lý dữ liệu JSON từ client
			receiveAndProcessJson(socket_php);
			setup_logger_fromk_json("received_data.json", logger);

		}
    });
    thread_1024.join();
    thread_12345.join();

    return 0;


}


//int main() {
//    sockpp::tcp_acceptor acceptor(12345);  // Cổng của server
//    if (!acceptor) {
//        std::cerr << "Error creating serve pr acceptor." << std::endl;
//        return 1;
//    }
//
//    std::cout << "Server is listening on port 12345" << std::endl;
//
//    // Chấp nhận kết nối từ client
//    sockpp::tcp_socket clientSocket = acceptor.accept();
//    if (!clientSocket) {
//        std::cerr << "Error accepting connection from client." << std::endl;
//        return 1;
//    }
//
//    // Nhận và xử lý dữ liệu JSON từ client
//    receiveAndProcessJson(clientSocket);

//    return 0;
//}


//
//#include <iostream>
//#include <stdio.h>
//#include <string.h>
//#include "checksum.h"
//
//#define Battery_Voltage 0x01
//#define Battery_Current 0x02
//#define Absolute_State_Of_Charge 0x03
//#define Remaining_Capacity 0x04
//#define Full_Charge_Capacity 0x05
//#define Charging_Current 0x06
//#define Battery_Status_Fault 0x07
//#define Cycle_Count 0x08
//#define Serial_Number 0x09
//
//static uint8_t sht75_crc_table[] = {
//
//	0,   49,  98,  83,  196, 245, 166, 151, 185, 136, 219, 234, 125, 76,  31,  46,
//	67,  114, 33,  16,  135, 182, 229, 212, 250, 203, 152, 169, 62,  15,  92,  109,
//	134, 183, 228, 213, 66,  115, 32,  17,  63,  14,  93,  108, 251, 202, 153, 168,
//	197, 244, 167, 150, 1,   48,  99,  82,  124, 77,  30,  47,  184, 137, 218, 235,
//	61,  12,  95,  110, 249, 200, 155, 170, 132, 181, 230, 215, 64,  113, 34,  19,
//	126, 79,  28,  45,  186, 139, 216, 233, 199, 246, 165, 148, 3,   50,  97,  80,
//	187, 138, 217, 232, 127, 78,  29,  44,  2,   51,  96,  81,  198, 247, 164, 149,
//	248, 201, 154, 171, 60,  13,  94,  111, 65,  112, 35,  18,  133, 180, 231, 214,
//	122, 75,  24,  41,  190, 143, 220, 237, 195, 242, 161, 144, 7,   54,  101, 84,
//	57,  8,   91,  106, 253, 204, 159, 174, 128, 177, 226, 211, 68,  117, 38,  23,
//	252, 205, 158, 175, 56,  9,   90,  107, 69,  116, 39,  22,  129, 176, 227, 210,
//	191, 142, 221, 236, 123, 74,  25,  40,  6,   55,  100, 85,  194, 243, 160, 145,
//	71,  118, 37,  20,  131, 178, 225, 208, 254, 207, 156, 173, 58,  11,  88,  105,
//	4,   53,  102, 87,  192, 241, 162, 147, 189, 140, 223, 238, 121, 72,  27,  42,
//	193, 240, 163, 146, 5,   52,  103, 86,  120, 73,  26,  43,  188, 141, 222, 239,
//	130, 179, 224, 209, 70,  119, 36,  21,  59,  10,  89,  104, 255, 206, 157, 172
//};
//
///*
// * uint8_t crc_8( const unsigned char *input_str, size_t num_bytes );
// *
// * The function crc_8() calculates the 8 bit wide CRC of an input string of a
// * given length.
// */
//
//uint8_t crc_8( const unsigned char *input_str, size_t num_bytes ) {
//
//	size_t a;
//	uint8_t crc;
//	const unsigned char *ptr;
//
//	crc = CRC_START_8;
//	ptr = input_str;
//
//	if ( ptr != NULL ) for (a=0; a<num_bytes; a++) {
//
//		crc = sht75_crc_table[(*ptr++) ^ crc];
//	}
//
//	return crc;
//
//}  /* crc_8 */
//
///*
// * uint8_t update_crc_8( unsigned char crc, unsigned char val );
// *
// * Given a databyte and the previous value of the CRC value, the function
// * update_crc_8() calculates and returns the new actual CRC value of the data
// * comming in.
// */
//
//uint8_t update_crc_8( unsigned char crc, unsigned char val ) {
//
//	return sht75_crc_table[val ^ crc];
//
//}
//
//void parseMsg(const unsigned char *data, size_t *number_of_bytes, unsigned short *value, char *id) {
//    // Assuming data contains a format like "%d,%hu,%d,%s"
//    sscanf((const char *)data, "%zd,%hu,%s", number_of_bytes, value, id);
//
//    printf("data: ");
//    for (int i = 0; i < sizeof(data) / sizeof(data[0]); i++) {
//        printf("%02X ", data[i]);
//    }
//    printf("\n");
//
//    printf("number_of_bytes: %zd\n", *number_of_bytes);
//    printf("value: %hu\n", *value);
//    // printf("id: %s\n", id);
//    unsigned char byte1 = (*value) & 0xFF;         // Lower byte
//    unsigned char byte2 = ((*value) >> 8) & 0xFF;  // Upper byte
//
//    printf("Byte 1: %02X\n", byte1);
//    printf("Byte 2: %02X\n", byte2);
//
//    // chuyen ky dau tin cua id thanh so nguyen de kiem tra
//    switch (id[0])
//    {
//    case Battery_Voltage:
//        std::cout << "id=" << "Battery_Voltage" << std::endl;
//        break;
//    case Battery_Current:
//        std::cout << "id=" << "Battery_Current" << std::endl;
//        break;
//    case Absolute_State_Of_Charge:
//        std::cout << "id=" << "Absolute_State_Of_Charge" << std::endl;
//        break;
//    case Remaining_Capacity:
//        std::cout << "id=" << "Remaining_Capacity" << std::endl;
//        break;
//    case Full_Charge_Capacity:
//        std::cout << "id=" << "Full_Charge_Capacity" << std::endl;
//        break;
//    case Charging_Current:
//        std::cout << "id=" << "Charging_Current" << std::endl;
//        break;
//    case Battery_Status_Fault:
//        std::cout << "id=" << "Battery_Status_Fault" << std::endl;
//        break;
//    case Cycle_Count:
//        std::cout << "id=" << "Cycle_Count" << std::endl;
//        break;
//    case Serial_Number:
//        std::cout << "id=" << "Serial_Number" << std::endl;
//        break;
//    default:
//        break;
//    }
//}
//
//int main() {
//    unsigned char data[] = {0x23, 0x02, 0x02, 0x02, 0x02, 0x09, 0xAA};
//    size_t numBytes = sizeof(data) / sizeof(data[0]) - 2;
//
//    unsigned short val;
//    char identifier[] = {0x02};
//    unsigned char crcData[numBytes];
//    std::copy(data, data + numBytes, crcData);
//    parseMsg(data, &numBytes, &val, identifier);
//    uint8_t crc;
//    crc = crc_8(crcData,numBytes);
//    if(crc == data[5]) {
//    	std::cout << "CRC Match!" << std::endl;
//	} else {
//		std::cout << "CRC Mismatch!" << std::endl;
//	}
//    return 0;
//}

