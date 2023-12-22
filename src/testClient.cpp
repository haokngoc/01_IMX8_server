#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <vector>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iomanip>
using namespace std;
typedef struct _CMD_PACKET {
   char HeadMarker;
   char Command;
   char Param1;
   char Param2;
   char TailMarker;
} CMD_PACKET, *PCMD_PACKET;
void computeMD5File(const char* filename, unsigned char* md5sum) {
   std::ifstream file(filename, std::ios::binary);
   if (!file.is_open()) {
       std::cerr << "Failed to open file for MD5 calculation." << std::endl;
       return;
   }


   MD5_CTX md5Context;
   MD5_Init(&md5Context);


   char buffer[2 * 1024 *1024];
   while (file.read(buffer, sizeof(buffer))) {
       MD5_Update(&md5Context, buffer, file.gcount());
   }


   MD5_Final(md5sum, &md5Context);


   file.close();
}
int main() {
   const char* server_ip = "127.0.0.1";  // Địa chỉ IP của server
   const int server_port = 1024;         // Cổng của server
   const char* output_file = "output.bin";


   // Khởi tạo cấu trúc CMD_PACKET với dữ liệu mẫu
   CMD_PACKET packet;
   packet.HeadMarker = 0xAA;
   packet.Command = 0x58;
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


   // Nhận buffer 2MB từ server
   const size_t buffer_size = 2 * 1024 *1024 ;  // 2MB
   //const size_t buffer_size = 5;
   char receive_buffer[1024];
   ssize_t totalReceived = 0;


   std::ofstream output_stream(output_file, std::ios::binary);
   if (!output_stream.is_open()) {
       std::cerr << "Không thể mở file đầu ra" << std::endl;
       close(client_socket);
       return 1;
   }


   while (true)
   {
       ssize_t bytes_received = recv(client_socket, receive_buffer, 1024, 0);
       if(bytes_received <= 0) {
           break;
       }
       output_stream.write(receive_buffer, bytes_received);
       totalReceived += bytes_received;
       std::cout << "Received " << bytes_received << " bytes. Total received: " << totalReceived << std::endl;
       if (totalReceived == 2097152)
       {
           break;
       }
      
   }
  


   unsigned char md5sum[MD5_DIGEST_LENGTH];
   computeMD5File("output.bin", md5sum);
   std::cout << "MD5 checksum: ";
   for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
       std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(md5sum[i]);
   }
   std::cout << std::endl;


   // Đóng kết nối và socket
   close(client_socket);
   return 0;
}
