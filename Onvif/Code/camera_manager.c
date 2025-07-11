#include "camera_manager.h"
#include "./Webserver/Code/html_tool.h"
#include "./Webserver/Code/html_config.h"


PTZCamera** cameras = NULL;
int current_camera_count = 0;
/**
 * @brief 获取当前时间字符串(HH:MM:SS.mmm格式)
 * @param buf 输出缓冲区
 * @param len 缓冲区长度
 */
void get_current_time(char *buf, size_t len) 
{
    struct timeval tv;
    struct tm *tm_info;
    
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);
    
    strftime(buf, len, "%H:%M:%S", tm_info);
    snprintf(buf + 8, len - 8, ".%03ld", tv.tv_usec / 1000);
}

PTZCamera** initialize_cameras(const CameraConfig* configs, int count) {
    if (count <= 0 || !configs) {
        return NULL;
    }

    // 分配指针数组内存
    PTZCamera** cameras = (PTZCamera**)malloc(count * sizeof(PTZCamera*));
    if (!cameras) {
        perror("Failed to allocate memory for cameras");
        return NULL;
    }

    // 初始化每个摄像头
    for (int i = 0; i < count; i++) {
        char ip[16];
        snprintf(ip, sizeof(ip), "%d.%d.%d.%d", 
                configs[i].ipa, configs[i].ipb, configs[i].ipc, configs[i].ipd);

        cameras[i] = ptz_camera_init(ip, configs[i].user, configs[i].pwd);
        if (!cameras[i]) 
        {
            printf("Failed to initialize camera %d at %s\n", i+1, ip);
            
            // 释放已成功初始化的摄像头
            for (int j = 0; j < i; j++) {
                ptz_camera_cleanup(cameras[j]);
            }
            free(cameras);
            return NULL;
        }

        printf("Successfully initialized camera %d: %s\n", i+1, ip);
    }

    return cameras;
}

void free_cameras(PTZCamera** cameras, int count) {
    if (!cameras) return;
    
    for (int i = 0; i < count; i++) {
        if (cameras[i]) {
            ptz_camera_cleanup(cameras[i]);
        }
    }
    free(cameras);
}




/**
 * @brief 移动指定摄像头到预置位
 * @param cameras 摄像头指针数组
 * @param camera_index 要控制的摄像头索引（从0开始）
 * @param preset 预置位编号（字符串形式，如"2"）
 * @return 0成功，-1失败
 */
int move_camera_to_preset(PTZCamera** cameras, int camera_index, const char* preset) 
{
    char time_buf[64];
    get_current_time(time_buf, sizeof(time_buf));
    
    // 参数检查
    if (!cameras || camera_index < 0) {
        printf("[%s] Invalid parameters\n", time_buf);
        return -1;
    }
    
    if (!cameras[camera_index]) {
        printf("[%s] Camera %d is NULL\n", time_buf, camera_index+1);
        return -1;
    }
    
    printf("[%s] Moving camera %d to preset %s...\n", time_buf, camera_index+1, preset);
    
    if (ptz_camera_goto_preset(cameras[camera_index], preset) != 0) {
        get_current_time(time_buf, sizeof(time_buf));
        printf("[%s] Failed to move camera %d to preset %s\n", time_buf, camera_index+1, preset);
        return -1;
    }
    
    get_current_time(time_buf, sizeof(time_buf));
    printf("[%s] Successfully moved camera %d to preset %s\n", time_buf, camera_index+1, preset);
    return 0;
}






int camera_init(void)
{
    int ret = -1;
    char* ipset_config_str = NULL;
    CameraConfig cameras_config[MAX_CAMERAS];
    current_camera_count = 0;

    // 1. 读取配置文件
    char config_path[256];
    snprintf(config_path, sizeof(config_path), "%s/Webserver/Config/ipset.json", getenv("PWD") ? getenv("PWD") : ".");
    
    printf("Trying to read config from: %s\n", config_path);
    if (access(config_path, R_OK) != 0) {
        perror("Access check failed");
        goto cleanup;
    }

    if (read_json_file(config_path, &ipset_config_str) != HTML_OK || !ipset_config_str) {
        printf("Failed to read ipset.json, errno: %d\n", errno);
        perror("Error details");
        goto cleanup;
    }
    printf("ipset_config_str:%s\n", ipset_config_str);

    // 2. 解析配置
    if (parse_ipset_config(ipset_config_str, &current_camera_count, cameras_config) != 0) {
        printf("Failed to parse ipset config\n");
        goto cleanup;
    }

    // 检查摄像头数量
    if (current_camera_count <= 0 || current_camera_count > MAX_CAMERAS) {
        printf("Invalid camera count: %d (max allowed: %d)\n", current_camera_count, MAX_CAMERAS);
        goto cleanup;
    }

    // 打印配置信息
    printf("成功解析配置！摄像头数量: %d\n", current_camera_count);
    for (int i = 0; i < current_camera_count; i++) {
        printf("摄像头 %d: %d.%d.%d.%d:%d, 用户: %s, 密码: %s\n",
               i + 1,
               cameras_config[i].ipa, cameras_config[i].ipb, 
               cameras_config[i].ipc, cameras_config[i].ipd,
               cameras_config[i].port, cameras_config[i].user, 
               cameras_config[i].pwd);
    }

    // 3. 初始化摄像头
    cameras = initialize_cameras(cameras_config, current_camera_count);
    if (!cameras) {
        printf("Failed to initialize cameras\n");
        goto cleanup;
    }

    ret = 0;

cleanup:
    // 释放资源
    if (ipset_config_str) {
        free(ipset_config_str);
    }
    return ret;
}
