#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[], char *envs[])
{
	int fd, rc;
	char buf[255] = "write to directory.";

	if (argc < 2) {
		printf("Invalid argument.\n");
		exit(EINVAL);
        }

	fd = open(argv[1], O_DIRECTORY | O_NONBLOCK);
	if (fd > 0) {
                rc = write(fd, buf, 255);
                printf("fd=%d, rc=%d, errno=%d, %s\n",
		       fd, rc, errno, strerror(errno));
                close(fd);
        } else {
                printf("can't open %s. error:%d\n", argv[1], fd);
        }
        return 0;
}
