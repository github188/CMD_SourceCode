/*
 * sms_function.c
 *
 *  Created on: 2013年12月7日
 *      Author: qinbh
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include "io_util.h"
#include "sms_function.h"
#include "uart_ops.h"
#include "device.h"
#include "types.h"
#include "file_ops.h"

#define SMS_SERIAL_DEV 		"/dev/ttyUSB3"
#define SMS_SERIAL_SPEED 	115200

#define SMS_NEWMESSAGE		"+CMTI"

static ssize_t readn(int fd, void *buf, size_t nbytes, unsigned int timout)
{
	int		nfds;
	fd_set	readfds;
	struct timeval	tv;

	tv.tv_sec = timout;
	tv.tv_usec = 0;
	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);
	nfds = select(fd+1, &readfds, NULL, NULL, &tv);
	if (nfds <= 0) {
		if (nfds == 0)
			errno = ETIME;
		return(-1);
	}
	return(read(fd, buf, nbytes));
}

int Modem_Init(void)
{
	int fd;

	fd = uart_open_dev(SMS_SERIAL_DEV);
	if (fd == -1) {
		perror("Modem port open error");
		return -1;
	}

	uart_set_speed(fd, SMS_SERIAL_SPEED);
	if(uart_set_parity(fd, 8, 1, 'N') == -1) {
		printf ("Set Parity Error");
		return -1;
	}

	return fd;
}

int Modem_SendCmd(int fd, char *buf, int retry)
{
	int i, err;

//	printf("SMS: Send CMD, %s\n", buf);
	for (i = 0; i < retry; i++) {
		err = io_writen(fd, buf, strlen(buf));
		if (err > 0)
			break;
	}
	if (i == retry)
		return -1;
	else
		return 0;
}

int Modem_WaitResponse(int fd, char *expect, int retry)
{
	char rbuf[512];
	int i, nread;

	for (i = 0; i < retry; i++) {
		memset(rbuf, 0, 512);
		nread = readn(fd, rbuf, 512, 5);
		if (nread < 0) {
			if (errno == ETIME)
				continue;
			else {
				perror("Modem read uart");
				return -1;
			}
		}

/*		{
			printf("nread = %d \n", nread);
			int j;
			for (j = 0; j < nread; j++) {
					if ((j % 16) == 0)
							printf("\n");
					printf("0x%x ", rbuf[j]);
			}
			printf("\n");
		} */
		if ((rbuf[2] != 0x5e)) {
			if (strstr(rbuf, expect)) {
				printf("SMS WaitResponse: found string: %s \n", expect);
				return 0;
			}
		}
	}

	return -1;
}

static int SMS_GetPhoneNum(char *msg, char *phone)
{
	int len = strlen(msg);
	char tmp[32];
	int i, j = 0;
	int count = 0;

//	printf("msg: %s \n", msg);

	memset(tmp, 0, 32);
	for (i = 0; i < len; i++) {
		if (count == 1)
			tmp[j++] = msg[i];
		if (msg[i] == ',')
			++count;
	}

	if (count == 0)
		return -1;

	len = strlen(tmp);
	if (tmp[len -1] == ',')
		tmp[len - 1] = '\0';
	memcpy(phone, tmp + 1, strlen(tmp) - 2);
//	printf("SMS Phone Number: %s , len = %d \n", phone, strlen(phone));

	return 0;
}

