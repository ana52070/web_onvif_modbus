#include "html_main.h"
#include "html_config.h"
#include "html_base.h"
#include "html_tool.h"






/**
 * @brief 解析 JSON 字符串并提取摄像头配置
 * @param config_str 输入的 JSON 字符串
 * @param camera_count 返回摄像头数量
 * @param cameras 返回摄像头配置数组（固定大小 5）
 * @return 0 成功，-1 失败
 */
int parse_ipset_config(const char *config_str, int *camera_count, CameraConfig cameras[5]) 
{
    cJSON *root = cJSON_Parse(config_str);
    if (!root) {
        fprintf(stderr, "JSON 解析失败: %s\n", cJSON_GetErrorPtr());
        return -1;
    }

    // 解析 camera_count
    cJSON *count_json = cJSON_GetObjectItem(root, "camera_count");
    if (!cJSON_IsNumber(count_json)) {
        fprintf(stderr, "无效的 camera_count\n");
        cJSON_Delete(root);
        return -1;
    }
    *camera_count = count_json->valueint;

    // 解析 cameras 数组
    cJSON *cameras_json = cJSON_GetObjectItem(root, "cameras");
    if (!cJSON_IsArray(cameras_json)) {
        fprintf(stderr, "无效的 cameras 数组\n");
        cJSON_Delete(root);
        return -1;
    }

    int num_cameras = cJSON_GetArraySize(cameras_json);
    if (num_cameras > 5) {
        fprintf(stderr, "摄像头数量超过最大值 5\n");
        num_cameras = 5;  // 限制最多 5 个
    }

    for (int i = 0; i < num_cameras; i++) {
        cJSON *camera_json = cJSON_GetArrayItem(cameras_json, i);
        if (!camera_json) continue;

        // 解析 IP 地址
        cJSON *ipa = cJSON_GetObjectItem(camera_json, "ipa");
        cJSON *ipb = cJSON_GetObjectItem(camera_json, "ipb");
        cJSON *ipc = cJSON_GetObjectItem(camera_json, "ipc");
        cJSON *ipd = cJSON_GetObjectItem(camera_json, "ipd");
        if (cJSON_IsNumber(ipa)) cameras[i].ipa = ipa->valueint;
        if (cJSON_IsNumber(ipb)) cameras[i].ipb = ipb->valueint;
        if (cJSON_IsNumber(ipc)) cameras[i].ipc = ipc->valueint;
        if (cJSON_IsNumber(ipd)) cameras[i].ipd = ipd->valueint;

        // 解析端口
        cJSON *port = cJSON_GetObjectItem(camera_json, "port");
        if (cJSON_IsNumber(port)) cameras[i].port = port->valueint;

        // 解析用户名和密码
        cJSON *user = cJSON_GetObjectItem(camera_json, "user");
        cJSON *pwd = cJSON_GetObjectItem(camera_json, "pwd");
        if (cJSON_IsString(user)) {
            strncpy(cameras[i].user, user->valuestring, sizeof(cameras[i].user) - 1);
            cameras[i].user[sizeof(cameras[i].user) - 1] = '\0';
        }
        if (cJSON_IsString(pwd)) {
            strncpy(cameras[i].pwd, pwd->valuestring, sizeof(cameras[i].pwd) - 1);
            cameras[i].pwd[sizeof(cameras[i].pwd) - 1] = '\0';
        }
    }

    cJSON_Delete(root);
    return 0;
}


/**
 * @brief 读取JSON文件内容
 * @param path 文件路径
 * @param json_str 用于存储文件内容的指针
 * @return 成功返回OK，失败返回ERROR
 */
int read_json_file(const char* path, char** json_str) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        perror("Failed to open json file");
        return HTML_ERROR;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    *json_str = (char*)malloc(file_size + 1);
    if (!*json_str) {
        fclose(fp);
        return HTML_ERROR;
    }

    size_t read_size = fread(*json_str, 1, file_size, fp);
    (*json_str)[read_size] = '\0';

    if (ferror(fp)) {
        free(*json_str);
        *json_str = NULL;
        fclose(fp);
        return HTML_ERROR;
    }

    fclose(fp);
    return HTML_OK;
}

