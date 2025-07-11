/**
 * @file myptz.c
 * @brief ONVIF PTZ控制程序
 * 
 * 本程序实现通过ONVIF协议控制PTZ摄像头的功能，包括：
 * - 获取设备能力
 * - 获取媒体配置
 * - 获取预置位列表
 * - 移动到指定预置位
 * 
 * 使用gSOAP库实现ONVIF协议通信
 */


#include "myptz.h"






/**
 * @brief 主函数，控制PTZ摄像头移动到预置位3
 * 
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return int 程序退出状态码
 * 
 * 程序执行流程：
 * 1. 初始化gSOAP环境
 * 2. 获取设备能力信息
 * 3. 获取媒体配置
 * 4. 获取预置位列表
 * 5. 查找预置位3的token
 * 6. 控制PTZ移动到预置位3
 */
/**
 * @brief 初始化PTZ相机
 * 
 * @param ip 相机IP地址
 * @param username 用户名
 * @param password 密码
 * @return PTZCamera* 初始化后的相机对象指针
 */
PTZCamera* ptz_camera_init(const char* ip, const char* username, const char* password)
{
    PTZCamera* cam = (PTZCamera*)malloc(sizeof(PTZCamera));
    if (!cam) return NULL;
    
    // 初始化gSOAP环境
    soap_init(&cam->soap);
    
    // 设置相机参数
    strncpy(cam->ip, ip, sizeof(cam->ip)-1);
    strncpy(cam->username, username, sizeof(cam->username)-1);
    strncpy(cam->password, password, sizeof(cam->password)-1);
    
    // 获取设备能力
    struct _tds__GetCapabilities req;
    struct _tds__GetCapabilitiesResponse rep;
    req.Category = (enum tt__CapabilityCategory*)soap_malloc(&cam->soap, sizeof(int));
    req.__sizeCategory = 1;
    *(req.Category) = (enum tt__CapabilityCategory)0;
    
    char endpoint[256];
    snprintf(endpoint, sizeof(endpoint), "http://%s/onvif/device_service", cam->ip);
    soap_call___tds__GetCapabilities(&cam->soap, endpoint, NULL, &req, &rep);
    
    if (cam->soap.error) {
        printf("GetCapabilities failed: %d, %s\n", cam->soap.error, *soap_faultstring(&cam->soap));
        free(cam);
        return NULL;
    }
    
    // 保存服务地址
    strncpy(cam->media_addr, rep.Capabilities->Media->XAddr, sizeof(cam->media_addr)-1);
     // 新增：保存PTZ服务端点
    if (rep.Capabilities->PTZ && rep.Capabilities->PTZ->XAddr) {
        strncpy(cam->ptz_addr, rep.Capabilities->PTZ->XAddr, sizeof(cam->ptz_addr)-1);
    } else {
        // 回退到硬编码路径
        snprintf(cam->ptz_addr, sizeof(cam->ptz_addr), "http://%s/onvif/PTZ", cam->ip);
    }
    printf("Media endpoint: %s\n", cam->media_addr);
    printf("PTZ endpoint: %s\n", cam->ptz_addr);
    
    // 获取Profile
    struct _trt__GetProfiles getProfiles;
    struct _trt__GetProfilesResponse response;
    soap_wsse_add_UsernameTokenDigest(&cam->soap, NULL, cam->username, cam->password);
    
    if (soap_call___trt__GetProfiles(&cam->soap, cam->media_addr, NULL, &getProfiles, &response) != SOAP_OK) {
        printf("GetProfiles failed: %d, %s\n", cam->soap.error, *soap_faultstring(&cam->soap));
        free(cam);
        return NULL;
    }
    strncpy(cam->profile, response.Profiles[0].token, sizeof(cam->profile)-1);

    // 初始化后加载预置位缓存
    if (ptz_camera_refresh_presets(cam) != 0) {
        printf("Warning: Failed to load presets cache\n");
    }
    
    return cam;
}



/**
 * @brief 刷新预置位缓存
 * 
 * @param cam 相机对象指针
 * @return int 0成功，-1失败
 */
int ptz_camera_refresh_presets(PTZCamera* cam)
{
    struct _tptz__GetPresets getPresets;
    struct _tptz__GetPresetsResponse getPresetsResponse;
    getPresets.ProfileToken = cam->profile;
    
    // 清理旧缓存
    if (cam->presets) {
        free(cam->presets);
        cam->presets = NULL;
        cam->num_presets = 0;
    }
    
    // 获取预置位列表
    soap_wsse_add_UsernameTokenDigest(&cam->soap, NULL, cam->username, cam->password);
    if (soap_call___tptz__GetPresets(&cam->soap, cam->ptz_addr, NULL, &getPresets, &getPresetsResponse) != SOAP_OK) {
        printf("GetPresets failed at endpoint %s: %d, %s\n", 
               cam->ptz_addr, cam->soap.error, *soap_faultstring(&cam->soap));
        return -1;
    }
    
    // 缓存预置位信息
    cam->num_presets = getPresetsResponse.__sizePreset;
    if (cam->num_presets > 0) {
        cam->presets = malloc(cam->num_presets * sizeof(struct Preset));
        for (int i = 0; i < cam->num_presets; i++) {
            strncpy(cam->presets[i].name, 
                   getPresetsResponse.Preset[i].Name ? getPresetsResponse.Preset[i].Name : "Unnamed",
                   sizeof(cam->presets[i].name)-1);
            strncpy(cam->presets[i].token, 
                   getPresetsResponse.Preset[i].token,
                   sizeof(cam->presets[i].token)-1);
        }
    }
    
    return 0;
}


