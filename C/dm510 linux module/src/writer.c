#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>



/*
 a program that writes a something to the device
 the file to write to should be first arg
 the thing to write should be the second
*/
int main(int argc, char *argv[])
{
    int fd, ret_val, size, fnum;
    void *buf;
    size = (strlen(argv[2])*sizeof(char)); 
    char *file;
    fnum = atoi(argv[1]);

    // get filename
    if (fnum == 0)
    {
        file = "/dev/dm510-0";
    } else
    {
        file = "/dev/dm510-1";
    }

    // open file
    // which file are we writing to
    printf("%s\n",file);
    fd = open(file, O_RDWR);
    if (fd == -1)
    {
        perror("tried to open");
        exit(1);
    }
    

    // write to file
    buf = argv[2];
    ret_val = write(fd,buf,4);
    // error occured
    if (ret_val == -1)
    {
        perror("Tried to write");
        exit(1);
    }
    
    // close the file and terminate
    // close(fd);
    return 0;
}
