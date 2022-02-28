#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void *thread_func(void *param){
	
	printf("Thread is running\n");

    int f = open((char*)param, O_WRONLY, 0666);
    if(f == -1){
        printf("Error while opened file: %s\n", (char*)param);
        exit(0);
    }
    char * string = "test!\n";
    size_t size = strlen(string);
    write(f, string, size);
    close(f);
	return NULL;
}

int main(){

    printf("NOTICE: it is required to create a node with:\nmknod /dev/test c 241 0\n");
    //instead of create it manually, use mknod()

	pthread_t thread_id;
	pthread_create(&thread_id, NULL, thread_func, "/dev/test");
	pthread_join(thread_id, NULL);
	exit(0);
}
