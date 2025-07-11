/* 包含必要的头文件 */
#include "html_main.h"
#include "html_config.h"
#include "html_base.h"
#include "html_tool.h"


#include "./Onvif/Code/camera_manager.h"
#include "./Command/Code/Command.h"


/* 全局变量定义 */
const char* Server_name = "Server: Web Server 1.0 - BooLo\r\n";
int ServerSock = 0;



int deal_move(char* get_url_data) 
{
    printf("Starting to parse move URL data...\n");
    printf("Input string: %s\n", get_url_data);

    // 复制输入字符串，避免修改原数据
    char *data_copy = strdup(get_url_data);
    if (!data_copy) {
        printf("Failed to duplicate input string\n");
        return -1;
    }

    // 初始化PTZ参数
    float pan = 0.0f, tilt = 0.0f, zoom = 0.0f;
    float pan_speed = 0.0f, tilt_speed = 0.0f, zoom_speed = 0.0f;
    int cam[5] = {0, 0, 0, 0, 0};  // 默认所有摄像头不操作
    int parse_success = 1;          // 假设解析成功

    // 分割字符串
    char *token = strtok(data_copy, "&");
    while (token != NULL) {
        char *key = token;
        char *value = strchr(token, '=');
        
        if (value) {
            *value = '\0';  // 分隔键值对
            value++;
            
            printf("Processing key: %s, value: %s\n", key, value);

            // 处理PTZ参数
            if (strcmp(key, "pan") == 0) {
                pan = atof(value);
            }
            else if (strcmp(key, "tilt") == 0) {
                tilt = atof(value);
            }
            else if (strcmp(key, "zoom") == 0) {
                zoom = atof(value);
            }
            else if (strcmp(key, "pan_speed") == 0) {
                pan_speed = atof(value);
            }
            else if (strcmp(key, "tilt_speed") == 0) {
                tilt_speed = atof(value);
            }
            else if (strcmp(key, "zoom_speed") == 0) {
                zoom_speed = atof(value);
            }
            // 处理摄像头选择
            else if (strcmp(key, "cam1") == 0) {
                cam[0] = atoi(value) ? 1 : 0;
            }
            else if (strcmp(key, "cam2") == 0) {
                cam[1] = atoi(value) ? 1 : 0;
            }
            else if (strcmp(key, "cam3") == 0) {
                cam[2] = atoi(value) ? 1 : 0;
            }
            else if (strcmp(key, "cam4") == 0) {
                cam[3] = atoi(value) ? 1 : 0;
            }
            else if (strcmp(key, "cam5") == 0) {
                cam[4] = atoi(value) ? 1 : 0;
            }
            else {
                printf("Warning: Unknown key: %s\n", key);
            }
        }

        token = strtok(NULL, "&");
    }

    free(data_copy);

    // 检查至少选择了一个摄像头
    int camera_selected = 0;
    for (int i = 0; i < 5; i++) {
        if (cam[i]) {
            camera_selected = 1;
            break;
        }
    }

    if (!camera_selected) {
        printf("Error: No camera selected for movement\n");
        parse_success = 0;
    }

    if (!parse_success) {
        printf("Failed to parse move data\n");
        return -1;
    }

    // 打印解析结果
    printf("\nParsed move data:\n");
    printf("pan: %.2f, tilt: %.2f, zoom: %.2f\n", pan, tilt, zoom);
    printf("pan_speed: %.2f, tilt_speed: %.2f, zoom_speed: %.2f\n", pan_speed, tilt_speed, zoom_speed);
    for (int i = 0; i < 5; i++) {
        printf("cam%d: %s\n", i+1, cam[i] ? "true" : "false");
    }

    // 执行PTZ移动命令
    for (int i = 0; i < 5; i++) {
        if (cam[i] && (i+1) <= current_camera_count) {
            printf("Moving camera %d...\n", i+1);
            if (cameras && (i+1) <= current_camera_count) 
            {
                int ret = ptz_camera_absolute_move(cameras, i,
                                                 pan, pan_speed,
                                                 tilt, tilt_speed,
                                                 zoom, zoom_speed);
                if (ret != 0) {
                    printf("Failed to move camera %d, error: %d\n", i+1, ret);
                }
            }
        }
    }

    return 0;
}



