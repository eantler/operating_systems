#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

//defining constant sizes for small/large inputs and threshold for the big sizes.
#define SMALL_SIZE 1024
#define BIG_SIZE 4096
#define THRESHOLD 1048576 // use big size over 1mb
#define READABLE_MIN 32 // mentioned by a student as 32 in the forum and approve by staff!
#define READABLE_MAX 126

// Functions Headers. Function description in next to function's code.
long long computeReadSizeFromArguments(char* inputArgument);
int openFileToRead(char* path);
int openFileToWrite(char* path);
long long getFileLength(int fd);
bool validate(char a);
int writeToFile(int fd, char *inputBuffer, char *outputBuffer, int bytesToRead);
void printStats();

// We want these to be available to both main and other functions;
int bufferOffset = 0;
long long goodChars = 0;
long long badChars = 0;
long long charsWritten = 0;
int BufferSize = SMALL_SIZE;
long long readSize = 0;

int main(int argc, char** argv)
{

  // handling input arguments. Assuming a valid input as mentioned in course forum.
  // only checking to see we have enough arguments to work with.
  if (argc != 4) {
    printf("Error in command line arguments, please use <size_to_read> <input_file> <output_file>\n");
    exit(-1);
  }

  //parsing the input read size i.e how many bytes are we requested to go through
  readSize = computeReadSizeFromArguments(argv[1]);

  //increasing buffer size if needed
  if ( readSize >= THRESHOLD ) {
    BufferSize = BIG_SIZE;
  }

  // defining buffers and opening files to read and write appropriatly.
  // buffers - both are in the same size as I can't astimate the % of "unreadable chars"
  char readBuffer[BufferSize+1];
  char writeBuffer[BufferSize+1];
  int ReadFD = openFileToRead(argv[2]);
  int WriteFD = openFileToWrite(argv[3]);
  int tmp;
  char fakeBuff[3];

  long long fileLength = getFileLength(ReadFD);

  //checking for 0 file length
  if (fileLength == 0) {
    printf("Input file is of length 0. Nothing to do here... Exiting..\n");
    close(ReadFD);
    close(WriteFD);
    exit(0);
  }

  // iterating over the file
  // this is the main thing
  long long location = 0; // our location tracking in the output file
  int readOffset = 0;
  int writeOffset = 0;
  int currReadSize;
  ssize_t currReadValue;
  ssize_t currWrite;
  off_t lseekTempResponse;


  while (location < readSize) {
    // if we're now really close to the end we want to read less
    if ((location+BufferSize)>=readSize) {
      currReadSize = readSize-location;
    } else {
      currReadSize = BufferSize;
    }

    currReadValue = read(ReadFD , readBuffer, currReadSize);

    if (currReadValue == -1) {
      printf("Error trying to read from input file: %s\n", strerror(errno));
      exit(-1);
    }


    if (currReadValue!=currReadSize) { // read didn't return the amount of bites we thought of
      //check if this is EOF
      tmp = read(ReadFD, fakeBuff,1);
      if (tmp==0) { // this is the end of the file, we'll move it to the beginning
        lseekTempResponse = lseek(ReadFD, SEEK_SET, 0);
        if (lseekTempResponse == -1) {
          printf("Something went wrong while working on input file (lseek): %s\n",strerror(errno));
          exit(-1);
        }
      }
    }

    int bytesWritten = writeToFile(WriteFD, readBuffer, writeBuffer, currReadValue);
    location = location+bytesWritten;
    charsWritten = charsWritten + bytesWritten;

  }

  if (bufferOffset > 0) {
    bufferOffset = bufferOffset+1;
    tmp = write(WriteFD, writeBuffer, bufferOffset);
    if (tmp!=bufferOffset) {
      printf("something went wront writing to file: %s",strerror(errno));
    }

    charsWritten = charsWritten + tmp;
  }

  printStats();
  //close files
  close(ReadFD);
  close(WriteFD);

}

//  HELPING FUNCTIONS

