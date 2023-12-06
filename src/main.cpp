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
    MsgHandler(sockpp::tcp_socket& socket) : socket_(socket) {}

    int parse_Msg_Client() {
        ssize_t n_read_bytes;
        unsigned char buf[5];
        memset(buf, 0, sizeof(buf));
        int ret = -1;
	try {
		/*------------------- Exception case -------------------------*/
		//1.Không đọc đủ số byte mong đợi: Khi client gửi ít hơn 5 byte mong đợi. Xử lý ngoại lệ này bằng cách thêm: throw std::runtime_error("Not enough bytes read");
			/*VD: Client chỉ gửi 4 byte
				HeadMarker = 0xAA;
				Command = 0x58;
				Param1 = 0x2b;
				Param2 = 0x55; */
		//2.Marker không khớp với giá trị mong đợi.
		n_read_bytes = socket_.read(buf,size(buf));
		if (n_read_bytes < sizeof(buf)) {
			throw std::runtime_error("Not enough bytes read");
		}

		if (buf[0] != MARKER_HEAD || buf[4] != MARKER_TAIL) {
			throw std::runtime_error("Invalid markers");
		}

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
private:
    sockpp::tcp_socket& socket_;
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

    while (true) {
        sockpp::inet_address peer;
        sockpp::tcp_socket sock = acc.accept(&peer);

        if (!sock) {
            cerr << "Error accepting incoming connection: " << acc.last_error_str() << endl;
            continue;
        }

        cout << "Received a connection request from " << peer << std::endl;

        // gọi đối tượng MsgHandler
        MsgHandler msgHandler(sock);

        // Nhận và xử lý gói tin từ client
        int result = msgHandler.parse_Msg_Client();
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

    return 0;
}


