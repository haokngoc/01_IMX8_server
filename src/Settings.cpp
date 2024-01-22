#include <iostream>
#include <string>
#include "Settings.h"
#include "Common.h"
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
			}  else if (strcmp(key, "logging-level") == 0) {
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
	this->_logger->info("ip-address: {}", ip_address);
	this->_logger->info("logging-level: {}", logging_level);
	this->_logger->info("wireless-mode: {}", wireless_mode);
	this->_logger->info("wireless-SSID: {}", wireless_ssid);
	this->_logger->info("wireless-Pass-Phrase: {}", wireless_pass_phrase);
}

void Settings::setIP(const std::string& ipAddress) {
    struct ifreq ifr;
#ifdef IMX8_SOM
    const char *name = "wlp1s0";
#else if
    const char *name = "wlo1";
#endif
    int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

    strncpy(ifr.ifr_name, name, IFNAMSIZ);
    ifr.ifr_addr.sa_family = AF_INET;

    struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
    inet_pton(AF_INET, ipAddress.c_str(), &addr->sin_addr);

    int ret = ioctl(fd, SIOCSIFADDR, &ifr);

    inet_pton(AF_INET, "255.255.0.0", &addr->sin_addr);
    ret = ioctl(fd, SIOCSIFNETMASK, &ifr);

    ret = ioctl(fd, SIOCGIFFLAGS, &ifr);
    strncpy(ifr.ifr_name, name, IFNAMSIZ);
    ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);

    ret = ioctl(fd, SIOCSIFFLAGS, &ifr);

}
struct in_addr Settings::getCurrentIP() {
	int fd=0;
	struct ifreq ifr;
	struct in_addr IP;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
#ifdef IMX8_SOM
	strncpy(ifr.ifr_name, "wlp1s0", IFNAMSIZ-1);
#else if
	strncpy(ifr.ifr_name, "wlo1", IFNAMSIZ-1);
#endif
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
void set_IP(const std::string& ipAddress) {
    struct ifreq ifr;
#ifdef IMX8_SOM
    const char *name = "wlp1s0";
#else if
    const char *name = "wlo1";
#endif
    int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

    strncpy(ifr.ifr_name, name, IFNAMSIZ);
    ifr.ifr_addr.sa_family = AF_INET;

    struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
    inet_pton(AF_INET, ipAddress.c_str(), &addr->sin_addr);

    int ret = ioctl(fd, SIOCSIFADDR, &ifr);

    inet_pton(AF_INET, "255.255.0.0", &addr->sin_addr);
    ret = ioctl(fd, SIOCSIFNETMASK, &ifr);

    ret = ioctl(fd, SIOCGIFFLAGS, &ifr);
    strncpy(ifr.ifr_name, name, IFNAMSIZ);
    ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);

    ret = ioctl(fd, SIOCSIFFLAGS, &ifr);

}

void Settings::added_cb(GObject *client, GAsyncResult *result, gpointer user_data) {
    NMRemoteConnection *remote;
    GError *error = NULL;

    remote = nm_client_add_connection_finish(NM_CLIENT(client), result, &error);

    if (error) {
//    	this->_logger->info("Error adding connection: {}");
        g_print("Error adding connection: %s", error->message);
        g_error_free(error);
    } else {
        g_object_unref(remote);
    }

    g_main_loop_quit((GMainLoop *)user_data);
}

void addAndActivateConnectionCallback(GObject *object, GAsyncResult *res, gpointer user_data)
{
	std::shared_ptr<spdlog::logger> _logger;
	_logger = spdlog::get("DET_logger");
    GError *error = nullptr;
    NMClient *client = NM_CLIENT(object);
    GMainLoop *mainLoop = static_cast<GMainLoop *>(user_data);

    nm_client_add_and_activate_connection_finish(client, res, &error);
    if (error)
    {
    	_logger->error("Error adding and activating connection: {}",error->message);
//        std::cerr << "Error adding and activating connection: " << error->message << std::endl;
        g_error_free(error);
    }
    else
    {
    	_logger->info("Connection added and activated successfully!");
    }

    g_main_loop_quit(mainLoop);
}

