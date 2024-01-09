#include <iostream>
#include <string>
#include "Settings.h"
#include <cstring>
#include <json-c/json.h>



std::string&  Settings::getIpAddress() {
	return ip_address;
}



std::string& Settings::getLoggingLevel() {
	return logging_level;
}



std::string& Settings::getLoggingMethod() {
	return logging_method;
}



std::string& Settings::getWirelessMode() {
	return wireless_mode;
}


std::string& Settings::getWirelessPassPhrase() {
	return wireless_pass_phrase;
}



std::string& Settings::getWirelessSsid() {
	return wireless_ssid;
}


void Settings::fromJson(json_object* j) {
	json_object* settingsObj = nullptr;
	if (json_object_object_get_ex(j, "settings", &settingsObj)) {
		json_object_object_foreach(settingsObj, key, val) {
			if (strcmp(key, "ip-address") == 0) {
				ip_address = json_object_get_string(val);
			} else if (strcmp(key, "logging-method") == 0) {
				logging_method = json_object_get_string(val);
			} else if (strcmp(key, "logging-level") == 0) {
				logging_level = json_object_get_string(val);
			} else if (strcmp(key, "wireless-mode") == 0) {
				wireless_mode = json_object_get_string(val);
			} else if (strcmp(key, "wireless-SSID") == 0) {
				wireless_ssid = json_object_get_string(val);
			} else if (strcmp(key, "wireless-Pass-Phrase") == 0) {
				wireless_pass_phrase = json_object_get_string(val);
			}
		}
	}
}
void Settings::printSetting() {
	std::cout << "ip-address: " << ip_address << std::endl;
	std::cout << "logging-method: " << logging_method << std::endl;
	std::cout << "logging-level: " << logging_level << std::endl;
	std::cout << "wireless-mode: " << wireless_mode << std::endl;
	std::cout << "wireless-SSID: " << wireless_ssid << std::endl;
	std::cout << "wireless-Pass-Phrase: " << wireless_pass_phrase << std::endl;

}


