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

DOCKER NETWORKING:
docker network create mynet <--- lager nytt netverk
docker run -itd --network=mynet busybox <--- starter docker med --network=mynet

dette gjør at flere containere kommer på sammme nett og kan referere til hverandre med f.eks curl slik:
curl -X GET http://navnpåkontainer:8888/forfatter

på denne måten slipper man å forholde seg til docker ipadresser som endres fra gang til gang når containerne starter

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
#include <dirent.h>

#define PORT 80
#define BAK_LOGG 10 // Max number of waiting connections

//Global variables
struct sockaddr_in lok_adr, client_addr;
int sd, client_sock, fd,dirfp;
socklen_t addr_len = sizeof(struct sockaddr_in);
const char * path = "/var/www/";
char errorMessage[200];
char *token;
DIR *dir;
char c[2];

int errorHandler(char *msg){

      //Log information and perrror to "error.log" file
      time_t t = time(NULL);
      struct tm *tm = localtime(&t);
      char s[64];
      strftime(s, sizeof(s), "%c", tm);

      if(msg == NULL){
      fprintf(stderr,"%s -  char * filePointerip: %s: ",s,inet_ntoa(client_addr.sin_addr));

      perror("ERROR: ");
      }

      else{
      fprintf(stderr, "%s - ip: %s: %s ",s,inet_ntoa(client_addr.sin_addr),msg);
      }

      return 0;
}//errorhandler()

void directoryListing(char *filsti){
  struct stat stat_buffer;
  struct dirent *ent;

  if ((dir = opendir (filsti)) == NULL) {
    errorHandler(NULL);
    exit(1); 
  }
  
  chdir(filsti);

  char stor[6000];
  sprintf(stor,"<html><head><title>Directory listing</title> \
  <link rel=\"stylesheet\" type=\"text/css\" href=\"/styles.css\">\
  </head><body><center>\
  <h1>Directory listing for: %s </h1>\
  <table id=\"normal\"><tr><th>Permissions</th><th>UID</th><th>GID</th><th>Name</th></tr>\n",filsti );

  send(client_sock,stor,strlen(stor),0); //send start of page

  //Loop through directory and send all entries
  while ((ent = readdir (dir)) != NULL) {

    if (stat (ent->d_name, &stat_buffer) < 0) {
      errorHandler(NULL);
      exit(2);
    }
    
    sprintf(stor,"<tr><td>%o</td><td>%d</td><td>%d</td><td>\
    <a href=\"%s%s\">%s</a></td></tr>\n",stat_buffer.st_mode & 0777, stat_buffer.st_uid,stat_buffer.st_gid, filsti,ent->d_name,ent->d_name);
    send(client_sock,stor,strlen(stor),0);
  } //while 
    
  //send the rest of the page  
  sprintf(stor,"</table>\
  <table cellspacing=\"20\">\
  <tr>\
  <td><p id=\"para\"><img src=\"/images/tux.png\" height=\"150px\"></p></td>\
  <td><p id=\"para\">\
  Q. What is the biggest lie in the entire universe? A. I have read and agree to the Terms and Conditions.</p></td></tr></table>\
  </center></body>\
  </html>");
  
  send(client_sock,stor,strlen(stor),0); 
  closedir (dir);
  close(dirfp);
  chdir("/");
}//directoryListing()

char * mimeTypeHandler( char* fileExtension){
  char * line = NULL;
  size_t len = 0;
  ssize_t read;
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
  return mimetype;
}//mimeTypeHandler()

