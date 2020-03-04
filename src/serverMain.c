#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

#define bufferSize 1024

int getIntLen (int value){
  int l=1;
  while(value>9){ l++; value/=10; }
  return l;
}

void serveFile(char *filePath, int *newSock){
  char output[bufferSize];
  int code =200;
  int *newSocket = newSock;
  char truePath[strlen(filePath)+4];
  sprintf(truePath,"www/%s",filePath);
  FILE *htmlFile = fopen(truePath, "r");

  bzero(output,bufferSize);

  if(!htmlFile){
    htmlFile = fopen("www/404.html","r");
    if(!htmlFile){
      code = 404;
    }
  }
  if(htmlFile){
    char *htmlContents;
    long htmlSize;
    fseek(htmlFile, 0, SEEK_END);
    htmlSize = ftell(htmlFile);
    rewind(htmlFile);
    htmlContents = malloc((htmlSize+1) * (sizeof(char)));
    fread(htmlContents, sizeof(char), htmlSize, htmlFile);
    htmlContents[htmlSize]='\0';
    fclose(htmlFile);

    int tmp = getIntLen(htmlSize);
    int outputBufferSize = 128+tmp+htmlSize;
    char longOutput[outputBufferSize];
    bzero(longOutput,outputBufferSize);
    if(code==200){
      sprintf(longOutput,"HTTP/1.1 200 OK\r\nContent-length:%ld\r\nContent-type: text/html\r\n\r\n%s",htmlSize,htmlContents);
      write(*newSocket,longOutput,outputBufferSize);
    }
    else{
      sprintf(longOutput,"HTTP/1.1 404 NOT FOUND\r\nContent-length:%ld\r\nContent-type: text/html\r\n\r\n%s",htmlSize,htmlContents);
      write(*newSocket,longOutput,outputBufferSize);
    }
    free(htmlContents);
  }
  else{
    sprintf(output,"HTTP/1.1 404 NOT FOUND\r\n\r\n");
    write(*newSocket,output,bufferSize);
  }
}

void *handleConnection(void *args){
  int *newSocket = (int *) args;
  char buffer[bufferSize];
  int n;

  while(1){
        if(read(*newSocket,buffer,bufferSize-1)<=0){
          fprintf(stderr, "Client disconnected.\n");
          break;
        }

        // Gets actual HTTP request from the incoming data
        int firstLineLength=0;
        while(buffer[firstLineLength]!='\n'){
          firstLineLength++;
        }
        char firstLine[firstLineLength];
        strncpy(firstLine,buffer,firstLineLength);

        // Rejects any non-GET requests
        char *checkGet = strstr(firstLine, "GET");
        if(checkGet){
          // Isolate /path/to/file

          char firstLineCopy[firstLineLength];
          char *filePath;

          strncpy(firstLineCopy,firstLine,firstLineLength);

          filePath= strtok(firstLineCopy," ");
          filePath=strtok(NULL," ");

          // Serve requested resource

          if(strcmp("/",filePath)==0){
            filePath="/index.html";
          }
          serveFile(filePath,newSocket);
        }
        else{
          bzero(buffer,bufferSize);
          fprintf(stderr, "Only HTTP/1.1 GET requests are supported.\n");
          sprintf(buffer,"HTTP/1.1 501 NOT IMPLEMENTED\r\n\r\n");
          write(*newSocket,buffer,bufferSize);

        }

        bzero(buffer,bufferSize);
    }

    free(newSocket);
    pthread_exit(NULL);
}






void exitErr(char *msg){
  fprintf(stderr, "%s\n",msg);
  exit(1);
}





int main(int argc, char *argv[]) {
  char *endp;
  int portnum, sock;
  struct sockaddr_in6 serverAddress, clientAddress;
  socklen_t clientLength;

  // Parse program arguments
  if (argc != 2) {
    exitErr("Incorrect usage. Use:\nserverMain <portnum>\n");
  }
  portnum = strtol (argv[1], &endp, 10);

  if (*endp != '\0') {
    exitErr("Incorrect usage. Use:\nserverMain<portnum>\n");
  }
  if ((portnum <  0) || (portnum > 65535)) {
    exitErr("Illegal port number\n");
  }

  memset (&serverAddress, '\0', sizeof (serverAddress));
  serverAddress.sin6_family = AF_INET6;
  serverAddress.sin6_addr = in6addr_any;
  serverAddress.sin6_port = htons (portnum);

  sock = socket(PF_INET6, SOCK_STREAM, 0);
  if (sock < 0){
    exitErr("Error on opening sock\n");
  }
  if (bind(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0){
    exitErr("Error on binding");
  }
  if(listen (sock,5) <0){
    exitErr("Error on listening");
  }


  // LISTEN LOOP --------------------------------------------
  while(1){
    pthread_t server_thread;
    int *newSocket;

    newSocket = malloc( sizeof(int));

    if(!newSocket){
      exitErr("Error allocating memory for new sock");
    }

    *newSocket = accept(sock, (struct sockaddr *) &clientAddress, &clientLength);
    if(*newSocket<0){
      exitErr("Error on accept");
    }

    if(pthread_create (&server_thread, 0, handleConnection, (void *) newSocket)!=0){
      exitErr("Thread creation failed");
    }

    (void) pthread_join(server_thread,NULL);
  }
  return 0;
}
