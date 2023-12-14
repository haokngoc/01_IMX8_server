#pragma once
#include <iostream>
#include <mutex>
class Singleton {
public:
	//Singletons should not be cloneable.
	Singleton(Singleton &other) = delete;

	//Singletons should not be assignable.
	void operator=(const Singleton &) = delete;
	static Singleton * getInstance();
	void calledSingleton();

private:
	static Singleton * uniqueInstance;
	static std::mutex mutex_;
	int index_;
	Singleton(const int index) {
		this->index_ = index;
	}
};
