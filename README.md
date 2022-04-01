# Multi-flow device file

## Specification

This specification is related to a Linux device driver implementing low and
high priority flows of data. Through an open session to the device file a thread
can read/write data segments. The data delivery follows a First-in-First-out
policy along each of the two different data flows (low and high priority). After
read operations, the read data disappear from the flow. Also, the high priority data flow must offer synchronous write operations while the low priority
data flow must offer an asynchronous execution (based on delayed work)
of write operations, while still keeping the interface able to synchronously
notify the outcome. Read operations are all executed synchronously. The
device driver should support 128 devices corresponding to the same amount
of minor numbers.
The device driver should implement the support for the _ioctl(..)_ service
in order to manage the I/O session as follows:

* setup of the priority level (high or low) for the operations
* blocking vs non-blocking read and write operations
* setup of a timeout regulating the awake of blocking operations

A few Linux module parameters and functions should be implemented in
order to enable or disable the device file, in terms of a specific minor number.
If it is disabled, any attempt to open a session should fail (but already open
sessions will be still managed). Further additional parameters exposed via
VFS should provide a picture of the current state of the device according to
the following information:

* enabled or disabled
* number of bytes currently present in the two flows (high vs low priority)
* number of threads currently waiting for data along the two flows (high vs low priority)

## Running

```bash
#Compile kernel module (on /multi_flow/code/)
make

#Clean objects file
make clean

#Insertion kernel module
sudo insmod multi_flow.ko

#Remove kernel module
sudo rmmod multi_flow.ko

#Compile user (on /multi_flow/code/user/)
make

#Run user 
sudo ./user /dev/test 240

#Set kernel module's parameters (i.e. enabled_device, hp_bytes, lp_bytes, hp_threads, lp_threads)
cd /sys/module/multi_flow/parameters
```

>NOTICE: _240_ identifies the major number for the driver. It could be different for each insertion. To see your, use _dmesg_ command.  

## Implementation

Please see [report](https://github.com/matt-merman/linux_device_driver/blob/main/documentation/report/report.pdf).

