#include "modbusTcp_master.h"

/*================内部变量与数据定义区================*/
#define MAX_RB 512

#pragma pack(1)
typedef struct mbap_pdu_struct{
	unsigned char SerialNumber[2];  //事务处理标识//
	unsigned char Proide[2];  //协议标识//
	unsigned char PduLength[2]; //PDU数据段长度//
	unsigned char Slave_ID;  //访问的目标从站ID//
	unsigned char Fun_Code;  //功能码//
	unsigned char Fun_Data[MAX_RB-8];  //纯数据部分//
}MBAP_PDU_T;
#pragma pack()

/*=================内部功能函数申明区域=================*/
static void delay_us(unsigned int time);
static char *GetIP_by_hostname(char *hostname);  //获取网卡设备的IPV4地址//

/*================客户端创建与销毁函数实现区===============*/
CLIENT_T *Create_modbustcpClient(CREATE_IN *client_per)
{
	if(NULL == client_per)
		return NULL;
	CLIENT_T *client_t = NULL;
	char *client_addr;
	unsigned int error_count;
	
	if(NULL == (client_per->Slave_addr))
	{
		perror("Null of slave addr!\n");
		return NULL;
	}
	/*1 获取使用的客户端网卡IP地址，检查配置是否合法*/
	client_addr = GetIP_by_hostname((client_per->Net_path));
	if(NULL == client_addr)
	{
		perror("Set client device error\n");
		return NULL;
	}
	// 添加调试信息
	printf("Client IP: %s\n", client_addr);
	printf("Slave IP: %s\n", client_per->Slave_addr);
	
	// 移除IP地址检查限制，允许跨子网连接
	// 仅保留调试信息用于诊断
	
	/*2 创建客户端结构体，开始匹配链接远端服务器*/
	client_t = (CLIENT_T*)malloc(sizeof(CLIENT_T));
	if(NULL == client_t)
	{
		perror("malloc");
		free(client_addr);
		return NULL;
	}
	memset(client_t,0,sizeof(CLIENT_T));
	
	client_t->link_time = (client_per->link_time);
	client_t->link_count = (client_per->link_count);
	client_t->SlaveID = (client_per->SlaveID);
	
	(client_t->client_sock).sin_family = AF_INET;
	(client_t->client_sock).sin_port = htons(client_per->Slave_id);
	(client_t->client_sock).sin_addr.s_addr = inet_addr(client_addr);
	(client_t->client_fd) = socket(AF_INET,SOCK_STREAM,0);  //创建客户端文件描述符//
	if(0 > (client_t->client_fd))
	{
		perror("socket");
		free(client_addr);
		free(client_t);
		return NULL;
	}
	
	(client_t->slave_sock).sin_family = AF_INET;
	(client_t->slave_sock).sin_port = htons(client_per->Slave_id);
	(client_t->slave_sock).sin_addr.s_addr = inet_addr(client_per->Slave_addr);
	for(error_count=0;error_count<(client_per->link_count)+1;++error_count)
	{
		if(0<=connect((client_t->client_fd),(void*)&(client_t->slave_sock),sizeof(client_t->slave_sock)))  //客户端与远端服务器连接//
			break;
		perror("connect");
		delay_us((client_per->link_time));
	}
	if(error_count>=((client_per->link_count)+1))  //超出失败重试次数，仍旧链接失败//
	{
		free(client_addr);
		close(client_t->client_fd);
		free(client_t);
		return NULL;
	}
	
	free(client_addr);
	return client_t;
}
int Free_modbustcpClient(CLIENT_T *client_t)
{
	if(NULL == client_t)
		return -1;
	int ret_var = 0;
	
	close(client_t->client_fd);
	free(client_t);
	
	return ret_var;
}