int sendHandler(int socket, char *URI, char * ext ){
  char header[200];
  char *sendbuf; //buffer
  char *responseCode ="HTTP/1.1 200 OK";
  int length;
  FILE *fp =NULL;
  fp = fopen(URI, "rb");   //try to opening the requested file

  if(fp == NULL && strcmp(URI,"/") != 0 ){
    fp = fopen((const char *)"/404.html", "rb");
    ext = "html";
    responseCode ="HTTP/1.1 404 Not Found";
    sprintf(errorMessage,"Page: %s - 404 not found\n",URI);
    errorHandler(errorMessage);
  }

  //send header first ( unless its an asis file)
  if(strcmp(ext,"asis") != 0){  //get header from mime.types file
    char * mimetype = mimeTypeHandler(ext);
    sprintf(header,"%s\r\n %s\n\r\n",responseCode,mimetype);
    send(client_sock,header,strlen(header),0);
  }

  //check for directory
  sprintf(c,"%c",URI[(strlen(URI)-1)]); //finds last char
  if( (strcmp(c,"/")) == 0 ){
    directoryListing(URI); //handles the rest here
    return 0; 
  }
  
  else{   //finally if it`s not a directory, we send the file

    fseek (fp, 0, SEEK_END); //seeks the end of the file
    length = ftell(fp); //total length og the file
    rewind(fp); //sets the pointer to start of the file again

    sendbuf = (char*) malloc (sizeof(char)*length); 
   
    size_t result = fread(sendbuf, 1, length, fp); //reads the whole file and stores length in result
    send(client_sock, sendbuf, result, 0); //sends the file     
    fclose(fp); //Close file again
    
    return 0;
  }
}//sendHandler()


int clienthandler(int client_sock){
  /*
  Reads requests from clients, splits it into variables
  and handles accordingly to type of request
  */

  char client_request[6000];
  ssize_t read_size;
  char *type,*request,*URI,*fileExtension;

  //Read clients request (GET)
  read_size = recv(client_sock,client_request,6000,0);

  //split request into tokens (variables)
  token = strtok (client_request," ");
  request = token; 
  token = strtok (NULL, " "); //next token
  URI = token;
  token = strtok(NULL," "); //next token
  type = token;    

  if ((dir = opendir (URI)) != NULL ){
    sprintf(c,"%c",URI[(strlen(URI)-1)]); //finds last char in URI
    if( (strcmp(c,"/")) != 0 ){
      char * chr ="/" ;
      char * result = malloc(strlen(URI) + strlen(chr) + 1);
      strcpy(result, URI);

      strcat(result,"/");
      strcpy(URI,result);
    }
    
    fileExtension="html";
    closedir(dir);
  }

  else {
    char ext[200];
    strcpy(ext,URI); //preserve original URI
    token = strtok (ext, "."); //tokenize
    token = strtok(NULL,"."); //next token
    fileExtension = token;     
  }

  sendHandler(client_sock,URI,fileExtension ); //send file
  sprintf(errorMessage,"Requested and Received %s\n",URI);
  errorHandler(errorMessage);
  return 0;
} //clientHandler()


//START OF MAIN
int main () {

  //START Demonizing process
  if (fork() != 0){ //process dies if it`s not the child
    raise(SIGSTOP);
    exit(0);
  }
      
  chdir("/var/www");    //First change work drectory
  chroot("/var/www");   //then we chroot
  setsid();   //not attached to the terminal

  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  signal(SIGTSTP,SIG_IGN);

  for(fd=0; fd < sysconf(_SC_OPEN_MAX); fd++){
    close(fd);
  }

  if(fork() != 0){ //END Demonizing
    exit(0);
  }

  mkdir("log/", 00777); // create log directory if it does not exist
  char per[] = "log/webserver.log"; //relative to chroot directory

  fd = open(per,O_APPEND | O_CREAT | O_WRONLY,00777);
  dup2(fd,2); //dup STDERR our to log file
      
  //Settting uo our socket
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
  Now we drop our Root privileges after we bound to port 80
  */

  if (getuid() == 0) { // process is running as root, drop privileges
    setgid(964);
    setuid(964);
  }

  //Waiting for incoming connection
  listen(sd, BAK_LOGG);
  while(1){ 
    
    //Accept incoming connection
    client_sock = accept(sd, (struct sockaddr *)&client_addr, &addr_len);    
    
    if(fork() == 0 ) {
      errorHandler("New request\n");   //Log all new requests
      clienthandler(client_sock);     //Receive and answer to our clients requests
      shutdown(client_sock, SHUT_RDWR);   //Close socket and free fd space
      errorHandler("connection closed\n\n");  //Log the closed connection
    
      exit(0);   
    }//fork()
      
    //The system ignores the signal given by the child upon termination so no zombie is created.
    signal(SIGCHLD,SIG_IGN); 

    //The parent process closes the socket filedescriptor and returns to wait for incoming connections.
    close(client_sock);
  }//while

  return 1; //should never go here 
} //main()
