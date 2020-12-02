#include <stdio.h>
#include <pthread.h>
void *doInChildThread(void *ptr);

int main(){
	pthread_t pid[10];
	for(int i = 0 ; i < 10; i++){
		int* input = new int(i);
		// create a new thread.
		// pthread_create (ptr to a pthread_id, NULL, function to run, pointer to  the parameter)
		pthread_create(&pid[i], NULL, doInChildThread,(void*)input);
	}
	
	for(int i = 0 ; i < 10; i++){
		// wait a thread to be finished
		// pthread_join(ptr to a pthread_id, NULL)
		pthread_join(pid[i],NULL);
	}
}

void *doInChildThread(void *arg){
	int input =  *(int*)arg;
	printf("Thread %d: print %d\n",input,input+1);
}
