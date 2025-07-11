#ifndef _MODBUSTCP_SLAVE_H_
#define _MODBUSTCP_SLAVE_H_

/*=====================================
*项目创建：modbustcp 客户端开发框架
*创建时间：2020年08月07日
*作者：广东捷胜电气技术有限公司-王晓坤
======================================*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <termios.h>

/*该结构体每描述一个客户端的相关属性*/
typedef struct modbustcp_client_struct{
	int client_fd;  //成功创建客户端后的文件描述符//
	struct sockaddr_in client_sock;  //记录客户端信息的sock结构体//
	struct sockaddr_in slave_sock;  //记录远端服务器信息的sock结构体//
	
	unsigned int link_time;  //连接请求超时时间，单位us//
	unsigned int link_count;  //连接失败重试次数//
	unsigned int response_time;  //等待服务器应答时间，单位us//
	unsigned int delay_time;  //通信延时时间，单位us//
	unsigned int sweep_time;  //周期扫描服务器时间，单位us//
	
	unsigned short data_count;  //数据包序号累加器//
	unsigned short SlaveID;  //客户端连接的服务器ID//
}CLIENT_T;
/*该结构体专为函数Create_modbustcpClient提供传入参数*/
typedef struct link_input_struct{
	unsigned short SlaveID;  //客户端连接的服务器ID//
	char *Slave_addr;  //远端服务器的ipv4地址//
	unsigned short Slave_id;  //远端服务器的网络端口//
	
	char *Net_path;  //客户端使用的联网网卡设备//
	unsigned int link_time;  //链接服务器超时时间//
	unsigned int link_count;
}CREATE_IN;

/*=====================================================
*函数Create_modbustcpClient用来创建一个客户端并连接到
*远端服务器。
*传入参数：
*CREATE_IN = 包含创建客户端并连接远端服务器的必要信息
*返回参数:
*NULL == 创建客户端失败
**CLIENT_T = 返回客户端描述对象
=====================================================*/
CLIENT_T *Create_modbustcpClient(CREATE_IN *client_per);

/*======================================================
*函数Free_modbustcpClient用来断开客户端与其服务器的链接
*，并释放客户端描述对象的资源
*传入参数：
*client_t = 有效的客户端描述对象
*返回参数:
*-1 = 传入的参数非法或为NULL
*
*0 = 成功关闭连接并释放了客户端资源
=======================================================*/
int Free_modbustcpClient(CLIENT_T *client_t);

/*============================================================
*以下函数接口可以按照格式自动发送功能码，读取远端服务器
*的数据。
*传入参数：
*client_t = 使用那个客户端来读取，有效的客户端对象
*Start_addr = 读取数据的起始地址，单位与读取的数据类型一致
*Read_length = 连续读取的数据长度，单位与读取的数据类型一致
*buffer = 用于装载读取到的数据
*DataMode = 选择请求的数据类型；0：线圈 1：寄存器
*返回参数：
*-1 = 传入参数非法或为NULL
*-2 = 传入参数中，buffer为NULL
*-3 = 系统分配数据内存空间失败
*-4 = 客户端与服务器断开了链接
*-5 = 等待服务器应答超时
*-6 = 监听服务器应答出现错误
*-7 = 接收到的数据错误
*-8 = 非目标服务器的回复
*-9 = 服务器回复的数据功能码异常
*0 = 正确的获取到远程服务器的数据了
*1 = 事件处理标识校验错误，只做提示
==============================================================*/
typedef struct read_data_struct{
	unsigned int Start_addr;
	unsigned int Read_length;
	void *buffer;
}READ_DATA_T;

/*
- Readdata_mode_bits():

  - 功能码: 0x01 (读取线圈)
  - 数据类型: 1位布尔值
  - 字节序: 不适用
  - 地址单位: 位地址

- Readdata_mode_bytes():

  - 功能码: 0x03 (读取保持寄存器)
  - 数据类型: 8位无符号字节
  - 字节序: 大端序
  - 地址单位: 字节地址(需转换为字地址)

- Readdata_mode_ints():

  - 功能码: 0x03 (读取保持寄存器)
  - 数据类型: 16位有符号整数
  - 字节序: 大端序
  - 地址单位: 字地址

- Readdata_mode_dints():

  - 功能码: 0x03 (读取保持寄存器)
  - 数据类型: 32位有符号整数(头文件明确注明)
  - 字节序: 大端序
  - 地址单位: 双字地址(需转换为字地址)
*/


int Readdata_mode_bits(CLIENT_T *client_t,READ_DATA_T *read_data_t);
int Readdata_mode_bytes(CLIENT_T *client_t,READ_DATA_T *read_data_t,const unsigned char DataMode);
int Readdata_mode_ints(CLIENT_T *client_t,READ_DATA_T *read_data_t);
int Readdata_mode_dints(CLIENT_T *client_t,READ_DATA_T *read_data_t);	

/*==================================================================
*以下的函数接口可以按照格式自动发送功能码，并将数据写入到远端服务器
*传入参数：
*client_t = 使用那个客户端来访问服务器，有效的客户端对象
*Start_addr = 写入数据的起始地址，单位与写入的数据类型一致
*Read_length = 连续写入的数据长度，单位与写入的数据类型一致
*buffer = 用于装载想要写到服务器的数据
*DataMode = 选择请求的数据类型；0：线圈 1：寄存器
*返回参数：
*-1 = 传入参数非法或为NULL
*-2 = 传入参数中，buffer为NULL
*-3 = 系统分配数据内存空间失败
*-4 = 客户端与服务器断开了链接
*-5 = 等待服务器应答超时
*-6 = 监听服务器应答出现错误
*-7 = 接收到的数据错误
*-8 = 非目标服务器的回复
*-9 = 服务器回复的数据功能码异常
*-10 = 指令的关键信息校验错误
*1 = 事件处理标识校验错误，只做提示
===================================================================*/
typedef struct write_data_struct{
	unsigned int Start_addr;
	unsigned int Write_length;
	void *buffer;
}WRITE_DATA_T;
int Writedata_mode_bits(CLIENT_T *client_t,const WRITE_DATA_T *write_data_t);
int Writedata_mode_bytes(CLIENT_T *client_t,const WRITE_DATA_T *write_data_t,const unsigned char DataMode);
int Writedata_mode_ints(CLIENT_T *client_t,const WRITE_DATA_T *write_data_t);
int Writedata_mode_dints(CLIENT_T *client_t,const WRITE_DATA_T *write_data_t);

#endif