#include <string>
#pragma once

class IPHandler {
public:
    static struct in_addr getCurrentIP();
    static void setIP(const std::string& ip);

};
