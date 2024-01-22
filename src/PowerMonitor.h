/*
 * PowerMonitor.h
 *
 *  Created on: Dec 14, 2023
 *      Author: hk
 */
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/async.h"
#include "spdlog/cfg/env.h"
#ifndef POWERMONITOR_H_
#define POWERMONITOR_H_

#define BATTERY_VOLTAGE 0x01
#define BATTERY_CURRENT 0x02
#define BATTERY_AVG_CURRENT 0x03
#define BATTERY_ABSOLUTE_STATE_OF_CHARGE 0x04
#define BATTERY_FULL_REMAIN_CAP 0x05
#define BAT_FULL_CHARGE_CAP 0x06
#define BAT_CHARGE_CURRENT 0x07
#define BAT_STATUS 0x08
#define BAT_CYCLE_COUNT 0x09
#define BAT_SERIAL_NUMBER 0x0A

#define IP_ADDR_01 0x10
#define IP_ADDR_02 0x11
#define WIFI_STRENGTH 0x12
#define WIFI_MODE 0x13

#define SEND_HEADER_MARKER 0x55
#define RECV_HEADER_MARKER 0x23
#define TAIL_MARKER 0xAA
enum {
	BAT01 = 0,
	BAT02
};

class PowerMonitor {
private:
    int battery_vol[2];
    int battery_ampere[2];
    int battery_avg_ampere[2];
    int absolute_state_charge[2];
    int capacity_remain[2];
    int capacity_full_charge[2];
    int charging_current[2];
    int battery_status[2];
    int cycle_count[2];
    unsigned char serial_number[4];
    int serial_port;
    unsigned char CRC8(const unsigned char* vptr, int len);
    std::shared_ptr<spdlog::logger> _logger;
    static PowerMonitor* instance;
    PowerMonitor();
public:
    int uart_init();
    int uart_get_data(unsigned char ID, unsigned char* data, unsigned char data_length);
    int uart_set_data(unsigned char ID, unsigned char* data, unsigned char data_length);
    int uart_close();

    int uart_get_bat_vol();
    int uart_get_bat_ampere();
    int uart_get_avg_ampere();
    int uart_get_absolute_state_charge();
    int uart_get_capacity_remain();
    int uart_get_capacity_full_charge();
    int uart_get_charging_current();
    int uart_get_battery_status();
    int uart_get_cycle_count();


    int BAT01_get_battery_vol() const;
	int BAT01_get_battery_ampere() const;
	int BAT01_get_avg_ampere() const;
	int BAT01_get_absolute_state_charge();
	int BAT01_get_capacity_remain() const;
    int BAT01_get_capacity_full_charge() const;
	int BAT01_get_charging_current() const;
	int BAT01_get_battery_status() const;
	int BAT01_get_cycle_count() const;

    int BAT02_get_battery_vol() const;
	int BAT02_get_battery_ampere() const;
	int BAT02_get_avg_ampere() const;
	int BAT02_get_absolute_state_charge();
	int BAT02_get_capacity_remain() const;
    int BAT02_get_capacity_full_charge() const;
	int BAT02_get_charging_current() const;
	int BAT02_get_battery_status() const;
	int BAT02_get_cycle_count() const;

	int BAT_get_serial_number(unsigned char * serial_number);

	static PowerMonitor* getInstance();
	static void destroyInstance();
	~PowerMonitor();
};




#endif /* POWERMONITOR_H_ */
