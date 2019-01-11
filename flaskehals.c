/*
########################################################################
#          A small simple webserver made in a school project           #
#          by Benjamin Bråthen and Petter Thorsen                      #
#                              :)                                      #
########################################################################
*/

#include <arpa/inet.h>
#include <unistd.h> 
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include<sys/wait.h> 
#define PORT 8080
#define BAK_LOGG 10 // Max number of waiting connections

//Global variables
struct sockaddr_in lok_adr, client_addr;
int sd, client_sock;
socklen_t addr_len = sizeof(struct sockaddr_in);

int fd;

int errorLog(){

      //Log errors to log file
      fprintf(stderr, "Client adress: %s: ",inet_ntoa(client_addr.sin_addr));
      perror("ERROR: ");
     
      return 0;
}

int sendResponse(int client_sock, char *fileContent){

//!!!!!!!!!!!!!!!!!!!HUSK OG ENDRE STATEMENT!!!!!!!!!!
  //send innholdet tilbake til klienten
  if (send(client_sock,fileContent,strlen(fileContent),0) == -1 ){
        errorLog();
        
        return -1;
  }
  
  else {return 0;}
}


int receive(int client_sock){

  char client_request[6000];
  ssize_t read_size;

  //Read clients request (GET)
  read_size = recv(client_sock,client_request,6000,0);
  char *file;
  printf("%s\n",client_request ); 

  /*
  strtok splits the string "GET /html/info.asis HTTP/1.1" into separate words:
  Get, /html/info.asis and HTTP/1.1 using escape character. 
  The line " file = strtok (NULL, " "); " moves to the 2nd word. /html/info.asis
  then we simply chop off the first char:
  html/info.asis
  */
  
  file = strtok (client_request," ");
  file = strtok (NULL, " "); //show 2nd word
  char* chopped_file = file + 1; //remove first char /

  FILE *filePointer = fopen(chopped_file, "r");

  //Send file to client if it exists and could be opened
  if( filePointer != NULL) {
    char buf[1000];
    int responseOK = 0;
    //Sends file line by line back to client
    while (fgets(buf, sizeof(buf), filePointer) != NULL && responseOK >= 0){
       responseOK = sendResponse(client_sock,buf);
    }      

      fclose(filePointer); //Close file again
      return 0;
  }

  //Else the file does not exist send 404 error
  else{
    sendResponse(client_sock,"HTTP/1.1 404 NOT FOUND\n");
    fclose(filePointer);
    return -1;
  }
}

//main

int main () {
    
  //STDERR points to log file
  char per[] = "error.log";
  fd = open(per,O_APPEND | O_CREAT | O_WRONLY,00660);
  dup2(fd,2);
      
  //Setter opp socket-strukturen
  sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  //For at operativsystemet ikke skal holdte porten reservert etter tjenerens død
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

  //Initiate local address
  lok_adr.sin_family      = AF_INET;
  lok_adr.sin_port        = htons((u_short)PORT); 
  lok_adr.sin_addr.s_addr = htonl(INADDR_ANY);

  //Binds socket and local address
  if ( 0==bind(sd, (struct sockaddr *)&lok_adr, sizeof(lok_adr)) )
    fprintf(stderr,"Webserver has pid: %d and is using port %d.\n\n", getpid(), PORT);
  else
    exit(1);

  //Waiting for connection
  listen(sd, BAK_LOGG); 
  while(1){ 

    //Accepts incoming connection
    client_sock = accept(sd, (struct sockaddr *)&client_addr, &addr_len);    

    if(0==fork() ) {

      //Change STDIN and STDOUT to client


      //Log client requests
      fprintf(stderr,"New request from %s. \n", inet_ntoa(client_addr.sin_addr));

      //Receive request from client.
      receive(client_sock);

      //Close socket and free fd space
      shutdown(client_sock, SHUT_RDWR);

      //Log closed connections
      fprintf(stderr,"Connection to %s closed. \n\n", inet_ntoa(client_addr.sin_addr));
      exit(0);
      
    }

    else {
      
      //The system ignores the signal given by the child upon termination and no zombie is created.
      signal(SIGCHLD,SIG_IGN); 

      //The parent process closes the filedescriptor and returns to wait for incoming connections.
      close(client_sock);
    }
  }

  close(fd);
  return 0;
}
