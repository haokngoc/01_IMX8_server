#include <iostream>
#include <string>
#include "Settings.h"
#include <cstring>
#include <json-c/json.h>
#include <string>
#include <arpa/inet.h>
#include <net/if.h>
#include <fstream>
#include <sys/ioctl.h>
#include <unistd.h>

Settings::Settings() {
	 this->_logger = spdlog::get("DET_logger");
}

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
	spdlog::info("ip-address: {}", ip_address);
	spdlog::info("logging-method: {}", logging_method);
	spdlog::info("logging-level: {}", logging_level);
	spdlog::info("wireless-mode: {}", wireless_mode);
	spdlog::info("wireless-SSID: {}", wireless_ssid);
	spdlog::info("wireless-Pass-Phrase: {}", wireless_pass_phrase);
}
void Settings::setIP(const std::string& ipAddress) {
    struct ifreq ifr;
    const char *name = "wlo1";
    int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

    strncpy(ifr.ifr_name, name, IFNAMSIZ);
    ifr.ifr_addr.sa_family = AF_INET;

    struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
    inet_pton(AF_INET, ipAddress.c_str(), &addr->sin_addr);

    ioctl(fd, SIOCSIFADDR, &ifr);

    inet_pton(AF_INET, "255.255.0.0", &addr->sin_addr);
    ioctl(fd, SIOCSIFNETMASK, &ifr);

    ioctl(fd, SIOCGIFFLAGS, &ifr);
    strncpy(ifr.ifr_name, name, IFNAMSIZ);
    ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);

    ioctl(fd, SIOCSIFFLAGS, &ifr);

}
struct in_addr Settings::getCurrentIP() {
	int fd=0;
	struct ifreq ifr;
	struct in_addr IP;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "wlo1", IFNAMSIZ-1);
	if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
		perror("SIOCGIFFLAGS");}
	if ((ifr.ifr_flags & IFF_UP) && (ifr.ifr_flags & IFF_RUNNING)){
		ioctl(fd, SIOCGIFADDR, &ifr);
		close(fd);
		IP = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
	} else {
		close(fd);
		inet_aton ("127.0.0.1",&IP);
	}
	return  IP;
}
void Settings::added_cb(GObject *client, GAsyncResult *result, gpointer user_data) {
    NMRemoteConnection *remote;
    GError *error = NULL;

    remote = nm_client_add_connection_finish(NM_CLIENT(client), result, &error);

    if (error) {
        g_print("Error adding connection: %s", error->message);
        g_error_free(error);
    } else {
        g_object_unref(remote);
    }

    g_main_loop_quit((GMainLoop *)user_data);
}

void Settings::add_wifi(NMClient *client, GMainLoop *loop, std::string& id, std::string& pass) {
    NMConnection *connection;

    const char *uuid;
    const char *password;
    const char* cString = id.c_str();
    spdlog::info("ssid: {}",id);
    spdlog::info("pass: {}",pass);
    /* Create a new connection object */
    uuid = nm_utils_uuid_generate();
    GString *ssid = g_string_new(cString);
    password = pass.c_str();
    connection = get_client_nmconnection(cString, uuid, ssid, password);

    nm_client_add_connection_async(client, connection, TRUE, NULL, added_cb, loop);
    g_object_unref(connection);
}
