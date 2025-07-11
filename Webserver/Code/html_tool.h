#ifndef __HTML_TOOL_H__
#define __HTML_TOOL_H__

#include "./Onvif/Code/camera_config.h"

/* 函数声明 */
int read_json_file(const char* path, char** json_str);
int split_url(const char* input, char* before, char* after);
int safe_write_file(const char* filename, const char* content);
void log_request_details(int sockfd, const char* uri);
int Inquire_File(char* URI);
int File_not_Inquire(int Socket);
int Send_File(char* URI, int Socket);
const char* Get_Data(const char* cur_time);
long get_memory_usage();
int parse_ipset_config(const char *config_str, int *camera_count, CameraConfig cameras[5]);

#endif
