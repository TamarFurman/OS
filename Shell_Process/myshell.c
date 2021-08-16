#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h> 

void print_dir(){
	char cwd[1024];
    printf("\e[1;36m%s\033[0m:\033[1;34m~%s\033[0m$ ",getenv("USER"),getcwd(cwd,sizeof(cwd)));
}

int prepare(void){

	struct sigaction int_act;
	int_act.sa_handler = SIG_IGN;
	sigaction(SIGINT,&int_act,NULL);
	
	printf("\033[2J\033[1;1H");
    printf("******************************************"); 
    printf("\n\n\t*******MY SHELL*******"); 
    printf("\e[1;31m\n\t-USE AT YOUR OWN RISK-\033[0m"); 
    printf("\n\n******************************************\n");  
	print_dir();
	return 0;
}

int finalize(void){
	while(-1!=wait(NULL)){}
	printf("\e[1;36m\n\t\t👋Goodbye!!👋\n");
	return 0;
}

int find_pipe(int count, char** arglist){
	for(int i=0;i<count;i++){
		if(!strcmp(arglist[i], "|"))
			return i;
	}
	return 0;
}

int execute_pipe(char** args1, char** args2, struct sigaction int_act){
	int fd_pipe[2];
	pid_t child1, child2;
	if (pipe(fd_pipe) == -1) {
        perror("cannot open pipe\n");
        exit(EXIT_FAILURE);
    }
	if((child1=fork()) == -1){
		perror("error in fork\n");
		exit(EXIT_FAILURE);
	}else if(child1 == 0){
		int_act.sa_handler = SIG_DFL;
		sigaction(SIGINT, &int_act, NULL);
		close(1);
		dup2(fd_pipe[1],1);
		execvp(args1[0],args1);
		perror("Command not found!\n");
		exit(EXIT_FAILURE);
	}
	else{
		close(fd_pipe[1]);
		if((child2=fork())==-1){
			perror("error in fork\n");
			exit(EXIT_FAILURE); 
		}else if(child2 == 0){
			int_act.sa_handler = SIG_DFL;
			sigaction(SIGINT, &int_act, NULL);
			close(0);
			dup2(fd_pipe[0],0);
			execvp(args2[0],args2);
			perror("Command not found!\n");
			exit(EXIT_FAILURE);
		}
	}
	close(fd_pipe[0]);
	wait(NULL);wait(NULL);
    return 1;
}

void chld_handler(int sig_num){
    waitpid(-1, NULL, WNOHANG);
}

int process_arglist(int count, char** arglist){

	struct sigaction int_act;
	int_act.sa_handler = SIG_IGN;
	sigaction(SIGINT,&int_act,NULL);

	struct sigaction chld_action;
	chld_action.sa_handler = chld_handler;
	chld_action.sa_flags = SA_RESTART;
	sigaction(SIGCHLD, &chld_action, NULL);
	
	int pipe_index = find_pipe(count, arglist);
	pid_t child;
	
	if(pipe_index>0){
		char** args2 = &arglist[pipe_index+1];
		arglist[pipe_index] = NULL;
		if(execute_pipe(arglist,args2,int_act)){
			print_dir();
			return 1;
		};
	}
	
	int is_bg = strcmp(arglist[count-1],"&");
	if(!is_bg){
		arglist[count - 1] = NULL;
	}
	if((child=fork())==-1){
		perror("error in fork\n");
		exit(EXIT_FAILURE);
	}else if (child==0){
		int_act.sa_handler = is_bg ? SIG_DFL : SIG_IGN;
		sigaction(SIGINT, &int_act, NULL);
		execvp(arglist[0],arglist);
		perror("Command not found!\n");
		exit(EXIT_FAILURE);
	}
	
	if(is_bg)
		wait(NULL);
	print_dir();
	return 1;
}