int deal_preset(char* get_url_data) 
{
    printf("Starting to parse preset URL data...\n");
    printf("Input string: %s\n", get_url_data);

    // 复制输入字符串，避免修改原数据
    char *data_copy = strdup(get_url_data);
    if (!data_copy) {
        printf("Failed to duplicate input string\n");
        return -1;
    }

    // 初始化变量
    int preset_num = -1;
    int cam[5] = {-1, -1, -1, -1, -1};  // -1 表示未初始化
    int parse_success = 1;  // 假设解析成功

    // 分割字符串
    char *token = strtok(data_copy, "&");
    while (token != NULL) {
        char *key = token;
        char *value = strchr(token, '=');
        
        if (value) {
            *value = '\0';  // 分隔键值对
            value++;
            
            printf("Processing key: %s, value: %s\n", key, value);

            // 处理 preset_num
            if (strcmp(key, "preset_num") == 0) {
                preset_num = atoi(value);
                if (preset_num < 0) {
                    printf("Error: Invalid preset_num value: %s\n", value);
                    parse_success = 0;
                }
            }
            // 处理 cam1-cam5
            else if (strcmp(key, "cam1") == 0) {
                cam[0] = atoi(value) ? 1 : 0;
            }
            else if (strcmp(key, "cam2") == 0) {
                cam[1] = atoi(value) ? 1 : 0;
            }
            else if (strcmp(key, "cam3") == 0) {
                cam[2] = atoi(value) ? 1 : 0;
            }
            else if (strcmp(key, "cam4") == 0) {
                cam[3] = atoi(value) ? 1 : 0;
            }
            else if (strcmp(key, "cam5") == 0) {
                cam[4] = atoi(value) ? 1 : 0;
            }
            else {
                printf("Warning: Unknown key: %s\n", key);
            }
        }

        token = strtok(NULL, "&");
    }

    free(data_copy);

    // 检查是否所有必要字段都已解析
    if (preset_num == -1) {
        printf("Error: Missing preset_num\n");
        parse_success = 0;
    }
    for (int i = 0; i < 5; i++) {
        if (cam[i] == -1) {
            printf("Error: Missing cam%d\n", i+1);
            parse_success = 0;
        }
    }

    if (!parse_success) {
        printf("Failed to parse preset data\n");
        return -1;
    }

    // 打印解析结果
    char preset_str[4];
    printf("\nParsed preset data:\n");
    printf("preset_num: %d\n", preset_num);
    for (int i = 0; i < 5; i++) {
        printf("cam%d: %s\n", i+1, cam[i] ? "true" : "false");
        if (cam[i] && (i+1) <= current_camera_count) 
        {  // 仅当cam[i]为true时执行,//解析成功,调用预置位操作
            snprintf(preset_str, sizeof(preset_str), "%d", preset_num);
            printf("start_preset:num:%d,preset:%s\r\n", i, preset_str);
            if (cameras && (i+1) <= current_camera_count) {
                move_camera_to_preset(cameras, i, preset_str);
            }
        }
    }
    
    
    

    return 0;
}


