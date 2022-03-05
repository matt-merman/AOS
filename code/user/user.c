#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

int i;
char buff[4096];
#define DATA "ciao a tutti\n"
#define SIZE strlen(DATA)

void * the_thread(void* path){

        char* device;
        int fd;

        device = (char*)path;
       
        int input = 0;        
        while (input != 8){

                printf("Setting parameters:\n1. LOW priority level\n2. HIGH priority level\n3. BLOCKING r/w operations\n4. NON-BLOCKING r/w operations\n5. TIMEOUT blocking operations\n6. EXIT\n");

                scanf("%d", &input);
                input += 2;
                int ret;
                unsigned long timeout = 0;

                if (input == 5 || input == 7){

                        //get timeout from user
                        printf("Insert a TIMEOUT for blocking operations\n");
                        scanf("%lu", &timeout);

                        ret = ioctl(fd, 7, timeout);
                        if (ret == -1){
                                printf("Error in ioctl() call (%d) (%s)\n", input, strerror(errno));
                                close(fd);
                                return NULL;
                        }
                
                }

                if (input == 7 || input == 8) break;

                ret = ioctl(fd, input, timeout);
                if (ret == -1){
                        printf("Error in ioctl() call (%d) (%s)\n", input, strerror(errno));
                        close(fd);
                        return NULL;
                }
                
        }
        
        input = 0;
        while(input != 3){

                printf("Choose operation:\n1. WRITE\n2. READ\n3. EXIT\n");
                scanf("%d", &input);
                
                fd = open(device,O_RDWR);
                if(fd == -1) {
                        printf("open error on device %s\n",device);
                        return NULL;
                }

                char test[SIZE];

                switch(input){

                        case 1:
                                write(fd,DATA,SIZE);
                                close(fd);
                                break;
                        case 2:

                                //to fix
                                read(fd,test,SIZE);
                                close(fd);
                                printf("read executed: %s\n", test);
                                break;
                        case 3:
                                break;
                        default:
                                printf("Wrong input, retry\n");
                }
        }

        return NULL;

}


int main(int argc, char** argv){

     int ret;
     int major;
     int minors;
     char *path;
     pthread_t tid;

     if(argc<4){
        printf("useg: prog pathname major minors\n");
        return -1;
     }

     path = argv[1];
     major = strtol(argv[2],NULL,10);
     minors = strtol(argv[3],NULL,10);
     printf("creating %d minors for device %s with major %d\n",minors,path,major);

     for(i=0;i<minors;i++){
        sprintf(buff,"mknod %s%d c %d %i\n",path,i,major,i);
        system(buff);
        sprintf(buff,"%s%d",path,i);
        pthread_create(&tid,NULL,the_thread,strdup(buff));
        pthread_join(tid, NULL);
     }

     return 0;

}
