#ifndef __CAMERA_CONFIG_H__
#define __CAMERA_CONFIG_H__

#define CAMERA_OK 0
#define CAMERA_ERROR -1

#define MAX_CAMERAS 5

typedef struct {
    int ipa, ipb, ipc, ipd;
    int port;
    char user[32];
    char pwd[32];
} CameraConfig;

#endif
