#define SHELL "/bin/sh"
#define _XOPEN_SOURCE 600

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
#include <sys/epoll.h>

#include "DTRACE.h"
#include "readline.c"

int init_server();
int test_client(int clntfd);
void handle_client(int clntfd);

int check_blkLst(int clntfd);
void add_blkLst(int clntfd);

int epfd;
int blkLst[4096];
int blNum = 0;

struct epoll_event evntLst[20];
struct epoll_event epEvnt;

int main(int argc , char *argv[]){
	printf("fd: %d\n",O_EXCL);
	printf("fd: %d\n",O_CREAT);
	printf("fd: %d\n",O_WRONLY);
	printf("fd: %d\n",O_RDONLY);
	printf("fd: %d\n",O_RDWR);
	printf("fd: %d\n",O_TRUNC);
	printf("fd: %d\n",O_NONBLOCK);
	printf("fd: %d\n",O_APPEND);
	printf("fd: %d\n",O_ASYNC);

	printf("fd: %d\n",O_NOCTTY);

	
	
	int sktfd, srcfd;
	

	if ((sktfd = init_server()) < 0)exit(EXIT_FAILURE);
	
	if ((epfd = epoll_create(1)) < 0 ){
		perror("ERROR epoll create\n");
		exit(EXIT_FAILURE);
	}
	
	
	epEvnt.events = EPOLLIN;
	epEvnt.data.fd = sktfd;
	
	if((epoll_ctl(epfd, EPOLL_CTL_ADD,sktfd,&epEvnt)) < 0 ){
		perror("ERROR add client to epoll\n");
		exit(EXIT_FAILURE);
	}
	
	
	
	
	char buf[4096];
	int evq,nread;
	
	while((evq = epoll_wait(epfd,evntLst,8,-1)) > 0 ) {
		for(int i = 0; i < evq; i++){
			     
			srcfd = evntLst[i].data.fd;
			if(check_blkLst(srcfd) == 1){ // check to see if the srcfd has been added to the blacklist
				
			
				   
				   
				if(evntLst[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)){
				printf("fd: %d\n", srcfd);}

			
			
				if(evntLst[i].events & EPOLLIN){ 
				
					if (fcntl(srcfd,F_GETFD)){ // see if fd is already connected
						
						nread = read(srcfd,buf,1);
						if (errno !=EWOULDBLOCK){ 
							if((srcfd%2) == 0) write((srcfd-1),buf, strlen(buf)); //means that file is a ptymfd
							else write((srcfd+1),buf, strlen(buf)); //should work since corresponding fds are added directly  after each other
												   
						}
						
						
					
					
						
		
					
					} else	if((srcfd = accept(sktfd,(struct sockaddr *)NULL, NULL))!= -1 ){//try to accept it
						
						
						if (test_client(srcfd)) handle_client(srcfd);
						fcntl(srcfd,F_SETFD,FD_CLOEXEC);	
			
					} 
				}
				}
		}
	}
		
	
	return EXIT_FAILURE;
}
	
	
int init_server(){
	int sktfd;
	struct sockaddr_in servaddr;
		
	sktfd = socket(AF_INET, SOCK_STREAM, 0);
  	if (sktfd < 0) 
    	perror("ERROR opening socket\n");

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(1234);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sktfd, (struct sockaddr *) &servaddr,
	 	sizeof(servaddr)) < 0) {
		perror("ERROR binding socket\n");
		exit(1);
	}
  	if(fcntl(sktfd, F_SETFL,O_NONBLOCK) < 0){
		perror("ERROR making socket Nonblocking\n");
		exit(1);
	}
	
	if (listen(sktfd,10)< 0){ perror("ERROR listening\n"); }
	printf("Listening on port\n");
	return sktfd;

}

int test_client(int clntfd){
	char *msg;
	
	if (write(clntfd,"<rembash>\n" ,strlen("<rembash>\n")) == -1){
		add_blkLst(clntfd);
		perror("ERROR writing to client\n");
		return -1;
	}
  	if ((msg = readline(clntfd)) == NULL){
		add_blkLst(clntfd);
		perror("ERROR reading shared secret\n");
		return -1;
	}
  	if (strcmp(msg,"PASS\n") != 0) {
		add_blkLst(clntfd);
    	perror("ERROR invalid secret\n");
    	write(clntfd,"<error\n>",strlen("<error\n>"));
		return -1;
    	}
	
	
	return 1;
}



void handle_client(int clntfd){

	
	char *ptyslnm;
	int ptymfd, ptyslfd;
	
	if ((ptymfd = posix_openpt(O_RDWR)) < 0) perror("ERROR opening file \n");
	
	fcntl(ptymfd,F_SETFD,FD_CLOEXEC);
	
	if (grantpt(ptymfd) == -1){
		close(ptymfd);  
		perror("ERROR granting\n");
	}
	if (unlockpt(ptymfd) == -1){
		close(ptymfd);  
		perror("ERROR unlocking\n");
	}
	
	ptyslnm = ptsname(ptymfd);
	printf("%s\n", ptyslnm);
	
	switch (fork()){
	case -1:
		perror("ERROR fork failed\n");
		exit(EXIT_FAILURE);
	case 0:
			
		close(clntfd);
		close(ptymfd);				
	
		if(setsid() == -1)	perror("ERROR set session ID\n");
	
		if((ptyslfd = open(ptyslnm, O_RDWR)) == -1 ){
			perror("ERROR open slave\n");
			exit(EXIT_FAILURE);
		}
	
		if(dup2(ptyslfd,0) == -1 || dup2(ptyslfd,1)  == -1 || dup2(ptyslfd,2)  == -1){
			perror("ERROR duplication socket for stdin/stdout/stderr\n");
			exit(EXIT_FAILURE);
		}
		close(ptyslfd);
			
		
	
		execlp("bash","bash",NULL);
		perror("ERROR exec failed");
		exit(EXIT_FAILURE);
	}
	
	epEvnt.events = EPOLLIN;
	epEvnt.data.fd = clntfd;
	
	if((epoll_ctl(epfd, EPOLL_CTL_ADD,clntfd,&epEvnt)) < 0 ){ // tell epoll to listen for this fd
		perror("ERROR add client to epoll\n");
		exit(EXIT_FAILURE); 
	}
	epEvnt.events = EPOLLIN;
	epEvnt.data.fd = ptymfd;
		
		if((epoll_ctl(epfd, EPOLL_CTL_ADD,ptymfd,&epEvnt)) < 0 ){ // tell epoll to listen for this fd
		perror("ERROR add pty to epoll\n");
		exit(EXIT_FAILURE);
	}
	
	 
	 if (write(clntfd, "<ok>\n", strlen("<ok>\n")) == -1) 
    	perror("ERROR writing to client\n");
	
	printf("new client: %d\n", clntfd);
	printf("new pty: %d\n", ptymfd);
	
	
	
		
}

int check_blkLst(int clntfd){
	for (int i = 0; i<blNum; i++){
		if (clntfd == blkLst[i]) return -1;
	}
	return 1;
}
void add_blkLst(int clntfd){
	blkLst[blNum] = clntfd;
	blNum ++;
}













