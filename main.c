// 网页相关头文件导入
#include "./Webserver/Code/html_main.h"
#include "./Webserver/Code/html_base.h"
#include "./Webserver/Code/html_tool.h"


// PTZ相关头文件导入
#include "../onvif/PTZBinding.nsmap"
#include "./Onvif/Code/myptz.h"
#include "./Onvif/Code/camera_manager.h"

//Modbus相关头文件导入
#include "./ModbusTCP/Code/modbusTcp_master.h"

//主程序相关头文件导入
#include <stdio.h>
#include <pthread.h>

//脚本程序相关头文件导入
#include "./Command/Code/Command.h"



// 任务1:网页服务的函数
void* task_webserver(void* arg) {
	// 默认端口号
	int port = SERVER_PORT;
	// 注册信号处理
	signal(SIGSEGV, handle_signals);
	signal(SIGABRT, handle_signals);
	signal(SIGTERM, handle_signals);
	signal(SIGPIPE, SIG_IGN);  // 忽略SIGPIPE信号
	static int request_count = 0;  // 请求计数器



	// 初始化服务器套接字
	int MessageSock;
	// Register signal handlers
	signal(SIGINT, handle_signals);
	signal(SIGSEGV, handle_signals);  // 捕获段错误
	signal(SIGABRT, handle_signals);  // 捕获异常终止
	struct sockaddr_in ClientAddr;
	int rval, Length;
	char revbuf[BUF_SIZE];

	
	ServerSock = Server_Socket_Init(port);
	

	while (keep_running) {
		/* 启动监听 */
		rval = listen(ServerSock, BACKLOG);
		if (rval < 0) {
			printf("Failed to listen socket!\n");
			exit(1);
		}
		printf("Listening the socket on port %d...\n", port);

		/* 接受客户端请求建立连接 */
		Length = sizeof(struct sockaddr);
		MessageSock = accept(ServerSock, (struct sockaddr*)&ClientAddr, (socklen_t*)&Length);
		if (MessageSock < 0) {
			printf("Failed to accept connection from client!\n");
			exit(1);
		}
		request_count++;

		/* 接收客户端请求数据 - 使用select实现带超时的recv */
		memset(revbuf, 0, BUF_SIZE);
		fd_set readfds;
		struct timeval timeout;
		timeout.tv_sec = 2;  // 2秒超时
		timeout.tv_usec = 0;
		
		FD_ZERO(&readfds);
		FD_SET(MessageSock, &readfds);
		
		rval = select(MessageSock+1, &readfds, NULL, NULL, &timeout);
		if(rval > 0) {
			rval = recv(MessageSock, revbuf, BUF_SIZE - 1, 0);
		} else if(rval == 0) {
			printf("Receive timeout occurred\n");
			close(MessageSock);
			continue;
		} else {
			printf("select error: %s\n", strerror(errno));
			close(MessageSock);
			continue;
		}
		
		if (rval <= 0) {
			if (rval == 0) {
				printf("Client closed connection\n");
			} else {
				printf("Failed to receive request: %s\n", strerror(errno));
			}
			close(MessageSock);
			continue;
		}
		revbuf[rval] = '\0'; // 确保字符串正确终止
		//printf("%s\n", revbuf);	//输出请求数据内容
		rval = Handle_Request_Message(revbuf, MessageSock);

		close(MessageSock);
	}

	close(ServerSock);	//关闭套接字

    return NULL;
}




// 任务2:onvif_PTZ的函数
void* task_onvif(void* arg) 
{


    if(camera_init() == -1)
    {
        printf("camera_init_faild!\r\n");
    }




    // while(1)
    // {
    //     // // 移动所有摄像头到预置位3
    //     // for (int i = 0; i < camera_count; i++) {
    //     //     move_camera_to_preset(cameras, i, "3");
    //     // }
    //     // sleep(3);
    //     // printf("\r\n");

    //     //在这里编写PTZ的主线程

    // }




    // 清理资源
    //free_cameras(cameras, current_camera_count);
    return NULL;
}


// 可修改的Modbus服务器参数
#define MODBUS_SERVER_IP "192.168.1.233"    // 改为本地测试
#define MODBUS_SERVER_PORT 502          // 标准Modbus端口
#define MODBUS_SLAVE_ID 1               // 预留修改
#define NET_INTERFACE "eth0"            // 预留修改

