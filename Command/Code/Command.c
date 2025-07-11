#include "Command.h"



void call_with_system() {
    printf("=== Using system() ===\n");
    int ret = system("./Command/Sh/test_helloworld.sh");
    printf("system() returned: %d\n\n", ret);
}

void call_with_exec() {
    printf("=== Using exec() ===\n");
    pid_t pid = fork();
    
    if (pid == 0) { // 子进程
        execl("/bin/sh", "sh", "./Command/Sh/test_helloworld.sh", (char *)NULL);
        perror("exec failed");
        _exit(1);
    } else if (pid > 0) { // 父进程
        wait(NULL);
    } else {
        perror("fork failed");
    }
    printf("\n");
}

void call_with_popen() {
    printf("=== Using popen() ===\n");
    FILE *fp = popen("./Command/Sh/test_helloworld.sh", "r");
    if (!fp) {
        perror("popen failed");
        return;
    }
    
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        printf("Script says: %s", buffer);
    }
    
    pclose(fp);
    printf("\n");
}


void run_script_command(const char *script_path) 
{
    int ret = system(script_path);
    printf("%s run returned: %d\n\n", script_path ,ret);
}



int edit_network_script(const char *script_path, int ipa, int ipb, int ipc, int ipd,
                       int maska, int maskb, int maskc, int maskd,
                       int gwa, int gwb, int gwc, int gwd) 
{
    // 1. 获取原文件权限
    struct stat st;
    if (stat(script_path, &st) != 0) {
        perror("无法获取文件状态");
        return -1;
    }
    mode_t original_mode = st.st_mode;

    // 2. 打开原文件（只读模式）
    FILE *fp = fopen(script_path, "r");
    if (!fp) {
        perror("无法打开脚本文件");
        return -1;
    }

    // 3. 创建临时文件（与原文件同目录，保持安全）
    char temp_path[PATH_MAX];
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", script_path);
    
    FILE *tmp = fopen(temp_path, "w");
    if (!tmp) {
        perror("无法创建临时文件");
        fclose(fp);
        return -1;
    }

    // 4. 处理文件内容
    char line[256];
    int ip_line_modified = 0;
    int mask_line_modified = 0;
    int gw_line_found = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strstr(line, "IP_ADDR=")) {
            fprintf(tmp, "IP_ADDR=\"%d.%d.%d.%d\"\n", ipa, ipb, ipc, ipd);
            ip_line_modified = 1;
        }
        else if (strstr(line, "NETMASK=")) {
            fprintf(tmp, "NETMASK=\"%d.%d.%d.%d\"\n", maska, maskb, maskc, maskd);
            mask_line_modified = 1;
        }
        else if (strstr(line, "GATEWAY=")) {
            fprintf(tmp, "GATEWAY=\"%d.%d.%d.%d\"\n", gwa, gwb, gwc, gwd);
            gw_line_found = 1;
        }
        else if (strstr(line, "INTERFACE=") && !gw_line_found) {
            fputs(line, tmp);
            fprintf(tmp, "GATEWAY=\"%d.%d.%d.%d\"\n", gwa, gwb, gwc, gwd);
            gw_line_found = 1;
        }
        else {
            fputs(line, tmp);
        }
    }

    if (!gw_line_found) {
        fprintf(tmp, "\nGATEWAY=\"%d.%d.%d.%d\"\n", gwa, gwb, gwc, gwd);
    }

    fclose(fp);
    if (fclose(tmp) != 0) {
        perror("写入临时文件失败");
        remove(temp_path);
        return -1;
    }

    // 5. 设置临时文件权限（与原文件相同）
    if (chmod(temp_path, original_mode) != 0) {
        perror("无法设置临时文件权限");
        remove(temp_path);
        return -1;
    }

    // 6. 原子替换文件
    if (rename(temp_path, script_path) != 0) {
        perror("无法替换原文件");
        remove(temp_path);
        return -1;
    }

    // 7. 检查修改结果
    if (!ip_line_modified) {
        printf("警告: 未找到IP_ADDR行\n");
    }
    if (!mask_line_modified) {
        printf("警告: 未找到NETMASK行\n");
    }

    printf("脚本修改成功，权限已保留！\n");
    return 0;
}
