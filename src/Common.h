#ifndef COMMON_H
#define COMMON_H

//#define BUFFER_SIZE (2*3456*3180)
#define BUFFER_SIZE 21*1024*1024
#define BUFFER_CAL_MD5 50*1024
#define MARKER_HEAD 0xAA
#define MARKER_TAIL 0x55
#define DET_STATE_WORK 0x58
#define DET_STATE_SLEEP 0x1b
#define DET_STATE_CLOSE 0x2b
#define DET_STATE_TRIGGER 0x66
#define CHUNK_SIZE 1024*10
#define TIMEOUT_MICRO_SECONDS 100000000000

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"



#endif