int SMS_ReadMessage(int fd, int index, char *data, char *phone)
{
	char rbuf[512];
	char cmd[256];
	char *p = NULL, *sp = NULL;
	char* const delim = "\n\n";
	int nread;
	int i;
	int retry = 5;

SMS_Retry:
	printf("SMS Start to read Message, index = %d\n", index);
	memset(cmd, 0, 256);
	sprintf(cmd, "at+cmgr=%d\r", index);

	if (Modem_SendCmd(fd, cmd, 5) < 0)
		return -1;

	for (i = 0; i < 5; i++) {
		memset(rbuf, 0, 512);
		nread = readn(fd, rbuf, 512, 2);
		if (nread < 0) {
			if (errno == ETIME)
				continue;
			else {
				perror("Modem read uart");
				return -1;
			}
		}
		if (rbuf[2] != 0x5e)
			break;
	}
	if (i == 5) {
		if (retry > 0) {
			retry--;
			sleep(2);
			goto SMS_Retry;
		}

		return -1;
	}

//	printf("Receive: num = %d, %s \n", nread, rbuf);
	sp = rbuf;

	while ((p = strsep(&sp, delim)) != NULL) {
		if (strlen(p) > 0) {
//			printf("strsep: %s \n", p);
			break;
		}
	}
//	printf("p = %s\nlen = %d\n", p, strlen(p));
	if (memcmp(p, "+CMGR", 5) == 0) {
		if (SMS_GetPhoneNum(p, phone) < 0) {
			printf("SMS Get Phone Number Error.\n");
			return -1;
		}
		while ((p = strsep(&sp, delim)) != NULL) {
			if (strlen(p) > 0) {
				break;
			}
		}
//		printf("p = %s , len = %d\n", p, strlen(p));
		if (p != NULL)
			memcpy(data, p, strlen(p));
		while ((p = strsep(&sp, delim)) != NULL) {
			if (strlen(p) > 0) {
				break;
			}
		}
		if (memcmp(p, "OK", 2) != 0)
			return -1;
	}
	else
		return -1;

	printf("SMS Read Message: phone = %s, data = %s \n", phone, data);

	return 0;
}

int SMS_SendMessage(int fd, char *phone, char *msg)
{
	char cmd[256];
	char buf[256] = { 0 };

	printf("SMS Start to Send Message, phone = %s, msg = %s\n", phone, msg);
	memset(cmd, 0, 256);
	sprintf(cmd, "AT+CMGS=\"%s\"\r\r", phone);
	if (Modem_SendCmd(fd, cmd, 5) < 0)
		return -1;

	if (Modem_WaitResponse(fd, ">", 5) < 0) {
		printf("SMS Wait > error.\n");
		return -1;
	}

	usleep(10 * 1000);
	memset(buf, 0, 256);
	memcpy(buf, msg, strlen(msg));
	buf[strlen(msg)] = 0x1a;
	if (Modem_SendCmd(fd, buf, 5) < 0)
		return -1;

	if (Modem_WaitResponse(fd, "OK", 20) < 0) {
		printf("SMS Send Message error.\n");
		return -1;
	}

	return 0;
}

int SMS_DelMessage(int fd, int index)
{
	char cmd[256];

	printf("SMS Start to Delet Message, index = %d\n", index);
	memset(cmd, 0, 256);
	sprintf(cmd, "AT+CMGD=%d\r", index);
	if (Modem_SendCmd(fd, cmd, 5) < 0)
		return -1;

	if (Modem_WaitResponse(fd, "OK", 5) < 0) {
		printf("SMS Delet Message error.\n");
		return -1;
	}

	return 0;
}

#define COMMAND_SERVER		"server"
#define COMMAND_RESET		"reset"
#define COMMAND_SETID		"setid"
#define COMMAND_ADDPHONE	"addphone"
#define COMMAND_DELPHONE	"delphone"

#define FILE_SMS_PHONE		"/CMD_Data/.sms_phone.txt"
#define SMS_SUPERPHONE1		"13811187586"
#define SMS_SUPERPHONE2		"18611171185"

static int SMS_AddPhone(char *phone)
{
	int num = 0;
	int i;
	char buf[16] = { 0 };

	num = File_GetNumberOfRecords(FILE_SMS_PHONE, 16);

	for (i = 0; i < num; i++) {
		memset(buf, 0, 16);
		if (File_GetRecordByIndex(FILE_SMS_PHONE, buf, 16, i) == 16) {
			if (strcmp(buf, phone) == 0)
				return 0;
		}
	}

	memset(buf, 0, 16);
	memcpy(buf, phone, strlen(phone));

	return File_AppendRecord(FILE_SMS_PHONE, buf, 16);
}

