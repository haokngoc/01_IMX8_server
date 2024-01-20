#include "ip_handler.h"
#include <iostream>
#include <fstream>
#include <json-c/json.h>
#include <thread>
#include "spdlog/sinks/rotating_file_sink.h"
#include "Settings.h"
#include <string>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>


struct in_addr IPHandler::getCurrentIP() {
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

void IPHandler::setIP(const std::string& ip) {
    // Implementation
	const char * ip_address = ip.c_str();
	struct ifreq ifr;
	const char *name = "wlo1";
	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

	strncpy(ifr.ifr_name, name, IFNAMSIZ);
	ifr.ifr_addr.sa_family = AF_INET;

	struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
	inet_pton(AF_INET, ip_address, &addr->sin_addr);

	ioctl(fd, SIOCSIFADDR, &ifr);

	inet_pton(AF_INET, "255.255.0.0", &addr->sin_addr);
	ioctl(fd, SIOCSIFNETMASK, &ifr);

	ioctl(fd, SIOCGIFFLAGS, &ifr);
	strncpy(ifr.ifr_name, name, IFNAMSIZ);
	ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);

	ioctl(fd, SIOCSIFFLAGS, &ifr);

	std::cout << "IP address changed successfully to: " << ip << std::endl;

	close(fd);
}
