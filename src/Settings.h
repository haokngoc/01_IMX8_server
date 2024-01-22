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
	static void added_cb(GObject *client, GAsyncResult *result, gpointer user_data);
//	void add_wifi(NMClient *client, GMainLoop *loop, std::string& id, std::string& pass);
//	void add_wifi(std::string& id, std::string& pass);
	void Read_Json_Configuration();
	void receive_processJson(sockpp::tcp_socket& clientSocket);
private:
    std::string ip_address;
    std::string logging_method;
    std::string logging_level;
    std::string wireless_mode;
    std::string wireless_ssid;
    std::string wireless_pass_phrase;
    std::shared_ptr<spdlog::logger> _logger;
};

#endif // SETTINGS_H