/**
 * @brief 分割URL字符串为两部分
 * 
 * @param input 输入字符串(原始URL)
 * @param before 输出?前的部分(调用者分配内存)
 * @param after 输出?后的部分(调用者分配内存)
 * @return int 成功返回0，失败返回-1
 */
int split_url(const char* input, char* before, char* after) 
{
    if (input == NULL || before == NULL || after == NULL) {
        return -1;
    }

    const char* qmark = strchr(input, '?');
    if (qmark == NULL) {
        // 如果没有找到?，整个字符串放入before，after为空
        strcpy(before, input);
        after[0] = '\0';
        return -1;
    }

    // 计算?前部分的长度
    size_t before_len = qmark - input;
    strncpy(before, input, before_len);
    before[before_len] = '\0';

    // ?后部分
    strcpy(after, qmark + 1);

    return 0;
}



// 安全写入文件函数
int safe_write_file(const char* filename, const char* content) {
    char tmpname[256];
    snprintf(tmpname, sizeof(tmpname), "%s.tmp", filename);
    
    FILE *fp = fopen(tmpname, "w");
    if(!fp) {
        perror("Failed to open temp file");
        return 0;
    }
    
    if(fputs(content, fp) == EOF) {
        perror("Failed to write content");
        fclose(fp);
        unlink(tmpname);
        return 0;
    }
    
    if(fclose(fp) != 0) {
        perror("Failed to close temp file");
        unlink(tmpname);
        return 0;
    }
    
    if(rename(tmpname, filename) != 0) {
        perror("Failed to rename temp file");
        unlink(tmpname);
        return 0;
    }
    
    return 1;
}


void log_request_details(int sockfd, const char* uri) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    if(getpeername(sockfd, (struct sockaddr*)&addr, &addr_len) == 0) {
        printf("[DEBUG] Request from %s:%d for %s\n", 
               inet_ntoa(addr.sin_addr), 
               ntohs(addr.sin_port),
               uri);
    } else {
        perror("Failed to get peer address");
    }
}


int Inquire_File(char* URI) {
	// 直接使用URI作为文件路径
	char abs_path[BUF_SIZE];
	strncpy(abs_path, URI, sizeof(abs_path)-1);
	abs_path[sizeof(abs_path)-1] = '\0';

	struct stat File_info;
	if (stat(abs_path, &File_info) == -1) {
		return HTML_ERROR;
	}
	else {
		return HTML_OK;
	}
}