int deal_trackcfg(char* get_url_data) 
{
    printf("Starting to parse track config URL data...\n");
    printf("Input string: %s\n", get_url_data);

    cJSON *root = NULL;
    cJSON *plc = NULL;
    cJSON *track = NULL;
    char *data_copy = strdup(get_url_data);
    char *json_str = NULL;
    
    if(!data_copy) {
        printf("Failed to duplicate input string\n");
        return -1;
    }

    root = cJSON_CreateObject();
    plc = cJSON_CreateObject();
    track = cJSON_CreateObject();
    
    if(!root || !plc || !track) {
        printf("Error creating JSON objects\n");
        free(data_copy);
        if(root) cJSON_Delete(root);
        return -1;
    }

    cJSON_AddItemToObject(root, "plc", plc);
    cJSON_AddItemToObject(root, "track", track);
    // 已在上方初始化data_copy
    char *token = strtok(data_copy, "&");
    
    while(token != NULL) {
        char *key = token;
        char *value = strchr(token, '=');
        
        if(value) {
            *value = '\0';  // 分隔键值对
            value++;
            
            printf("Processing key: %s, value: %s\n", key, value);

            // 处理PLC参数
            if(strcmp(key, "raiseh") == 0) {
                cJSON_AddNumberToObject(plc, "raiseh", atoi(value));
            }
            else if(strcmp(key, "carframe") == 0) {
                cJSON_AddNumberToObject(plc, "carframe", atoi(value));
            }
            else if(strcmp(key, "dirver") == 0) {
                cJSON_AddNumberToObject(plc, "dirver", atoi(value));
            }
            else if(strcmp(key, "carSed") == 0) {
                cJSON_AddNumberToObject(plc, "carSed", atoi(value));
            }
            else if(strcmp(key, "raiSped") == 0) {
                cJSON_AddNumberToObject(plc, "raiSped", atoi(value));
            }
            else if(strcmp(key, "Data_unit") == 0) {
                cJSON_AddNumberToObject(plc, "Data_unit", atoi(value));
            }
            else if(strcmp(key, "Data_refer") == 0) {
                cJSON_AddNumberToObject(plc, "Data_refer", atoi(value));
            }
            // 处理track参数
            else if(strcmp(key, "H_install") == 0) {
                cJSON_AddNumberToObject(track, "H_install", atof(value));
            }
            else if(strcmp(key, "H_change") == 0) {
                cJSON_AddNumberToObject(track, "H_change", atof(value));
            }
            else if(strcmp(key, "preset_num") == 0) {
                cJSON_AddNumberToObject(track, "preset_num", atoi(value));
            }
        }

        token = strtok(NULL, "&");
    }

    free(data_copy);

    // 打印解析结果
    json_str = cJSON_Print(root);
    if(!json_str) {
        printf("Error generating JSON string\n");
        cJSON_Delete(root);
        return -1;
    }

    printf("\nFinal JSON output:\n%s\n", json_str);

    // 写入文件
    FILE *fp = fopen("Webserver/Config/track.json", "w");
    if(!fp) {
        free(json_str);
        cJSON_Delete(root);
        return -1;
    }

    fputs(json_str, fp);
    fclose(fp);

    int ret = 0;
    if(json_str) {
        if(!safe_write_file("Webserver/Config/track.json", json_str)) {
            printf("Failed to write track.json\n");
            ret = -1;
        }
        free(json_str);
        json_str = NULL;
    }
    
    if(root) {
        cJSON_Delete(root);
        root = NULL;
    }
    return ret;
}



