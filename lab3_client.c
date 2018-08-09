#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#include "DTRACE.h"
#include "readline.c"



int main(int argc , char *argv[]){
	
	int sktfd,nread,cpid;
	struct sockaddr_in servaddr;
	struct hostent *hostp;
	char outbuff[256], inbuff[256];
	char *msg;
	
	sktfd = socket(AF_INET,SOCK_STREAM, 0);
	if (sktfd < 0) 
    	perror("ERROR opening socket:\n");

	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(1234);
	inet_aton("127.0.0.1",&servaddr.sin_addr);

	hostp = gethostbyname("127.0.0.1");
	if(hostp == ( struct hostent *) NULL) perror("ERROR host not found:\n");

	
	if (connect(sktfd, (struct sockaddr *)&servaddr, 
		sizeof(servaddr)) < 0) 
		perror("ERROR connecting\n");

	printf("Connected to port\n");
	
	
	if ((msg = readline(sktfd)) == NULL) {
    	if (errno)	perror("ERROR reading from server:\n");
    	else		fprintf(stderr,"Server connection closed\n");
    	exit(EXIT_FAILURE); }
		
		
  	if (strcmp(msg,"<rembash>\n") != 0) {
    	perror("ERROR Protocol ID is Invalid:\n");
    	exit(EXIT_FAILURE); }

  	if (write(sktfd,"PASS\n",strlen("PASS\n")) == -1) {
    	perror("ERROR writing shared secret:\n");
    	exit(EXIT_FAILURE); }
	
	
  	if ((msg = readline(sktfd)) == NULL) {
    	if (errno)	perror("ERROR reading acknowledgement:\n");
		else	 	fprintf(stderr,"Server connection closed\n");
    	exit(EXIT_FAILURE); }
	
  	if (strcmp(msg,"<ok>\n") != 0) {
    	perror("ERROR Invalid aknowledgement:\n");
    	exit(EXIT_FAILURE); }
	
	printf("recieved 'OK' \n");
	switch (cpid = fork()){
	case -1:
    	perror("ERROR fork() call failed:\n");
    	exit(EXIT_FAILURE);

  	case 0: 
    	while (fgets(outbuff,256,stdin) != NULL) {
			
      		if (write(sktfd,outbuff,strlen(outbuff)) == -1) break;
    	}

    	if (errno) {	perror("ERROR read write loop terminated:\n");
      		exit(EXIT_FAILURE); }
    	exit(EXIT_SUCCESS);
  	}
	
	
	while ((nread = read(sktfd,outbuff,4096)) != -1) {
		
      	if (write(1,outbuff,nread) == -1)	break;
	
  	}
	printf("exiting");
	kill(cpid,SIGTERM);
  	wait(NULL);
	exit(EXIT_SUCCESS);

}


