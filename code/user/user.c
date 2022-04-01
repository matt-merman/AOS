#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#define DATA "123456789\n"
#define SIZE strlen(DATA)
#define OUT_LEN 16
#define MINORS 128

char buff[4096];
char out[OUT_LEN];
int blocking;

void *get_setting(int fd)
{
        int input = 0, ret;
        long int timeout;
        blocking = 0;
        while (input != 7)
        {

                printf("\nSetting parameters:\n\n1. LOW priority level\n2. HIGH priority level\n3. BLOCKING r/w operations\n4. NON-BLOCKING r/w operations\n5. GO TO OPERATIONS\n");

                scanf("%d", &input);
                input += 2;

                switch (input)
                {

                case 3: case 4: case 5: case 6:

                        ret = ioctl(fd, input, timeout);
                        if (ret == -1) goto exit;
                        break;
                case 7:
                        break;
                default:
                        printf("Wrong input!\n");
                }

                if (input == 5)
                {

                        printf("Insert a TIMEOUT (microsecs) for blocking operations\n");
                        scanf("%ld", &timeout);
                        while (timeout <= 0){

                                printf("Please insert a valid timer!\n");
                                scanf("%ld", &timeout);
                        }
                        
                        blocking = 1;
                        ret = ioctl(fd, 7, timeout);
                        if (ret == -1) goto exit;
                }
        }

        return NULL;

exit:

        printf("Error on ioctl() (%s)\n", strerror(errno));
        close(fd);
        return NULL;

}

void *get_operation(int fd)
{

        int input = 0, ret;

        while (input != 4)
        {

                printf("\nChoose operation:\n1. WRITE\n2. READ\n3. GO TO SETTINGS\n4. EXIT\n");
                scanf("%d", &input);

                switch (input)
                {

                case 1:

                        if(blocking) printf("WARNING: That's a blocking invocation, you might wait for a while ...\n");
                        ret = write(fd, DATA, SIZE);
                        if (ret == -1) printf("Error on write operation (%s)\n", strerror(errno));
                        else printf("Written %d Bytes, from user: '%s'\n", ret, DATA);
                        break;
                case 2:
                        if(blocking) printf("WARNING: That's a blocking invocation, you might wait for a while ...\n");                
                        ret = read(fd, out, OUT_LEN);
                        if (ret == -1) printf("Error on read operation (%s)\n", strerror(errno));
                        else printf("Read(%d Bytes): %s\n", ret, out);
                        memset(out, 0, OUT_LEN);
                        break;
                case 3:
                        get_setting(fd);
                case 4:
                        break;
                default:
                        printf("Wrong input!\n");
                }
        }

        close(fd);
        return NULL;
}

int main(int argc, char **argv)
{       
        int major, minor, i;
        char *path;

        if (argc < 4)
        {
                printf("Usage: ./sudo user [Path Device File] [Major Number] [Minor number]\n");
                return -1;
        }

        path = argv[1];
        major = strtol(argv[2], NULL, 10);
        minor = strtol(argv[3], NULL, 10);

        system("clear");
        printf("**************************************\n");
        printf("**      Multi-flow device file      **\n");
        printf("**************************************\n");

        for (i = 0; i < MINORS; i++)
        {
                sprintf(buff, "mknod %s%d c %d %i 2> /dev/null\n", path, i, major, i);
                system(buff);
        }

        sprintf(buff, "%s%d", path, minor);

        printf("(NOTICE: 128 devices were created. Your is %d in [0;127])\n", minor);

        char *device = (char *)strdup(buff);

        int fd = open(device, O_RDWR);
        if (fd == -1)
        {
                printf("open error on device %s, %s\n", device, strerror(errno));
                return -1;
        }

        get_setting(fd);
        get_operation(fd);

        return 0;
}