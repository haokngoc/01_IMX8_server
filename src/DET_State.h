#pragma once
#include <cstdio>
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
    void handle(Mgard300_Handler& handler) override;
private:
    std::thread work_thread;
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
