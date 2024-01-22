/*
 * PRB_IMG.cpp
 *
 *  Created on: Jan 20, 2024
 *      Author: duclong
 */

#include "PRB_IMG.h"
#include <mutex>
#include <random>
#include <iostream>
#include <cstdlib>
#include <thread>
#include <cstring>
#include <pthread.h>
#include "Common.h"



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


int PRB_IMG::xioctl(int fh, int request, void *arg)
{
        int ret =0;
    	int n_try = 10;
#ifdef 	IMX8_SOM
    	while(n_try > 0){
    		ret = v4l2_ioctl(fh, request, arg);
    		if (ret ==  0)
    			break;
    		else
    			n_try --;
    		usleep(10);
    	}
#endif
    	return ret;
}

int PRB_IMG::trigger_event(){
// Initialize PRB
	  int ret = 0;
#ifdef 	IMX8_SOM
      struct v4l2_dbg_register reg_w = {0};
      /* Set Subdevice-0 */
      reg_w.match.type = reg_w.match.type = V4L2_CHIP_MATCH_SUBDEV;
      reg_w.match.addr = reg_w.match.addr = 0;
      /* Initialize register addr */
      reg_w.reg = TRIGGER_REG ;
      reg_w.size =2;
      reg_w.val = 0x1 ;

      /* Write register */
      reg_w.val = 0x1;
      ret = this->xioctl(this->fd_prb, VIDIOC_DBG_S_REGISTER, &reg_w) ;
	  if(ret < 0)
		   return ret;
      v4l2_close(this->fd_prb);
#endif
      return ret;
}



int PRB_IMG::intialize_stream() {

		int ret  = 0;
		unsigned int                    i, n_buffers;
#ifdef 	IMX8_SOM
		struct v4l2_format              v4l2_fmt;
		struct v4l2_requestbuffers      v4l2_req;
//		enum v4l2_buf_type              v4l2_type;
		struct v4l2_buffer   			v4l2_buf;
	    csi_sam_format					m_csi_sam_format;
		this->dev_name_mxc = 			"/dev/video1";
		this->dev_name_csi = 			"/dev/v4l-subdev0";
		this->dev_name_prb = 			"/dev/v4l-subdev2";
	//    struct buffer                	*dma_buffers = this->p_dma_buffers;

	    this->fd_mxc = v4l2_open(this->dev_name_mxc, O_RDWR | O_NONBLOCK, 0);
	    if (this->fd_mxc < 0) {
	            perror("It looks like video chain is not install properly! \n");
	            exit(EXIT_FAILURE);
	    }

	    this->fd_csi = v4l2_open(this->dev_name_csi, O_RDWR | O_NONBLOCK, 0);
	    if (this->fd_csi < 0) {
	            perror("MIPI camera driver doesn't work properly!");
	            exit(EXIT_FAILURE);
	    }

	    this->fd_prb = v4l2_open(this->dev_name_prb, O_RDWR | O_NONBLOCK, 0);
	    if (this->fd_prb < 0) {
	            perror("MIPI camera driver doesn't work properly!");
	            exit(EXIT_FAILURE);
	    }

	    CLEAR(v4l2_fmt);
	    CLEAR(m_csi_sam_format);
	    CLEAR(v4l2_req);

	    m_csi_sam_format.format =   v4l2_fmt.fmt.pix.pixelformat  = V4L2_PIX_FMT_RGB565;
	    m_csi_sam_format.width =  v4l2_fmt.fmt.pix.width   = PRB_WIDTH_DEFAULT; //640;
	    m_csi_sam_format.height = v4l2_fmt.fmt.pix.height    = PRB_HEIGHT_DEFAULT; // 480;
	    v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	// set format for /dev/video1 - mxc-capture
	    v4l2_fmt.fmt.pix.field       = V4L2_FIELD_NONE;
	// Set format for MIPI-CSI module
	   ret = this->xioctl(this->fd_csi, VVCSIOC_S_FMT, &m_csi_sam_format);
	   if(ret < 0)
		   return ret;

	   ret = this->xioctl(this->fd_mxc, VIDIOC_S_FMT, &v4l2_fmt);
	   if(ret < 0)
		   return ret;

	    v4l2_req.count = N_BUF;
	    v4l2_req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	    v4l2_req.memory = V4L2_MEMORY_MMAP;
	    ret = this->xioctl(this->fd_mxc, VIDIOC_REQBUFS, &v4l2_req);
		if(ret < 0)
			 return ret;

	    this->p_dma_buffers = (struct buffer *)calloc(v4l2_req.count, sizeof(*this->p_dma_buffers));
	    for (n_buffers = 0; n_buffers < v4l2_req.count; ++n_buffers) {
	            CLEAR(v4l2_buf);

	            v4l2_buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	            v4l2_buf.memory      = V4L2_MEMORY_MMAP;
	            v4l2_buf.index       = n_buffers;

	            ret = this->xioctl(this->fd_mxc, VIDIOC_QUERYBUF, &v4l2_buf);
	            if(ret < 0)
	     		   return ret;

	            this->p_dma_buffers[n_buffers].length = v4l2_buf.length;
	            this->p_dma_buffers[n_buffers].start = v4l2_mmap(NULL, v4l2_buf.length,
	                          PROT_READ | PROT_WRITE, MAP_SHARED,
							  this->fd_mxc, v4l2_buf.m.offset);

	            if (MAP_FAILED == this->p_dma_buffers[n_buffers].start) {
	                    perror("mmap");
	                    exit(EXIT_FAILURE);
	            }
	    }

	    for (i = 0; i < n_buffers; ++i) {
	           CLEAR(v4l2_buf);
	           v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	           v4l2_buf.memory = V4L2_MEMORY_MMAP;
	           v4l2_buf.index = i;
	           ret = this->xioctl(this->fd_mxc, VIDIOC_QBUF, &v4l2_buf);
	     	   if(ret < 0)
	     		   return ret;
	    }
		CLEAR(v4l2_buf);
#endif
	return ret;
}


