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
int PRB_IMG::IMG_acquire() {
	pthread_mutex_lock(&this->get_connection_mutex());
	for(size_t i=0; i<BUFFER_SIZE; ++i) {
	 this->img_buf[i] = static_cast<unsigned char>(rand() % 256);
	}
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
