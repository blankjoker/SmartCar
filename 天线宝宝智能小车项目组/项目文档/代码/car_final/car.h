

#ifndef _CAR_

#define _CAR_

#define DEV_PATH "/dev/ttyATH0"

#define	REQBUFS_COUNT	4

struct cam_buf {
	void *start;
	size_t length;
};

struct v4l2_requestbuffers reqbufs;
struct cam_buf bufs[REQBUFS_COUNT];

int serial_init(char *devpath);

int serial_send_data(int fd, char *data_buf, int data_size);

int camera_init(char *devpath, unsigned int *width, unsigned int *height, unsigned int *size, unsigned int *ismjpeg);

int camera_start(int fd);

int camera_dqbuf(int fd, void **buf, unsigned int *size, unsigned int *index);

int camera_eqbuf(int fd, unsigned int index);

int camera_stop(int fd);

int camera_exit(int fd);


#endif