// 任务3:ModbusTCP的函数
void* task_modbusTCP(void* arg) {

    // int result = edit_network_script(
    // "./Command/Sh/ipset.sh",  // 脚本路径
    // 192, 168, 1, 200,           // IP地址
    // 255, 255, 255, 0,          // 子网掩码
    // 192, 168, 1, 1              // 网关
    // );
    
    // if (result != 0) {
    //     printf("脚本修改失败\n");
    //     return 1;
    // }

    // run_script_command("./Command/Sh/ipset.sh");




    // CREATE_IN client_params = {
    //     .SlaveID = MODBUS_SLAVE_ID,
    //     .Slave_addr = MODBUS_SERVER_IP,
    //     .Slave_id = MODBUS_SERVER_PORT,
    //     .Net_path = NET_INTERFACE,
    //     .link_time = 1000000,  // 1秒超时
    //     .link_count = 3         // 重试3次
    // };
    
    // CLIENT_T *client = Create_modbustcpClient(&client_params);
    // if (!client) {
    //     perror("Failed to create Modbus client");
    //     return NULL;
    // }
    
    // // 设置响应时间
    // client->response_time = 500000;  // 0.5秒响应超时
    
    // // 主循环
    // while (1) {
    //     int32_t registers[2];  // 16位寄存器值
    //     READ_DATA_T read_data = {
    //         .Start_addr = 0,  // 从寄存器1开始(Modbus地址从1开始)
    //         .Read_length = 2,  // 读取2个寄存器
    //         .buffer = registers
    //     };



        
    //     int ret = Readdata_mode_dints(client, &read_data);
    //     if (ret < 0) {
    //         printf("Error reading registers: %d\n", ret);
    //     } else {
    //         printf("Register values: %d, %d\n", 
    //                registers[0], registers[1]);
    //     }



        
    //     sleep(3);  // 3秒间隔
    //     printf("\r\n");
    // }
    
    // Free_modbustcpClient(client);
    return NULL;
}



int main() {
    pthread_t thread1, thread2;//,thread3;
    int ret;
    
    // 创建线程1
    ret = pthread_create(&thread1, NULL, task_webserver, NULL);
    if(ret != 0) {
        fprintf(stderr, "无法创建线程1: %d\n", ret);
        return 1;
    }
    
    // 创建线程2
    ret = pthread_create(&thread2, NULL, task_onvif, NULL);
    if(ret != 0) {
        fprintf(stderr, "无法创建线程2: %d\n", ret);
        return 1;
    }
    
    // 创建线程3
    // ret = pthread_create(&thread3, NULL, task_modbusTCP, NULL);
    // if(ret != 0) {
    //     fprintf(stderr, "无法创建线程3: %d\n", ret);
    //     return 1;
    // }
    
    // 等待所有线程完成
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    // pthread_join(thread3, NULL);
    
    printf("所有任务已完成\n");
    return 0;
}













/*ModbusTCP测试代码*/
// // 可修改的Modbus服务器参数
// #define MODBUS_SERVER_IP "192.168.1.233"    // 改为本地测试
// #define MODBUS_SERVER_PORT 502          // 标准Modbus端口
// #define MODBUS_SLAVE_ID 1               // 预留修改
// #define NET_INTERFACE "eth0"            // 预留修改

// int main() {
//     // 创建客户端
//     CREATE_IN client_params = {
//         .SlaveID = MODBUS_SLAVE_ID,
//         .Slave_addr = MODBUS_SERVER_IP,
//         .Slave_id = MODBUS_SERVER_PORT,
//         .Net_path = NET_INTERFACE,
//         .link_time = 1000000,  // 1秒超时
//         .link_count = 3         // 重试3次
//     };
    
//     CLIENT_T *client = Create_modbustcpClient(&client_params);
//     if (!client) {
//         perror("Failed to create Modbus client");
//         return 1;
//     }
    
//     // 设置响应时间
//     client->response_time = 500000;  // 0.5秒响应超时
    
