#Our build

FROM alpine:3.5

COPY flaskehals /var/www/

ADD html /var/www/html

COPY response.asis /var/www/

COPY tux.png /var/www/

EXPOSE 80

CMD ["/usr/bin/flaskehals"]
