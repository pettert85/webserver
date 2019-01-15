#!/bin/bash
#A docker run command to remove containers ability to run KILL command. 
#--rm is added in order to remove and clean up the filesystem after the contianer is closed.
#The best would be to run --cap-drop ALL and --cap-add "Needed capabilities" .
#Replace [USERNAME] with your docker username.
docker run --rm -p 80:80 --name web -v www:/var/www --cpus 0.1 --cap-drop KILL [USERNAME]/webserver