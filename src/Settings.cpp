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
/*---------------------------------------------Scan_wifi--------------------------------------------------------*/
enum BandType
{
    BAND_NONE = 0, /* unknown */
    BAND_2GHZ = 1, /* 2.4GHz */
    BAND_5GHZ = 2, /* 5GHz */
    BAND_ANY = 3   /* Dual-mode frequency band */
};
struct AccessPointInfo
{
    std::string ssid;
    std::string bssid;
    int rssi;
    int frequency;
    BandType band;
    std::string security;
};
struct WifiStatus
{
    bool wifi_enabled;
    NMClient *client;
    GMainLoop *loop;
    NMDevice *wifi_device;
    int attempts;
};

BandType get_band(guint32 frequency)
{
    if (frequency >= 2400 && frequency <= 2495)
    {
        return BAND_2GHZ;
    }
    else if (frequency >= 5150 && frequency <= 5925)
    {
        return BAND_5GHZ;
    }
    else
    {
        return BAND_NONE;
    }
}
const char *get_wifi_security_type(NMAccessPoint *ap)
{
    NM80211ApSecurityFlags wpa_flags = nm_access_point_get_wpa_flags(ap);
    NM80211ApSecurityFlags rsn_flags = nm_access_point_get_rsn_flags(ap);

    if (wpa_flags & NM_802_11_AP_SEC_KEY_MGMT_PSK || rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_PSK)
    {
        return "WPA-PSK";
    }
    else if (wpa_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X || rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X)
    {
        return "WPA-Enterprise";
    }
    else if (wpa_flags == 0 && rsn_flags == 0)
    {
        return "None";
    }
    else
    {
        return "Unknown";
    }
}
void on_wifi_scan_finished(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    WifiStatus *wifi_status = (WifiStatus *)user_data;
    GError *error = NULL;
    gboolean success = nm_device_wifi_request_scan_finish(NM_DEVICE_WIFI(source_object), res, &error);
    if (!success)
    {
        if (g_strrstr(error->message, "Scanning not allowed") != NULL)
        {
            // Scanning not allowed immediately following previous scan
            // Retry after a short delay
            g_print("Scanning not allowed, retrying in 4 seconds...\n");
            g_error_free(error);
            sleep(4);

            // Check if attempts is less than 10
            if (wifi_status->attempts < 10)
            {
                wifi_status->attempts++; // Increment attempts
                nm_device_wifi_request_scan_async(NM_DEVICE_WIFI(source_object), NULL, on_wifi_scan_finished, wifi_status);
            }
            else
            {
                g_print("Reached maximum retries, terminating...\n");
            }

            return;
        }

        g_print("Error scanning for WiFi networks: %s\n", error->message);
        g_error_free(error);
    }
    GMainLoop *loop = wifi_status->loop;
    g_main_loop_quit(loop);
}

