obj-m += multi_flow.o
mymodule-objs := timer.o multi_flow.o work_queue.o info.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