/**
 * @brief 移动到指定预置位（使用缓存）
 * 
 * @param cam 相机对象指针
 * @param preset_id 预置位ID（名称或token）
 * @return int 0成功，-1失败
 */
int ptz_camera_goto_preset(PTZCamera* cam, const char* preset_id)
{
    // 查找缓存的预置位
    char* presetToken = NULL;
    for (int i = 0; i < cam->num_presets; i++) {
        // printf("Preset %d: Name='%s', Token='%s'\n", 
        //        i+1, 
        //        cam->presets[i].name, 
        //        cam->presets[i].token);
        
        // 尝试名称匹配
        if (strcmp(cam->presets[i].name, preset_id) == 0) {
            presetToken = cam->presets[i].token;
            break;
        }
        
        // 尝试token匹配
        if (strcmp(cam->presets[i].token, preset_id) == 0) {
            presetToken = cam->presets[i].token;
            break;
        }
    }
    
    if (!presetToken) {
        printf("Error: Preset %s not found in cache!\n", preset_id);
        return -1;
    }
    
    // 移动到预置位
    struct _tptz__GotoPreset gotoPreset;
    struct _tptz__GotoPresetResponse gotoPresetResponse;
    gotoPreset.ProfileToken = cam->profile;
    gotoPreset.PresetToken = presetToken;
    gotoPreset.Speed = NULL;
    
    soap_wsse_add_UsernameTokenDigest(&cam->soap, NULL, cam->username, cam->password);
    if (soap_call___tptz__GotoPreset(&cam->soap, cam->ptz_addr, NULL, &gotoPreset, &gotoPresetResponse) != SOAP_OK) {
        printf("GotoPreset failed at endpoint %s: %d, %s\n", 
               cam->ptz_addr, cam->soap.error, *soap_faultstring(&cam->soap));
        return -1;
    }
    
    return 0;
}



/**
 * @brief 清理PTZ相机资源
 * 
 * @param cam 相机对象指针
 */
void ptz_camera_cleanup(PTZCamera* cam)
{
    if (cam) {
        soap_destroy(&cam->soap);
        soap_end(&cam->soap);
        soap_done(&cam->soap);
        free(cam);
        // 释放预置位缓存
        if (cam->presets) 
        {
            free(cam->presets);
        }
    }
}

/**
 * @brief 控制PTZ摄像头绝对移动
 * 
 * @param cameras PTZ相机对象数组
 * @param camera_index 相机索引
 * @param pan 水平坐标(-1.0到1.0)
 * @param panSpeed 水平移动速度(0.0-1.0)
 * @param tilt 垂直坐标(-1.0到1.0)
 * @param tiltSpeed 垂直移动速度(0.0-1.0)
 * @param zoom 缩放坐标(0.0到1.0)
 * @param zoomSpeed 缩放速度(0.0-1.0)
 * @return int 0成功，-1失败
 */
int ptz_camera_absolute_move(PTZCamera** cameras, int camera_index, float pan, float panSpeed, float tilt, float tiltSpeed, float zoom, float zoomSpeed)
{
    // 参数检查
    if (!cameras || camera_index < 0 || !cameras[camera_index]) return -1;
    PTZCamera* cam = cameras[camera_index];
    
    if (pan < -1.0 || pan > 1.0 || tilt < -1.0 || tilt > 1.0 || zoom < 0.0 || zoom > 1.0) {
        printf("Error: Invalid coordinates (pan:%.2f, tilt:%.2f, zoom:%.2f)\n", pan, tilt, zoom);
        return -1;
    }
    if (panSpeed < 0 || panSpeed > 1.0 || tiltSpeed < 0 || tiltSpeed > 1.0 || zoomSpeed < 0 || zoomSpeed > 1.0) {
        printf("Error: Invalid speed values (panSpeed:%.2f, tiltSpeed:%.2f, zoomSpeed:%.2f)\n", panSpeed, tiltSpeed, zoomSpeed);
        return -1;
    }

    // 构造AbsoluteMove请求
    struct _tptz__AbsoluteMove req;
    struct _tptz__AbsoluteMoveResponse resp;
    memset(&req, 0, sizeof(req));
    
    req.ProfileToken = cam->profile;
    
    // 设置PTZ位置
    req.Position = soap_new_tt__PTZVector(&cam->soap, -1);
    req.Position->PanTilt = soap_new_tt__Vector2D(&cam->soap, -1);
    req.Position->PanTilt->x = pan;
    req.Position->PanTilt->y = tilt;
    req.Position->Zoom = soap_new_tt__Vector1D(&cam->soap, -1);
    req.Position->Zoom->x = zoom;
    
    // 设置速度
    req.Speed = soap_new_tt__PTZSpeed(&cam->soap, -1);
    req.Speed->PanTilt = soap_new_tt__Vector2D(&cam->soap, -1);
    req.Speed->PanTilt->x = panSpeed;
    req.Speed->PanTilt->y = tiltSpeed;
    req.Speed->Zoom = soap_new_tt__Vector1D(&cam->soap, -1);
    req.Speed->Zoom->x = zoomSpeed;
    
    // 发送请求
    soap_wsse_add_UsernameTokenDigest(&cam->soap, NULL, cam->username, cam->password);
    if (soap_call___tptz__AbsoluteMove(&cam->soap, cam->ptz_addr, NULL, &req, &resp) != SOAP_OK) {
        printf("AbsoluteMove failed at endpoint %s: %d, %s\n", 
               cam->ptz_addr, cam->soap.error, *soap_faultstring(&cam->soap));
        return -1;
    }
    
    return 0;
}


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
