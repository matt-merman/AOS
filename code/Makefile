obj-m += multi_flow.o
mymodule-objs := info.o common.o multi_flow.o work_queue.o read.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

show-device:
	cat /sys/module/multi_flow/parameters/enabled_device

show-hp_bytes:
	cat /sys/module/multi_flow/parameters/hp_bytes

show-lp_bytes:
	cat /sys/module/multi_flow/parameters/lp_bytes

show-hp_threads:
	cat /sys/module/multi_flow/parameters/hp_threads

show-lp_threads:
	cat /sys/module/multi_flow/parameters/lp_threads

	