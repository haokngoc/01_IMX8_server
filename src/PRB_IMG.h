/*
 * PRB_IMG.h
 *
 *  Created on: Jan 20, 2024
 *      Author: Long Le
 */
#pragma once
#include <iostream>
#include <mutex>
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

struct buffer {
        void   *start;
        size_t length;
};

class PRB_IMG {
public:
	PRB_IMG(PRB_IMG &other) = delete;
	void operator=(const PRB_IMG &) = delete;
	static PRB_IMG * getInstance();
	pthread_mutex_t& get_connection_mutex();
	void calledPRB_IMG();
	int IMG_acquire();
	int get_IMG(char* buf);
	int intialize_stream();
	int open_stream();
	int close_stream();
	int trigger_event();

private:
	static PRB_IMG * uniqueInstance;
	int index_;
	struct buffer  *p_dma_buffers;
//	struct v4l2_buffer   v4l2_buf;
	PRB_IMG(const int index) {
		this->index_ = index;
	}
	unsigned char img_buf[BUFFER_SIZE];
	pthread_mutex_t connection_mutex;
	int fd_mxc;
	int fd_csi;
	int fd_prb;
	char const * dev_name_mxc;
	char const * dev_name_csi;
	char const * dev_name_prb;
	int xioctl(int fh, int request, void *arg);

};