int deal_ipcfg(char* get_url_data) 
{
    printf("[DEBUG] deal_ipcfg start - memory: %ldMB\n", (long)get_memory_usage());
    printf("Starting to parse URL data...\n");
    printf("Input string: %s\n", get_url_data);
    printf("[DEBUG] Before JSON parsing - memory: %ldMB\n", (long)get_memory_usage());

    cJSON *root = NULL;
    cJSON *network = NULL;
    cJSON *plc = NULL; 
    cJSON *cameras = NULL;
    cJSON *cams[5] = {NULL};
    //cJSON *camera_count = NULL;
    char *data_copy = strdup(get_url_data);
    char *json_str = NULL;
    
    if(!data_copy) {
        printf("Failed to duplicate input string\n");
        return -1;
    }

    root = cJSON_CreateObject();
    network = cJSON_CreateObject();
    plc = cJSON_CreateObject();
    cameras = cJSON_CreateArray();
    //camera_count = cJSON_CreateArray();
    
    if(!root || !network || !plc || !cameras ) {
        printf("Error creating JSON objects\n");
        free(data_copy);
        if(root) cJSON_Delete(root);
        return -1;
    }

    cJSON_AddItemToObject(root, "network", network);
    cJSON_AddItemToObject(root, "plc", plc);
    cJSON_AddItemToObject(root, "cameras", cameras);
    // // 默认相机数量为5
    // cJSON_AddNumberToObject(root, "camera_count", 5);
    for(int i=0; i<5; i++) {
        cams[i] = cJSON_CreateObject();
        if(!cams[i]) {
            printf("Error creating camera %d object\n", i+1);
            cJSON_Delete(root);
            return -1;
        }
        cJSON_AddItemToArray(cameras, cams[i]);
    }

    // 已在上方初始化data_copy
    char *token = strtok(data_copy, "&");
    
    while(token != NULL) {
        char *key = token;
        char *value = strchr(token, '=');
        
        if(value) {
            *value = '\0';  // 分隔键值对
            value++;
            
            printf("Processing key: %s, value: %s\n", key, value);

            // 处理网络参数
            if(strncmp(key, "ipa", 3) == 0 || strncmp(key, "ipb", 3) == 0 || 
               strncmp(key, "ipc", 3) == 0 || strncmp(key, "ipd", 3) == 0 ||
               strncmp(key, "maska", 5) == 0 || strncmp(key, "maskb", 5) == 0 ||
               strncmp(key, "maskc", 5) == 0 || strncmp(key, "maskd", 5) == 0 ||
               strncmp(key, "gwa", 3) == 0 || strncmp(key, "gwb", 3) == 0 ||
               strncmp(key, "gwc", 3) == 0 || strncmp(key, "gwd", 3) == 0 ||
               strncmp(key, "remoipa", 7) == 0 || strncmp(key, "remoipb", 7) == 0 ||
               strncmp(key, "remoipc", 7) == 0 || strncmp(key, "remoipd", 7) == 0) {
                printf("Adding network param: %s=%s\n", key, value);
                cJSON_AddNumberToObject(network, key, atof(value));
            }
            // 处理PLC参数
            else if(strncmp(key, "PLCaddr", 7) == 0) {
                printf("Adding PLC param: addr=%s\n", value);
                cJSON_AddNumberToObject(plc, "addr", atof(value));
            }
            else if(strncmp(key, "funum", 5) == 0) {
                printf("Adding PLC param: funum=%s\n", value);
                cJSON_AddNumberToObject(plc, "funum", atof(value));
            }
            else if(strncmp(key, "stareg", 6) == 0) {
                printf("Adding PLC param: stareg=%s\n", value);
                cJSON_AddNumberToObject(plc, "stareg", atof(value));
            }
            else if(strncmp(key, "numbr", 5) == 0) {
                printf("Adding PLC param: numbr=%s\n", value);
                cJSON_AddNumberToObject(plc, "numbr", atof(value));
            }
            else if(strncmp(key, "remoteport", 10) == 0) {
                printf("Adding PLC param: remoteport=%s\n", value);
                cJSON_AddNumberToObject(plc, "remoteport", atof(value));
            }
            // 处理相机参数
            else if(strncmp(key, "cam_", 4) == 0) 
            {
                // 获取相机索引(1-5)
                int cam_idx = key[strlen(key)-1] - '1';
                printf("Camera index: %d (from key %s)\n", cam_idx+1, key);
                
                if(cam_idx < 0 || cam_idx > 4) {
                    printf("Invalid camera index: %d\n", cam_idx+1);
                    token = strtok(NULL, "&");
                    continue;
                }
                // 处理相机IP地址
                else if(strstr(key, "ipa") || strstr(key, "ipb") || 
                   strstr(key, "ipc") || strstr(key, "ipd")) {
                    printf("Adding camera %d IP part: %s=%s\n", cam_idx+1, key+4, value);
                    // 修正IP参数键名 - 去掉前面的"cam_"和后面的数字
                    char ip_key[5] = {0};
                    strncpy(ip_key, key+4, strlen(key+4)-1);
                    cJSON_AddNumberToObject(cams[cam_idx], ip_key, atof(value));
                }
                else if(strstr(key, "port")) {
                    printf("Adding camera %d port: %s\n", cam_idx+1, value);
                    cJSON_AddNumberToObject(cams[cam_idx], "port", atof(value));
                }
                else if(strstr(key, "user")) {
                    printf("Adding camera %d user: %s\n", cam_idx+1, value);
                    cJSON_AddStringToObject(cams[cam_idx], "user", value);
                }
                else if(strstr(key, "pwd")) {
                    printf("Adding camera %d password: %s\n", cam_idx+1, value);
                    cJSON_AddStringToObject(cams[cam_idx], "pwd", value);
                }
            }
            // 处理相机数量
            if(strcmp(key, "camera_count") == 0) 
            {
                printf("Adding camera_count: %s\n", value);
                cJSON_AddNumberToObject(root, "camera_count", atof(value));
            }
        }

        token = strtok(NULL, "&");
    }

    free(data_copy);

    // 打印解析结果
    json_str = cJSON_Print(root);
    if(!json_str) {
        printf("Error generating JSON string\n");
        cJSON_Delete(root);
        return -1;
    }

    printf("\nFinal JSON output:\n%s\n", json_str);


	    FILE *fp = fopen("Webserver/Config/ipset.json", "w");
    if(!fp) {
        free(json_str);
        cJSON_Delete(root);
        return -1;
    }

    fputs(json_str, fp);
    fclose(fp);

    int ret = 0;
    if(json_str) {
        if(!safe_write_file("Webserver/Config/ipset.json", json_str)) {
            printf("Failed to write ipset.json\n");
            ret = -1;
        }
    }
    
    printf("deal_ipcfg memory cleaned, returning %d\n", ret);
    printf("[DEBUG] After deal_ipcfg - memory: %ldMB\n", (long)get_memory_usage());




    //重新配置ip信息
    int result = edit_network_script(
    "./Command/Sh/ipset.sh",  // 脚本路径
    cJSON_GetObjectItem(network, "ipa")->valueint,
    cJSON_GetObjectItem(network, "ipb")->valueint,
    cJSON_GetObjectItem(network, "ipc")->valueint,
    cJSON_GetObjectItem(network, "ipd")->valueint,
    cJSON_GetObjectItem(network, "maska")->valueint,
    cJSON_GetObjectItem(network, "maskb")->valueint,
    cJSON_GetObjectItem(network, "maskc")->valueint,
    cJSON_GetObjectItem(network, "maskd")->valueint,
    cJSON_GetObjectItem(network, "gwa")->valueint,
    cJSON_GetObjectItem(network, "gwb")->valueint,
    cJSON_GetObjectItem(network, "gwc")->valueint,
    cJSON_GetObjectItem(network, "gwd")->valueint
    );



    if (result != 0) {
        printf("脚本修改失败\n");
        return 1;
    }

    run_script_command("./Command/Sh/ipset.sh");





    // 现在可以安全释放内存
    if(json_str) {
        free(json_str);
        json_str = NULL;
    }
    
    if(root) {
        cJSON_Delete(root);
        root = NULL;
    }


    //重新配置相机参数
    camera_init();

    return ret;
}







