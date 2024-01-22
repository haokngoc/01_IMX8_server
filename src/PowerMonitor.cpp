#include "PowerMonitor.h"
// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
// C library headers
#include <stdio.h>
#include <string.h>
// Khởi tạo instance thành nullptr
PowerMonitor* PowerMonitor::instance = nullptr;

PowerMonitor::PowerMonitor(){
	this->battery_vol[BAT01] = 0;
	this->battery_vol[BAT02] = 0;

	this->battery_ampere[BAT01] = 0;
	this->battery_ampere[BAT02] = 0;

	this->battery_avg_ampere[BAT01] = 0;
	this->battery_avg_ampere[BAT02] = 0;

	this->absolute_state_charge[BAT01] = 0;
	this->absolute_state_charge[BAT02] = 0;

	this->capacity_remain[BAT01] = 0;
	this->capacity_remain[BAT02] = 0;

	this->capacity_full_charge[BAT01] = 0;
	this->capacity_full_charge[BAT02] = 0;

	this->charging_current[BAT01] = 0;
	this->charging_current[BAT02] = 0;

	this->battery_status[BAT01] = 0;
	this->battery_status[BAT02] = 0;

	this->cycle_count[BAT01] = 0;
	this->cycle_count[BAT02] = 0;
	this->_logger = spdlog::get("DET_logger");
}

PowerMonitor::~PowerMonitor(){
}



unsigned char PowerMonitor::CRC8(const unsigned char* vptr, int len) {
  const unsigned char *data = vptr;
  unsigned crc = 0;
  int i, j;
  for (j = len; j; j--, data++) {
    crc ^= (*data << 8);
    for(i = 8; i; i--) {
      if (crc & 0x8000)
        crc ^= (0x1070 << 3);
      crc <<= 1;
    }
  }
  return (unsigned char)(crc >> 8);
}

int PowerMonitor::uart_get_data(unsigned char ID, unsigned char* data, unsigned char data_length){
	 unsigned char msg[4] ;
	 unsigned char read_buf [16];
	 unsigned char crc8_tmp;
	 int recv_length;
	 memset(&read_buf, '\0', sizeof(read_buf));

	 msg[0] = SEND_HEADER_MARKER;
	 msg[1] = ID;
	 msg[2] = this->CRC8(msg,2);
	 msg[3] = TAIL_MARKER;
	 write(this->serial_port, msg, sizeof(msg));
	 int num_bytes = read(serial_port, &read_buf, sizeof(read_buf));
	 if(num_bytes == 0)
		  return -1;
	 crc8_tmp = CRC8(read_buf,num_bytes-2);
	 if(crc8_tmp != read_buf[num_bytes-1] && read_buf[0] != RECV_HEADER_MARKER && read_buf[num_bytes-1] != TAIL_MARKER && ID !=read_buf[1] )
		  return -1;
	 recv_length = read_buf[2];
	 if (data_length < recv_length)
		  return -2;
	 memcpy(data,read_buf+3,recv_length);
	 return recv_length;
}

int PowerMonitor::uart_set_data(unsigned char ID, unsigned char* data, unsigned char data_length){
	 unsigned char msg_length = data_length + 4; // 1 byte for Header, 1 byte ID, 1 byte msg length, 1 byte Tailer
	 unsigned char msg[msg_length] ;
	 unsigned char read_buf [16];
	 unsigned char crc8_tmp;
//	 int recv_length;
	 memset(&read_buf, '\0', sizeof(read_buf));

	 msg[0] = SEND_HEADER_MARKER;
	 msg[1] = ID;
	 msg[2] = msg_length;
	 memcpy(msg + 3, data, data_length);
	 msg[msg_length-2] = CRC8(msg,msg_length-2);
	 msg[msg_length-1] = TAIL_MARKER;
	 write(this->serial_port, msg, sizeof(msg));

	 int num_bytes = read(serial_port, &read_buf, sizeof(read_buf));
	 if(num_bytes == 0)
		  return -1;

	 crc8_tmp = this->CRC8(read_buf,num_bytes-2);
	 if(crc8_tmp != read_buf[num_bytes-1] && read_buf[0] != RECV_HEADER_MARKER && read_buf[num_bytes-1] != TAIL_MARKER && ID !=read_buf[1] )
			  return -1;
	return 0;
}
int PowerMonitor::uart_close(){
	  close(this->serial_port);
	  return 0;
}


int PowerMonitor::uart_init(){
	 this->serial_port = open("/dev/ttymxc0", O_RDWR);

	  // Create new termios struct, we call it 'tty' for convention
	  struct termios tty;

	  // Read in existing settings, and handle any error
	  if(tcgetattr(this->serial_port, &tty) != 0) {
		  this->_logger->error("Error {} from tcgetattr: {}",errno,strerror(errno));
//	      printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
	      return -1;
	  }

	  tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
	  tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
	  tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size
	  tty.c_cflag |= CS8; // 8 bits per byte (most common)
	  tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
	  tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

	  tty.c_lflag &= ~ICANON;
	  tty.c_lflag &= ~ECHO; // Disable echo
	  tty.c_lflag &= ~ECHOE; // Disable erasure
	  tty.c_lflag &= ~ECHONL; // Disable new-line echo
	  tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
	  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	  tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

	  tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	  tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
	  // tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
	  // tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

	  tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
	  tty.c_cc[VMIN] = 0;

	  // Set in/out baud rate to be 115200
	  cfsetispeed(&tty, B115200);
	  cfsetospeed(&tty, B115200);

	  // Save tty settings, also checking for error
	  if (tcsetattr(this->serial_port, TCSANOW, &tty) != 0) {
		  this->_logger->error("Error {} from tcgetattr: {}",errno,strerror(errno));
//	      printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
	      return -1;
	  }
	  return 0;
}


