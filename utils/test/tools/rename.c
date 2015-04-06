#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[], char **envs)
{
	int rc;

	if (argc < 3) {
		printf("Invalid argument.\n");
		exit(EINVAL);
        }

	rc = rename(argv[1], argv[2]);
	printf("rc=%d, errno=%d, %s\n", rc, errno, strerror(errno));
        return errno;
}