int File_not_Inquire(int Socket) {
    const char* File_err_line = "HTTP/1.1 404 Not Found\r\n";
    const char* cur_time = "";
    const char* File_err_type = "Content-type: text/html\r\n";
    const char* File_err_end = "\r\n";

    FILE* file;
    struct stat file_stat;
    char sendbuf[BUF_SIZE];
    int send_length;

    // Get the current working directory
    char cwd[BUF_SIZE];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        return HTML_ERROR;
    }

    // Construct the absolute path to 404.html
    char abs_path[BUF_SIZE];
    size_t cwd_len = strlen(cwd);

    // 确保有足够空间容纳 "/404.html" 和终止符
    if (cwd_len < BUF_SIZE - 10) {
        // 安全情况下拼接完整路径
        memcpy(abs_path, cwd, cwd_len);
        memcpy(abs_path + cwd_len, "/404.html", 10); // 包含终止符
    } else {
        // 路径太长，使用默认路径
        const char *default_path = "/404.html";
        size_t default_len = strlen(default_path);
        if (default_len >= BUF_SIZE) {
            //这几乎不可能发生，除非 BUF_SIZE 非常小
            default_len = BUF_SIZE - 1;
        }
        memcpy(abs_path, default_path, default_len);
        abs_path[default_len] = '\0';
    }

    // Open and read 404.html
    file = fopen(abs_path, "rb");
    if (file != NULL) {
        fstat(fileno(file), &file_stat);

        // Send 404 Not Found response
        if (Send_Ifon(Socket, File_err_line, strlen(File_err_line)) == HTML_ERROR) {
            printf("Sending file_error_line error!\n");
            fclose(file);
            return HTML_ERROR;
        }

        if (Send_Ifon(Socket, Server_name, strlen(Server_name)) == HTML_ERROR) {
            printf("Sending Server_name failed!\n");
            fclose(file);
            return HTML_ERROR;
        }

        cur_time = Get_Data(cur_time);
        Send_Ifon(Socket, "Date: ", 6);
        if (Send_Ifon(Socket, cur_time, strlen(cur_time)) == HTML_ERROR) {
            printf("Sending cur_time error!\n");
            fclose(file);
            return HTML_ERROR;
        }

        if (Send_Ifon(Socket, File_err_type, strlen(File_err_type)) == HTML_ERROR) {
            printf("Sending file_error_type error!\n");
            fclose(file);
            return HTML_ERROR;
        }

        // Calculate and send Content-Length
        char content_length[BUF_SIZE];
        snprintf(content_length, sizeof(content_length), "Content-Length: %ld\r\n", file_stat.st_size);
        if (Send_Ifon(Socket, content_length, strlen(content_length)) == HTML_ERROR) {
            printf("Sending file_error_length error!\n");
            fclose(file);
            return HTML_ERROR;
        }

        // Send end of header
        if (Send_Ifon(Socket, File_err_end, strlen(File_err_end)) == HTML_ERROR) {
            printf("Sending file_error_end error!\n");
            fclose(file);
            return HTML_ERROR;
        }

        // Send 404.html content
        while ((send_length = fread(sendbuf, 1, BUF_SIZE, file)) > 0) {
            if (Send_Ifon(Socket, sendbuf, send_length) == HTML_ERROR) {
                printf("Sending 404.html content error!\n");
                break;
            }
        }

        fclose(file);
    } else {
        printf("Failed to open 404.html!\n");
        return HTML_ERROR;
    }

    return HTML_OK;
}


/**
 * 发送请求的文件内容到客户端
 * @param URI 请求的文件路径(相对路径)
 * @param Socket 客户端套接字描述符
 * @return 成功返回OK，失败返回ERROR
 *
 * 函数处理流程：
 * 1. 构建文件的绝对路径
 * 2. 判断文件类型并设置Content-Type
 * 3. 打开文件并获取文件信息
 * 4. 构建HTTP 200 OK响应头:
 *    - HTTP状态行
 *    - Server信息
 *    - Date头
 *    - Content-Type头
 *    - Content-Length头
 * 5. 分块发送文件内容(每次最多发送1024字节)
 */
