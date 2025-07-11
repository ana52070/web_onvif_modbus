#include "html_main.h"
#include "html_config.h"
#include "html_base.h"
#include "html_tool.h"


/**
 * 初始化并创建服务器套接字
 * @param port 服务器监听的端口号
 * @return 成功返回创建的套接字描述符，失败则退出程序
 * 
 * 函数流程：
 * 1. 创建TCP套接字
 * 2. 设置服务器地址信息
 * 3. 绑定套接字到指定端口
 */
int Server_Socket_Init(int port) {
	int ServerSock;                 // 服务器套接字描述符
	struct sockaddr_in ServerAddr;  // 服务器地址结构
	int rval;                       // 函数返回值

	/* 创建TCP套接字 */
	ServerSock = socket(AF_INET, SOCK_STREAM, 0);
	if (ServerSock < 0) {
		perror("Failed to create socket");
		exit(EXIT_FAILURE);
	}
	printf("Successfully created server socket (fd: %d)\n", ServerSock);

	/* 设置SO_REUSEADDR选项以允许端口重用 */
	int opt = 1;
	if (setsockopt(ServerSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		perror("Failed to set SO_REUSEADDR");
		close(ServerSock);
		exit(EXIT_FAILURE);
	}

	/* 配置服务器地址信息 */
	memset(&ServerAddr, 0, sizeof(ServerAddr));  // 清空地址结构
	ServerAddr.sin_family = AF_INET;             // 使用IPv4地址族
	ServerAddr.sin_port = htons(port);           // 设置端口号(网络字节序)
	ServerAddr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDR); // 设置监听IP

	/* 绑定套接字到指定端口 */
	rval = bind(ServerSock, (struct sockaddr*)&ServerAddr, sizeof(ServerAddr));
	if (rval < 0) {
		perror("Failed to bind socket");
		close(ServerSock);
		exit(EXIT_FAILURE);
	}
	printf("Successfully bound socket to port %d\n", port);

	return ServerSock;  // 返回已绑定的套接字描述符
}



/**
 * 判断HTTP请求方法是否有效
 * @param method 请求方法字符串(GET/POST等)
 * @param Socket 客户端套接字描述符(用于错误响应)
 * @return 有效返回方法字符串，无效返回ERROR并发送501错误响应
 * 
 * 支持的请求方法:
 * - GET: 获取资源
 * - POST: 提交数据
 * 其他方法将返回501 Not Implemented错误
 */
const char* Judge_Method(char* method, int Socket) {
	if (strcmp(method, "GET") == 0) {
		return "GET";  // GET方法有效
	}
	else if (strcmp(method, "POST") == 0) {
		return "POST"; // POST方法有效
	}
	else {
		Error_Request_Method(Socket); // 发送501错误响应
		return HTML_ERROR;   // 不支持的请求方法
	}
}



/**
 * 验证请求URI的有效性
 * @param URI 请求的URI路径
 * @param Socket 客户端套接字描述符(用于错误响应)
 * @return 有效返回OK，无效返回ERROR并发送404响应
 *
 * 函数处理流程：
 * 1. 调用Inquire_File检查请求的文件是否存在
 * 2. 如果文件不存在，调用File_not_Inquire发送404响应
 * 3. 如果文件存在，返回OK继续处理
 */
int Judge_URI(char* URI, int Socket) {
	if (Inquire_File(URI) == HTML_ERROR) {
		File_not_Inquire(Socket);  // 发送404 Not Found响应
		return HTML_ERROR;  // URI无效
	}
	return HTML_OK;  // URI有效
}


/**
 * 发送数据到客户端套接字
 * @param Socket 客户端套接字描述符
 * @param sendbuf 要发送的数据缓冲区
 * @param Length 要发送的数据长度
 * @return 成功返回OK，失败返回ERROR
 *
 * 函数特点：
 * 1. 实现可靠的分块发送机制，确保完整发送所有数据
 * 2. 处理部分发送的情况，自动继续发送剩余数据
 * 3. 返回实际发送的字节数
 */
