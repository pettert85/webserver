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

int getFile(int client_sock){

  char client_request[6000];
  ssize_t read_size;

  //Read clients request (GET)
  read_size = recv(client_sock,client_request,6000,0);

  //parse request and extract only the filename
  /*
  const char *filename = strchr(client_request, '/');
  printf("%s",filename);
  */

  puts(client_request);

  //send file back to client

  //return correct value dependent on file exist or not

  return -1; 

}

int sendResponse(int fd, char * fileContent){

  //send innholdet tilbake til klienten
  int rv = send(fd,fileContent,strlen(fileContent),0);

  if (rv < 0){
    perror("send");
  }
  
  return rv;
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
      if (getFile(client_sock) < 0 ){
        
        //fila finnes ikke,send error beskjed til stderr og til bruker
        sendResponse(client_sock,"HTTP/1.1 404 NOT FOUND\n");
        //error_fd = open("./error.log",O_WRONLY);
        FILE *f = fopen("error.log", "a+");

        //skriver til log fila !!Mangler hendelse (perror)!!
        fprintf(f,"IP adresse: %s : hendelse:  \n", inet_ntoa(client_addr.sin_addr));
        fclose(f);
      }

      else {
        //filen finnes og sendes  ut til klienten
        
        sendResponse(client_sock,"HTTP/1.1 200 OK\n");
      }

      
      /*
      //dup2(client_sock, 1); // redirigerer socket til standard utgang

      asis_fd = open("./response.asis",O_RDONLY);
      //leser inneholdet i .asis fila og sender til socket.
      while ((buf = read(asis_fd,buffer, BUFSIZ)) > 0 ){
        write(1, buffer, buf);
      }
        */

      //lukker fila etter bruk
      //close(asis_fd);

      // Sørger for å stenge socket for skriving og lesing
      // NB! Frigjør ingen plass i fildeskriptortabellen
      shutdown(client_sock, SHUT_RDWR);
      printf("%s Connection to client closed. \n", inet_ntoa(client_addr.sin_addr));
      
      exit(0);


    }

    else {
      //foreldreprosessen lukker fd og går tilbake å venter på connections.
      close(client_sock);
    }
  }
  return 0;
}