int Send_File(char* URI, int Socket) {
	// 直接使用URI作为文件路径
	char abs_path[BUF_SIZE];
	strncpy(abs_path, URI, sizeof(abs_path)-1);
	abs_path[sizeof(abs_path)-1] = '\0';
	
	printf("[DEBUG] Trying to open file: %s\n", abs_path);

	// HTTP响应头字段
	const char* File_ok_line = "HTTP/1.1 200 OK\r\n";  // 状态行
	const char* cur_time = "";                         // 当前时间字符串
	const char* File_ok_type = "";                     // Content-Type头
	const char* File_ok_length = "Content-Length: ";   // Content-Length前缀
	const char* File_ok_end = "\r\n";                  // 头结束标记

	FILE* file;                   // 文件指针
	struct stat file_stat;        // 文件状态信息
	char Length[BUF_SIZE];        // 文件大小字符串
	char sendbuf[BUF_SIZE];       // 发送缓冲区
	int send_length;              // 实际发送长度

	// 判断文件类型
	if (Judge_File_Type(abs_path, File_ok_type) == HTML_ERROR) {
		printf("Unsupported file type: %s\n", abs_path);
		return HTML_ERROR;
	}

	// 打开文件
	file = fopen(abs_path, "rb");
	if (file == NULL) {
		printf("Failed to open file: %s\n", abs_path);
		return HTML_ERROR;
	}

	// 获取文件信息
	fstat(fileno(file), &file_stat);
	snprintf(Length, BUF_SIZE, "%ld", file_stat.st_size);

	/* 构建并发送HTTP响应头 */
	
	// 发送状态行 (带重试机制)
	int retry_count = 0;
	while (retry_count < 3) {
		if (Send_Ifon(Socket, File_ok_line, strlen(File_ok_line)) == HTML_OK) {
			break;
		}
		retry_count++;
		usleep(100000); // 100ms延迟
	}
	if (retry_count >= 3) {
		printf("Failed to send status line after 3 retries\n");
		fclose(file);
		return HTML_ERROR;
	}

	// 发送Server头 (带重试机制)
	retry_count = 0;
	while (retry_count < 3) {
		if (Send_Ifon(Socket, Server_name, strlen(Server_name)) == HTML_OK) {
			break;
		}
		retry_count++;
		usleep(100000); // 100ms延迟
	}
	if (retry_count >= 3) {
		printf("Failed to send Server header after 3 retries\n");
		fclose(file);
		return HTML_ERROR;
	}

	// 发送Date头
	cur_time = Get_Data(cur_time);
	Send_Ifon(Socket, "Date: ", 6);
	if (Send_Ifon(Socket, cur_time, strlen(cur_time)) == HTML_ERROR) {
		printf("Failed to send Date header\n");
		fclose(file);
		return HTML_ERROR;
	}

	// 发送Content-Type头
	File_ok_type = Judge_File_Type(abs_path, File_ok_type);
	if (Send_Ifon(Socket, File_ok_type, strlen(File_ok_type)) == HTML_ERROR) {
		printf("Failed to send Content-Type header\n");
		fclose(file);
		return HTML_ERROR;
	}

	// 发送Content-Length头
	if (Send_Ifon(Socket, File_ok_length, strlen(File_ok_length)) != HTML_ERROR) {
		if (Send_Ifon(Socket, Length, strlen(Length)) != HTML_ERROR) {
			if (Send_Ifon(Socket, "\n", 1) == HTML_ERROR) {
				printf("Failed to send Content-Length header\n");
				fclose(file);
				return HTML_ERROR;
			}
		}
	}

	// 发送空行结束头部
	if (Send_Ifon(Socket, File_ok_end, strlen(File_ok_end)) == HTML_ERROR) {
		printf("Failed to send header terminator\n");
		fclose(file);
		return HTML_ERROR;
	}

	/* 分块发送文件内容 */
	while (file_stat.st_size > 0) {
		// 计算本次发送的数据量(不超过1024字节)
		int chunk_size = file_stat.st_size < 1024 ? file_stat.st_size : 1024;
		send_length = fread(sendbuf, 1, chunk_size, file);
		
		// 发送数据块
		if (Send_Ifon(Socket, sendbuf, send_length) == HTML_ERROR) {
			printf("Failed to send file chunk\n");
			break;
		}
		
		// 更新剩余文件大小
		file_stat.st_size -= send_length;
	}

	fclose(file);
	return HTML_OK;
}


const char* Get_Data(const char* cur_time) {
	//获取Web服务器的当前时间作为响应时间 
	time_t curtime;
	time(&curtime);
	cur_time = ctime(&curtime);

	return cur_time;
}

long get_memory_usage() {
    long mem = 0;
    FILE* fp = fopen("/proc/self/statm", "r");
    if(fp) {
        if(fscanf(fp, "%ld", &mem) != 1) {
            printf("Failed to read memory usage\n");
            mem = 0;
        }
        fclose(fp);
    } else {
        perror("Failed to open /proc/self/statm");
    }
    return mem * (sysconf(_SC_PAGESIZE) / 1024 / 1024); // 转换为MB
}
