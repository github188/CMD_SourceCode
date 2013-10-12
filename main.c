#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>
#include <pthread.h>
#include "cma_commu.h"
#include "device.h"
#include "socket_lib.h"
#include "sensor_ops.h"
#include "zigbee_ops.h"
#include "list.h"
#include "types.h"

char *config_file = NULL;

static void usage(FILE * fp, int argc, char **argv)
{
	fprintf (fp,
	   "Usage: %s [options]\n\n"
	   "Options:\n"
	   "-c | --config        config file \n"
	   "-h | --help          Print this message\n"
	   "", argv[0]);
}

static const char short_options[] = "c:h";

static const struct option long_options[] = {
	{"config", required_argument, NULL, 'c'},
	{"help", no_argument, NULL, 'h'},
	{0, 0, 0, 0}
};

void *tcpServerThrFxn(void * arg)
{
	ServerEnv * envp = (ServerEnv *)arg;
	int socketfd = envp->m_hSocket;
	byte rbuf[MAX_COMBUF_SIZE];
	int ret;

	printf("socke fd = %d\n", socketfd);
	while (1) {
		memset(rbuf, 0, MAX_DATA_BUFSIZE);
		ret = Commu_GetPacket(socketfd, rbuf, MAX_COMBUF_SIZE);
		if (ret == -2) {
			printf("CMD Server receive MSG error!\n");
			break;
		}
		else if (ret < 0)
			continue;

		if (CMA_Server_Process(socketfd, rbuf) < 0) {
			printf("CMD Server Process error.\n");
			continue;
		}
	}

	close_socket(socketfd);
	return 0;
}

void udpServerThrFxn(void)
{
	byte rbuf[MAX_COMBUF_SIZE];
	int ret;

	while (1) {
		memset(rbuf, 0, MAX_DATA_BUFSIZE);
		ret = Commu_GetPacket(-1, rbuf, MAX_COMBUF_SIZE);
		if (ret < 0)
			continue;

		if (CMA_Server_Process(-1, rbuf) < 0) {
			printf("CMD: Server Process error.\n");
			continue;
		}
	}

	return;
}

void *main_sample_loop(void * arg)
{
	int cycle = 10;
	printf("Enter func: %s \n", __func__);
	while (1) {
		/*
		 *  Get Data from Sensor
		 */

		alarm(2);
		printf("RTC timer: %d sec\n", cycle);
		if (rtc_timer(cycle) < 0) {
			printf("CMD: Rtc time error.\n");
			alarm(0);
			usleep(500000);
			continue;
		}
	}

	return 0;
}

void enter_sleep(int sig)
{
//	char *cmd_shell = "echo mem >/sys/power/state";
	printf("Enter func: %s\n", __func__);
//	system(cmd_shell);

	return;
}

int main(int argc, char *argv[])
{
	int index, c;
	int l2_type = 0;
	pid_t pid;
	int s_socket;
	pthread_t pthread;
	int ret;
//	int fd;
//	usint pan_id = 0;

	config_file = CMA_CONFIG_FILE;

	for (;;) {
		c = getopt_long (argc, argv, short_options, long_options, &index);

		if (-1 == c)
			break;
		switch (c) {
		case 0:		/* getopt_long() flag */
			break;
		case 'c':
			config_file = optarg;
			break;
		case 'h':
			usage (stdout, argc, argv);
			exit (EXIT_SUCCESS);
		default:
			usage (stderr, argc, argv);
			return 0;
		}
	}
	
	if (Device_Env_init() < 0){
		printf("CMD: Init Env data error.\n");
		return -1;
	}

	l2_type = CMA_Env_Parameter.l2_type;
	printf("L2 type: %d\n", l2_type);
	switch (l2_type) {
	case 0:
		/* 3G Module init */
		if (Device_W3G_init() < 0)
			return -1;
		break;
	case 1:
		/* Wifi device Init */
		if (Device_wifi_init() < 0)
			return -1;
		break;
	case 2:
		/* Ethernet configuration */
		if (Device_eth_init() < 0)
			return -1;
		break;
	default:
		printf("Invalid L2 interface type.\n");
	}

	printf("Device ID: %s\n", CMA_Env_Parameter.id);

	if (Device_can_init() < 0) {
//		return -1;
	}

	INIT_LIST_HEAD(&s_head);
	Sensor_Scanning();

	if (signal(SIGALRM, enter_sleep) == SIG_ERR) {
		printf("CMD: sinal init error.\n");
		return -1;
	}

	alarm(2);

	/*
	if ((fd = Zigbee_Get_Device()) < 0)
		return -1;

	pan_id = 0x1234;
	Zigbee_Set_PanID(fd, (byte *)&pan_id);
	*/

	pid = fork();
	if(pid == -1) {
		perror("fork error");
		exit(EXIT_FAILURE);
	}
	if(pid == 0){
		if (CMA_Env_Parameter.s_protocal == 0) {
			udpServerThrFxn();
		}
		else {
			if ((s_socket = start_server(CMA_Env_Parameter.local_port, tcpServerThrFxn)) < 0) {
				printf("Start Local Socket Server error.\n");
				return -1;
			}
			close_socket(s_socket);
		}
		exit(0);
	}

	ret = pthread_create(&pthread, NULL, main_sample_loop, NULL);
	if (ret != 0)
		printf("CMD: can't create thread.");

	ret = pthread_join(pthread, NULL);
	if (ret != 0)
		printf("CMD: can't join with thread.");

	return 0;
}