bool scan_wifi(WifiStatus *wifi_status)
{
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    wifi_status->loop = loop;
    nm_device_wifi_request_scan_async(NM_DEVICE_WIFI(wifi_status->wifi_device), NULL, on_wifi_scan_finished, wifi_status);
    g_main_loop_run(loop);
    g_main_loop_unref(loop);
    return true;
}
std::vector<AccessPointInfo> get_scan_info(WifiStatus *wifi_status, const std::string &target_ssid)
{
    std::vector<AccessPointInfo> ap_info_list;

    if (wifi_status->wifi_device != NULL)
    {
        const GPtrArray *aps = nm_device_wifi_get_access_points(NM_DEVICE_WIFI(wifi_status->wifi_device));
        if (aps != NULL)
        {
            for (guint i = 0; i < aps->len; i++)
            {
                NMAccessPoint *ap = (NMAccessPoint *)g_ptr_array_index(aps, i);
                GBytes *ssid_bytes = nm_access_point_get_ssid(ap);
                if (ssid_bytes != NULL)
                {
                    gsize ssid_len;
                    const gchar *ssid_str = (const gchar *)g_bytes_get_data(ssid_bytes, &ssid_len);
                    std::string ssid(ssid_str, ssid_len);
                    g_bytes_unref(ssid_bytes);

                    // Kiểm tra xem SSID có khớp với mục tiêu không
                    if (ssid == target_ssid)
                    {
                        AccessPointInfo ap_info;

                        ap_info.ssid = ssid;
                        ap_info.bssid = nm_access_point_get_bssid(ap);
                        ap_info.rssi = nm_access_point_get_strength(ap);
                        ap_info.frequency = nm_access_point_get_frequency(ap);
                        ap_info.band = get_band(nm_access_point_get_frequency(ap));
                        ap_info.security = get_wifi_security_type(ap);

                        ap_info_list.push_back(ap_info);
                    }
                }
            }
        }
    }

    return ap_info_list;
}
bool init_wifi_device(WifiStatus *wifi_status)
{
    GError *error = NULL;
    NMClient *client = NULL;
    client = nm_client_new(NULL, &error);
    if (!client)
    {
        g_print("Could not connect to NetworkManager: %s\n", error->message);
        g_error_free(error);
        return false;
    }
    wifi_status->client = client;

    const GPtrArray *devices = nm_client_get_devices(client);
    if (devices == NULL)
    {
        g_print("Could not get devices: %s\n", error->message);
        g_error_free(error);
        return false;
    }

    for (guint i = 0; i < devices->len; i++)
    {
        NMDevice *device = (NMDevice *)g_ptr_array_index(devices, i);
        if (NM_IS_DEVICE_WIFI(device))
        {
            wifi_status->wifi_device = device;
            break;
        }
    }
    if (wifi_status->wifi_device == NULL)
    {
        g_print("Could not find WiFi device\n");
        return false;
    }
    return true;
}
std::string getWifiInfoAsString(const AccessPointInfo &ap_info)
{
    std::ostringstream oss;
    oss << "SSID: " << ap_info.ssid << "\n"
        << "BSSID: " << ap_info.bssid << "\n"
        << "RSSI: " << ap_info.rssi << "/100\n"
        << "Frequency: " << ap_info.frequency << " MHz\n"
        << "Band: " << ap_info.band << " GHz\n"
        << "Security: " << ap_info.security << "\n\n";

    return oss.str();
}
std::string scan_info_wifi(const std::string& ssid){
	std::shared_ptr<spdlog::logger> _logger;
	_logger = spdlog::get("DET_logger");
	WifiStatus wifi_status;
	wifi_status.wifi_enabled = init_wifi_device(&wifi_status);
	wifi_status.attempts = 0;
	std::string target_ssid = ssid;
	std::string wifiInfoString;

	if (wifi_status.wifi_enabled)
	{
		if (scan_wifi(&wifi_status))
		{
			std::vector<AccessPointInfo> ap_info_list = get_scan_info(&wifi_status, target_ssid);
			for (const auto &ap_info : ap_info_list)
			{
				wifiInfoString += getWifiInfoAsString(ap_info);
			}
		}
		else
		{
			_logger->error("Scan failed\n");
		}
	}
	_logger->info(wifiInfoString);
//	std::cout << wifiInfoString;
	return wifiInfoString;
}
/*---------------------------------------------Scan_wifi--------------------------------------------------------*/

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

