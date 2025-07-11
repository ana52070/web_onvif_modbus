#ifndef __CAMERA_MANAGER_H__
#define __CAMERA_MANAGER_H__

#include "myptz.h"  // PTZ相机库头文件
#include "camera_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "soapH.h"



extern PTZCamera** cameras;
extern int current_camera_count;

// 初始化所有摄像头
PTZCamera** initialize_cameras(const CameraConfig* configs, int count);

// 释放所有摄像头资源
void free_cameras(PTZCamera** cameras, int count);

int move_camera_to_preset(PTZCamera** cameras, int camera_index, const char* preset);

void get_current_time(char *buf, size_t len);

int camera_init(void);

#endif