int Send_Ifon(int Socket, const char* sendbuf, int Length) {
	int sendtotal = 0;    // 已发送字节数
	int bufleft = Length; // 剩余待发送字节数
	int rval = 0;         // 单次发送返回值

	// 循环发送直到所有数据发送完成或出错
	while (sendtotal < Length) {
		rval = send(Socket, sendbuf + sendtotal, bufleft, 0);
		if (rval < 0) {
			break;  // 发送出错
		}
		sendtotal += rval;  // 更新已发送字节数
		bufleft -= rval;    // 更新剩余字节数
	}

	Length = sendtotal;  // 更新实际发送长度

	return rval < 0 ? HTML_ERROR : HTML_OK;  // 返回发送结果
}



int Error_Request_Method(int Socket) {
	//501 Not Implemented响应 
	const char* Method_err_line = "HTTP/1.1 501 Not Implemented\r\n";
	const char* cur_time = "";
	const char* Method_err_type = "Content-type: text/plain\r\n";
	// 
	const char* Method_err_end = "\r\n";
	const char* Method_err_info = "The request method is not yet completed!\n";

	printf("The request method from client's request message is not yet completed!\n");

	if (Send_Ifon(Socket, Method_err_line, strlen(Method_err_line)) == HTML_ERROR) {
		printf("Sending method_error_line failed!\n");
		return HTML_ERROR;
	}

	if (Send_Ifon(Socket, Server_name, strlen(Server_name)) == HTML_ERROR) {
		printf("Sending Server_name failed!\n");
		return HTML_ERROR;
	}

	cur_time = Get_Data(cur_time);
	Send_Ifon(Socket, "Data: ", 6);
	if (Send_Ifon(Socket, cur_time, strlen(cur_time)) == HTML_ERROR) {
		printf("Sending cur_time error!\n");
		return HTML_ERROR;
	}

	if (Send_Ifon(Socket, Method_err_type, strlen(Method_err_type)) == HTML_ERROR) {
		printf("Sending method_error_type failed!\n");
		return HTML_ERROR;
	}

	if (Send_Ifon(Socket, Method_err_end, strlen(Method_err_end)) == HTML_ERROR) {
		printf("Sending method_error_end failed!\n");
		return HTML_ERROR;
	}

	if (Send_Ifon(Socket, Method_err_info, strlen(Method_err_info)) == HTML_ERROR) {
		printf("Sending method_error_info failed!\n");
		return HTML_ERROR;
	}

	return HTML_OK;
}



const char* Judge_File_Type(char* URI, const char* content_type) {
	//文件类型判断 
	const char* suffix;

	if ((suffix = strrchr(URI, '.')) != NULL)
		suffix = suffix + 1;

	if (strcmp(suffix, "html") == 0) {
		return "Content-type: text/html\r\n";
	}
	else if (strcmp(suffix, "jpg") == 0) {
		return "Content-type: image/jpg\r\n";
	}
	else if (strcmp(suffix, "png") == 0) {
		return "Content-type: image/png\r\n";
	}
	else if (strcmp(suffix, "gif") == 0) {
		return "Content-type: image/gif\r\n";
	}
	else if (strcmp(suffix, "txt") == 0) {
		return "Content-type: text/plain\r\n";
	}
	else if (strcmp(suffix, "xml") == 0) {
		return "Content-type: text/xml\r\n";
	}
	else if (strcmp(suffix, "rtf") == 0) {
		return "Content-type: text/rtf\r\n";
	}
	else if (strcmp(suffix, "js") == 0) {  // 添加对js文件的支持
		return "Content-type: application/javascript\r\n";
	}
	else if (strcmp(suffix, "css") == 0) {  // 添加对css文件的支持
		return "Content-type: text/css\r\n";
	}
	else if (strcmp(suffix, "json") == 0) {  // 添加对json文件的支持
		return "Content-type: application/json\r\n";
	}
	else if (strstr(URI, ".mp3") != NULL) {
		return "Content-Type: audio/mpeg\r\n";
	}
	else if (strcmp(suffix, "ico") == 0) {
    	return "Content-type: image/x-icon\r\n";
	}
	else
		return HTML_ERROR;
}


void handle_signals(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    if (ServerSock > 0) {
        close(ServerSock);
    }
    exit(EXIT_SUCCESS);
}