/**
 * 处理HTTP请求消息
 * @param message HTTP请求消息字符串
 * @param Socket 客户端套接字描述符
 * @return 成功返回OK，失败返回ERROR
 *
 * 函数处理流程：
 * 1. 解析请求行(GET/POST请求方法、URI、HTTP版本)
 * 2. 验证请求方法是否支持(GET/POST)
 * 3. 处理POST请求数据(如果有)
 * 4. 处理根目录请求(重定向到/login.html)
 * 5. 验证请求URI的有效性
 * 6. 发送请求的文件内容给客户端
 */
int Handle_Request_Message(char* message, int Socket) {
    // 设置socket超时
    struct timeval tv;
    tv.tv_sec = 2;  // 2秒接收超时
    tv.tv_usec = 0;
    if (setsockopt(Socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Failed to set receive timeout");
    }
    
    tv.tv_sec = 2; // 2秒发送超时
    if (setsockopt(Socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Failed to set send timeout");
    }

    int rval = 0;                // 函数返回值
    char Method[BUF_SIZE];       // 存储请求方法(GET/POST等)
    char URI[BUF_SIZE];          // 存储请求URI
    char Version[BUF_SIZE];      // 存储HTTP版本
	
	/* 解析HTTP请求行 */
	if (sscanf(message, "%s %s %s", Method, URI, Version) != 3) {
		printf("Invalid request line format\n");
		return HTML_ERROR;  // 请求行格式错误
	}
	printf("URI:%s\r\n",URI);
	//log_request_details(Socket, URI);
	/* 验证请求方法 */
    const char* method_type = Judge_Method(Method, Socket);
    if (method_type == HTML_ERROR) {
        return HTML_ERROR;  // 不支持的请求方法
    } 
    else if (strcmp(method_type, "POST") == 0) {
		// 处理登录请求
		if (strcmp(URI, "/checklogin.cgi") == 0) {
			const char* login_result = Post_Value(message);
			if (strcmp(login_result, "OK") == 0) {
				// 登录成功，返回ipset.html
				strcpy(URI,"Webserver/HTML/ipset.html");
			} else {
				// 登录失败，返回login.html并添加错误参数
				strcpy(URI, "Webserver/HTML/login.html?error=1");
			}
		} else {
			Post_Value(message);  // 处理其他POST请求
		}
	}

    else if (strcmp(method_type, "GET") == 0)
	{
        if(strcmp(URI,"/track.html") == 0)
        {
            strcpy(URI,"Webserver/HTML/track.html"); //返回当前页面
        }
        else if(strcmp(URI,"/ipset.html") == 0)
        {
            strcpy(URI,"Webserver/HTML/ipset.html"); //返回当前页面
        }
		// 处理CGI请求
		if(strstr(URI, ".cgi")) {
			char get_url_command[20], get_url_data[1024];
			if(split_url(URI, get_url_command, get_url_data) == -1) {
				printf("Invalid CGI request format\n");
				return HTML_ERROR;
			}
			//printf("get_url_command:%s,get_url_data:%s\n", get_url_command, get_url_data);

		//判断是哪个Get请求，并作出对应的处理
		if(strcmp(get_url_command,"/ipcfg.cgi") == 0)
		{
			printf("judge_sussceful:%s\r\n",get_url_data);
			if(deal_ipcfg(get_url_data) == -1)
			{
				printf("deal_ipcfg:error\r\n");
			}
			else
			{
				printf("deal_ipcfg processed successfully\n");
		strcpy(URI, "Webserver/HTML/ipset.html"); //返回当前页面
			}
		}
		else if(strcmp(get_url_command,"/trackcfg.cgi") == 0)
		{
			printf("judge_sussceful:%s\r\n",get_url_data);
			if(deal_trackcfg(get_url_data) == -1)
			{
				printf("deal_trackcfg:error\r\n");
			}
			else
			{
				printf("deal_trackcfg processed successfully\n");
				strcpy(URI,  "Webserver/HTML/track.html"); //返回当前页面
			}
		}
		else if(strcmp(get_url_command,"/preset.cgi") == 0)
		{
			printf("judge_sussceful:%s\r\n",get_url_data);
            //在此处调用预置位操作
			if(deal_preset(get_url_data) == -1)
			{
				printf("deal_trackcfg:error\r\n");
			}
            {
                //解析成功，调用预置位操作已在deal_preset中完成,现在返回当前页面
				printf("preset processed successfully\n");
				strcpy(URI,  "Webserver/HTML/ipset.html"); //返回当前页面
			}
		}
		else if(strcmp(get_url_command,"/move.cgi") == 0)
		{
			printf("judge_sussceful:%s\r\n",get_url_data);
            //在此处调用PTZ绝对移动操作
			if(deal_move(get_url_data) == -1)
			{
				printf("deal_move:error\r\n");
			}
            {
                //解析成功，调用PTZ绝对移动操作已在deal_move中完成,现在返回当前页面
				printf("move processed successfully\n");
				strcpy(URI,  "Webserver/HTML/ipset.html"); //返回当前页面
			}
		}
        else if(strcmp(get_url_command,"/reboot.cgi") == 0)
        {
            printf("judge_sussceful:%s\n",get_url_data);
            strcpy(URI,  "Webserver/HTML/reboot.html"); //返回当前页面
            run_script_command("./Command/Sh/reboot.sh");
        }
        
		} 
		else {
			// 处理普通文件请求
			if (strcmp(URI, "/") == 0) {
				strcpy(URI, "Webserver/HTML/login.html");
			}
			
			// 处理JSON配置文件请求
			if (strstr(URI, ".json")) {
				char json_path[256];
				snprintf(json_path, sizeof(json_path), "Webserver/Config/%s", strrchr(URI, '/') + 1);
				strcpy(URI, json_path);
			}
            
            //处理图片文件请求
            if (strstr(URI, ".png")) {
				char png_path[256];
				snprintf(png_path, sizeof(png_path), "Webserver/HTML/%s", strrchr(URI, '/') + 1);
				strcpy(URI, png_path);
			}

            //处理标签图标文件请求
            if (strstr(URI, ".ico")) {
				char png_path[256];
				snprintf(png_path, sizeof(png_path), "Webserver/HTML/%s", strrchr(URI, '/') + 1);
				strcpy(URI, png_path);
			}
			
			if (Judge_URI(URI, Socket) == HTML_ERROR) {
				return HTML_ERROR;
			}
            
			return Send_File(URI, Socket);
		}
	}

	/* 处理根目录请求 */
	if (strcmp(URI, "/") == 0) {
		strcpy(URI, "Webserver/HTML/login.html");  // 重定向到登录页面
	}

	/* 验证URI有效性并发送文件 */
	if (Judge_URI(URI, Socket) == HTML_ERROR) {
		return HTML_ERROR;  // URI无效
	} 
	else {
		rval = Send_File(URI, Socket);  // 发送请求的文件
	}

	if (rval == HTML_OK) {
		printf("Request processed successfully\n");
	}

	return rval;  // 返回处理结果
}






const char* Post_Value(char* message) {
    // 解析并验证登录信息
    const char* suffix;
    char username[32] = {0};
    char password[32] = {0};

    // 获取POST数据部分
    if ((suffix = strstr(message, "\r\n\r\n")) != NULL) {
        suffix += 4; // 跳过头部
    } else {
        return "ERROR";
    }

    // 解析用户名和密码
    if (sscanf(suffix, "username=%31[^&]&password=%31s", username, password) != 2) {
        return "ERROR";
    }

    // 验证用户名和密码
    if (strcmp(username, VALID_USERNAME) == 0 && 
        strcmp(password, VALID_PASSWORD) == 0) {
        return "OK";
    }
    
    return "ERROR";
}




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
