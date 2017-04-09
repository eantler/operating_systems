#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h> // for open flags
#include <unistd.h>
#include <time.h> // for time measurement
#include <sys/time.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

// list contents of directory
int main(int argc, char** argv)
{
    // assert first argument filename
    assert ( argc == 2 );

    struct timeval start, end;
    long mtime, seconds, useconds;

    struct dirent *dp;
    DIR *dfd;

    char *dir ;
    dir = argv[1] ;

    // open directory stream
    if ((dfd = opendir(dir)) == NULL)
    {
      printf("Error opening directory: %s\n", strerror(errno));
      return -1; // ERROR!
    }

    char filename_qfd[100] ;

    // read stream entries
    printf("list directory contents\n");

    // start time measurement
    if (gettimeofday(&start, NULL) != 0)
    {
      printf("Error getting time: %s\n", strerror(errno));
      return -1;
    }

    while ((dp = readdir(dfd)) != NULL)
    {
        // full path to file
        sprintf(filename_qfd, "%s/%s", dir,dp->d_name) ;

        // call stat to get file metadata
        struct stat statbuf ;
        if (stat(filename_qfd,&statbuf ) < 0)
        {
          printf("Error getting stat of file: %s\n", strerror(errno));
          return -1; // ERROR!
        }

        // skip directories
        if (!S_ISDIR(statbuf.st_mode))
        {
          printf("%s\n", filename_qfd);
        }
    }

    closedir(dfd);

    // end time measurement and print result
    if (gettimeofday(&end, NULL) != 0)
    {
      printf("Error getting time: %s\n", strerror(errno));
      return -1;
    }

    seconds  = end.tv_sec  - start.tv_sec;
    useconds = end.tv_usec - start.tv_usec;
    mtime = (seconds*1000 + useconds/1000.0);
    printf("Elapsed time: %.3ld milliseconds (%ld useconds)\n", mtime, useconds);
}
