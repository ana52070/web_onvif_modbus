# 设置交叉编译工具链路径
TOOLCHAIN_DIR := /home/chuiyu/Lyra-sdk/prebuilts/gcc/linux-x86/arm/gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf/bin
CROSS_COMPILE := $(TOOLCHAIN_DIR)/arm-none-linux-gnueabihf-

CC            := $(CROSS_COMPILE)gcc 
CPP           := $(CROSS_COMPILE)g++
LD            := $(CROSS_COMPILE)ld
AR            := $(CROSS_COMPILE)ar
STRIP         := $(CROSS_COMPILE)strip

# 设置sysroot路径（工具链自带）
SYSROOT := $(TOOLCHAIN_DIR)/../arm-none-linux-gnueabihf/libc

# 临时文件目录
TEMPDIR := .obj/

# 设置交叉编译的OpenSSL路径（需要先编译）
OPENSSL_DIR := /home/chuiyu/arm_libs_build/arm_openssl

# 编译选项：移除调试符号(-g)和调试定义(-DDEBUG)
CFLAGS += -c -Wall -DWITH_DOM -DWITH_OPENSSL -fno-common
CFLAGS += --sysroot=$(SYSROOT)
CFLAGS += $(INCLUDE)

# 源文件
SOURCES_ONVIF += \
           ../../onvif/soapC.c                       \
           ../../onvif/soapClient.c                  \
           ../../onvif/stdsoap2.c                    \
           ../../onvif/wsaapi.c                      \
           ../../onvif/dom.c                         \
           ../../onvif/mecevp.c                      \
           ../../onvif/smdevp.c                      \
           ../../onvif/threads.c                     \
           ../../onvif/wsseapi.c                     \
           ../../onvif/struct_timeval.c 

SOURCES_MODBUS += \
           ../ModbusTCP/Code/modbusTcp_master.c

SOURCES_WEBSERVER += \
           ../Webserver/Code/html_base.c             \
           ../Webserver/Code/html_main.c             \
           ../Webserver/Code/html_tool.c

SOURCES_COMMAND += \
           ../Command/Code/Command.c             \

SOURCES += \
           ../main.c                                \
           ../Onvif/Code/myptz.c                    \
           ../Onvif/Code/camera_manager.c           

# 目标文件
OBJECTS_ONVIF := $(patsubst %.c,$(TEMPDIR)%.o,$(filter %.c, $(SOURCES_ONVIF)))
OBJECTS_MODBUS := $(patsubst %.c,$(TEMPDIR)%.o,$(filter %.c, $(SOURCES_MODBUS)))
OBJECTS_WEBSERVER := $(patsubst %.c,$(TEMPDIR)%.o,$(filter %.c, $(SOURCES_WEBSERVER)))
OBJECTS_COMMAND := $(patsubst %.c,$(TEMPDIR)%.o,$(filter %.c, $(SOURCES_COMMAND)))
OBJECTS := $(patsubst %.c,$(TEMPDIR)%.o,$(filter %.c, $(SOURCES)))

# 头文件路径
INCLUDE += -I../../onvif/                            \
           -I$(OPENSSL_DIR)/include                  \
           -I../ModbusTCP/Code                       \
           -I../Webserver/Code                       \
           -IOnvif/Code                              \
           -I/home/chuiyu/arm_libs_build/include     \
           -I.                                       \
           -I/home/chuiyu/gsoap-2.8/gsoap/bin/onvif  \

# 链接库 - 使用交叉编译的OpenSSL
LDLIBS += $(OPENSSL_DIR)/lib/libssl.a                \
          $(OPENSSL_DIR)/lib/libcrypto.a             \
          -ldl                                       \
          /home/chuiyu/arm_libs_build/lib/libcjson.a \

# 链接库(其他)
LDLIBS += -lpthread

%.o: %.cpp
	@echo "  CPP     " $@;
	@$(CPP) $(CFLAGS) -c -o $@ $<
 
%.o: %.c
	@echo "  CC      " $@;
	@$(CC) $(CFLAGS) -c -o $@ $<
 
.PHONY: all clean