//     // 主循环
//     while (1) {
//         int32_t registers[2];  // 16位寄存器值
//         READ_DATA_T read_data = {
//             .Start_addr = 0,  // 从寄存器1开始(Modbus地址从1开始)
//             .Read_length = 2,  // 读取2个寄存器
//             .buffer = registers
//         };



        
//         int ret = Readdata_mode_dints(client, &read_data);
//         if (ret < 0) {
//             printf("Error reading registers: %d\n", ret);
//         } else {
//             printf("Register values: %d, %d\n", 
//                    registers[0], registers[1]);
//         }



        
//         sleep(3);  // 3秒间隔
//         printf("\r\n");
//     }
    
//     Free_modbustcpClient(client);
//     return 0;
// }









/*PTZ预置位功能测试项*/
// /**
//  * @brief 主函数，演示PTZ相机操作
//  */
// int main(int argc, char** argv)
// {
//     char time_buf[32];

//     // 初始化相机
//     const char* ip_1 = (argc > 1) ? argv[1] : "192.168.1.50";
//     PTZCamera* cam_1 = ptz_camera_init(ip_1, "admin", "asdqwe123");
//     if (!cam_1) {
//         printf("Failed to initialize camera\n");
//         return 1;
//     }

//     const char* ip_2 = (argc > 1) ? argv[1] : "192.168.1.60";
//     PTZCamera* cam_2 = ptz_camera_init(ip_2, "admin", "asdqwe123");
//     if (!cam_2) {
//         printf("Failed to initialize camera\n");
//         return 1;
//     }

//     // const char* ip_3 = (argc > 1) ? argv[1] : "192.168.1.64";
//     // PTZCamera* cam_3 = ptz_camera_init(ip_3, "admin", "admin123");
//     // if (!cam_3) {
//     //     printf("Failed to initialize camera\n");
//     //     return 1;
//     // }

//     while(1)
//     {
//         get_current_time(time_buf, sizeof(time_buf));
//         printf("[%s] Start moving to preset\n", time_buf);
//         // 移动到预置位3（使用token）
//         if (ptz_camera_goto_preset(cam_1, "2") != 0) {
//             printf("Failed to move to preset 2\n");
//         }
//         get_current_time(time_buf, sizeof(time_buf));
//         printf("[%s] Successfully moved to Preset_1\n", time_buf);


//         get_current_time(time_buf, sizeof(time_buf));
//         printf("[%s] Start moving to preset\n", time_buf);
//         // 移动到预置位3（使用token）
//         if (ptz_camera_goto_preset(cam_2, "2") != 0) {
//             printf("Failed to move to preset 2\n");
//         }
//         get_current_time(time_buf, sizeof(time_buf));
//         printf("[%s] Successfully moved to Preset_2\n", time_buf);


//         // get_current_time(time_buf, sizeof(time_buf));
//         // printf("[%s] Start moving to preset\n", time_buf);
//         // // 移动到预置位3（使用token）
//         // if (ptz_camera_goto_preset(cam_3, "2") != 0) {
//         //     printf("Failed to move to preset 2\n");
//         // }
//         // get_current_time(time_buf, sizeof(time_buf));
//         // printf("[%s] Successfully moved to Preset_3\n", time_buf);


//         sleep(3);
//         printf("\r\n");

//         get_current_time(time_buf, sizeof(time_buf));
//         printf("[%s] Start moving to preset\n", time_buf);
//         // 移动到预置位3（使用token）
//         if (ptz_camera_goto_preset(cam_1, "3") != 0) {
//             printf("Failed to move to preset 3\n");
//         }
//         get_current_time(time_buf, sizeof(time_buf));
//         printf("[%s] Successfully moved to Preset_1\n", time_buf);


//         get_current_time(time_buf, sizeof(time_buf));
//         printf("[%s] Start moving to preset\n", time_buf);
//         // 移动到预置位3（使用token）
//         if (ptz_camera_goto_preset(cam_2, "3") != 0) {
//             printf("Failed to move to preset 3\n");
//         }
//         get_current_time(time_buf, sizeof(time_buf));
//         printf("[%s] Successfully moved to Preset_2\n", time_buf);