/*===============读远程服务器功能函数实现区============*/
int Readdata_mode_bits(CLIENT_T *client_t,READ_DATA_T *read_data_t)
{
	int ret_var=0;
	if((NULL == client_t) || (NULL == read_data_t))
		return -1;
	if(NULL == (read_data_t->buffer))
		return -2;
	
	unsigned char *Send_data;  //要发送的数据请求报文//
	MBAP_PDU_T *Read_data;  //接收装载服务器发送回来的数据//
	fd_set rset;
	struct timeval tv;
	tv.tv_usec = (client_t->response_time)+(client_t->delay_time);
	FD_ZERO(&rset);
	FD_SET((client_t->client_fd),&rset);
	
	/*1 组装报文数据请求远端服务器，这里使用01H指令读取线圈*/
	Send_data = (unsigned char*)malloc(sizeof(unsigned char)*12);
	if(NULL == Send_data)
		return -3;
	memset(Send_data,0,sizeof(unsigned char)*12);
	Send_data[0] = ((client_t->data_count)>>8);
	Send_data[1] = ((client_t->data_count)&0xff);
	Send_data[5] = 0x06;
	Send_data[6] = ((client_t->SlaveID)&0xff);
	Send_data[7] = 0x01;
	// 寄存器地址直接使用，不需要转换
	Send_data[8] = ((read_data_t->Start_addr)>>8);
	Send_data[9] = ((read_data_t->Start_addr)&0xff);
	Send_data[10] = 0x00;  // 读取长度高字节
	Send_data[11] = read_data_t->Read_length;  // 读取长度低字节
	/*将请求数据包发送到远程服务器*/
	int write_ret=0,length=0;
	for(;length<12;)
	{
		write_ret = write((client_t->client_fd),&(Send_data[length]),12);
		if(0>write_ret)  //客户端与服务器已经断开了链接//
		{
			free(Send_data);
			return -4;
		}
		length += write_ret;
	}
	free(Send_data);
	Send_data=NULL;
	
	/*2 数据发送完成后，立马等待远端服务器的回复*/
	ret_var = select((client_t->client_fd)+1,&rset,NULL,NULL,&tv);
	if(0 == ret_var)
		return -5;
	else if(0>ret_var)
		return -6;
	else if(0 != FD_ISSET((client_t->client_fd),&rset))  //开始采集回复的数据//
	{
		Read_data = (MBAP_PDU_T*)malloc(sizeof(MBAP_PDU_T));
		if(NULL == Read_data)
			return -3;
		memset(Read_data,0,sizeof(MBAP_PDU_T));
		ret_var = read((client_t->client_fd),Read_data,MAX_RB);
		if(0 >= ret_var)  //客户端与服务器断开了链接//
		{
			free(Read_data);
			return -4;
		}
	}
	
	/*3 开始对服务器回复的数据进行分析并送回调用者*/
	ret_var = 0;
	unsigned short SerialNumber;
	unsigned short Proide;
	SerialNumber = Read_data->SerialNumber[0];
	SerialNumber <<= 8;
	SerialNumber |= Read_data->SerialNumber[1];
	Proide = Read_data->Proide[0];
	Proide <<= 8;
	Proide |= Read_data->Proide[1];
	if(0 != Proide)  //接受到的数据包错误或不属于modbustcp协议//
	{
		free(Read_data);
		return -7;
	}
	if((client_t->SlaveID) != (Read_data->Slave_ID))  //非目标服务器回复//
	{
		free(Read_data);
		return -8;
	}
	if(0x01 != (Read_data->Fun_Code))  //回复的数据包功能码不对应
	{
		free(Read_data);
		return -9;
	}
	if(SerialNumber != (client_t->data_count))  //提示数据包似乎不是回复此次事件的//
		ret_var = 1;
	
	/*4 开始将关键数据提取并返回*/
	unsigned char byte_length;
	unsigned char *read_buffer = (unsigned char*)(read_data_t->buffer);
	for(byte_length=0;byte_length<(Read_data->Fun_Data[0]);++byte_length)
		read_buffer[byte_length] = Read_data->Fun_Data[byte_length+1];
	free(Read_data);
	
	(client_t->data_count) ++;
	if(65534 < (client_t->data_count))
		(client_t->data_count) = 0;
	return ret_var;
}
int Readdata_mode_bytes(CLIENT_T *client_t,READ_DATA_T *read_data_t,const unsigned char DataMode)
{
	int ret_var=0;
	if((NULL == client_t) || (NULL == read_data_t))
		return -1;
	if(NULL == (read_data_t->buffer))
		return -2;
	
	unsigned char *Send_data;  //要发送的数据请求报文//
	MBAP_PDU_T *Read_data;  //接收装载服务器发送回来的数据//
	fd_set rset;
	struct timeval tv;
	tv.tv_usec = (client_t->response_time)+(client_t->delay_time);
	FD_ZERO(&rset);
	FD_SET((client_t->client_fd),&rset);
	
	/*1 根据访问的不同数据内容，组装请求数据报文*/
	Send_data = (unsigned char*)malloc(sizeof(unsigned char)*12);
	if(NULL == Send_data)
		return -3;
	memset(Send_data,0,sizeof(unsigned char)*12);
	Send_data[0] = ((client_t->data_count)>>8);
	Send_data[1] = ((client_t->data_count)&0xff);
	Send_data[5] = 0x06;
	Send_data[6] = ((client_t->SlaveID)&0xff);
	if(0 == DataMode)  //01H指令功能码//
	{
		Send_data[7] = 0x01;
		Send_data[8] = ((read_data_t->Start_addr)>>5);  //因为这个要将字节单位转换为bit//
		Send_data[9] = (((read_data_t->Start_addr)<<3)&0xff);
		Send_data[10] = ((read_data_t->Read_length)>>5);
		Send_data[11] = (((read_data_t->Read_length)<<3)&0xff);
	}
	else  //03H读取保持寄存器//
	{
		Send_data[7] = 0x03;
		Send_data[8] = ((read_data_t->Start_addr)>>9);  //要将字节单位转换为字单位//
		Send_data[9] = (((read_data_t->Start_addr)>>1)&0xff);
		Send_data[10] = ((read_data_t->Read_length)>>9);
		Send_data[11] = (((read_data_t->Read_length)>>1)&0xff);
	}
	/*将组装好的数据发送给服务器*/
	int write_ret=0,length=0;
	for(;length<12;)
	{
		write_ret = write((client_t->client_fd),&(Send_data[length]),12);
		if(0>write_ret)  //客户端与服务器已经断开了链接//
		{
			free(Send_data);
			return -4;
		}
		length += write_ret;
	}
	free(Send_data);
	Send_data=NULL;
	
	/*2 开始计时等待远端服务器的数据包回复*/
	ret_var = select((client_t->client_fd)+1,&rset,NULL,NULL,&tv);
	if(0 == ret_var)
		return -5;
	else if(0>ret_var)
		return -6;
	else if(0 != FD_ISSET((client_t->client_fd),&rset))  //开始采集回复的数据//
	{
		Read_data = (MBAP_PDU_T*)malloc(sizeof(MBAP_PDU_T));
		if(NULL == Read_data)
			return -3;
		memset(Read_data,0,sizeof(MBAP_PDU_T));
		ret_var = read((client_t->client_fd),Read_data,MAX_RB);
		if(0 >= ret_var)  //客户端与服务器断开了链接//
		{
			free(Read_data);
			return -4;
		}
	}
	
	/*3 开始根据不同的请求数据类型读取数据并返回*/
	ret_var = 0;
	unsigned short SerialNumber;
	unsigned short Proide;
	SerialNumber = Read_data->SerialNumber[0];
	SerialNumber <<= 8;
	SerialNumber |= Read_data->SerialNumber[1];
	Proide = Read_data->Proide[0];
	Proide <<= 8;
	Proide |= Read_data->Proide[1];
	if(0 != Proide)  //接受到的数据包错误或不属于modbustcp协议//
	{
		free(Read_data);
		return -7;
	}
	if((client_t->SlaveID) != (Read_data->Slave_ID))  //非目标服务器回复//
	{
		free(Read_data);
		return -8;
	}
	if(0 == DataMode)  //要判断回复的功能码//
	{
		if(0x01 != (Read_data->Fun_Code))
		{
			free(Read_data);
			return -9;
		}
	}
	else
	{
		if(0x03 != (Read_data->Fun_Code))
		{
			free(Read_data);
			return -9;
		}
	}
	if(SerialNumber != (client_t->data_count))  //提示数据包似乎不是回复此次事件的//
		ret_var = 1;
	
	/*4 读取数据报中有效数据，开始返回给函数调用对象*/
	unsigned char byte_length;
	unsigned char *read_buffer = (unsigned char*)(read_data_t->buffer);
	for(byte_length=0;byte_length<(Read_data->Fun_Data[0]);++byte_length)
		read_buffer[byte_length] = Read_data->Fun_Data[byte_length+1];
	free(Read_data);
	
	(client_t->data_count) ++;
	if(65534 < (client_t->data_count))
		(client_t->data_count) = 0;
	return ret_var;
}
int Readdata_mode_ints(CLIENT_T *client_t,READ_DATA_T *read_data_t)
{
	int ret_var=0;
	if((NULL == client_t) || (NULL == read_data_t))
		return -1;
	if(NULL == (read_data_t->buffer))
		return -2;
	
	unsigned char *Send_data;  //要发送的数据请求报文//
	MBAP_PDU_T *Read_data;  //接收装载服务器发送回来的数据//
	fd_set rset;
	struct timeval tv;
	tv.tv_usec = (client_t->response_time)+(client_t->delay_time);
	FD_ZERO(&rset);
	FD_SET((client_t->client_fd),&rset);
	
	/*1 根据请求，组装报文数据包：03H功能码*/
	Send_data = (unsigned char*)malloc(sizeof(unsigned char)*12);
	if(NULL == Send_data)
		return -3;
	memset(Send_data,0,sizeof(unsigned char)*12);
	Send_data[0] = ((client_t->data_count)>>8);
	Send_data[1] = ((client_t->data_count)&0xff);
	Send_data[5] = 0x06;
	Send_data[6] = ((client_t->SlaveID)&0xff);
	Send_data[7] = 0x03;
	Send_data[8] = ((read_data_t->Start_addr)>>8);
	Send_data[9] = ((read_data_t->Start_addr)&0xff);
	Send_data[10] = ((read_data_t->Read_length)>>8);
	Send_data[11] = ((read_data_t->Read_length)&0xff);
	/*将组装好的数据发送出去*/
	int write_ret=0,length=0;
	for(;length<12;)
	{
		write_ret = write((client_t->client_fd),&(Send_data[length]),12);
		if(0>write_ret)  //客户端与服务器已经断开了链接//
		{
			free(Send_data);
			return -4;
		}
		length += write_ret;
	}
	free(Send_data);
	Send_data=NULL;
	
	/*2 立即等待远端服务器回复数据包*/
	ret_var = select((client_t->client_fd)+1,&rset,NULL,NULL,&tv);
	if(0 == ret_var)
		return -5;
	else if(0>ret_var)
		return -6;
	else if(0 != FD_ISSET((client_t->client_fd),&rset))  //开始采集回复的数据//
	{
		Read_data = (MBAP_PDU_T*)malloc(sizeof(MBAP_PDU_T));
		if(NULL == Read_data)
			return -3;
		memset(Read_data,0,sizeof(MBAP_PDU_T));
		ret_var = read((client_t->client_fd),Read_data,MAX_RB);
		if(0 >= ret_var)  //客户端与服务器断开了链接//
		{
			free(Read_data);
			return -4;
		}
	}
	
	/*3 对远端服务器的回复数据包进行分析*/
	ret_var = 0;
	unsigned short SerialNumber;
	unsigned short Proide;
	SerialNumber = Read_data->SerialNumber[0];
	SerialNumber <<= 8;
	SerialNumber |= Read_data->SerialNumber[1];
	Proide = Read_data->Proide[0];
	Proide <<= 8;
	Proide |= Read_data->Proide[1];
	if(0 != Proide)  //接受到的数据包错误或不属于modbustcp协议//
	{
		free(Read_data);
		return -7;
	}
	if((client_t->SlaveID) != (Read_data->Slave_ID))  //非目标服务器回复//
	{
		free(Read_data);
		return -8;
	}
	if(0x03 != (Read_data->Fun_Code))  //回复的数据包功能码不对应
	{
		free(Read_data);
		return -9;
	}
	if(SerialNumber != (client_t->data_count))  //提示数据包似乎不是回复此次事件的//
		ret_var = 1;
	
	/*4 提取核心数据返回函数的调用者*/
	unsigned char byte_length;
	unsigned short *read_buffer = (unsigned short*)(read_data_t->buffer);
	for(byte_length=0;byte_length<((Read_data->Fun_Data[0])>>1);++byte_length)
	{
		// 大端序直接读取
		read_buffer[byte_length] = (Read_data->Fun_Data[(byte_length<<1)+1] << 8) | 
								Read_data->Fun_Data[(byte_length<<1)+2];
	}
	free(Read_data);
	
	(client_t->data_count)++;
	if(65534 < (client_t->data_count))
		(client_t->data_count) = 0;
	return ret_var;
}
int Readdata_mode_dints(CLIENT_T *client_t,READ_DATA_T *read_data_t)
{
	int ret_var=0;
	if((NULL == client_t) || (NULL == read_data_t))
		return -1;
	if(NULL == (read_data_t->buffer))
		return -2;
	
	unsigned char *Send_data;  //要发送的数据请求报文//
	MBAP_PDU_T *Read_data;  //接收装载服务器发送回来的数据//
	fd_set rset;
	struct timeval tv;
	tv.tv_usec = (client_t->response_time)+(client_t->delay_time);
	FD_ZERO(&rset);
	FD_SET((client_t->client_fd),&rset);
	
	/*1 根据请求，组装报文数据包：03H功能码*/
	Send_data = (unsigned char*)malloc(sizeof(unsigned char)*12);
	if(NULL == Send_data)
		return -3;
	memset(Send_data,0,sizeof(unsigned char)*12);
	Send_data[0] = ((client_t->data_count)>>8);
	Send_data[1] = ((client_t->data_count)&0xff);
	Send_data[5] = 0x06;
	Send_data[6] = ((client_t->SlaveID)&0xff);
	Send_data[7] = 0x03;
	Send_data[8] = ((read_data_t->Start_addr)>>8);
	Send_data[9] = ((read_data_t->Start_addr)&0xff);
	Send_data[10] = ((read_data_t->Read_length)>>7);  //双字转换为字，要乘2//
	Send_data[11] = (((read_data_t->Read_length)<<1)&0xff);
	/*将组装好的数据发送出去*/
	int write_ret=0,length=0;
	for(;length<12;)
	{
		write_ret = write((client_t->client_fd),&(Send_data[length]),12);
		if(0>write_ret)  //客户端与服务器已经断开了链接//
		{
			free(Send_data);
			return -4;
		}
		length += write_ret;
	}
	free(Send_data);
	Send_data=NULL;
	
	/*2 立即等待远端服务器回复数据包*/
	ret_var = select((client_t->client_fd)+1,&rset,NULL,NULL,&tv);
	if(0 == ret_var)
		return -5;
	else if(0>ret_var)
		return -6;
	else if(0 != FD_ISSET((client_t->client_fd),&rset))  //开始采集回复的数据//
	{
		Read_data = (MBAP_PDU_T*)malloc(sizeof(MBAP_PDU_T));
		if(NULL == Read_data)
			return -3;
		memset(Read_data,0,sizeof(MBAP_PDU_T));
		ret_var = read((client_t->client_fd),Read_data,MAX_RB);
		if(0 >= ret_var)  //客户端与服务器断开了链接//
		{
			free(Read_data);
			return -4;
		}
	}
		
	/*3 对远端服务器回复的数据进行分析*/
	ret_var = 0;
	unsigned short SerialNumber;
	unsigned short Proide;
	SerialNumber = Read_data->SerialNumber[0];
	SerialNumber <<= 8;
	SerialNumber |= Read_data->SerialNumber[1];
	Proide = Read_data->Proide[0];
	Proide <<= 8;
	Proide |= Read_data->Proide[1];
	if(0 != Proide)  //接受到的数据包错误或不属于modbustcp协议//
	{
		free(Read_data);
		return -7;
	}
	if((client_t->SlaveID) != (Read_data->Slave_ID))  //非目标服务器回复//
	{
		free(Read_data);
		return -8;
	}
	if(0x03 != (Read_data->Fun_Code))  //回复的数据包功能码不对应
	{
		free(Read_data);
		return -9;
	}
	if(SerialNumber != (client_t->data_count))  //提示数据包似乎不是回复此次事件的//
		ret_var = 1;
		
	/*4 提取核心数据，返回给函数调用者*/
	unsigned char byte_length;
	unsigned int *read_buffer = (unsigned int*)(read_data_t->buffer);
	for(byte_length=0;byte_length<((Read_data->Fun_Data[0])>>2);++byte_length)
	{
		read_buffer[byte_length] = ((Read_data->Fun_Data[(byte_length<<2)+1])<<24);
		read_buffer[byte_length] |= ((Read_data->Fun_Data[(byte_length<<2)+2])<<16);
		read_buffer[byte_length] |= ((Read_data->Fun_Data[(byte_length<<2)+3])<<8);
		read_buffer[byte_length] |= ((Read_data->Fun_Data[(byte_length<<2)+4])&0xff);
	}
	free(Read_data);
	
	(client_t->data_count)++;
	if(65534 < (client_t->data_count))
		(client_t->data_count) = 0;
	return ret_var;
}
/*===============写远程服务器功能函数实现区============*/
int Writedata_mode_bits(CLIENT_T *client_t,const WRITE_DATA_T *write_data_t)
{
	int ret_var=0;
	if((NULL == client_t) || (NULL == write_data_t))
		return -1;
	if(NULL == (write_data_t->buffer))
		return -2;
	
	MBAP_PDU_T *Send_data,*Read_data;
	fd_set rset;
	struct timeval tv;
	tv.tv_usec = (client_t->response_time)+(client_t->delay_time);
	FD_ZERO(&rset);
	FD_SET((client_t->client_fd),&rset);
	
	/*1 提取有关的数据，组装指令数据报,0FH指令*/
	Send_data = (MBAP_PDU_T*)malloc(sizeof(MBAP_PDU_T));
	if(NULL == Send_data)
		return -3;
	memset(Send_data,0,sizeof(MBAP_PDU_T));
	unsigned char *Write_buffer = (unsigned char*)(write_data_t->buffer);
	unsigned char Byte_length = ((write_data_t->Write_length)>>3);
	unsigned char buffer_count;
	if(0 != ((write_data_t->Write_length)%8))
		Byte_length += 1;
	Send_data->SerialNumber[0] = ((client_t->data_count)>>8);
	Send_data->SerialNumber[1] = ((client_t->data_count)&0xff);
	Send_data->PduLength[0] = ((Byte_length+7)>>8);
	Send_data->PduLength[1] = ((Byte_length+7)&0xff);
	Send_data->Slave_ID = ((client_t->SlaveID)&0xff);
	Send_data->Fun_Code = 0x0f;
	Send_data->Fun_Data[0] = ((write_data_t->Start_addr)>>8);
	Send_data->Fun_Data[1] = ((write_data_t->Start_addr)&0xff);
	Send_data->Fun_Data[2] = ((write_data_t->Write_length)>>8);
	Send_data->Fun_Data[3] = ((write_data_t->Write_length)&0xff);
	Send_data->Fun_Data[4] = Byte_length;
	for(buffer_count=5;buffer_count<(Byte_length+5);++buffer_count)
		Send_data->Fun_Data[buffer_count] = Write_buffer[buffer_count-5];
	/*将组装好的数据包发送到远端服务器*/
	int write_ret=0,length=0;
	Write_buffer = (unsigned char*)Send_data;
	for(;length<(13+Byte_length);)
	{
		write_ret = write((client_t->client_fd),&(Write_buffer[length]),(13+Byte_length));
		if(0>write_ret)  //客户端与服务器已经断开了链接//
		{
			free(Send_data);
			return -4;
		}
		length += write_ret;
	}
	
	/*2 立即等待服务器回复数据包*/
	ret_var = select((client_t->client_fd)+1,&rset,NULL,NULL,&tv);
	if(0 == ret_var)
	{
		free(Send_data);
		return -5;
	}
	else if(0>ret_var)
	{
		free(Send_data);
		return -6;
	}
	else if(0 != FD_ISSET((client_t->client_fd),&rset))  //开始采集回复的数据//
	{
		Read_data = (MBAP_PDU_T*)malloc(sizeof(MBAP_PDU_T));
		if(NULL == Read_data)
		{
			free(Send_data);
			return -3;
		}
		memset(Read_data,0,sizeof(MBAP_PDU_T));
		ret_var = read((client_t->client_fd),Read_data,MAX_RB);
		if(0 >= ret_var)  //客户端与服务器断开了链接//
		{
			free(Read_data);
			free(Send_data);
			return -4;
		}
	}
		
	/*3 提取服务器回复的数据，检查数据写入请求是否被正确接收*/
	ret_var = 0;
	unsigned short SerialNumber;
	unsigned short Proide;
	SerialNumber = Read_data->SerialNumber[0];
	SerialNumber <<= 8;
	SerialNumber |= Read_data->SerialNumber[1];
	Proide = Read_data->Proide[0];
	Proide <<= 8;
	Proide |= Read_data->Proide[1];
	if(0 != Proide)  //接受到的数据包错误或不属于modbustcp协议//
	{
		free(Send_data);
		free(Read_data);
		return -7;
	}
	if((client_t->SlaveID) != (Read_data->Slave_ID))  //非目标服务器回复//
	{
		free(Send_data);
		free(Read_data);
		return -8;
	}
	if(0x0f != (Read_data->Fun_Code))  //回复的数据包功能码不对应
	{
		free(Send_data);
		free(Read_data);
		return -9;
	}
	if((Send_data->Fun_Data[1]) != (Read_data->Fun_Data[1]))  //写入数据内存信息校验错误//
	{
		free(Send_data);
		free(Read_data);
		return -10;
	}
	if((Send_data->Fun_Data[3]) != (Read_data->Fun_Data[3]))
	{
		free(Send_data);
		free(Read_data);
		return -10;
	}
	if(SerialNumber != (client_t->data_count))  //提示数据包似乎不是回复此次事件的//
		ret_var = 1;
	free(Send_data);
	free(Read_data);
	
	(client_t->data_count) ++;
	if(65534 < (client_t->data_count))
		(client_t->data_count) = 0;
	return ret_var;
}
int Writedata_mode_bytes(CLIENT_T *client_t,const WRITE_DATA_T *write_data_t,const unsigned char DataMode)
{
	int ret_var=0;
	if((NULL == client_t) || (NULL == write_data_t))
		return -1;
	if(NULL == (write_data_t->buffer))
		return -2;
	
	MBAP_PDU_T *Send_data,*Read_data;
	fd_set rset;
	struct timeval tv;
	tv.tv_usec = (client_t->response_time)+(client_t->delay_time);
	FD_ZERO(&rset);
	FD_SET((client_t->client_fd),&rset);
	
	/*1 根据请求的数据类型，组装服务器请求数据报*/
	Send_data = (MBAP_PDU_T*)malloc(sizeof(MBAP_PDU_T));
	if(NULL == Send_data)
		return -3;
	memset(Send_data,0,sizeof(MBAP_PDU_T));
	unsigned char *Write_buffer = (unsigned char*)(write_data_t->buffer);
	unsigned char Byte_length = 0;
	unsigned char buffer_count;
	Send_data->SerialNumber[0] = ((client_t->data_count)>>8);
	Send_data->SerialNumber[1] = ((client_t->data_count)&0xff);
	Send_data->Slave_ID = ((client_t->SlaveID)&0xff);
	Send_data->Fun_Data[0] = ((write_data_t->Start_addr)>>8);
	Send_data->Fun_Data[1] = ((write_data_t->Start_addr)&0xff);
	Send_data->Fun_Data[2] = ((write_data_t->Write_length)>>8);
	Send_data->Fun_Data[3] = ((write_data_t->Write_length)&0xff);
	if(0 == DataMode)  //请求写入线圈指令，0FH//
	{
		Byte_length = ((write_data_t->Write_length)>>3);
		if(0 != ((write_data_t->Write_length)%8))
			Byte_length += 1;
		Send_data->Fun_Code = 0x0f;
	}
	else  //请求写入寄存器指令，10H//
	{
		Byte_length = ((write_data_t->Write_length)<<1);
		Send_data->Fun_Code = 0x10;
	}
	Send_data->PduLength[0] = ((Byte_length+7)>>8);
	Send_data->PduLength[1] = ((Byte_length+7)&0xff);
	Send_data->Fun_Data[4] = Byte_length;
	for(buffer_count=5;buffer_count<(Byte_length+5);++buffer_count)
		Send_data->Fun_Data[buffer_count] = Write_buffer[buffer_count-5];
	/*将组装好的数据报发送给服务器*/
	int write_ret=0,length=0;
	Write_buffer = (unsigned char*)Send_data;
	for(;length<(13+Byte_length);)
	{
		write_ret = write((client_t->client_fd),&(Write_buffer[length]),(13+Byte_length));
		if(0>write_ret)  //客户端与服务器已经断开了链接//
		{
			free(Send_data);
			return -4;
		}
		length += write_ret;
	}
	
	/*2 立即等待服务器的回复数据报文*/
	ret_var = select((client_t->client_fd)+1,&rset,NULL,NULL,&tv);
	if(0 == ret_var)
	{
		free(Send_data);
		return -5;
	}
	else if(0>ret_var)
	{
		free(Send_data);
		return -6;
	}
	else if(0 != FD_ISSET((client_t->client_fd),&rset))  //开始采集回复的数据//
	{
		Read_data = (MBAP_PDU_T*)malloc(sizeof(MBAP_PDU_T));
		if(NULL == Read_data)
		{
			free(Send_data);
			return -3;
		}
		memset(Read_data,0,sizeof(MBAP_PDU_T));
		ret_var = read((client_t->client_fd),Read_data,MAX_RB);
		if(0 >= ret_var)  //客户端与服务器断开了链接//
		{
			free(Read_data);
			free(Send_data);
			return -4;
		}
	}
	
	/*3 对接收到的服务器的报文进行解析*/
	ret_var = 0;
	unsigned short SerialNumber;
	unsigned short Proide;
	SerialNumber = Read_data->SerialNumber[0];
	SerialNumber <<= 8;
	SerialNumber |= Read_data->SerialNumber[1];
	Proide = Read_data->Proide[0];
	Proide <<= 8;
	Proide |= Read_data->Proide[1];
	if(0 != Proide)  //接受到的数据包错误或不属于modbustcp协议//
	{
		free(Send_data);
		free(Read_data);
		return -7;
	}
	if((client_t->SlaveID) != (Read_data->Slave_ID))  //非目标服务器回复//
	{
		free(Send_data);
		free(Read_data);
		return -8;
	}
	if((Send_data->Fun_Data[1]) != (Read_data->Fun_Data[1]))  //写入数据内存信息校验错误//
	{
		free(Send_data);
		free(Read_data);
		return -10;
	}
	if((Send_data->Fun_Data[3]) != (Read_data->Fun_Data[3]))
	{
		free(Send_data);
		free(Read_data);
		return -10;
	}
	if(0 == DataMode)  //期待回复的数据为0x0fH//
	{
		if(0x0f != (Read_data->Fun_Code))  //回复的数据包功能码不对应
		{
			free(Send_data);
			free(Read_data);
			return -9;
		}
	}
	else
	{
		if(0x10 != (Read_data->Fun_Code))  //回复的数据包功能码不对应
		{
			free(Send_data);
			free(Read_data);
			return -9;
		}
	}
	if(SerialNumber != (client_t->data_count))  //提示数据包似乎不是回复此次事件的//
		ret_var = 1;
	free(Send_data);
	free(Read_data);
	
	(client_t->data_count) ++;
	if(65534 < (client_t->data_count))
		(client_t->data_count) = 0;
	return ret_var;
}
int Writedata_mode_ints(CLIENT_T *client_t,const WRITE_DATA_T *write_data_t)
{
	int ret_var=0;
	if((NULL == client_t) || (NULL == write_data_t))
		return -1;
	if(NULL == (write_data_t->buffer))
		return -2;
	
	MBAP_PDU_T *Send_data,*Read_data;
	fd_set rset;
	struct timeval tv;
	tv.tv_usec = (client_t->response_time)+(client_t->delay_time);
	FD_ZERO(&rset);
	FD_SET((client_t->client_fd),&rset);
	
	/*1 提取重要的数据，组装请求服务器数据报文,10H指令*/
	Send_data = (MBAP_PDU_T*)malloc(sizeof(MBAP_PDU_T));
	if(NULL == Send_data)
		return -3;
	memset(Send_data,0,sizeof(MBAP_PDU_T));
	unsigned short *Write_buffer = (unsigned short*)(write_data_t->buffer);
	unsigned char Byte_length = ((write_data_t->Write_length)<<1);
	unsigned char buffer_count,data_count;
	Send_data->SerialNumber[0] = ((client_t->data_count)>>8);
	Send_data->SerialNumber[1] = ((client_t->data_count)&0xff);
	Send_data->PduLength[0] = ((Byte_length+7)>>8);
	Send_data->PduLength[1] = ((Byte_length+7)&0xff);
	Send_data->Slave_ID = ((client_t->SlaveID)&0xff);
	Send_data->Fun_Code = 0x10;
	Send_data->Fun_Data[0] = ((write_data_t->Start_addr)>>8);
	Send_data->Fun_Data[1] = ((write_data_t->Start_addr)&0xff);
	Send_data->Fun_Data[2] = ((write_data_t->Write_length)>>8);
	Send_data->Fun_Data[3] = ((write_data_t->Write_length)&0xff);
	Send_data->Fun_Data[4] = Byte_length;
	for(buffer_count=5,data_count=0;buffer_count<(Byte_length+5);++data_count)
	{
		Send_data->Fun_Data[buffer_count++] = (Write_buffer[data_count]>>8);
		Send_data->Fun_Data[buffer_count++] = (Write_buffer[data_count]&0xff);
	}
	/*将组装好的数据报发送到服务器*/
	int write_ret=0,length=0;
	unsigned char* byte_buffer = (unsigned char*)Send_data;
	for(;length<(13+Byte_length);)
	{
		write_ret = write((client_t->client_fd),&(byte_buffer[length]),(13+Byte_length));
		if(0>write_ret)  //客户端与服务器已经断开了链接//
		{
			free(Send_data);
			return -4;
		}
		length += write_ret;
	}
	
	/*2 立即等待服务器回复数据报文*/
	ret_var = select((client_t->client_fd)+1,&rset,NULL,NULL,&tv);
	if(0 == ret_var)
	{
		free(Send_data);
		return -5;
	}
	else if(0>ret_var)
	{
		free(Send_data);
		return -6;
	}
	else if(0 != FD_ISSET((client_t->client_fd),&rset))  //开始采集回复的数据//
	{
		Read_data = (MBAP_PDU_T*)malloc(sizeof(MBAP_PDU_T));
		if(NULL == Read_data)
		{
			free(Send_data);
			return -3;
		}
		memset(Read_data,0,sizeof(MBAP_PDU_T));
		ret_var = read((client_t->client_fd),Read_data,MAX_RB);
		if(0 >= ret_var)  //客户端与服务器断开了链接//
		{
			free(Read_data);
			free(Send_data);
			return -4;
		}
	}
	
	/*3 对服务器回复的报文进行解析*/
	ret_var = 0;
	unsigned short SerialNumber;
	unsigned short Proide;
	SerialNumber = Read_data->SerialNumber[0];
	SerialNumber <<= 8;
	SerialNumber |= Read_data->SerialNumber[1];
	Proide = Read_data->Proide[0];
	Proide <<= 8;
	Proide |= Read_data->Proide[1];
	if(0 != Proide)  //接受到的数据包错误或不属于modbustcp协议//
	{
		free(Send_data);
		free(Read_data);
		return -7;
	}
	if((client_t->SlaveID) != (Read_data->Slave_ID))  //非目标服务器回复//
	{
		free(Send_data);
		free(Read_data);
		return -8;
	}
	if(0x10 != (Read_data->Fun_Code))  //回复的数据包功能码不对应
	{
		free(Send_data);
		free(Read_data);
		return -9;
	}
	if((Send_data->Fun_Data[1]) != (Read_data->Fun_Data[1]))  //写入数据内存信息校验错误//
	{
		free(Send_data);
		free(Read_data);
		return -10;
	}
	if((Send_data->Fun_Data[3]) != (Read_data->Fun_Data[3]))
	{
		free(Send_data);
		free(Read_data);
		return -10;
	}
	if(SerialNumber != (client_t->data_count))  //提示数据包似乎不是回复此次事件的//
		ret_var = 1;
	free(Send_data);
	free(Read_data);
	
	(client_t->data_count) ++;
	if(65534 < (client_t->data_count))
		(client_t->data_count) = 0;
	return ret_var;
}
int Writedata_mode_dints(CLIENT_T *client_t,const WRITE_DATA_T *write_data_t)
{
	int ret_var=0;
	if((NULL == client_t) || (NULL == write_data_t))
		return -1;
	if(NULL == (write_data_t->buffer))
		return -2;
	
	MBAP_PDU_T *Send_data,*Read_data;
	fd_set rset;
	struct timeval tv;
	tv.tv_usec = (client_t->response_time)+(client_t->delay_time);
	FD_ZERO(&rset);
	FD_SET((client_t->client_fd),&rset);
	
	/*1 提取核心数据，组装请求服务器的数据报文，10H指令*/
	Send_data = (MBAP_PDU_T*)malloc(sizeof(MBAP_PDU_T));
	if(NULL == Send_data)
		return -3;
	memset(Send_data,0,sizeof(MBAP_PDU_T));
	unsigned int *Write_buffer = (unsigned int*)(write_data_t->buffer);
	unsigned char Byte_length = ((write_data_t->Write_length)<<2);
	unsigned char buffer_count,data_count;
	Send_data->SerialNumber[0] = ((client_t->data_count)>>8);
	Send_data->SerialNumber[1] = ((client_t->data_count)&0xff);
	Send_data->PduLength[0] = ((Byte_length+7)>>8);
	Send_data->PduLength[1] = ((Byte_length+7)&0xff);
	Send_data->Slave_ID = ((client_t->SlaveID)&0xff);
	Send_data->Fun_Code = 0x10;
	Send_data->Fun_Data[0] = ((write_data_t->Start_addr)>>8);
	Send_data->Fun_Data[1] = ((write_data_t->Start_addr)&0xff);
	Send_data->Fun_Data[2] = ((write_data_t->Write_length)>>7);  //将双字转换为单字的长度//
	Send_data->Fun_Data[3] = (((write_data_t->Write_length)<<1)&0xff);
	Send_data->Fun_Data[4] = Byte_length;
	for(buffer_count=5,data_count=0;buffer_count<(Byte_length+5);++data_count)
	{
		Send_data->Fun_Data[buffer_count++] = (Write_buffer[data_count]>>24);
		Send_data->Fun_Data[buffer_count++] = (Write_buffer[data_count]>>16);
		Send_data->Fun_Data[buffer_count++] = (Write_buffer[data_count]>>8);
		Send_data->Fun_Data[buffer_count++] = (Write_buffer[data_count]&0xff);
	}
	/*将组装好的数据报文发送到服务器*/
	int write_ret=0,length=0;
	unsigned char* byte_buffer = (unsigned char*)Send_data;
	for(;length<(13+Byte_length);)
	{
		write_ret = write((client_t->client_fd),&(byte_buffer[length]),(13+Byte_length));
		if(0>write_ret)  //客户端与服务器已经断开了链接//
		{
			free(Send_data);
			return -4;
		}
		length += write_ret;
	}
	
	/*2 立即开始等待接收服务器的回复数据报文*/
	ret_var = select((client_t->client_fd)+1,&rset,NULL,NULL,&tv);
	if(0 == ret_var)
	{
		free(Send_data);
		return -5;
	}
	else if(0>ret_var)
	{
		free(Send_data);
		return -6;
	}
	else if(0 != FD_ISSET((client_t->client_fd),&rset))  //开始采集回复的数据//
	{
		Read_data = (MBAP_PDU_T*)malloc(sizeof(MBAP_PDU_T));
		if(NULL == Read_data)
		{
			free(Send_data);
			return -3;
		}
		memset(Read_data,0,sizeof(MBAP_PDU_T));
		ret_var = read((client_t->client_fd),Read_data,MAX_RB);
		if(0 >= ret_var)  //客户端与服务器断开了链接//
		{
			free(Read_data);
			free(Send_data);
			return -4;
		}
	}
	
	/*3 对服务器回复的数据报文进行解析处理*/
	ret_var = 0;
	unsigned short SerialNumber;
	unsigned short Proide;
	SerialNumber = Read_data->SerialNumber[0];
	SerialNumber <<= 8;
	SerialNumber |= Read_data->SerialNumber[1];
	Proide = Read_data->Proide[0];
	Proide <<= 8;
	Proide |= Read_data->Proide[1];
	if(0 != Proide)  //接受到的数据包错误或不属于modbustcp协议//
	{
		free(Send_data);
		free(Read_data);
		return -7;
	}
	if((client_t->SlaveID) != (Read_data->Slave_ID))  //非目标服务器回复//
	{
		free(Send_data);
		free(Read_data);
		return -8;
	}
	if(0x10 != (Read_data->Fun_Code))  //回复的数据包功能码不对应
	{
		free(Send_data);
		free(Read_data);
		return -9;
	}
	if((Send_data->Fun_Data[1]) != (Read_data->Fun_Data[1]))  //写入数据内存信息校验错误//
	{
		free(Send_data);
		free(Read_data);
		return -10;
	}
	if((Send_data->Fun_Data[3]) != (Read_data->Fun_Data[3]))
	{
		free(Send_data);
		free(Read_data);
		return -10;
	}
	if(SerialNumber != (client_t->data_count))  //提示数据包似乎不是回复此次事件的//
		ret_var = 1;
	free(Send_data);
	free(Read_data);
	
	(client_t->data_count) ++;
	if(65534 < (client_t->data_count))
		(client_t->data_count) = 0;
	return ret_var;
}


