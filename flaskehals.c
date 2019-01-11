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
#define PORT 8080
#define BAK_LOGG 10 // Størrelse på for kø ventende forespørsler 

//Global variables
struct sockaddr_in lok_adr, client_addr;
int sd, client_sock;
socklen_t addr_len = sizeof(struct sockaddr_in);

int fd;

int errorLog(){

      //log errors to log file
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
  char * file;
 
  //parse request and extract only the filename
  file = strtok(client_request,"GET /");
  file = strtok(file," ");

  //try opening requested file RONLY
  FILE *filePointer = fopen(file, "r");

  //Send file to client if it exists and could be opened
  if( filePointer != NULL) {
    char buf[1000];
    int responseOK = 0;
    //sends file line by line back to client
    while (fgets(buf, sizeof(buf), filePointer) != NULL && responseOK >= 0){
       responseOK = sendResponse(client_sock,buf);
    }      

      fclose(filePointer); //close file again
      return 0;
  }

  //else the file does not exist send 404 error
  else{
    sendResponse(client_sock,"HTTP/1.1 404 NOT FOUND\n");
    fclose(filePointer);
    return -1;
  }
}

//main

int main () {
    
  //STDERR points to  log file
  char per[] = "error.log";
  fd = open(per,O_APPEND | O_CREAT | O_WRONLY,00660);
  dup2(fd,2);
      
  // Setter opp socket-strukturen
  sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  // For at operativsystemet ikke skal holdte porten reservert etter tjenerens død
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

  // Initierer lokal adresse
  lok_adr.sin_family      = AF_INET;
  lok_adr.sin_port        = htons((u_short)PORT); 
  lok_adr.sin_addr.s_addr = htonl(INADDR_ANY);

  // Kobler sammen socket og lokal adresse
  if ( 0==bind(sd, (struct sockaddr *)&lok_adr, sizeof(lok_adr)) )
    fprintf(stderr,"Webserver has pid: %d and is using port %d.\n\n", getpid(), PORT);
  else
    exit(1);

  // Venter på forespørsel om forbindelse
  listen(sd, BAK_LOGG); 
  while(1){ 

    // Aksepterer mottatt forespørsel
    client_sock = accept(sd, (struct sockaddr *)&client_addr, &addr_len);    

    if(0==fork() ) {

      //change STDIN and STDOUT to client


      //log client requests
      fprintf(stderr,"New request from %s. \n", inet_ntoa(client_addr.sin_addr));

      //receive request from client.
      receive(client_sock);

      // close socket and free fd space
      shutdown(client_sock, SHUT_RDWR);

      //log closed connections
      fprintf(stderr,"Connection to %s closed. \n\n", inet_ntoa(client_addr.sin_addr));
      exit(0);
      
    }

    else {
      //foreldreprosessen lukker fd og går tilbake å venter på connections.
      close(client_sock);
    }
  }

  close(fd);
  return 0;
}
