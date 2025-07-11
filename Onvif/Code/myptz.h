#ifndef __MYPTZ_H__
#define __MYPTZ_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include "soapH.h"
#include "wsseapi.h"
#include "wsaapi.h"
#include <unistd.h> //延时用

extern struct Namespace namespaces[];
/**
 * @brief PTZ相机结构体（添加预置位缓存）
 */
typedef struct {
    struct soap soap;       // gSOAP环境
    char ip[64];            // 设备IP地址
    char username[32];      // 用户名
    char password[32];      // 密码
    char media_addr[256];   // 媒体服务地址
    char ptz_addr[256]; 
    char profile[256];      // 媒体配置token
    char ptz_endpoint[256]; // PTZ服务端点
    
    // 预置位缓存
    struct Preset {
        char name[64];      // 预置位名称
        char token[64];     // 预置位token
    } *presets;             // 预置位数组
    int num_presets;        // 预置位数量
} PTZCamera;


PTZCamera* ptz_camera_init(const char* ip, const char* username, const char* password);
int ptz_camera_refresh_presets(PTZCamera* cam);
int ptz_camera_goto_preset(PTZCamera* cam, const char* preset_name);
void ptz_camera_cleanup(PTZCamera* cam);
int ptz_camera_absolute_move(PTZCamera** cameras, int camera_index, float pan, float panSpeed, float tilt, float tiltSpeed, float zoom, float zoomSpeed);


#endif
