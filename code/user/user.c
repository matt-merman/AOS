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
#define MINORS 1

char buff[4096];
char out[OUT_LEN];

void *get_setting(int fd, char *device)
{

        int input = 0, ret;
        unsigned long timeout;
        
        while (input != 7)
        {

                printf("\nSetting parameters:\n\n1. LOW priority level\n2. HIGH priority level\n3. BLOCKING r/w operations\n4. NON-BLOCKING r/w operations\n5. GO TO OPERATIONS\n\n");

                scanf("%d", &input);
                input += 2;

                switch (input)
                {

                case 3: case 4: case 5: case 6:

                        ret = ioctl(fd, input, timeout);
                        if (ret == -1)
                        {
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

                if (input == 5)
                {

                        printf("Insert a TIMEOUT for blocking operations\n");
                        scanf("%lu", &timeout);
                        if (!timeout)
                        {

                                printf("Please insert a valid timer!\n");
                                get_setting(fd, device);
                                close(fd);

                                fd = open(device, O_RDWR);
                                if (fd == -1)
                                {
                                        printf("open error on device %s\n", device);
                                        return NULL;
                                }

                                get_setting(fd, device);
                        }

                        ret = ioctl(fd, 7, timeout);
                        if (ret == -1)
                        {
                                printf("Error in ioctl() call (7) (%s)\n", strerror(errno));
                                close(fd);
                                return NULL;
                        }
                }
        }

        // close(fd);
        return NULL;
}

void *get_operation(int fd, char *device)
{

        int input = 0, ret;

        while (input != 4)
        {

                printf("Choose operation:\n1. WRITE\n2. READ\n3. GO TO SETTINGS\n4. EXIT\n");
                scanf("%d", &input);

                switch (input)
                {

                case 1:
                        printf("%ld\n", SIZE);
                        ret = write(fd, DATA, SIZE);
                        if (ret == -1)
                        {
                                printf("Error in write operation (%s)\n", strerror(errno));
                        }
                        else
                        {
                                printf("Written(%d Bytes): %s\n", ret, DATA);
                        }
                        break;
                case 2:
                        ret = read(fd, out, OUT_LEN);
                        if (ret == -1)
                        {
                                printf("Error in read operation (%s)\n", strerror(errno));
                        }
                        else
                        {
                                printf("Read(%d Bytes): %s\n", ret, out);
                        }
                        memset(out, 0, OUT_LEN);
                        break;
                case 3:
                        get_setting(fd, device);
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

        int ret, major, i;
        char *path;

        if (argc < 3)
        {
                printf("useg: prog pathname major\n");
                return -1;
        }

        path = argv[1];
        major = strtol(argv[2], NULL, 10);

        system("clear");
        printf("**************************************\n");
        printf("**             WELCOME              **\n");
        printf("**************************************\n");

        for (i = 0; i < MINORS; i++)
        {
                sprintf(buff, "mknod %s%d c %d %i 2> /dev/null\n", path, i, major, i);
                system(buff);
                sprintf(buff, "%s%d", path, i);
        }

        char *device = (char *)strdup(buff);

        int fd = open(device, O_RDWR);
        if (fd == -1)
        {
                printf("open error on device %s %s\n", device, strerror(errno));
                return -1;
        }

        get_setting(fd, device);
        get_operation(fd, device);

        return 0;
}