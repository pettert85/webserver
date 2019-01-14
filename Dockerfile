#Our build

FROM alpine:3.5

RUN adduser -D -u 964 -g apache apache

COPY flaskehals /bin/

ADD html /var/www/html

COPY response.asis /var/www/

COPY tux.png /var/www/

EXPOSE 80

CMD /bin/flaskehals


