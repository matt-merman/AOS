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
       
        fd = open(device,O_RDWR);
        if(fd == -1) {
                printf("open error on device %s\n",device);
                return NULL;
        }

        int input; 
setting:        
        input = 0;
        while (input != 7){

                printf("Setting parameters:\n1. LOW priority level\n2. HIGH priority level\n3. BLOCKING r/w operations\n4. NON-BLOCKING r/w operations\n5. EXIT\n");

                scanf("%d", &input);
                input += 2;
                int ret;
                unsigned long timeout = 0;

                switch(input){

                        case 3: case 4: case 5: case 6:
                                
                                ret = ioctl(fd, input, timeout);
                                if (ret == -1){
                                        printf("Error in ioctl() call (%d) (%s)\n", input, strerror(errno));
                                        close(fd);
                                        return NULL;
                                }
                                break;
                        case 7:
                                break;
                        default:
                                printf("Wrong input!\n");
                }
                
                if (input == 5){

                        //get timeout from user
                        printf("Insert a TIMEOUT for blocking operations\n");
                        scanf("%lu", &timeout);

                        ret = ioctl(fd, 7, timeout);
                        if (ret == -1){
                                printf("Error in ioctl() call (7) (%s)\n", strerror(errno));
                                close(fd);
                                return NULL;
                        }
                }
        }
        
        close(fd);
        input = 0;
        
        while(input != 4){

                printf("Choose operation:\n1. WRITE\n2. READ\n3. SETTING\n4. EXIT\n");
                scanf("%d", &input);
                
                fd = open(device,O_RDWR);
                if(fd == -1) {
                        printf("open error on device %s\n",device);
                        return NULL;
                }

                int ret = 0;
                char out[SIZE];

                switch(input){

                        case 1:
                                ret = write(fd,DATA,SIZE);
                                if (ret == -1){
                                        printf("Error in write operation (%s)\n", strerror(errno));
                                }else{
                                        printf("Written %d bytes\n", ret);
                                }
                                break;
                        case 2:
                                //to fix
                                ret = read(fd,out,SIZE);
                                if (ret == -1){
                                        printf("Error in read operation (%s)\n", strerror(errno));
                                }else{
                                        printf("Read %s string\n", out);
                                }
                                break;
                        case 3:
                                goto setting;
                        case 4:
                                break;
                        default:
                                printf("Wrong input!\n");
                }

                close(fd);
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
