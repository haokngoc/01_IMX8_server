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

#define Battery_Voltage 0x01
#define Battery_Current 0x02
#define Absolute_State_Of_Charge 0x03
#define Remaining_Capacity 0x04
#define Full_Charge_Capacity 0x05
#define Charging_Current 0x06
#define Battery_Status(Fault) 0x07
#define Cycle_Count 0x08
#define Serial_Number 0x09

void receiveAndProcessJson(sockpp::tcp_socket& clientSocket) {
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
}


int main(int argc, char* argv[]) {
	auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logfile.txt", 1024*1024 * 100, 3);  // 100 MB size limit, 3 rotated files
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();


	// Tạo logger với cả hai sinks
	auto logger = std::make_shared<spdlog::logger>("DET_logger", spdlog::sinks_init_list{console_sink, file_sink});

	// Đặt mức độ log
	logger->set_level(spdlog::level::debug);
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


