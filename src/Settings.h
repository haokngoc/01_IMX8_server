#include <iostream>
#include <string>
#include <cstring>
#include <json-c/json.h>

class Settings {
public:
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
private:
    std::string ip_address;
    std::string logging_method;
    std::string logging_level;
    std::string wireless_mode;
    std::string wireless_ssid;
    std::string wireless_pass_phrase;
};
