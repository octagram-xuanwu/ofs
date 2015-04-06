#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argc, char *argv[], char **envs)
{
	int rc;

	if (argc < 3) {
		printf("Invalid argument.\n");
		exit(EINVAL);
        }

	rc = renameat2(AT_FDCWD, argv[1], AT_FDCWD, argv[2], RENAME_EXCHANGE);
	printf("rc=%d, errno=%d, %s\n", rc, errno, strerror(errno));
        return errno;
}
