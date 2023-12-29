#pragma once

class Mgard300_Handler;

class DET_State {
public:
    virtual ~DET_State() {}
    virtual void handle(Mgard300_Handler& handler) = 0;
    void set_current_state(int current_state);
private:
    int current_state;
};

class WorkState : public DET_State {
public:
    void handle(Mgard300_Handler& handler) override;
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
