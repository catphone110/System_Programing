/**
 * Networking
 * CS 241 - Spring 2017
 */
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

int read_from_server(){
    // Good luck!
    int s;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    char* port = "12345";

    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    s = getaddrinfo(NULL, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(1);
    }

    if (bind(sock_fd, result->ai_addr, result->ai_addrlen) != 0) {
        perror("bind()");
        exit(1);
    }

    if (listen(sock_fd, 10) != 0) {
        perror("listen()");
        exit(1);
    }

    printf("Waiting for connection...\n");
    int client_fd = accept(sock_fd, NULL, NULL);
    printf("Connection made: client_fd=%d\n", client_fd);

    //for loop until amount of job recieved
    char buffer[8192];
    int len = read(client_fd, (void*)buffer, sizeof(buffer) - 1);
    buffer[len] = '\n';

    printf("romote receving...");
    printf("Read %d chars\n", len);
    printf("===\n");
    printf("%s\n", buffer);
    //write(client_fd,"ok", 3);
    //close (client_fd);
    
    return 0;

   }