// computeReadSizeFromArguments receives argument line string of the format 'NN..NNT'
// when N..N are number letters and T is either B/b K/k M/m G/g representing the units
// the number is referring to.
// The function returnes the number of bytes represented by the argument.
long long computeReadSizeFromArguments(char* inputArgument) {
  int length = strlen(inputArgument);
  char numberString[length-1];
  strncpy(numberString,inputArgument, length-1);
  int number = atoi(numberString);
  char letter = inputArgument[length-1];
  letter = toupper(letter);

  switch (letter) {
    case 'B':
    return number;
    case 'K':
    return 1024*number;
    case 'M':
    return 1024*1024*number;
    case 'G':
    return 1024*1024*1024;
    default:
      printf("Error in read size argument, last letter is invalid. letter is %c\n", letter);
      exit(-1);
  }


}

// openFileToRead received a file path as an argument
// the function returns a file descriptor that allows read only access to that file.
int openFileToRead(char* path) {

  int fd = open(path, O_RDONLY); // opening file for reading

  if (fd<0) {
    printf("Error opening file %s for reading: %s\n",path,strerror(errno));
    exit(-1);
  }

  return fd;
}

// openFileToWrite receives a file path as an argument
// the function returns a file descriptor that allows writing only access to that file
// if file exists it's truncated, else the file is created.
// access privliges: read and write for user, read only for group and others.
int openFileToWrite(char* path) {

  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644); // opening file for reading, user get write permission, group and others are read-only

  if ( fd < 0 ) {
    printf("Error opening file %s for writing: %s\n",path,strerror(errno));
    exit(-1);
  }

  return fd;
}

//getFileLength receive a file descriptor (int)
// the function returns the length of the file.
// -1 for infinite file (i.e /dev/urandom or something of that sort)

long long getFileLength(int fd) {

    // seeking end of file
    off_t fileSize = lseek(fd, SEEK_END,0);
    off_t checkError = lseek(fd, SEEK_SET,0);

    // checking for failiure of lseek
    if (fileSize == ((off_t) -1) || checkError == ((off_t) -1)) {
      printf("Error accessing file: %s\n",strerror(errno));
      exit(-1);
    }

    //cheking for length 0
    if (fileSize == 0) {
      // if it's length 0 then read will return 0 as well. as - infinite
      char fakeBuf[3];
      int readByte = read(fd,fakeBuf,1);

      if (readByte == -1 ) {
        printf("Error accessing file: %s\n",strerror(errno));
        exit(-1);
      }
        //checking file for infinite
       if (readByte != 0) { // 0 is unsigned so all good

        return -1;
      }
    }

    // no error and no length 0 so this is the true length
    return (long long) fileSize;


}

// writeToFile receives a file descriptor, an input and output buffers, length of buffers,  and bytes to read from
// it returnes the number of byts written (to either buffer of file, good or bad chars)
int writeToFile(int fd, char *inputBuffer, char *outputBuffer, int bytesToRead) {
  char letter;
  bool isLetterGood;
  int written = 0;
  ssize_t writeValue;

    // iterate over input buffer
      for (int i = 0; i < bytesToRead; i = i+1) {
        letter = inputBuffer[i];
        isLetterGood = validate(letter);

        // checking if letter is good
        if (isLetterGood) {
          goodChars = goodChars+1;
          written = written+1;
          // we hit a good letter! lets write it to buffer
          // first check if buffer is full
          if (bufferOffset == BufferSize) {
            // buffer is full, flushing....
            writeValue = write(fd, outputBuffer, BufferSize);
            if (writeValue==-1) {
              printf("Error while trying to write to the output file: %s",strerror(errno));
              exit(-1);
            }

            if (writeValue!=BufferSize) { // for some reason write didn't wrote all the bytes we were expecting!!
              printf("Error: Something went wrong while writing to file. This probably caused from system low on resource or interrupt.");
              exit(-1);
            }
            charsWritten = charsWritten + writeValue;
            bufferOffset=0; // set offset to 0
          }

          //writing to buffer incementing offset

          outputBuffer[bufferOffset] = letter;
          bufferOffset = bufferOffset+1;

        } else {
          badChars = badChars+1;
          written = written+1;
        }

        }
        return written;
  }



// validate takes a char and returnes if its valid open
bool validate(char a){
  if (a >= READABLE_MIN && a<=READABLE_MAX) {
    return true;
  }
  return false;
}

// print statistics
void printStats() {
  printf("%lld characters requested, %lld characters read, %lld are printable \n",readSize,goodChars+badChars,goodChars);
}