//         // get_current_time(time_buf, sizeof(time_buf));
//         // printf("[%s] Start moving to preset\n", time_buf);
//         // // 移动到预置位3（使用token）
//         // if (ptz_camera_goto_preset(cam_3, "3") != 0) {
//         //     printf("Failed to move to preset 3\n");
//         // }
//         // get_current_time(time_buf, sizeof(time_buf));
//         // printf("[%s] Successfully moved to Preset_3\n", time_buf);

//         sleep(3);
//         printf("\r\n");

//     }




//     // 清理资源
//     ptz_camera_cleanup(cam_1);

    
//     return 0;
// }

























/*网页功能测试项*/
// int main(int argc, char* argv[]) {
// 	// 默认端口号
// 	int port = SERVER_PORT;
// 	// 注册信号处理
// 	signal(SIGSEGV, handle_signals);
// 	signal(SIGABRT, handle_signals);
// 	signal(SIGTERM, handle_signals);
// 	signal(SIGPIPE, SIG_IGN);  // 忽略SIGPIPE信号
// 	static int request_count = 0;  // 请求计数器

// 	// 如果通过命令行传入端口号，则使用该端口号
// 	if (argc == 2) {
// 		port = atoi(argv[1]);
// 	}

// 	// 初始化服务器套接字
// 	int MessageSock;
// 	// Register signal handlers
// 	signal(SIGINT, handle_signals);
// 	signal(SIGSEGV, handle_signals);  // 捕获段错误
// 	signal(SIGABRT, handle_signals);  // 捕获异常终止
// 	struct sockaddr_in ClientAddr;
// 	int rval, Length;
// 	char revbuf[BUF_SIZE];

// 	Logo();
// 	printf("Web Server is starting on port %d...\n\n", port);
	
// 	ServerSock = Server_Socket_Init(port);
	
// 	printf("\n-----------------------------------------------------------\n");

// 	while (keep_running) {
// 		/* 启动监听 */
// 		rval = listen(ServerSock, BACKLOG);
// 		if (rval < 0) {
// 			printf("Failed to listen socket!\n");
// 			exit(1);
// 		}
// 		printf("Listening the socket on port %d...\n", port);

// 		/* 接受客户端请求建立连接 */
// 		Length = sizeof(struct sockaddr);
// 		MessageSock = accept(ServerSock, (struct sockaddr*)&ClientAddr, (socklen_t*)&Length);
// 		if (MessageSock < 0) {
// 			printf("Failed to accept connection from client!\n");
// 			exit(1);
// 		}
// 		request_count++;
// 		printf("[DEBUG] 请求#%d 来自 [%s:%d]\n", request_count, inet_ntoa(ClientAddr.sin_addr), ntohs(ClientAddr.sin_port));
//         printf("[DEBUG] 当前内存使用: %ldMB\n", get_memory_usage());

// 		/* 接收客户端请求数据 - 使用select实现带超时的recv */
// 		memset(revbuf, 0, BUF_SIZE);
// 		fd_set readfds;
// 		struct timeval timeout;
// 		timeout.tv_sec = 2;  // 2秒超时
// 		timeout.tv_usec = 0;
		
// 		FD_ZERO(&readfds);
// 		FD_SET(MessageSock, &readfds);
		
// 		rval = select(MessageSock+1, &readfds, NULL, NULL, &timeout);
// 		if(rval > 0) {
// 			rval = recv(MessageSock, revbuf, BUF_SIZE - 1, 0);
// 		} else if(rval == 0) {
// 			printf("Receive timeout occurred\n");
// 			close(MessageSock);
// 			continue;
// 		} else {
// 			printf("select error: %s\n", strerror(errno));
// 			close(MessageSock);
// 			continue;
// 		}
		
// 		if (rval <= 0) {
// 			if (rval == 0) {
// 				printf("Client closed connection\n");
// 			} else {
// 				printf("Failed to receive request: %s\n", strerror(errno));
// 			}
// 			close(MessageSock);
// 			continue;
// 		}
// 		revbuf[rval] = '\0'; // 确保字符串正确终止
// 		printf("%s\n", revbuf);	//输出请求数据内容
// 		rval = Handle_Request_Message(revbuf, MessageSock);

// 		close(MessageSock);
// 		printf("\n-----------------------------------------------------------\n");
// 	}

// 	close(ServerSock);	//关闭套接字

// 	return OK;
// }
