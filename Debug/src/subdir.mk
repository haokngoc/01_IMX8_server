################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/test.cc 

CPP_SRCS += \
../src/DET_Sate.cpp \
../src/Mgard300_Handler.cpp \
../src/PRB_IMG.cpp \
../src/PowerMonitor.cpp \
../src/acceptor.cpp \
../src/connector.cpp \
../src/exception.cpp \
../src/inet_address.cpp \
../src/main.cpp \
../src/result.cpp \
../src/socket.cpp \
../src/stream_socket.cpp 

CC_DEPS += \
./src/test.d 

OBJS += \
./src/DET_Sate.o \
./src/Mgard300_Handler.o \
./src/PRB_IMG.o \
./src/PowerMonitor.o \
./src/acceptor.o \
./src/connector.o \
./src/exception.o \
./src/inet_address.o \
./src/main.o \
./src/result.o \
./src/socket.o \
./src/stream_socket.o \
./src/test.o 

CPP_DEPS += \
./src/DET_Sate.d \
./src/Mgard300_Handler.d \
./src/PRB_IMG.d \
./src/PowerMonitor.d \
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
	g++ -DDEBUG -I"/home/hk/eclipse-workspace/01_IMX8_Server_x86/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.cc src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DDEBUG -I"/home/hk/eclipse-workspace/01_IMX8_Server_x86/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


