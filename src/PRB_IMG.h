#pragma once
#include <iostream>
#include <mutex>
#define BUFFER_SIZE (21 * 1024 * 1024)
class PRB_IMG {
public:

	PRB_IMG(PRB_IMG &other) = delete;
	void operator=(const PRB_IMG &) = delete;
	static PRB_IMG * getInstance();
	pthread_mutex_t& get_connection_mutex();
	void calledPRB_IMG();
	int IMG_acquire();
	int get_IMG(char* buf);

private:
	static PRB_IMG * uniqueInstance;
	int index_;
	PRB_IMG(const int index) {
		this->index_ = index;
	}
	unsigned char img_buf[BUFFER_SIZE];
	pthread_mutex_t connection_mutex;
};
