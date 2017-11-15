#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include "queue.c"
#include <signal.h>
//NOT PROCESS YET
# define POLICY_0 (0)
//PROCESSED LOCALLY 
# define POLICY_1 (1)
//PROCESSED REMOTELY
# define POLICY_2 (2)

# define TOTAL_JOB (512)
# define HALF_SIZE (512/2)
# define JOB_SIZE  (8192)
# define READ_BYTES  (65544)

typedef struct{
        int job_id;
        int status; // need initialized zero
        double data[JOB_SIZE]; //chunck of data
}job;
struct addrinfo hints, *result;
job* my_job;
double *A; //data
queue_t * job_queue;
queue_t * final_queue;
static volatile int serverSocket;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; // global variable
pthread_mutex_t SENT = PTHREAD_MUTEX_INITIALIZER; // global variable

int total_sent;
int total_received;
// THIS IS THE FUNCTION FOR RECIEVING & AGGREGATING DATA


void job_initial(){
        my_job = malloc(TOTAL_JOB * sizeof(job));
        total_sent = 0;
        total_received = 0;
        //initialzed the jobs:
        for (int i = 0; i < TOTAL_JOB; i++){
                my_job[i].status = POLICY_0;
                my_job[i].job_id = i;
                for (int j = 0; j < JOB_SIZE; j++){
                        my_job[i].data[j] = 1.111111;
                }


                queue_push(job_queue, &my_job[i]);

        }

        //testing 
        printf("size of queue  = %d \n", job_queue->size);
}


void* sending_function(void* ptr){

        for (int i = 0; i<256 && job_queue->size>1 ; i++){
                        job *send_this_job = (job *) queue_pull(job_queue);
                        write(serverSocket, (void*)send_this_job, sizeof(job));
                        printf("sending job_id = %d \n", send_this_job->job_id );
        }
        freeaddrinfo(result);
        //close(serverSocket);
}

void* receiving_function(void* ptr){
        int running=1;

        //printf("hey i am here");
        while(running){

                int len = 0;
                void *buffer = malloc (sizeof(job));
                while(len < READ_BYTES){
                        //printf("remote is reading...\n");
                        int temp= read(serverSocket, (buffer + len), READ_BYTES - len);
                        if(temp==0){
                                running=0;
                                break;
                        }
                        len+=temp;
                        //printf("Read %d chars \n", len);
                }
                //void *buffer = malloc (sizeof(job));
                //read(serverSocket,buffer, sizeof(job));
                if(running){

                job *rec_data = (job *)buffer;

                printf("received remote id: %d \n", rec_data->job_id);

                //push in r queue
                if(rec_data->status==0){
                queue_push(job_queue, rec_data);
                }else{
                queue_push(final_queue, rec_data);
                }

                }


        }
        freeaddrinfo(result);
        close(serverSocket);
}

void* myWorkingFunction(void* ptr){

        int empty = 0;
        while(!empty){
                job* getJob = queue_pull(job_queue);

                if (getJob == NULL){
                        empty = 1;
                }else {
                        int job_id=getJob->job_id;
                        int start = job_id*JOB_SIZE;
                        int end = (job_id + 1)*JOB_SIZE - 1;

                        pthread_mutex_lock(&m);
                        for (int i = start; i<= end; i++){
                                for (int j=0; j < 6000; j++){
                                        A[i] += 1.111111;
                                }
                        }
                getJob->status=POLICY_1;//status of job
                pthread_mutex_unlock(&m);
                printf("Job processed locally %d\n", job_id);
                queue_push(final_queue, getJob);
                }

        }


}

int run_client(const char*host,const char*port) {
    //Returns char* array in form of {host, port, method, remote, local, NULL}

    serverSocket= socket(AF_INET, SOCK_STREAM, 0);
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; /* IPv4 only */
    hints.ai_socktype = SOCK_STREAM; /* TCP */

    int s = getaddrinfo(host, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo:error %s\n", gai_strerror(s));
        exit(1);
    }

    if(connect(serverSocket, result->ai_addr, result->ai_addrlen) == -1){
        perror("connect\n");
                exit(1);
    }

}


/*
void close_program(int signal) {
  if (signal == SIGINT) {
    shutdown(serverSocket, SHUT_RDWR);
        close(serverSocket);
  }
}*/


int main (int argc, char** argv){
        A = (double*)malloc(sizeof(double)*1024*1024*4);

        //initializing A
        pthread_mutex_lock(&m);
        for (int i = 0; i< 1024*1024*4; i++){
                A[i] = 1.111111;
        }
        pthread_mutex_unlock(&m);
        //queue initialized
        job_queue = queue_create(512);
        final_queue= queue_create(512);
        //job_initial push all jobs in queue
        job_initial();
        //signal(SIGINT, close_program);//set signal handler 
        const char* host = "127.0.0.1";
    const char* port = "12345";

        run_client(host,port);

        pthread_t SENDING_THREAD, LOCALWORKER, RECEIVING_THREAD;

        printf("start sending half data...\n");
        pthread_create(&SENDING_THREAD, NULL, sending_function, NULL);
        printf("finishing sending remote node...\n");
        //run_client(host,port);
        pthread_create(&LOCALWORKER, NULL, myWorkingFunction, NULL);
        pthread_create(&RECEIVING_THREAD, NULL, receiving_function, NULL);

        pthread_join(RECEIVING_THREAD, NULL);
        pthread_join(LOCALWORKER, NULL);
        pthread_join(SENDING_THREAD, NULL);

        printf("final queue has size %d\n",final_queue->size);


        return 0;
}
