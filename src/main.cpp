#include <iostream>
#include <thread>
#include "sockpp/tcp_acceptor.h"
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

using namespace std;



#define CLEAR(x) memset(&(x), 0, sizeof(x))

struct buffer {
    void   *start;
    size_t length;
};



enum {
	VVCSIOC_RESET = 0x100,
	VVCSIOC_POWERON,
	VVCSIOC_POWEROFF,
	VVCSIOC_STREAMON,
	VVCSIOC_STREAMOFF,
	VVCSIOC_S_FMT,
	VVCSIOC_S_HDR,
};


struct csi_sam_format {
	int64_t format;
	__u32 width;
	__u32 height;
};


// --------------------------------------------------------------------------
// The main thread runs the TCP port acceptor. Each time a connection is
// made a new thread is spawned to handle it leaving this main thread to
// immediately wait for the next incoming connection.

enum {
	DET_STATE_WORK = 11,
	DET_STATE_SLEEP,
	DET_STATE_CLOSE,
	DET_STATE_REVERSED,
};

#define MARKER_HEAD 0xAA
#define MARKER_TAIL 0x55

class MsgHandler {
public:
    MsgHandler(sockpp::tcp_acceptor& acceptor) : acceptor_(acceptor) {}

    void handleConnections() {
        while (true) {
            sockpp::inet_address peer;
            sockpp::tcp_socket sock = acceptor_.accept(&peer);

            if (!sock) {
                cerr << "Error accepting incoming connection: " << acceptor_.last_error_str() << endl;
                continue;
            }

            cout << "Received a connection request from " << peer << std::endl;

            // Gọi đối tượng MsgHandler để xử lý gói tin từ client
            int result = parse_Msg_Client(sock);
            if (result == DET_STATE_SLEEP) {
                cout << "Change working status: DET_STATE_SLEEP" << endl;
            } else if (result == DET_STATE_WORK) {
                cout << "Change working status: DET_STATE_WORK" << endl;
            } else {
                cout << "Error" << endl;
            }

            // Đóng kết nối với client
            sock.close();
            cout << "Connection closed from " << peer << endl;
        }
    }

private:
    sockpp::tcp_acceptor& acceptor_;
    ssize_t n_read_bytes;
    unsigned char buf[5];
//    memset(buf, 0, sizeof(buf));
    int parse_Msg_Client(sockpp::tcp_socket& socket) {


        int ret = -1;

        try {
            // Đọc dữ liệu từ socket
            n_read_bytes = socket.read(buf, sizeof(buf));

            // Kiểm tra xem đã đọc đúng số byte hay không
            if (n_read_bytes < sizeof(buf)) {
                throw std::runtime_error("Not enough bytes read");
            }

            // Kiểm tra marker không khớp
            if (buf[0] != MARKER_HEAD || buf[4] != MARKER_TAIL) {
                throw std::runtime_error("Invalid markers");
            }

            // Xử lý dữ liệu
            switch (buf[1]) {
                case 0x1b:
                    ret = DET_STATE_SLEEP;
                    break;
                case 0x58:
                    ret = DET_STATE_WORK;
                    break;
                default:
                    ret = -1;
                    break;
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
        }

        return ret;
    }
};

int main(int argc, char* argv[]) {
    in_port_t port = 1024;
    sockpp::initialize();
    sockpp::tcp_acceptor acc(port);

    if (!acc) {
        cerr << "Error creating the acceptor: " << acc.last_error_str() << endl;
        return 1;
    }

    cout << "Awaiting connections on port " << port << "..." << endl;

    //đối tượng MsgHandler và xử lý kết nối
    MsgHandler msgHandler(acc);
    msgHandler.handleConnections();

    return 0;
}


