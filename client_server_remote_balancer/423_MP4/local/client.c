/**
 * Networking
 * CS 241 - Spring 2017
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>






int send_from_client(int job_id) {
    //Returns char* array in form of {host, port, method, remote, local, NULL}
    double* A = calloc(1024*1024*4,8);

    int num_total = 512;
    char* host = "127.0.0.1";
    char* port = "12345";
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(socket_fd,SOL_SOCKET,SO_REUSEADDR|SO_REUSEPORT, &optval, sizeof(optval));
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; /* IPv4 only */
    hints.ai_socktype = SOCK_STREAM; /* TCP */

    int s = getaddrinfo(host, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo:error %s\n", gai_strerror(s));
        exit(1);
    }

    if(connect(socket_fd, result->ai_addr, result->ai_addrlen) == -1){
        perror("connect\n");
        //exit(1);
    }
    //char* job_id = "200";
    int j_id = job_id;

    printf("job_id %d\n",j_id);
    //write(socket_fd , &job_id, sizeof(job_id));

    //close(socket_fd);
    //printf("Before read: %s\n",response);
    //printf("response: %s\n",response);

    //write(socket_fd , A+8192*j_id, 8192*8);
    write(socket_fd, &j_id, sizeof(int));

    freeaddrinfo(result);

}
