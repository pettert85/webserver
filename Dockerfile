#Our build

#FROM alpine:3.5

FROM scratch
COPY flaskehals /bin/flaskehals
COPY dumb-init_1.2.2_amd64 /bin/dumb-init
EXPOSE 80

#CMD ["/bin/flaskehals"]
ENTRYPOINT ["/bin/dumb-init","/bin/flaskehals"]

