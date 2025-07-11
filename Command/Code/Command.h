#ifndef __COMMAND_H__
#define __COMMAND_H__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>  // 包含PATH_MAX的定义
#include <sys/wait.h> // 包含wait()的声明




void call_with_system();

void call_with_exec();

void call_with_popen();
int edit_network_script(const char *script_path,int ipa, int ipb, int ipc, int ipd,int maska, int maskb, int maskc, int maskd,int gwa, int gwb, int gwc, int gwd );
void run_script_command(const char *script_path) ;
#endif
