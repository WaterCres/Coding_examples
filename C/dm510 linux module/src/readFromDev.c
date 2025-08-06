#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>


int main(int argc, char *argv[])
{
    //arg 1: which file to read.
    //arg 2: how much to read.
    if (argc!=3){
        printf("%d are not enough arguments to read: try with 2.", (argc-1));
        return 0;
    }

    void *buf = calloc(4,sizeof(char));
    long count = atoi(argv[2]);
    char *dev;

    // get the file name to read from
    int argi = atoi(argv[1]);
    if (argi == 0){
        dev = "/dev/dm510-0";
    }

    else if (argi == 1)
    {
        dev = "/dev/dm510-1";
    }
    // or not?
    else
    {
        printf("No valid device given.");
        return 0;
    }

    // which device are we reading from
    printf("%s\n",dev);
    // open it
    int fd = open(dev, O_RDWR);
    char *str;

    // read 'count' number of entries 
    while (count > 0) {
       int ret;
       ret = read(fd, buf, 4);
        // error occured
       if (ret == -1) {
           perror("Tried to read: ");
           exit(1);
        }
        count -= 1;
        // print the read string
        str = buf;
        printf("%s \n", str);
    }


    return 0;
}
