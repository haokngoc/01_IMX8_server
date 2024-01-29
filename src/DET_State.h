#pragma once
#include <cstdio>
#include <mutex>
#include "spdlog/cfg/env.h"
#include "spdlog/sinks/stdout_color_sinks.h"
class Mgard300_Handler;

class DET_State {
public:
	DET_State();
    virtual ~DET_State() {}
    virtual void handle(Mgard300_Handler& handler) = 0;
    void set_current_state(int current_state);
private:
    int current_state;
protected:
    std::shared_ptr<spdlog::logger> _logger;

};


class WorkState : public DET_State {
public:
	WorkState() : is_working(false), a(0) {}
    void handle(Mgard300_Handler& handler) override;
    void set_is_working(bool check_) {
    	std::lock_guard<std::mutex> lock(this->connection_mutex);
    	this->is_working = check_;
    }
    bool get_is_working() {
    	std::lock_guard<std::mutex> lock(this->connection_mutex);
    	return this->is_working;
    }
    void set_is_a(int z) {
    	std::lock_guard<std::mutex> lock(this->connection_mutex);
    	this->a = z;
    }
    int get_is_a() {
    	std::lock_guard<std::mutex> lock(this->connection_mutex);
    	return this->a;
    }
private:
    std::thread work_thread;
    std::mutex connection_mutex;
    bool is_working;
    int a;
};

class SleepState : public DET_State {
public:

    void handle(Mgard300_Handler& handler) override;
};

class CloseState : public DET_State {
public:

	void handle(Mgard300_Handler& handler) override;

};
class TriggerState : public DET_State {
public:
	void handle(Mgard300_Handler& handler) override;
};
