#Our build

#FROM alpine:3.5

FROM scratch

#ADD rootfs.tar.xz /

#RUN adduser -D -u 964 -g  apache apache

COPY flaskehals /bin/flaskehals

#ADD html /var/www/html

#COPY response.asis /var/www/

#COPY tux.png /var/www/
#WORKDIR /bin

#COPY --from=builder /etc/passwd /etc/passwd
EXPOSE 80


#CMD ["/bin/flaskehals"]
ENTRYPOINT ["/bin/flaskehals"]