void add_wifi(std::string& id, std::string& pass) {
    std::shared_ptr<spdlog::logger> logger;
    logger = spdlog::get("DET_logger");
    if (id.empty() || pass.empty()) {
        logger->error("Invalid SSID or password");
        return;
    }
    const char *uuid;
    const char *password;
    const char* cString = id.c_str();
//    const char* name;

    NMDevice *wifiDevice;
    NMActiveConnection *active_con;

    NMClient *client = nm_client_new(NULL, NULL);

#ifdef IMX8_SOM
    const char *name = "wlp1s0";
#else if
    const char *name = "wlo1";
#endif
    wifiDevice = nm_client_get_device_by_iface(client, name);


    // active_con->
    logger->info("ssid: {}",id);
    logger->info("pass: {}",pass);
    /* Create a new connection object */
    uuid = nm_utils_uuid_generate();
    GString *ssid = g_string_new(cString);
    password = pass.c_str();
    // Create a new connection profile
     NMConnection *connection = nm_simple_connection_new();

     // Create and set the connection settings
     NMSettingConnection *connectionSetting = NM_SETTING_CONNECTION(nm_setting_connection_new());
     g_object_set(G_OBJECT(connectionSetting),
                  NM_SETTING_CONNECTION_ID, id.c_str(),
                  NM_SETTING_CONNECTION_UUID, nm_utils_uuid_generate(),
                  NM_SETTING_CONNECTION_TYPE, NM_SETTING_WIRELESS_SETTING_NAME,
                  NM_SETTING_CONNECTION_AUTOCONNECT, FALSE,
                  NULL);
     nm_connection_add_setting(connection, NM_SETTING(connectionSetting));


     // Create and set the wireless settings
     NMSettingWireless *wirelessSetting = NM_SETTING_WIRELESS(nm_setting_wireless_new());
     int ssidSize = id.size(); // Length of "Zarka"
   //  GBytes *ssid = g_bytes_new(id.c_str(), ssidSize);
     g_object_set(G_OBJECT(wirelessSetting),
                  NM_SETTING_WIRELESS_SSID, ssid,
                  NM_SETTING_WIRELESS_MODE, NM_SETTING_WIRELESS_MODE_INFRA,
                  NULL);
     nm_connection_add_setting(connection, NM_SETTING(wirelessSetting));




     // Create and set the wireless security settings
       NMSettingWirelessSecurity *wirelessSecuritySetting = NM_SETTING_WIRELESS_SECURITY(nm_setting_wireless_security_new());
       g_object_set(G_OBJECT(wirelessSecuritySetting),
                    NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "wpa-psk",
                    NM_SETTING_WIRELESS_SECURITY_PSK, password,
                    NULL);
       nm_connection_add_setting(connection, NM_SETTING(wirelessSecuritySetting));

       GMainLoop *mainLoop = g_main_loop_new(NULL, FALSE);

       // Increment reference counts for connection and client objects
       // to prevent premature destruction during asynchronous operation
       g_object_ref(connection);
       g_object_ref(client);

       nm_client_add_and_activate_connection_async(client, connection, wifiDevice, nullptr, nullptr, addAndActivateConnectionCallback, mainLoop);

       // Start the main loop to process asynchronous operations
       g_main_loop_run(mainLoop);

       g_main_loop_unref(mainLoop);

       g_object_unref(connection);
       g_object_unref(client);

}
struct in_addr get_IP() {
	int fd=0;
	struct ifreq ifr;
	struct in_addr IP;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
#ifdef IMX8_SOM
	strncpy(ifr.ifr_name, "wlp1s0", IFNAMSIZ-1);
#else if
	strncpy(ifr.ifr_name, "wlo1", IFNAMSIZ-1);
#endif
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

void Settings::Read_Json_Configuration() {
    std::shared_ptr<spdlog::logger> logger;
    logger = spdlog::get("DET_logger");
    std::ifstream file(JSON_FILE_NAME);
    if (!file.is_open()) {
        logger->error("Error opening file.");
        return;
    }
    // Đọc nội dung của tệp JSON vào một chuỗi
    std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    // Đóng tệp
    file.close();
    // Phân tích chuỗi JSON để tạo đối tượng json_object
    json_object *j = json_tokener_parse(jsonContent.c_str());

    // Kiểm tra nếu phân tích thành công
    if (j != nullptr) {
        // In ra nội dung của đối tượng JSON
    	logger->info("JSON Content: {}", json_object_to_json_string_ext(j, JSON_C_TO_STRING_PRETTY));
        // Giải phóng bộ nhớ của đối tượng JSON khi không cần thiết nữa

    } else {
    	logger->error("Error parsing JSON content.");
    }

//    this->fromJson(j);
	std::string ssid;
	std::string password;
	json_object* settingsObj = nullptr;
	if (json_object_object_get_ex(j, "settings", &settingsObj)) {
		json_object_object_foreach(settingsObj, key, val) {
			if (strcmp(key, "ip-address") == 0) {
				this->ip_address = json_object_get_string(val);
			} else if (strcmp(key, "logging-level") == 0) {
				this->logging_level = json_object_get_string(val);
			} else if (strcmp(key, "wireless-mode") == 0) {
				this->wireless_mode = json_object_get_string(val);
			} else if (strcmp(key, "wireless-SSID") == 0) {
				this->wireless_ssid = json_object_get_string(val);
				ssid = json_object_get_string(val);
			} else if (strcmp(key, "wireless-Pass-Phrase") == 0) {
				this->wireless_pass_phrase = json_object_get_string(val);
				password = json_object_get_string(val);
			}
		}
	}
//    this->printSetting();
	logger->info("ip-address: {}", this->ip_address);
	logger->info("logging-level: {}", this->logging_level);
	logger->info("wireless-mode: {}", this->wireless_mode);
	logger->info("wireless-SSID: {}", this->wireless_ssid);
	logger->info("wireless-Pass-Phrase: {}", this->wireless_pass_phrase);
    json_object_put(j);
    // add wifi
    add_wifi(ssid, password);
    // get ip address
    struct in_addr currentIP = get_IP();

    char ipString[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &currentIP, ipString, INET_ADDRSTRLEN) == NULL) {
    	logger->error("inet_ntop");
    }
    logger->info("Current IP address: {}",ipString);
    // set ip
    set_IP(ip_address);
}
void Settings::receive_processJson(sockpp::tcp_socket& clientSocket) {
    std::shared_ptr<spdlog::logger> logger;
    logger = spdlog::get("DET_logger");
	std::string cmdID;
	const int bufferSize = 1024;
	char buffer[bufferSize];
	// Nhận dữ liệu JSON từ client
	ssize_t bytesRead = clientSocket.read(buffer, bufferSize);
	if (bytesRead < 0) {
		spdlog::error("Error receiving data from client.");
		return;
	}
	// Phân tích dữ liệu JSON
	struct json_object* j = json_tokener_parse(buffer);
	if (j == nullptr) {
		 spdlog::error("Error parsing JSON data received from client.");
		return;
	}
	// Hiển thị giá trị JSON nhận được
	logger->info("Received JSON data:");
	logger->info("{}",json_object_to_json_string_ext(j, JSON_C_TO_STRING_PRETTY));
	// Lưu dữ liệu JSON vào tệp tin
	std::ofstream outputFile(JSON_FILE_NAME);
	if (outputFile.is_open()) {
		outputFile << json_object_to_json_string_ext(j, JSON_C_TO_STRING_PRETTY) << std::endl;
		outputFile.close();
		logger->info( "Saved JSON data to 'received_data.json");
	} else {
		logger->error("Error saving JSON data to file.");
	}
	std::string ssid;
	std::string password;
	std::string ip;
	json_object* settingsObj = nullptr;
	if (json_object_object_get_ex(j, "settings", &settingsObj)) {
		json_object_object_foreach(settingsObj, key, val) {
			if (strcmp(key, "ip-address") == 0) {
				this->ip_address = json_object_get_string(val);
				ip = json_object_get_string(val);
			} else if (strcmp(key, "logging-method") == 0) {
				this->logging_method = json_object_get_string(val);
			} else if (strcmp(key, "logging-level") == 0) {
				this->logging_level = json_object_get_string(val);
			} else if (strcmp(key, "wireless-mode") == 0) {
				this->wireless_mode = json_object_get_string(val);
			} else if (strcmp(key, "wireless-SSID") == 0) {
				this->wireless_ssid = json_object_get_string(val);
				ssid = json_object_get_string(val);
			} else if (strcmp(key, "wireless-Pass-Phrase") == 0) {
				this->wireless_pass_phrase = json_object_get_string(val);
				password = json_object_get_string(val);
			}
		}
	}
//    this->printSetting();
	logger->info("ip-address: {}", this->ip_address);
	logger->info("logging-method: {}", this->logging_method);
	logger->info("logging-level: {}", this->logging_level);
	logger->info("wireless-mode: {}", this->wireless_mode);
	logger->info("wireless-SSID: {}", this->wireless_ssid);
	logger->info("wireless-Pass-Phrase: {}", this->wireless_pass_phrase);
	json_object_put(j);

	// Gửi thông báo xác nhận về cho client
	const char* confirmationMsg = "Server recevied data sucessfuly";
	clientSocket.write(confirmationMsg, strlen(confirmationMsg));
	// add wifi
	add_wifi(ssid,password);

	// Bắt đầu vòng lặp chính
//	g_main_loop_run(loop);

	// Giải phóng tài nguyên trước khi thoát chương trình
//	g_main_loop_unref(loop);
//	g_object_unref(client);

	// get ip address
	struct in_addr currentIP = get_IP();
	char ipString[INET_ADDRSTRLEN];
	if (inet_ntop(AF_INET, &currentIP, ipString, INET_ADDRSTRLEN) == NULL) {
	   perror("inet_ntop");
	}
	logger->info("Current IP address: {}",ipString);
	// Gửi địa chỉ IP về cho client
	clientSocket.write(ipString, strlen(ipString));
	// set ip
	set_IP(ip);
	// gửi thông báo thay đổi ip thành công
	const char *ipAddress = ip.c_str();
	std::string confirmMessage = "IP address changed successfully to " + std::string(ipAddress);
	const char *confirm_ip = confirmMessage.c_str();
	clientSocket.write(confirm_ip, strlen(confirm_ip));
}

