#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

int main(int argc, char *argv[])
{
    int fd;
    long ret;

    fd = open("/dev/dm510-0", O_RDWR);
    ret = ioctl(fd, 0);
    printf("Buffersize = %ld\n", ret);

    if (argc > 1)
    {
        ioctl(fd,1,atoi(argv[1]));
    }else
    {
        ioctl(fd, 1, 10000);
    }
    

    ret = ioctl(fd, 0);

    printf("Buffersize = %ld\n", ret);


    // close(fd);

}