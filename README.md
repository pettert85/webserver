# webserver

A simple webserver written in C, created as a school project.

* The name flaskehals was chosen in lack of a better name, it`s norwegian for bottle neck :) 
* The server runs in a Docker built from a scratch image with the /var/www directory inside docker mountet from the host (opt/www). It uses namespaces for added security and cgroups to limit the CPU usage and chroots to /var/ww inside the container, these were also requirements from the project.

*There`s an included script we made also, which compiles the sources as a static file, copies the files,builds the docker image and then runs it. The first argument of this script should be your DockerCloud ID.

*Many of the commands used in regards to docker is well commented at the top of the sourcecode in flaskehals.c. 

*The server runs as a deamon, and after it binds to port 80 it drops it`s root privileges.

*it handles files with extension *.asis which include a http header in the file itself, in addition to all type of files defined in /etc/mime.types file.

*The server also logs some events (far from everything) in log/webserver.log (relative to / )

*All directories, including the root directory shows all files and folders in it, with their permissions and UID, GID.
This was a requirerment in our project and not something one would want to use in a real server, unless you like giving attackers a little push in the right direction :)

*The server handles 404 Not Found aswell, with a correct header response and an HTML file sent back to the client.

**Note that We had to keep the www directory to be mounted in the Docker in /opt/www on the host system to avoid some permission issues on Arch Linux, This did not seem to be a problem in Linux Mint though.**
