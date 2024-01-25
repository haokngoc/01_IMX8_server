#ifndef SETTINGS_H
#define SETTINGS_H
#include <iostream>
#include <string>
#include <cstring>
#include <json-c/json.h>
#include "get_connection.hpp"
#include "spdlog/cfg/env.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "sockpp/tcp_acceptor.h"
#include <vector>
#include <NetworkManager.h>
#include <glib.h>
class Settings {
public:
	Settings();
	std::string& getIpAddress();
	void setIpAddress(const std::string &ipAddress);
	std::string& getLoggingLevel();
	void setLoggingLevel(const std::string &loggingLevel);

	std::string& getLoggingMethod();

	void setLoggingMethod(const std::string &loggingMethod);

	std::string& getWirelessMode() ;

	void setWirelessMode(const std::string &wirelessMode) ;

	std::string& getWirelessPassPhrase();

	void setWirelessPassPhrase(const std::string &wirelessPassPhrase) ;

	std::string& getWirelessSsid() ;

	void setWirelessSsid(const std::string &wirelessSsid);
	void fromJson(json_object* j);
	void printSetting();
	static void setIP(const std::string& ipAddress);
	struct in_addr getCurrentIP();
	void Read_Json_Configuration();
	void receive_processJson(sockpp::tcp_socket& clientSocket);
	void set_check(bool check_) {
		this->check = check_;
	}
	bool get_check() {
		return check;
	}
	void handle_scan_wf(sockpp::tcp_socket& clientSocket);
private:
    std::string ip_address;
    std::string logging_method;
    std::string logging_level;
    std::string wireless_mode;
    std::string wireless_ssid;
    std::string wireless_pass_phrase;
    std::shared_ptr<spdlog::logger> _logger;
    bool check;
};

#endif // SETTINGS_H
