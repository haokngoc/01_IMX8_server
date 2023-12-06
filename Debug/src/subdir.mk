################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/acceptor.cpp \
../src/connector.cpp \
../src/exception.cpp \
../src/inet_address.cpp \
../src/main.cpp \
../src/result.cpp \
../src/socket.cpp \
../src/stream_socket.cpp 

OBJS += \
./src/acceptor.o \
./src/connector.o \
./src/exception.o \
./src/inet_address.o \
./src/main.o \
./src/result.o \
./src/socket.o \
./src/stream_socket.o 

CPP_DEPS += \
./src/acceptor.d \
./src/connector.d \
./src/exception.d \
./src/inet_address.d \
./src/main.d \
./src/result.d \
./src/socket.d \
./src/stream_socket.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/hk/eclipse-workspace/01_IMX8_Server_x86/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