int PRB_IMG::open_stream(){
	int ret = 0;
#ifdef 	IMX8_SOM
	fd_set                          fds;
	struct timeval                  tv;
	int                             r ;
	enum v4l2_buf_type              v4l2_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = this->xioctl(this->fd_mxc, VIDIOC_STREAMON, &v4l2_type);
	if(ret < 0)
		return ret;

	FD_ZERO(&fds);
	FD_SET(this->fd_mxc, &fds);
	/* Timeout. */
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	r = select(this->fd_mxc + 1, &fds, NULL, NULL, &tv);
	if (r == -1) {
			perror("Waiting for Frame");
			return errno;
	}
#endif

	return ret;

}

int PRB_IMG::close_stream(){
	int ret = 0;
#ifdef 	IMX8_SOM
	enum v4l2_buf_type   type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = this->xioctl(this->fd_mxc, VIDIOC_STREAMOFF, &type);
	if(ret < 0)
		return ret;

    for (int i = 0; i < N_BUF; ++i)
            v4l2_munmap(this->p_dma_buffers[i].start, this->p_dma_buffers[i].length);
    v4l2_close(this->fd_mxc);
    v4l2_close(this->fd_csi);
    v4l2_close(this->fd_prb);
#endif
    return ret;
}

int PRB_IMG::IMG_acquire() {
	int ret = 0;
	pthread_mutex_lock(&this->get_connection_mutex());
#ifdef 	IMX8_SOM
	/* Write register */
    auto start = high_resolution_clock::now();
    struct v4l2_buffer   			v4l2_buf;
    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;
    v4l2_buf.index = 0;
    ret = this->xioctl(this->fd_mxc, VIDIOC_DQBUF, &v4l2_buf);
	if(ret < 0)
		return ret;
	memcpy(this->img_buf,this->p_dma_buffers[v4l2_buf.index].start,v4l2_buf.bytesused );
//    FILE * fout = fopen("before_sending.raw", "w");
//    fwrite(this->img_buf, 21980160, 1, fout);
//    fclose(fout);
    auto stop = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(stop - start);
	cout << "-----------Time taken by function: "
			<< duration.count() << " microseconds------------ number of bytes: " << v4l2_buf.bytesused << endl;

#else
	for(size_t i=0; i<BUFFER_SIZE; ++i) {
	 this->img_buf[i] = static_cast<unsigned char>(rand() % 256);
	}
#endif

	pthread_mutex_unlock(&this->get_connection_mutex());
	return ret;
}
int PRB_IMG::get_IMG(char* buf) {
	pthread_mutex_lock(&this->get_connection_mutex());
	std::memcpy(buf, img_buf, BUFFER_SIZE);
	pthread_mutex_unlock(&this->get_connection_mutex());
	return 0;
}
pthread_mutex_t& PRB_IMG::get_connection_mutex() {
    return this->connection_mutex;
}
