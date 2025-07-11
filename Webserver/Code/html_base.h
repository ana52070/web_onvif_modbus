#ifndef __HTML_BASE_H__
#define __HTML_BASE_H__

int Server_Socket_Init(int port);
const char* Judge_Method(char* method, int Socket);
int Judge_URI(char* URI, int Socket);
int Send_Ifon(int Socket, const char* sendbuf, int Length);
int Error_Request_Method(int Socket);
const char* Judge_File_Type(char* URI, const char* content_type);
void handle_signals(int sig);


#endif
