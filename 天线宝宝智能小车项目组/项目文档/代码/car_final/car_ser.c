
#include <stdlib.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/un.h>
#include <signal.h>
#include "car.h"

int main(int argc, char *argv[])
{

	if(0 == fork())
	{
		unsigned char up[5] = {0xff, 0x00, 0x01, 0x00, 0xff};
		unsigned char down[5] ={0xff, 0x00, 0x02, 0x00, 0xff};
		unsigned char left[5] ={0xff, 0x00, 0x03, 0x00, 0xff};
		unsigned char right[5] ={0xff, 0x00, 0x04, 0x00, 0xff};
		unsigned char stop[5] ={0xff, 0x00, 0x00, 0x00, 0xff};

		int car_fd = serial_init(DEV_PATH);
		int sockfd, connfd;
		/*1. 创建套接字*/
		if (0 > (sockfd = socket(AF_INET, SOCK_STREAM, 0)))
		{
			perror("socket");
			exit(EXIT_FAILURE);
		}
		printf("socoket........\n");

		/*2. 绑定本机地址和端口号*/
		struct sockaddr_in myaddr;
		memset(&myaddr, 0, sizeof(myaddr));
		myaddr.sin_family = AF_INET;
		myaddr.sin_port = htons(6666);
		myaddr.sin_addr.s_addr = inet_addr("0.0.0.0");
		if (0 > bind(sockfd, (struct sockaddr*)&myaddr, sizeof(myaddr)))
		{
			perror("bind");
			exit(EXIT_FAILURE);
		}
		printf("bind.............\n");

		/*3. 设置监听套接字*/
		if (0 > listen(sockfd, 1024))
		{
			perror("listen");
			exit(EXIT_FAILURE);
		}
		printf("listen..........\n");

		/*4. 接收客户端连接，并生成通信套接字*/
		struct sockaddr_in cliaddr;

		char cmd[10];
		int ret;
		while (1)
		{
			int addrlen = sizeof(cliaddr);
			if (0 > (connfd = accept(sockfd, (struct sockaddr*)&cliaddr, &addrlen)))
			{
				perror("accept");
				exit(EXIT_FAILURE);
			}
			printf("accept: %s\n", inet_ntoa(cliaddr.sin_addr));


			while(1)
			{
				if (0 > (ret = recv(connfd, cmd, sizeof(cmd), 0)))
				{
					perror("recv");
					break;
				}
				else if (0 == ret)
				{
					printf("write close!\n");
					break;
				}
				puts(cmd);
				switch(cmd[0])
				{
					case 'w':
						serial_send_data(car_fd, up, sizeof(up));
						break;
					case 's':
						serial_send_data(car_fd, down, sizeof(down));
						break;
					case 'a':
						serial_send_data(car_fd, left, sizeof(left));
						break;
					case 'd':
						serial_send_data(car_fd, right, sizeof(right));
						break;
					case 'x':
						serial_send_data(car_fd, stop, sizeof(stop));
						break;

				}
			}
			close(connfd);
		}
		close(sockfd);
	}
	else
	{
		int i;
		int fd;
		int ret;
		unsigned int width;
		unsigned int height;
		unsigned int size;
		unsigned int index;
		unsigned int ismjpeg;

		width = 320;
		height = 240;

		int id = socket(AF_INET, SOCK_STREAM, 0);
		if(-1 == id)
		{
			perror("socket");
			return -1;
		}

		//connect to server
		struct sockaddr_in serveraddr = {0}, clientaddr = {0};
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_addr.s_addr = inet_addr("0.0.0.0");
		serveraddr.sin_port = htons(8888);	
		int len = sizeof(serveraddr);

		if(-1 == bind(id, (struct sockaddr *)&serveraddr, len) )
		{
			perror("bind");
			return -1;
		}

		listen(id, 10);
		printf("listening------------!\n");
		while(1)
		{
			int clientid = accept(id, (struct sockaddr *)&clientaddr, &len);
			printf("client IP: %s\n", inet_ntoa(clientaddr.sin_addr) );

			while(1)
			{
				char temp[8]= {0};
				char buf[54+320*240*3] = {0};
				char buf1[20] = {0};
				//		gets(buf);		
				//get buf(CMD) from socket!!!
				if(0 == read(clientid, buf1, sizeof(buf)) )
				{
					printf("client exited !\n");
					break;
				}
				else
					if(0 == strcmp(buf1 , "go" ))
					{
						fd = camera_init("/dev/video0", &width, &height, &size, &ismjpeg);
						if (fd == -1)
							return -1;
						ret = camera_start(fd);
						if (ret == -1)
							return -1;
						// 采集几张图片丢弃 
						char *jpeg_ptr = NULL;
						for (i = 0; i < 8; i++) 
						{
							ret = camera_dqbuf(fd, (void **)&jpeg_ptr, &size, &index);
							if (ret == -1)
								exit(EXIT_FAILURE);

							ret = camera_eqbuf(fd, index);
							if (ret == -1)
								exit(EXIT_FAILURE);
						}

						fprintf(stdout, "init camera success\n");

						while(1)
						{
							int i;	
							//	sleep(1);
							ret = camera_dqbuf(fd, (void **)&jpeg_ptr, &size, &index);
							if (ret == -1)
								return -1;
							//printf("size = %d\n", size);

							//save into a file
							char picname[20] = {0};
							sprintf(picname, "%d.jpg", i);

							int fileid = open(picname, O_RDWR|O_CREAT);
							write(fileid, jpeg_ptr, size);
							close(fileid);

							int fileid1 = open(picname, O_RDWR);
							int len = read(fileid1, buf, size);
							sprintf(temp ,"%d", len);

							ret = write(clientid, temp, 8); 
							if(0 == ret || -1 ==ret)
								break;
							ret = write(clientid, buf, len);
							if(ret == 0 || -1 == ret)
								break;
							close(fileid1);


							ret = camera_eqbuf(fd, index);
							if (ret == -1)
								return -1;
						}

						ret = camera_stop(fd);
						if (ret == -1)
							return -1;

						ret = camera_exit(fd);
						if (ret == -1)
							return -1;
					}

			}

		}
	}
}