void creat_wifi_voyance(sockpp::tcp_socket& clientSocket) {
	std::shared_ptr<spdlog::logger> _logger;
	_logger = spdlog::get("DET_logger");
	const char* command = "nmcli con add type wifi ifname wlo1 mode ap con-name WIFI_AP ssid voyance && \
						   nmcli con modify WIFI_AP 802-11-wireless.band bg && \
						   nmcli con modify WIFI_AP 802-11-wireless.channel 1 && \
						   nmcli con modify WIFI_AP 802-11-wireless-security.key-mgmt wpa-psk && \
						   nmcli con modify WIFI_AP 802-11-wireless-security.proto rsn && \
						   nmcli con modify WIFI_AP 802-11-wireless-security.group ccmp && \
						   nmcli con modify WIFI_AP 802-11-wireless-security.pairwise ccmp && \
						   nmcli con modify WIFI_AP 802-11-wireless-security.psk 123456789 && \
						   nmcli con modify WIFI_AP ipv4.method shared && \
						   nmcli con up WIFI_AP";

	//Execute system commands
	int result = std::system(command);
	if (result == 0) {
		_logger->info("Command executed successfully.");
		std::string message = "Switched to Access Point.";
		_logger->info("Switched to Access Point.");
		clientSocket.write_n(message.c_str(), message.size());
	} else {
		_logger->info("Command execution failed.");
	}
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
		if (strstr(error->message, "SSID not found") != nullptr || strstr(error->message, "Invalid key") != nullptr)
		{
			_logger->error("Connection failed: Invalid SSID or password");
		}
		else
		{
			_logger->error("Error adding and activating connection: {}", error->message);
		}
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
    // Read the contents of a JSON file into a string
    std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    // Đóng tệp
    file.close();
    // Parse the JSON string to create a json_object
    json_object *j = json_tokener_parse(jsonContent.c_str());

    if (j != nullptr) {
    	logger->info("JSON Content: {}", json_object_to_json_string_ext(j, JSON_C_TO_STRING_PRETTY));
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
    // scan_wifi
    std::string info_wifi = scan_info_wifi(ssid);
}
void Settings::receive_processJson(sockpp::tcp_socket& clientSocket) {
    std::shared_ptr<spdlog::logger> logger;
    logger = spdlog::get("DET_logger");
	std::string cmdID;
	const int bufferSize = 1024;
	char buffer[bufferSize];
	// Receive JSON data from client
	ssize_t bytesRead = clientSocket.read(buffer, bufferSize);
	if (bytesRead < 0) {
		spdlog::error("Error receiving data from client.");
		return;
	}
	//Parse JSON data
	struct json_object* j = json_tokener_parse(buffer);
	if (j == nullptr) {
		 spdlog::error("Error parsing JSON data received from client.");
		return;
	}
	// Displays the received JSON value
	logger->info("Received JSON data:");
	logger->info("{}",json_object_to_json_string_ext(j, JSON_C_TO_STRING_PRETTY));
	//Save JSON data to file
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
	std::string wl_mode;
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
				wl_mode = json_object_get_string(val);
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

	// Send a confirmation message to the client
	const char* confirmationMsg = "Server recevied data sucessfuly";
	clientSocket.write(confirmationMsg, strlen(confirmationMsg));
	logger->info("wl mode: {}",wl_mode);
	if(wl_mode == "station") {
			// add wifi
		add_wifi(ssid,password);

		// get ip address
		struct in_addr currentIP = get_IP();
		char ipString[INET_ADDRSTRLEN];
		if (inet_ntop(AF_INET, &currentIP, ipString, INET_ADDRSTRLEN) == NULL) {
		   perror("inet_ntop");
		}
		logger->info("Current IP address: {}",ipString);
		// Send IP address to client
		clientSocket.write(ipString, strlen(ipString));
		// set ip
		set_IP(ip);
		// Send notification of ip change successfully
		const char *ipAddress = ip.c_str();
		std::string confirmMessage = "IP address changed successfully to " + std::string(ipAddress);
		const char *confirm_ip = confirmMessage.c_str();
		clientSocket.write(confirm_ip, strlen(confirm_ip));
	    // scan_wifi
	    std::string info_wifi = scan_info_wifi(ssid);
	    const char *wifi_info = info_wifi.c_str();
	    clientSocket.write(wifi_info, strlen(wifi_info));
	} else {
		creat_wifi_voyance(clientSocket);
	}
}