/*==================内部功能函数的实现区================*/
static char *GetIP_by_hostname(char *hostname)
{
       struct ifaddrs *ifaddr, *ifa;
       char *host = (char*)malloc(INET_ADDRSTRLEN);
	   if(NULL == host)
			return NULL;
	   memset(host,0,INET_ADDRSTRLEN);
       if (getifaddrs(&ifaddr) == -1)  //函数的作用是:创建一个描述本地系统网络接口的结构链表//
       {
			perror("getifaddrs");
			free(host);
			return NULL;
       }

       for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)  //遍历系统中所有的网卡信息，找到名字与目标网卡对应的设备信息//
       {
           if (ifa->ifa_addr == NULL)
               continue;

           if((strcmp(ifa->ifa_name,hostname)==0)&&(ifa->ifa_addr->sa_family==AF_INET))  //确认获取的IPV4是符合要求的，返回//
           {
               struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
               if(NULL == inet_ntop(AF_INET, &addr->sin_addr, host, INET_ADDRSTRLEN))
               {
                   perror("inet_ntop");
                   break;
               }
               freeifaddrs(ifaddr);
               return host;
           }
       }
	   
	   freeifaddrs(ifaddr);
	   free(host);
	   return NULL;
}
static void delay_us(unsigned int time)
{
	struct timeval timeOut;
	timeOut.tv_sec = 0;
	timeOut.tv_usec = time;
	select(0,NULL,NULL,NULL,&timeOut);
}
