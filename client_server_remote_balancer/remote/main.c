#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "queue.c"
#include <signal.h>
#include <time.h>

//NOT PROCESS YET
# define POLICY_0 (0)
//PROCESSED LOCALLY 
# define POLICY_1 (1)
//PROCESSED REMOTELY
# define POLICY_2 (2)

# define TOTAL_JOB   (512)
# define HALF_SIZE   (512/2)
# define JOB_SIZE    (8192)
# define READ_BYTES  (65544)

typedef struct{
	int job_id;	
	int status; // need initialized zero
	double data[JOB_SIZE]; //chunck of data
}job;
struct addrinfo hints, *result;
static volatile int serverSocket;
static volatile int client_fd;
void* remoteWorkingFunction(void* nth);
void* receive_job_function(void* nth);
static volatile double user_throttling =0.5;//default 
queue_t * rjob_queue;
queue_t * finished_job_queue;

void* send_from_remote(void* ptr) {
	//write(client_fd, (void*)finished_job, sizeof(job));

	
	int running=1;
	while(running){

 
		job* finished_job = queue_pull(finished_job_queue);
		if(finished_job==NULL){
			running=0;
			break;
		}
		if(running){
		printf("sending data back... ID = [%d]\n", finished_job->job_id);
    	write(client_fd, (void*)finished_job, sizeof(job));
		}
	
	}


    freeaddrinfo(result);
	close(client_fd);
	return 0;
}
void* send_from_job(void* ptr) {
	//write(client_fd, (void*)finished_job, sizeof(job));

	
	int running=1;
	while(running){

 
		job* extra = queue_pull(rjob_queue);
		if(extra==NULL){
			running=0;
			break;
		}
		if(running){
		printf("sending data back... ID = [%d]\n", extra->job_id);
    	write(client_fd, (void*)extra, sizeof(job));
		}
	
	}


    freeaddrinfo(result);
	close(client_fd);
	return 0;
}
int run_remote(const char* port){
    // Good luck!
    int s;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    s = getaddrinfo(NULL, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        //exit(1);
    }
    
    if (bind(sock_fd, result->ai_addr, result->ai_addrlen) != 0) {
        perror("bind()");
        return 1;
    }
    
    if (listen(sock_fd, 10) != 0) {
        perror("listen()");
        return 1;
    }

    printf("Waiting for connection...\n");
    client_fd = accept(sock_fd, NULL, NULL);
    printf("Connection made: client_fd=%d\n", client_fd);

}

double monitor(){
	double a[4], b[4], loadavg;
    FILE *fp;
    char dump[50];

        fp = fopen("/proc/stat","r");
        fscanf(fp,"%*s %lf %lf %lf %lf",&a[0],&a[1],&a[2],&a[3]);
        fclose(fp);
        sleep(1);

        fp = fopen("/proc/stat","r");
        fscanf(fp,"%*s %lf %lf %lf %lf",&b[0],&b[1],&b[2],&b[3]);
        fclose(fp);

        loadavg = ((b[0]+b[1]+b[2]) - (a[0]+a[1]+a[2])) / ((b[0]+b[1]+b[2]+b[3]) - (a[0]+a[1]+a[2]+a[3]));
        printf("The current CPU utilization is : %lf\n",loadavg);
    

    return loadavg;
}

void state_manager(double cur,int remainingJobs,double estimated_time){
	//get the current setting value
	remainingJobs=rjob_queue->size;
	cur=monitor();
	estimated_time=remainingJobs*0.17;
	 
}
//adaptor
void adaptor(double cur,double estimated_time){
	//if it exceeds limit sleep the worker thread
	if (cur>user_throttling){
		sleep(20);
	}

	//transfering
	//overload
	pthread_t overloader, underloader;
	if(estimated_time>500.0){
	pthread_create(&overloader, NULL, send_from_job, NULL);
	}
	if(estimated_time>100.0){
	pthread_create(&underloader, NULL, receive_job_function, NULL);
	}
	//underload
}


void* remoteWorkingFunction(void* nth){
	int stop=1;
	clock_t start = clock();
 	double time_spent;
 	double c=0.0;
	int r=0;
	double e=0.0;
	while(stop){
		//printf("working...");
		time_spent = (double)(clock() - start) / CLOCKS_PER_SEC;
		//update state info
    		if(time_spent>=5.0){
				start=clock();//resetting counts
				state_manager(c,r,e);
		}
		adaptor(c,e);
		/*
		if(user_throttling!=0){
		long double temp=monitor();
		state_manager(temp);
		}*/
		job * finished_job = queue_pull(rjob_queue);
		
		if (finished_job==NULL){
			stop=0;
		}else{
        int t=finished_job->job_id;
		//printf("pulled : %d  and queue size is %d\n", t,rjob_queue->size);
		//clock_t s = clock();
		finished_job->status = POLICY_2;
		for (int i = 0; i<JOB_SIZE; i++){
			for (int j=0; j < 6000; j++){
				finished_job->data[i] += 1.111111;
			}
		}
		//double onejob = (double)(clock() - s) / CLOCKS_PER_SEC;
		//printf("time used by one is %lf\n",onejob);------by calcution its about 0.173541 we used 0.17here
		printf("remote processed job : %d \n", finished_job->job_id);
		queue_push(finished_job_queue, finished_job);
		
		}
	}
	
	// sending back data
	// CALL SENDING FUNCTION
	// push finshed queue

}

void* receive_job_function(void* nth){
		
	
	int running=1;
	while(running){
		int len = 0;
		void *buffer = malloc (sizeof(job));
		while(len < READ_BYTES){
			//printf("remote is reading...\n");
			int temp= read(client_fd, (buffer + len), READ_BYTES - len);
			if(temp==0){
				running=0;
				break;
			}
			len+=temp;
			//printf("Read %d chars \n", len);
		}
		if(running){
		job *rec_data = (job *)buffer;

		printf("received id: %d and this is the state %d\n", rec_data->job_id, rec_data->status);

		//push in r queue
		queue_push(rjob_queue, rec_data);
		}
	}

	//done closing now
    close (client_fd);
  
}
/*
void handle_sigint(int signal) {
	if(signal==SIGINT){
	char*buffer=NULL;
  	fgets(buffer,0,stdin);
	
	printf("new throttling value: %s\n",buffer);
	}
}
*/
int main (int argc, char** argv){

	if(argc>1){
	user_throttling=atof(argv[1]);
	printf("current throttling value: %lf\n",user_throttling);
	}//get the values of throttling

	rjob_queue = queue_create(512);
	finished_job_queue = queue_create(512);
	//signal(SIGINT, handle_sigint);
   
	const char* port = "12345";
	run_remote(port);

	pthread_t RECEIVE_JOB_THREAD, SENDING_DATA_THREAD;
	
	printf("receiving...\n");
	pthread_create(&RECEIVE_JOB_THREAD, NULL, receive_job_function, NULL);
	printf("done receiving and starting working remotely\n");
	printf("bootstrap phase ends------------------------\n");

	pthread_t remote_working_thread;
	pthread_create(&remote_working_thread, NULL, remoteWorkingFunction, NULL);
	
	printf("aggregation phase starts------------------------\n");
	pthread_create(&SENDING_DATA_THREAD, NULL, send_from_remote, NULL);

	pthread_join(SENDING_DATA_THREAD, NULL);
	pthread_join(remote_working_thread, NULL);
	pthread_join(RECEIVE_JOB_THREAD, NULL);
	return 0;
}

