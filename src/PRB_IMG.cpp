#include "PRB_IMG.h"

#include <mutex>
#include <random>
#include <iostream>
#include <cstdlib>
#include <thread>
#include <cstring>
#include <pthread.h>
#include "Common.h"

#ifdef IMX8_SOM
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
#include <libv4l2.h>
#include<unistd.h>
#include <algorithm>
#include <chrono>
#include <memory>

using namespace std;
using namespace std::chrono;
#define CLEAR(x) memset(&(x), 0, sizeof(x))

struct buffer {
        void   *start;
        size_t length;
};
static void xioctl(int fh, int request, void *arg)
{
        int r;

        do {
                r = v4l2_ioctl(fh, request, arg);
        } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

        if (r == -1) {
                fprintf(stderr, "error %d, %s\\n", errno, strerror(errno));
                exit(EXIT_FAILURE);
        }
}

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
#endif


PRB_IMG* PRB_IMG::uniqueInstance{nullptr};

PRB_IMG *PRB_IMG::getInstance(){
	if(uniqueInstance == nullptr) {
		
		// random giá trị index;
		std::random_device rd;
		std::default_random_engine engine(rd());
		std::uniform_int_distribution<int> distribution(1, 10);
		int index = distribution(engine);
		uniqueInstance = new PRB_IMG(index);
	}
	return uniqueInstance;
}

void PRB_IMG::calledPRB_IMG() {
	std::cout << "Singleton " <<  this->index_ <<  " called" << std::endl;;
}
int PRB_IMG::IMG_acquire() {
	pthread_mutex_lock(&this->get_connection_mutex());
#ifdef 	IMX8_SOM
	struct v4l2_format              fmt;
	struct v4l2_buffer              buf;
	struct v4l2_requestbuffers      req;
	enum v4l2_buf_type              type;
	fd_set                          fds;
	struct timeval                  tv;
	int                             r, fd = -1,fd2= -1;
	unsigned int                    i, n_buffers;
	char                            *dev_name = "/dev/video1";
	char                            *dev_name2 = "/dev/v4l-subdev0";
	char                            *dev_name3 = "/dev/v4l-subdev1";
	char                            *dev_name4 = "/dev/v4l-subdev2";

    struct buffer                   *buffers;
    csi_sam_format	m_csi_sam_format;
    fd = v4l2_open(dev_name, O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) {
            perror("It looks like video chain is not install properly! \n");
            exit(EXIT_FAILURE);
    }

    fd2 = v4l2_open(dev_name2, O_RDWR | O_NONBLOCK, 0);
    if (fd2 < 0) {
            perror("MIPI camera driver doesn't work properly!");
            exit(EXIT_FAILURE);
    }
    CLEAR(fmt);
    CLEAR(m_csi_sam_format);
    CLEAR(req);

    m_csi_sam_format.format =   fmt.fmt.pix.pixelformat  = V4L2_PIX_FMT_RGB565;
    m_csi_sam_format.width =  fmt.fmt.pix.width   = 3456; //640;
    m_csi_sam_format.height = fmt.fmt.pix.height    = 3180; // 480;

// Set format for MIPI-CSI module
    xioctl(fd2, VVCSIOC_S_FMT, &m_csi_sam_format);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
// set format for /dev/video1 - mxc-capture
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;
    xioctl(fd, VIDIOC_S_FMT, &fmt);

    req.count = 3;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    xioctl(fd, VIDIOC_REQBUFS, &req);

    buffers = calloc(req.count, sizeof(*buffers));
    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
            CLEAR(buf);

            buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory      = V4L2_MEMORY_MMAP;
            buf.index       = n_buffers;

            xioctl(fd, VIDIOC_QUERYBUF, &buf);

            buffers[n_buffers].length = buf.length;
            buffers[n_buffers].start = v4l2_mmap(NULL, buf.length,
                          PROT_READ | PROT_WRITE, MAP_SHARED,
                          fd, buf.m.offset);

            if (MAP_FAILED == buffers[n_buffers].start) {
                    perror("mmap");
                    exit(EXIT_FAILURE);
            }
    }

    for (i = 0; i < n_buffers; ++i) {
            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            xioctl(fd, VIDIOC_QBUF, &buf);
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

// enable the streaming
       auto start = high_resolution_clock::now();
       xioctl(fd, VIDIOC_STREAMON, &type);
// enable test pattern
//  v4l2_control disable_tp ={V4L2_CID_TEST_PATTERN, 0};
//  xioctl(fd3, VIDIOC_S_CTRL, &disable_tp);

	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	/* Timeout. */
	tv.tv_sec = 10;
	tv.tv_usec = 0;

	r = select(fd + 1, &fds, NULL, NULL, &tv);
	if (r == -1) {
			perror("Waiting for Frame");
			return errno;
	}

	CLEAR(buf);
	/* Write register */
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = 1;
	xioctl(fd, VIDIOC_DQBUF, &buf);
	memcpy(this->img_buf,buffers[buf.index].start,buf.bytesused );
//    FILE * fout = fopen("before_sending.raw", "w");
//    fwrite(this->img_buf, 21980160, 1, fout);
//    fclose(fout);
    auto stop = high_resolution_clock::now();

	// Get duration. Substart timepoints to
	// get duration. To cast it to proper unit
	// use duration cast method
	auto duration = duration_cast<microseconds>(stop - start);
	cout << "-----------Time taken by function: "
			<< duration.count() << " microseconds------------ number of bytes: " << buf.bytesused << endl;
	xioctl(fd, VIDIOC_QBUF, &buf);
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMOFF, &type);
    for (i = 0; i < n_buffers; ++i)
            v4l2_munmap(buffers[i].start, buffers[i].length);
    v4l2_close(fd);
    v4l2_close(fd2);

#else
	for(size_t i=0; i<BUFFER_SIZE; ++i) {
	 this->img_buf[i] = static_cast<unsigned char>(rand() % 256);
	}
#endif
	pthread_mutex_unlock(&this->get_connection_mutex());
	return 1;
}
int PRB_IMG::get_IMG(char* buf) {
	pthread_mutex_lock(&this->get_connection_mutex());
	std::memcpy(buf, img_buf, BUFFER_SIZE);
	pthread_mutex_unlock(&this->get_connection_mutex());
	return 1;
}
pthread_mutex_t& PRB_IMG::get_connection_mutex() {
    return this->connection_mutex;
}
