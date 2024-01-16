#pragma once
#include <iostream>
#include <mutex>
#include "Common.h"

class PRB_IMG {
public:
	PRB_IMG(PRB_IMG &other) = delete;
	void operator=(const PRB_IMG &) = delete;
	static PRB_IMG * getInstance();
	pthread_mutex_t& get_connection_mutex();
	void calledPRB_IMG();
	int IMG_acquire();
	int get_IMG(char* buf);
	int IMG_prepare();
	int IMG_close();
private:
	static PRB_IMG * uniqueInstance;
	int index_;
	PRB_IMG(const int index) {
		this->index_ = index;
	}
	unsigned char img_buf[BUFFER_SIZE];
	pthread_mutex_t connection_mutex;
};
