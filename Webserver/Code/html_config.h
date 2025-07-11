#ifndef __HTML_CONFIG_H__
#define __HTML_CONFIG_H__




/* 登录验证信息 */
#define VALID_USERNAME "admin"
#define VALID_PASSWORD "asdqwe123"


/* 服务器配置常量定义 */
#define SERVER_IP_ADDR "0.0.0.0"	// 服务器监听IP地址，0.0.0.0表示监听所有网络接口
#define SERVER_PORT 80		// 服务器默认监听端口号
#define BACKLOG 10			// 最大等待连接队列长度
#define BUF_SIZE 8192			// 缓冲区大小(8KB)
#define HTML_OK 1				// 操作成功返回值
#define HTML_ERROR 0				// 操作失败返回值

/* 文件路径配置 */
#define CONFIG_PATH "Webserver/Config/"
#define HTML_PATH "Webserver/HTML/"






#endif
