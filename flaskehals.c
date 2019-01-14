/*
########################################################################
#          A small simple webserver made in a school project           #
#          by Benjamin Bråthen and Petter Thorsen                      #
#                              :)                                      #
########################################################################
sudo useradd -r -s /bin/nologin apache
chmod +S --> for sudo. needs root to bind port 80.
Drops the privileges again after bind

Must change GID,UID --> needs to be dynamically loaded 
sudo ./flaskehals

####DOCKER commands#####
docker build -t USERNAME/webserver .
docker run -p 80:80 --name web -it USERNAME/webserver /bin/sh --privileged
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
#include <sys/wait.h> 
#include <signal.h>
#include <errno.h>

#define PORT 80
#define BAK_LOGG 10 // Max number of waiting connections

//Global variables
struct sockaddr_in lok_adr, client_addr;
int sd, client_sock, fd;
socklen_t addr_len = sizeof(struct sockaddr_in);


int errorLog(){

      //Log information and perrror to "error.log" file
      fprintf(stderr, "Client adress: %s: ",inet_ntoa(client_addr.sin_addr));
      perror("ERROR: ");
     
      return 0;
}

int sendResponse(int client_sock, char *fileContent){

  //Sends data from *fileContent back through client_sock
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
  char *fileExtension;

  /*
  strtok splits the string "GET /html/info.asis HTTP/1.1" into separate words:
  Get, /html/info.asis and HTTP/1.1 using escape character. 
  The line " file = strtok (NULL, " "); " moves to the 2nd word. /html/info.asis
  then we simply chop off the first char so it looks like:
  html/info.asis
  */

  file = strtok (client_request," ");
  file = strtok (NULL, " "); //show 2nd word
  file = file + 1; //remove first char /

  //try to opening the requested file
  FILE *filePointer = fopen(file, "rb");

  //split the string some more to find correct file extension
  fileExtension = strtok(file,".");
  fileExtension = strtok(NULL,".");

  //Send file to client if it exists and could be opened
  if( filePointer != NULL) {
   
    if(strcmp(fileExtension,"asis") != 0){
      //Send correct header first -  Must use variables here to change mime type and http 200 ok to other values.
      char * mimetype="text/html"; // needs to be changed by lookup in /etc/mimetypes
      char * conttype="Content-Transfer-Encoding: binary"; // must only be used for binary files eg images
      char header[200];
      sprintf(header,"HTTP/1.1 200 OK\r\n Content-Type: %s\r\n %s\n\r\n",mimetype,conttype);
      send(client_sock,header,strlen(header),0); //sends the header first header
    }

    //send file
    char *sendbuf; //buffer
    fseek (filePointer, 0, SEEK_END); //seeks the end of the file
    int fileLength = ftell(filePointer); //total length og the file
    rewind(filePointer); //sets the pointer to start of the file again

    sendbuf = (char*) malloc (sizeof(char)*fileLength); 
   
    size_t result = fread(sendbuf, 1, fileLength, filePointer); //reads the whole file and stores length in result
    send(client_sock, sendbuf, result, 0); //sends the file     
    
    //-----------------------------------------------------------

    //Sends file line by line back to client
    /*
    
    char buf[1000];
    int responseOK = 0;

    while (fgets(buf, sizeof(buf), filePointer) != NULL && responseOK >= 0){
       responseOK = sendResponse(client_sock,buf);
    } 
    */     

      fclose(filePointer); //Close file again
      return 0;
  }

  
  else{ //The file does not exist send 404 back to client
    sendResponse(client_sock,"HTTP/1.1 404 NOT FOUND\n");
    fclose(filePointer);
    return -1;
  }
}

//main

int main () {

  //START Demonizing
  if (fork() == 0){

  setsid(); //not attached to the terminal

  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  signal(SIGTSTP,SIG_IGN);

  for(fd=0; fd < sysconf(_SC_OPEN_MAX); fd++){
    close(fd);
  }

  if(fork() == 0){
    
  //END Demonizing

  //STDERR points to log file
  char per[] = "/var/log/flaskehals.log";
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

  else { //something went wrong
    errorLog();
    exit(1); 
  }

  /*
  Drop our Root privileges after bind to port 80
  */

  if (getuid() == 0) {
    /* process is running as root, drop privileges */
    if (setgid(964) != 0);
       // fatal("setgid: Unable to drop group privileges: %s", strerror(errno));
    if (setuid(964) != 0);
       // fatal("setuid: Unable to drop user privileges: %S", strerror(errno));
  
}


  //Waiting for incoming connection
  listen(sd, BAK_LOGG);

  while(1){ 

    //Accepts incoming connection
    client_sock = accept(sd, (struct sockaddr *)&client_addr, &addr_len);    

    if(0==fork() ) {

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
    else{
      exit(0);
    }

  }

  //demonizing
  else{
    raise(SIGSTOP);
    //exit(0);
  }


} //main