static int SMS_DelPhone(char *phone)
{
	int num = 0;
	int i;
	char buf[16] = { 0 };

	num = File_GetNumberOfRecords(FILE_SMS_PHONE, 16);

	for (i = 0; i < num; i++) {
		memset(buf, 0, 16);
		if (File_GetRecordByIndex(FILE_SMS_PHONE, buf, 16, i) == 16) {
			if (strcmp(buf, phone) == 0) {
				return File_DeleteRecordByIndex(FILE_SMS_PHONE, 16, i);
			}
		}
	}

	return 0;
}

static int SMS_CheckPhone(char *phone)
{
	int num = 0;
	int i;
	char buf[16] = { 0 };

	printf("Enter func: %s ------\n", __func__);

	if ((strcmp(phone, SMS_SUPERPHONE1) == 0) || (strcmp(phone, SMS_SUPERPHONE2) == 0)) {
		return 0;
	}

	if (File_Exist(FILE_SMS_PHONE) == 0)
		return -1;

	num = File_GetNumberOfRecords(FILE_SMS_PHONE, 16);
	if (num == 0)
		return -1;

	for (i = 0; i < num; i++) {
		memset(buf, 0, 16);
		if (File_GetRecordByIndex(FILE_SMS_PHONE, buf, 16, i) == 16) {
			if (strcmp(buf, phone) == 0)
				return 0;
		}
	}

	return -1;
}

int SMS_CMDProcess(char *data, char *phone)
{
	char *p = NULL, *sp = NULL;
	char* const delim = "+";
	char* const delim2 = ":";
	char *p_cmd = NULL;
	char *p_data = NULL;
	char *p_ip = NULL;
	char *p_port = NULL;

	printf("CMD: Receive Message, phone = %s, data = %s \n", phone, data);

	sp = data;
	p = strsep(&sp, delim);
	p_cmd = p;
	p = strsep(&sp, delim);
	p_data = p;

	printf("CMD: command = %s, data = %s \n", p_cmd, p_data);

	if (strcasecmp(p_cmd, COMMAND_SERVER) == 0) {
		Ctl_up_device_t  up_device;

		printf("SMS: Change server to: %s\n", p_data);
		sp = p_data;
		p = strsep(&sp, delim2);
		p_ip = p;
		p = strsep(&sp, delim2);
		p_port = p;
		printf("Server: ip = %s, port = %d \n", p_ip, atoi(p_port));

		if (Device_getServerInfo(&up_device) < 0)
			return -1;
		up_device.IP_Address = inet_addr(p_ip);
		up_device.Port = atoi(p_port);

		if (Device_setServerInfo(&up_device) < 0)
			return -1;
	}
	else if (strcasecmp(p_cmd, COMMAND_RESET) == 0) {
		printf("SMS: Reset system.\n");
//		Device_reset(0);
		system("/sbin/reboot -d 10 &");
	}
	else if (strcasecmp(p_cmd, COMMAND_SETID) == 0) {
		printf("SMS: Set Device ID to: %s\n", p_data);
		if (strlen(p_data) != 17) {
			printf("SMS: Invalid ID Value.\n");
			return -1;
		}
		if (Device_setId((byte *)p_data, (byte *)CMA_Env_Parameter.c_id, CMA_Env_Parameter.org_id) < 0)
			return -1;
	}
	else if (strcasecmp(p_cmd, COMMAND_ADDPHONE) == 0) {
		printf("SMS: Add Control Phone: %s\n", p_data);
		if ((strcmp(phone, SMS_SUPERPHONE1) != 0) && (strcmp(phone, SMS_SUPERPHONE2) != 0)) {
			printf("SMS: phone %s isn't super user.\n", phone);
		}

		if (SMS_AddPhone(p_data) < 0)
			return -1;
	}
	else if (strcasecmp(p_cmd, COMMAND_DELPHONE) == 0) {
		printf("SMS: Del Control Phone: %s\n", p_data);
		if ((strcmp(phone, SMS_SUPERPHONE1) != 0) && (strcmp(phone, SMS_SUPERPHONE2) != 0)) {
			printf("SMS: phone %s isn't super user.\n", phone);
		}

		if (SMS_DelPhone(p_data) < 0)
			return -1;
	}
	else {
		printf("SMS: Invalid SMS Command.\n");
	}

	return 0;
}