int PowerMonitor::uart_get_bat_vol(){
	int ret = 0;
	unsigned char data[4];
	unsigned char data_length = 4;
	ret = this->uart_get_data(BATTERY_VOLTAGE, data, data_length);
	if (ret < 0 )
		return ret;
	this->battery_vol[BAT01] = data[1]*256 + data[0];
	this->battery_vol[BAT02] = data[3]*256 + data[2];
    return ret;
}


int PowerMonitor::uart_get_bat_ampere(){
	int ret = 0;
	unsigned char data[4];
	unsigned char data_length = 4;
	ret = this->uart_get_data(BATTERY_CURRENT, data, data_length);
	if (ret < 0 )
		return ret;
	this->battery_ampere[BAT01] = data[1]*256 + data[0];
	this->battery_ampere[BAT02] = data[3]*256 + data[2];
    return ret;
}



int PowerMonitor::uart_get_avg_ampere(){
	int ret = 0;
	unsigned char data[4];
	unsigned char data_length = 4;
	ret = this->uart_get_data(BATTERY_AVG_CURRENT, data, data_length);
	if (ret < 0 )
		return ret;
	this->battery_avg_ampere[BAT01] = data[1]*256 + data[0];
	this->battery_avg_ampere[BAT02] = data[3]*256 + data[2];
    return ret;
}



int PowerMonitor::uart_get_absolute_state_charge(){
	int ret = 0;
	unsigned char data[4];
	unsigned char data_length = 4;
	ret = this->uart_get_data(BATTERY_ABSOLUTE_STATE_OF_CHARGE, data, data_length);
	if (ret < 0 )
		return ret;
	this->absolute_state_charge[BAT01] = data[1]*256 + data[0];
	this->absolute_state_charge[BAT02] = data[3]*256 + data[2];
    return ret;
}



int PowerMonitor::uart_get_capacity_remain(){
	int ret = 0;
	unsigned char data[4];
	unsigned char data_length = 4;
	ret = this->uart_get_data(BATTERY_FULL_REMAIN_CAP, data, data_length);
	if (ret < 0 )
		return ret;
	this->capacity_remain[BAT01] = data[1]*256 + data[0];
	this->capacity_remain[BAT02] = data[3]*256 + data[2];
    return ret;
}



int PowerMonitor::uart_get_capacity_full_charge(){
	int ret = 0;
	unsigned char data[4];
	unsigned char data_length = 4;
	ret = this->uart_get_data(BAT_FULL_CHARGE_CAP, data, data_length);
	if (ret < 0 )
		return ret;
	this->capacity_full_charge[BAT01] = data[1]*256 + data[0];
	this->capacity_full_charge[BAT02] = data[3]*256 + data[2];
    return ret;
}


int PowerMonitor::uart_get_charging_current(){
	int ret = 0;
	unsigned char data[4];
	unsigned char data_length = 4;
	ret = this->uart_get_data(BAT_CHARGE_CURRENT, data, data_length);
	if (ret < 0 )
		return ret;
	this->charging_current[BAT01] = data[1]*256 + data[0];
	this->charging_current[BAT02] = data[3]*256 + data[2];
    return ret;
}



int PowerMonitor::uart_get_battery_status(){
	int ret = 0;
	unsigned char data[4];
	unsigned char data_length = 4;
	ret = this->uart_get_data(BAT_STATUS, data, data_length);
	if (ret < 0 )
		return ret;
	this->battery_status[BAT01] = data[1]*256 + data[0];
	this->battery_status[BAT02] = data[3]*256 + data[2];
    return ret;
}

int PowerMonitor::uart_get_cycle_count(){
	int ret = 0;
	unsigned char data[4];
	unsigned char data_length = 4;
	ret = this->uart_get_data(BAT_CYCLE_COUNT, data, data_length);
	if (ret < 0 )
		return ret;
	this->cycle_count[BAT01] = data[1]*256 + data[0];
	this->cycle_count[BAT02] = data[3]*256 + data[2];
    return ret;
}


int PowerMonitor::BAT_get_serial_number(unsigned char* serial_number){
	int ret = 0;
	unsigned char data[4];
	unsigned char data_length = 4;
	ret = this->uart_get_data(BAT_CYCLE_COUNT, data, data_length);
	if (ret < 0 )
		return ret;
	memcpy(serial_number, data, sizeof(data));
    return ret;
}



int PowerMonitor::BAT01_get_battery_vol() const {
    return battery_vol[BAT01];
}

int PowerMonitor::BAT01_get_battery_ampere() const {
    return battery_ampere[BAT01];
}

int PowerMonitor::BAT01_get_avg_ampere() const {
    return battery_avg_ampere[BAT01];
}

int PowerMonitor::BAT01_get_capacity_remain() const {
    return capacity_remain[BAT01];
}


int PowerMonitor::BAT02_get_battery_vol() const {
    return battery_vol[BAT02];
}

int PowerMonitor::BAT02_get_battery_ampere() const {
    return battery_ampere[BAT02];
}

int PowerMonitor::BAT02_get_avg_ampere() const {
    return battery_avg_ampere[BAT02];
}

int PowerMonitor::BAT02_get_capacity_remain() const {
    return capacity_remain[BAT02];
}
PowerMonitor* PowerMonitor::getInstance() {
    if (instance == nullptr) {
        instance = new PowerMonitor();
    }
    return instance;
}
void PowerMonitor::destroyInstance() {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
}



