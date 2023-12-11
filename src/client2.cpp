#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <vector>
typedef struct _CMD_PACKET {
    char HeadMarker;
    char Command;
    char Param1;
    char Param2;
    char TailMarker;
} CMD_PACKET, *PCMD_PACKET;

int main() {
    const char* server_ip = "127.0.0.1";  // Địa chỉ IP của server
    const int server_port = 1024;         // Cổng của server

    // Khởi tạo cấu trúc CMD_PACKET với dữ liệu mẫu
    CMD_PACKET packet;
    packet.HeadMarker = 0xAA;
    packet.Command = 0x1b;
    packet.Param1 = 0x2b;
    packet.Param2 = 0x55;
    packet.TailMarker = 0x55;

    // Tạo socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        std::cerr << "Không thể tạo socket" << std::endl;
        return 1;
    }

    // Thiết lập thông tin địa chỉ của server
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0) {
        std::cerr << "Không thể chuyển đổi địa chỉ IP" << std::endl;
        close(client_socket);
        return 1;
    }

    // Kết nối đến server
    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        std::cerr << "Không thể kết nối đến server" << std::endl;
        close(client_socket);
        return 1;
    }

    // Gửi bản tin đến server
    ssize_t bytes_sent = send(client_socket, &packet, sizeof(packet), 0);
    if (bytes_sent == -1) {
        std::cerr << "Gửi bản tin không thành công" << std::endl;
        close(client_socket);
        return 1;
    } else {
        std::cout << "Đã gửi " << bytes_sent << " byte đến server" << std::endl;
    }
    

    // Đóng kết nối và socket
    close(client_socket);
    return 0;
}
