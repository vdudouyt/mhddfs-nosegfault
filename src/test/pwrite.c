#define _XOPEN_SOURCE 500

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

void usage(void)
{
  printf("usage: pwrite_test file_name\n");
  _exit(0);
}


#define COUNT_WRITE (50*1024*1024)

int main(int argc, char **argv)
{
  if (argc < 2) usage();
  int hfile=open(argv[1], O_CREAT|O_EXCL|O_RDWR);
  if (hfile == -1)
  {
    fprintf(stderr, "Can not create file %s: %s\n",
      argv[1], strerror(errno));
    _exit(-1);
  }

  fchmod(hfile, 0644);
  printf("file %s has been created\n", argv[1]);

  printf("writing %d + %d bytes ... ", COUNT_WRITE, COUNT_WRITE);
  char * buf=malloc(COUNT_WRITE);
  ssize_t write = pwrite(hfile, buf, COUNT_WRITE, COUNT_WRITE);
  
  if (write == -1 || write<COUNT_WRITE)
  {
    printf("%s\n", strerror(errno));
  }
  else
  {
    printf("done: %ld bytes\n", write);
  }
  
  if (unlink(argv[1])==-1)
  {
    printf("Error delete file %s: %s\n",
      argv[1], strerror(errno));
  }
}
