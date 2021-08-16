#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fnmatch.h>
#include <unistd.h>
#include <signal.h>
#define MAX_THREAD 100

size_t counter_files = 0;
size_t exit_error = 0;
size_t c_thread;
size_t active_thread = 0;
pthread_cond_t q_empty;
pthread_mutex_t q_lock;
pthread_t threads[MAX_THREAD];
char* term;

typedef struct Node
{
	char* dir;
	struct Node* next;
}Node;

typedef struct Queue
{
	Node* head;
	Node* tail;
}Queue;

Queue queue = {NULL,NULL};

Node* create_node(char* buff) 
{ 
    Node* node = (Node*)malloc(sizeof(Node));
    
    node->dir = (char*)malloc(strlen(buff)); 
    strcpy(node->dir,buff);
    node->next = NULL; 
    return node; 
} 

_Bool Q_empty(){
	return !queue.head;
}

void enqueue(Node* node) {
	pthread_mutex_lock(&q_lock);   
    if (Q_empty()) { 
        queue.head = node;
        queue.tail = queue.head; 
    }
    else {
    	queue.tail->next=node;
    	queue.tail = node;
    }
    pthread_cond_signal(&q_empty);
    pthread_mutex_unlock(&q_lock);
}

char* dequeue() {
  	pthread_mutex_lock(&q_lock);
  	pthread_testcancel();
  	while(Q_empty()){
  		pthread_cond_wait(&q_empty,&q_lock);
  		pthread_testcancel();
  	} 
  	pthread_testcancel();
  	active_thread++;
    Node* temp = queue.head; 
    queue.head = queue.head->next; 
    pthread_mutex_unlock(&q_lock);
    char* chr = temp->dir;
    free(temp);
    return chr;
} 

void free_queue(){
	while(queue.head){
		Node* temp=queue.head->next;
		free(queue.head->dir);
		free(queue.head);
		queue.head=temp;
	}
}

void treat_file(char * path){
	if(!fnmatch(term,strrchr(path,'/')+1,0)){
		printf("%s\n",path);
		__sync_fetch_and_add( &counter_files, 1);
	}
}
void clean_up_handlers(void* clock){
	pthread_mutex_t *pm = (pthread_mutex_t*)clock;
	pthread_mutex_unlock(pm);
}
void* search_file(){
	pthread_cleanup_push(clean_up_handlers,&q_lock);
	while(!Q_empty()){
		char* directory = dequeue();
		DIR *dir = opendir(directory);
		struct dirent *entry;
		struct stat dir_stat;
		if (!dir) { 
			 perror(directory);
			 __sync_fetch_and_add( &exit_error, 1);
			 if(exit_error == c_thread){
			 	printf("Done searching, found %ld files\n",counter_files);
	            pthread_mutex_destroy(&q_lock);
	            free(term);
	            free_queue();
	            pthread_exit(NULL);
			 }
			 pthread_exit(NULL);
		 
		}
		while((entry = readdir(dir))){
			char buff[strlen(directory)+strlen(entry->d_name)+2];
	        sprintf(buff,"%s/%s",directory,entry->d_name);
			stat(buff, &dir_stat);
			if(strcmp(entry->d_name,"..") != 0){
				if(((dir_stat.st_mode & __S_IFMT) == __S_IFDIR)  && strcmp(entry->d_name, ".") != 0){
					enqueue(create_node(buff));
				}
				else
					treat_file(buff);
			 }	
		 }
		 if(directory)
			free(directory);
		closedir(dir);
		__sync_fetch_and_sub( &active_thread, 1);
		pthread_testcancel();
		
		if(Q_empty() && !active_thread ){
			raise(SIGUSR1);
		}
	}
	pthread_cleanup_pop(1);
	return (void*)NULL;
	
}
void* thread_func(void* a){
	search_file();
	return NULL;
}

void int_handler(){
	pthread_t current_thread = pthread_self();
	for(size_t i = 0;i< c_thread;i++){
		if (current_thread != threads[i])
			pthread_cancel(threads[i]);
	}
	pthread_mutex_destroy(&q_lock);
	pthread_cond_destroy(&q_empty);
	free_queue();
	free(term);
	printf("Search stopped, found %ld files\n", counter_files);
	pthread_exit(NULL);
}
void usr_handler(){
	pthread_t current_thread = pthread_self();
	for(size_t i = 0;i< c_thread;i++){
		if (current_thread != threads[i])
			pthread_cancel(threads[i]);
	}
	pthread_mutex_destroy(&q_lock);
	pthread_cond_destroy(&q_empty);
	free_queue();
	free(term);
	printf("Done searching, found %ld files\n", counter_files);
	pthread_exit(NULL);
}
int main(int argc, char** argv){
 	struct sigaction int_act;
 	int_act.sa_handler = int_handler;
 	sigaction(SIGINT,&int_act, NULL);
 	
 	struct sigaction self_act;
 	self_act.sa_handler = usr_handler;
 	sigaction(SIGUSR1,&self_act, NULL);
	
	if(argc!=4){
		perror("Not enough arguments!\n");
		exit(1);
	}
	enqueue(create_node(argv[1]));
	c_thread = atoi(argv[3]);
	term=malloc(strlen(argv[2])+2);
	sprintf(term,"%c%s%c",'*',argv[2],'*');
	int rc = pthread_mutex_init(&q_lock, NULL);
    if( rc ){
        printf("ERROR in pthread_mutex_init(): %s\n", strerror(rc));
        exit(1);
    }
    for (size_t i = 0; i < c_thread; i++) {
        rc = pthread_create(&threads[i], NULL, thread_func, (void*)i);
        if (rc){
            printf("Failed creating thread: %s\n", strerror(rc));
            exit(EXIT_FAILURE);
        }
    }
    for (size_t i = 0; i < c_thread; i++){
        pthread_join(threads[i], NULL);
    }
    return 0;
}
