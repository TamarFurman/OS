#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
_Bool try_sigint = false;
_Bool proccessing = false;
unsigned long int printible_char[95];


int conect_client(char* argv){
  int total_printible_char = 0;
  char buff;
  unsigned long int totalget = 0;
  unsigned long int nget     = -1;
  int listenfd  = -1;
  int connfd    = -1;

  struct sockaddr_in serv_addr;
  struct sockaddr_in peer_addr;
  socklen_t addrsize = sizeof(struct sockaddr_in );
  listenfd = socket( AF_INET, SOCK_STREAM, 0 );
  memset( &serv_addr, 0, addrsize );
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(atoi(argv));
  int flag2 = 1;  
  if (-1 == setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag2, sizeof(flag2))) {  
       perror("\n Error : Setsocket Failed.");  
  }  
  if( 0 != bind( listenfd,(struct sockaddr*) &serv_addr,addrsize ) ){
    printf("\n Error : Bind Failed. %s \n", strerror(errno));
    return 1;
  }

  if( 0 != listen( listenfd, 10 ) ){
    printf("\n Error : Listen Failed. %s \n", strerror(errno));
    return 1;
  }
  while( 1 ){
  	total_printible_char = 0;
  	totalget = 0;
    connfd = accept( listenfd,(struct sockaddr*) &peer_addr,&addrsize);
    proccessing = true;
    if( connfd < 0 ){
      printf("\n Error : Accept Failed. %s \n", strerror(errno));
      return 1;
    }
	unsigned long int bytes_to_read;
	nget = read(connfd,&bytes_to_read,sizeof(bytes_to_read));
	assert( nget >= 0);
	bytes_to_read = ntohl(bytes_to_read);
    while( totalget <  bytes_to_read){
      if(read(connfd,&buff,sizeof(buff))==-1){
      	exit(1);
      }
      totalget += 1;
      if(buff >=32 && buff<= 126){
      	printible_char[buff-32] += 1;
      	total_printible_char += 1;
      }
    }
    unsigned long int  count = htonl(total_printible_char);
  	if(write(connfd,&count,sizeof(count)) == -1)
  		printf("\n Error : Coudnt write. %s \n", strerror(errno));
  	proccessing = false;
  	if(try_sigint)
		raise(SIGINT);
  	close(connfd);
  }
  
}
void treat_signal(){
	if(proccessing){
		try_sigint = true;
	}
	else{
		for(int i = 0; i<95 ;i++){
			printf("char '%c' : %lu times\n ",i+32,printible_char[i]);
		}
	}
	exit(0);
}
int main(int argc, char* argv[]){
	struct sigaction int_act;
	int_act.sa_handler = treat_signal;
	sigaction(SIGINT,&int_act,NULL);
	if(argc != 2){
		printf("not enough argumants\n");
	}
	conect_client(argv[1]);
	
	return 0;
}
