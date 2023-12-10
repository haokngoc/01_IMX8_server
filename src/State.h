#pragma once

class MsgHandler; // Forward declaration

class State {
public:
    virtual ~State() {}
    virtual void handle(MsgHandler& handler) = 0;
};

class WorkState : public State {
	public:
		void handle(MsgHandler& handler) override;
};

class SleepState : public State {
	public:
		void handle(MsgHandler& handler) override;
};


