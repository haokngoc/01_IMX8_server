#include "Singleton.h"
#include <mutex>
#include <random>
#include <iostream>
Singleton* Singleton::uniqueInstance{nullptr};
std::mutex Singleton::mutex_;

Singleton *Singleton::getInstance(){
	std::lock_guard<std::mutex> lock(mutex_);
	if(uniqueInstance == nullptr) {
		// random giá trị index;
		std::random_device rd;
		std::default_random_engine engine(rd());
		std::uniform_int_distribution<int> distribution(1, 10);
		int index = distribution(engine);
		uniqueInstance = new Singleton(index);
	}
	return uniqueInstance;
}

void Singleton::calledSingleton() {
	std::cout << "Singleton " <<  this->index_ <<  " called" << std::endl;;
}
