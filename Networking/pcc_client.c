#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>

unsigned long int read_client_file(char* path_file){
	unsigned long int  count_bytes = 0;
	FILE* fp;
  	fp = fopen(path_file,"r");
  	if (fp == NULL){
  		printf("\n error in open file \n");
  	}
  	int get_char  = getc(fp);
  	while (get_char != EOF) {
      	count_bytes +=1;
      	get_char = getc(fp);
   	}
   	fclose(fp);
   	return count_bytes;
}
int conect_server(char** argv){
	char buff;
	void* data_buff;
	unsigned long int count = 0;
	unsigned long int bytes_to_send = 0;
	int bytes_write =  0;
	int sockfd = 0;
  	struct sockaddr_in client_addr;
  	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    	printf("\n Error : Could not create socket \n");
    	return 1;
  	}
  	memset(&client_addr, 0, sizeof(client_addr));
  	client_addr.sin_family = AF_INET;
  	client_addr.sin_port = htons(atoi(argv[2]));
  	client_addr.sin_addr.s_addr = inet_addr(argv[1]);
  	if( connect(sockfd,(struct sockaddr*) &client_addr,sizeof(client_addr)) < 0){
    	printf("\n Error : Connect Failed. %s \n", strerror(errno));
    	return 1;
  	}
  	bytes_to_send = read_client_file(argv[3]);
  	FILE* fp;
  	fp = fopen(argv[3],"r");
  	if (fp == NULL)
  		printf("\n Error : Could not open file \n");
  	unsigned long int bytes_to_send_= htonl(bytes_to_send);
  	bytes_write = write(sockfd,&bytes_to_send_,sizeof(bytes_to_send));
  	if( bytes_write == -1 )
  		printf("\n Error : Could not send data");
   	while(count <  bytes_to_send ){
   		buff = getc(fp);
    	bytes_write = write(sockfd,&buff,sizeof(buff));
    	if( bytes_write == -1 )
      		printf("\n Error : Could not send data");
      	count += 1;
  }
  fclose(fp);
  if(read(sockfd,&data_buff,sizeof(data_buff))<0)
  	printf("\n Error : Could not read data");
  unsigned long int to_print = ntohl((unsigned long int )data_buff);
  printf("# of printable characters: %lu\n",to_print);
  close(sockfd);
  return 0;
}

int main(int argc,char* argv[]){
	if (argc != 4){
		perror("not enough agument");
	}
	conect_server(argv);
	return 0;
}
