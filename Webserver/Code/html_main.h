#ifndef __HTML_MAIN_H__
#define __HTML_MAIN_H__

#include "html_config.h"
#include "html_base.h"
#include "html_tool.h"
#include <stdio.h>		// 标准输入输出
#include <stdlib.h>		// 标准库函数
#include <string.h>		// 字符串操作
#include <unistd.h>		// POSIX系统调用
#include <sys/stat.h>		// 文件状态信息
#include <sys/socket.h>		// 套接字接口
#include <netinet/in.h>		// 互联网地址族
#include <arpa/inet.h>		// 互联网地址转换
#include <time.h>		// 时间处理
#include <signal.h>     // 信号处理
#include <sys/time.h>   // timeval结构体
#include <sys/types.h>  // 确保timeval定义完整
#include <errno.h>      // 错误号定义
#include <fcntl.h>      // 非阻塞socket设置
#include <sys/select.h> // select函数
//#include "cJSON.h"      // JSON处理
#include "../../../arm_libs_build/cJSON/include/cjson/cJSON.h"

/* 服务器标识信息 */
extern const char* Server_name;
/* Global variables for signal handling */
static volatile int keep_running = 1;
extern int ServerSock;  // Server socket file descriptor

/* 函数声明部分 */
int deal_move(char* get_url_data);
int deal_preset(char* get_url_data);
int deal_trackcfg(char* get_url_data);
int deal_ipcfg(char* get_url_data);
int Handle_Request_Message(char* message, int Socket) ;
const char* Post_Value(char* message);

#endif
