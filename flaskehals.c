/*
########################################################################
#          A small simple webserver made in a school project           #
#          by Benjamin Bråthen and Petter Thorsen                      #
#                              :)                                      #
########################################################################
sudo useradd -r -s /bin/nologin apache
chmod +S --> for sudo. needs root to bind port 80.

#### Basic Docker commands #####
docker build -t USERNAME/webserver .
docker run -p 80:80 --name web USERNAME/webserver
docker run -p 80:80 --name web -it USERNAME/webserver /bin/sh 

##### Mount volumes #########
docker volume create www
docker run -p 80:80 --name web -v www:/var/www USERNAME/webserver
put files in /var/lib/docker/volumes/www/_data/

########### Good reading about my_init ###########
https://blog.phusion.nl/2015/01/20/docker-and-the-pid-1-zombie-reaping-problem/

HEAD
########## CGROUPS - limit CPU usage of docker ####################
docker run -p 80:80 --name web -v www:/var/www --cpus 0.1  petterth/webserver (0.1 = 10 %)
docker stats -> shows

########## Namespaces - How to ##########
Prerequisites:
1. Need to add entries in /etc/subuid and /etc/subgid:
apache:200000:65536 (make sure that the subuid and subgid does not overlap with other entries in these files).
The system will now make a new folder: /var/lib/docker/200000:200000/www/_data 
and you need to copy all the files from the volume you created in the Mount volumes section.

Option 1: 
dockerd --userns-remap="apache:apache" 
This starts the docker daemon with the flag user namespaces and adds a user "testuser" to the system
Warning: Some distributions, such as RHEL and CentOS 7.3, do not automatically add the new group to the /etc/subuid and /etc/subgid files.
You are responsible for editing these files and assigning non-overlapping ranges

Option 2 (Recomended by docker) or copy the daemon.json file from this repository to /etc/docker/:
Edit /etc/docker/daemon.json and add:
{
  "userns-remap": "apache"
}

The docker daemon needs to be reloaded and restarted. In Ubuntu use the following commands:
sudo systemctl daemon reload
sudo systemctl restart docker.service

Note: To use the dockremap user and have Docker create it for you, set the value to default rather than testuser.

For more detailed information see https://docs.docker.com/engine/security/userns-remap/

##########CGROUPS - limit CPU usage of docker.####################
docker run -p 80:80 --name web -v www:/var/www --cpus 0.1  USERNAME/webserver (0.1 = 10 %)
docker stats -> viser bruken
a4c566f337b95d4db43855bf58214286c20b3693

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
const char * path = "/var/www/";
char errorMessage[200];

int errorHandler(char *msg){

      //Log information and perrror to "error.log" file
      time_t t = time(NULL);
      struct tm *tm = localtime(&t);
      char s[64];
      strftime(s, sizeof(s), "%c", tm);

      if(msg == NULL){
      fprintf(stderr, "%s - ip: %s: ",s,inet_ntoa(client_addr.sin_addr));
      perror("ERROR: ");
      }

      else{
      fprintf(stderr, "%s - ip: %s: %s ",s,inet_ntoa(client_addr.sin_addr),msg);
      }

      return 0;
}


int clienthandler(int client_sock){

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


  if( filePointer != NULL) {   //if file exists and can be opened, send it!
   
    if(strcmp(fileExtension,"asis") != 0){  //get header from mime.types file

      char * line = NULL;
      size_t len = 0;
      ssize_t read;
      char *token;
      char *match;
      FILE *mimeFilePointer; 
      char delim[] = "\t\n"; //FML so very very much
      char yeah[400];
      char * mimetype;
      mimeFilePointer = fopen("mime.types","r"); 
      
      while ((read = getline(&line, &len, mimeFilePointer)) != -1) { //read mime.types line by line
        strcpy(yeah,line);
        token = strtok(yeah,delim); //split each line into tokens

        while ( token != NULL ) { 
          if((strcmp(token,fileExtension) ) == 0 ){ //we have match
            //fprintf(stderr, "match: %s and %s - header is: %s\n",token,fileExtension,match );
            mimetype=match;
            break;
          }

          else{ //no match, keep going
          match = strdup(token); //keep previous token for header            
          token = strtok(NULL,delim); //point to next element
          }
        }
      }
      free(line);
      fclose(mimeFilePointer); //close mime.types file

      //char * conttype="Content-Transfer-Encoding: binary"; // must only be used for binary files eg images
      char header[200];
      sprintf(header,"HTTP/1.1 200 OK\r\n %s\n\r\n",mimetype);
      send(client_sock,header,strlen(header),0); //sends the header first header
    }

    //send the file
    char *sendbuf; //buffer
    fseek (filePointer, 0, SEEK_END); //seeks the end of the file
    int fileLength = ftell(filePointer); //total length og the file
    rewind(filePointer); //sets the pointer to start of the file again

    sendbuf = (char*) malloc (sizeof(char)*fileLength); 
   
    size_t result = fread(sendbuf, 1, fileLength, filePointer); //reads the whole file and stores length in result
    send(client_sock, sendbuf, result, 0); //sends the file     
    fclose(filePointer); //Close file again
    return 0;
  }

  else {  //page not found, send 404 error back to client 

    char header[200];
    char *sendbuf; //buffer
    
    FILE *pointer = fopen((const char *)"404.html", "rb");
    if(pointer == NULL){
      //perror("Open 404 page: ");
    }

    sprintf(header,"HTTP/1.1 404 Not Found\r\n text/html\n\r\n");
    send(client_sock,header,strlen(header),0); //sends the header first  

    fseek (pointer, 0, SEEK_END); //seeks the end of the file
    int fileLength = ftell(pointer); //total length og the file
    rewind(pointer); //sets the pointer to start of the file again

    sendbuf = (char*) malloc (sizeof(char)*fileLength); 
    
    size_t result = fread(sendbuf, 1, fileLength, pointer); //reads the whole file and stores length in result
    send(client_sock, sendbuf, result, 0); //sends the file     

    sprintf(errorMessage,"Error 404: The file \"%s.%s\" was not found on this server.\n\n",file,fileExtension);
    errorHandler(errorMessage);
    fclose(pointer);
    fclose(filePointer); //was not found

    
    return -1;

  }
} //receive

//START OF MAIN
int main () {

  //START Demonizing
  if (fork() != 0){ //process dies if not child
    raise(SIGSTOP);
    exit(0);
  }
      
  chdir("/var/www");  //First change work drectory
  chroot("/var/www"); //then chroot

  setsid(); //not attached to the terminal

  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  signal(SIGTSTP,SIG_IGN);

  for(fd=0; fd < sysconf(_SC_OPEN_MAX); fd++){
    close(fd);
  }

  if(fork() != 0){ //END Demonizing
    //raise(SIGSTOP); //process dies if not child
    exit(0);
  }
 
  mkdir("log/", 00777); // create log directory if it does not exist
  char per[] = "log/webserver.log"; //relative to chroot directory
  fd = open(per,O_APPEND | O_CREAT | O_WRONLY,00777);
  dup2(fd,2); //STDERR points to log file
      
  //Setter opp socket-strukturen
  sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  //Prevent system from holding on to a port after termination.
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

  //Initiate local address
  lok_adr.sin_family      = AF_INET;
  lok_adr.sin_port        = htons((u_short)PORT); 
  lok_adr.sin_addr.s_addr = htonl(INADDR_ANY);


  
  //Binds socket and local address
  if ( 0==bind(sd, (struct sockaddr *)&lok_adr, sizeof(lok_adr))  ){
  
  sprintf(errorMessage,"Webserver has pid: %d and is using port %d.\n\n", getpid(), PORT);
  errorHandler((char *) errorMessage);

  }

  else { //something went wrong
    errorHandler(NULL);
    exit(1); 
  }

  /*
  Drop our Root privileges after bind to port 80
  */

  if (getuid() == 0) { // process is running as root, drop privileges
  if (setgid(964) != 0);
  // fatal("setgid: Unable to drop group privileges: %s", strerror(errno));
  if (setuid(964) != 0);
  // fatal("setuid: Unable to drop user privileges: %S", strerror(errno));

  //Waiting for incoming connection
  listen(sd, BAK_LOGG);

  while(1){ 

    //Accepts incoming connection
    client_sock = accept(sd, (struct sockaddr *)&client_addr, &addr_len);    

    if(fork() == 0 ) {
  
    //Log client requests
    sprintf(errorMessage,"New request from %s. \n", inet_ntoa(client_addr.sin_addr) );
    errorHandler(errorMessage);
    //Receive requests and send data to and from client.
    clienthandler(client_sock);

    //Close socket and free fd space
    shutdown(client_sock, SHUT_RDWR);

    //Log closed connections
    sprintf(errorMessage,"Connection to %s closed. \n\n", inet_ntoa(client_addr.sin_addr));
    errorHandler(errorMessage);
    
    exit(0);   

    }//fork()

      
    //The system ignores the signal given by the child upon termination and no zombie is created.
    signal(SIGCHLD,SIG_IGN); 

    //The parent process closes the filedescriptor and returns to wait for incoming connections.
    close(client_sock);
  }//while

  return 0;
  } 
} //main
