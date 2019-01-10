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
#define PORT 8080
#define BAK_LOGG 10 // Størrelse på for kø ventende forespørsler 

int sendResponse(int client_sock, char *fileContent){

  //send innholdet tilbake til klienten
  printf("sender innholdet tilbake...\n");
  int rv = send(client_sock,fileContent,strlen(fileContent),0);

  if (rv < 0){
    perror("send");
  }
  
  return rv;
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

/*
  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  må lese fila til en char * før den sendes til metoden sendResponse
  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  
*/
  if( filePointer > 0) {
    int c;
    char * string[10000];
    while ((c = getc(filePointer)) != EOF)
        //putchar(c);
    printf("fil finnes\n");
    //char * per = "HTTP/1.1 200 OK\n er";
        
    //sendResponse(client_sock, (char *)filePointer);

    fclose(filePointer); //close file again
    return 0;
  }

  //else the file does not exist send 404 error
  else{
    printf("fil finnes ikke\n");
    //char * per = "HTTP/1.1 404 NOT FOUND\n";
    sendResponse(client_sock,"HTTP/1.1 404 NOT FOUND\n");
    fclose(filePointer);
    return -1;
  }
}




//main

int main ()
{

  struct sockaddr_in  lok_adr, client_addr;
  int sd, client_sock;
  socklen_t addr_len = sizeof(struct sockaddr_in);

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
    printf("Webserver har pid: %d og er knyttet til port %d.\n", getpid(), PORT);
  else
    exit(1);

  // Venter på forespørsel om forbindelse
  listen(sd, BAK_LOGG); 
  while(1){ 

    // Aksepterer mottatt forespørsel
    client_sock = accept(sd, (struct sockaddr *)&client_addr, &addr_len);    

    if(0==fork() ) {

      //skriver ut klientens ip-adresse på stdout
      printf("New connection from IP: %s \n", inet_ntoa(client_addr.sin_addr));

      //lytte etter forespørsel og se om den eksisterer
      if (receive(client_sock) < 0 ){
        
        //fila finnes ikke,send error beskjed til stderr og til bruker
        //sendResponse(client_sock,"HTTP/1.1 404 NOT FOUND\n");
        //error_fd = open("./error.log",O_WRONLY);
        FILE *f = fopen("error.log", "a+");

        //skriver til log fila !!Mangler hendelse (perror)!!
        fprintf(f,"IP adresse: %s : hendelse:  \n", inet_ntoa(client_addr.sin_addr));
        fclose(f);
      }

      else {

        // Sørger for å stenge socket for skriving og lesing
        // NB! Frigjør ingen plass i fildeskriptortabellen
        shutdown(client_sock, SHUT_RDWR);
        printf("%s Connection to client closed. \n", inet_ntoa(client_addr.sin_addr));
        exit(0);
      }
    }

    else {
      //foreldreprosessen lukker fd og går tilbake å venter på connections.
      close(client_sock);
    }
  }
  return 0;
}