int SMS_ProcessMessage(int fd, char *msg)
{
	int index = 0;
	char *p = NULL;
	char data[512];
	char phone[32];

	p = strrchr(msg, 0x2c);
	if (p != NULL) {
		index = atoi(p+1);
		printf("New Message Index = %d \n", index);
	}
	else
		return -1;

	memset(data, 0, 512);
	memset(phone, 0, 32);
	if (SMS_ReadMessage(fd, index, data, phone) < 0) {
		printf("SMS Read message error.\n");
		return -1;
	}

	if (memcmp(phone, "+86", 3) == 0) {
		int len = strlen(phone);
		memmove(phone, (phone + 3), len -3);
		memset((phone + len - 3), 0, 3);
	}

	if (SMS_CheckPhone(phone) == 0) {
		if (SMS_CMDProcess(data, phone) == 0) {
			SMS_SendMessage(fd, phone, "Command Sucess.");
		}
		else {
			SMS_SendMessage(fd, phone, "Command Failure.");
		}
	}

	index = (index + 1) % 20;
	SMS_DelMessage(fd, index);

	return 0;
}

static void SMS_SetFunction(int fd)
{
	char cmd[256];

	memset(cmd, 0, 256);
	sprintf(cmd, "AT+CNMI=2,1,0,0,0\r");
	Modem_SendCmd(fd, cmd, 5);
	Modem_WaitResponse(fd, "OK", 5);

	memset(cmd, 0, 256);
	sprintf(cmd, "AT+CMGF=1\r");
	Modem_SendCmd(fd, cmd, 5);
	Modem_WaitResponse(fd, "OK", 5);

	memset(cmd, 0, 256);
	sprintf(cmd, "AT+CPMS=\"SM\",\"SM\",\"SM\"\r");
	Modem_SendCmd(fd, cmd, 5);
	Modem_WaitResponse(fd, "OK", 5);

	return;
}
void *SMS_WaitNewMessage(void *arg)
{
	char rbuf[512];
	char* const delim = "\n\n";
	int fd = Modem_Init();
	char *p = NULL, *sp = NULL;
	char *result[5] = { NULL };
	int i = 0, num = 0;
	int nread;

	pthread_detach(pthread_self());

	SMS_SetFunction(fd);

	while (1) {
		if (fd < 0) {
			sleep(10);
			fd = Modem_Init();
			SMS_SetFunction(fd);
			continue;
		}
		memset(rbuf, 0, 512);
		nread = read(fd, rbuf, 512);
		if (nread < 0) {
			perror("Modem read uart");
			close(fd);
			fd = -1;
			continue;
		}

		num = 0;
		sp = rbuf;
		while ((p = strsep(&sp, delim)) != NULL) {
			if (strlen(p) > 0) {
				result[num++] = p;
				if (p[0] != 0x5e)
					printf("p = %s\n", p);
			}
		}

		for (i = 0; i < num; i++) {
			if (memcmp(result[i], SMS_NEWMESSAGE, 5) == 0) {
				fprintf(stdout, "SMS: New Message arrived.\n");
				usleep(100 * 1000);
				if (SMS_ProcessMessage(fd, result[i]) < 0) {
					close(fd);
					fd = -1;
				}
			}
		}

	}

	return 0;
}

int SMS_Init(void)
{
	pthread_t pid;
	int ret;

	ret = pthread_create(&pid, NULL, SMS_WaitNewMessage, NULL);
	if (ret != 0)
		printf("CMD: can't create SMS thread.");

	return ret;
}



