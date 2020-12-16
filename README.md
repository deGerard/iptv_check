# iptv_check
Multicast IPTV RTP Packet drop checker

This program can be used to detect and log packet drops on multicast streams that use RTP.
It detect packet drops by compare the received packet sequence number with the sequence number of the previous packet. If there is a gap detected then it will print and/or log the amount of packets that there are missing.


## Build
The program is developed in Netbeans and runs under Linux. For build the program it is not necessary to have Netbeans installed. The only tools that are needed are GCC and make.

To build the debug version use the following command:
```
make
```
The executable can be found in dist/Debug/GNU-Linux/

To build the release version use the following command:
```
make CONF=Release
```
The executable can be found in dist/Release/GNU-Linux/

The difference between the 2 version is only compiler optimalizations and the release version doesn't have debug symbols in the executable.


## Usage
```
iptv_check 224.0.252.136:7272
```
This example will send an IGMP multicast join for 224.0.252.136 on all available interfaces and then start analyse the stream comming on UDP port 7272.

```
iptv_check -l logfile.txt 224.0.252.136:7272
```
This example will send an IGMP multicast join for 224.0.252.136 on all available interfaces and then start analyse the stream comming on UDP port 7272. In this case there will be also a logfile.txt written to disk with all the information about packet drops.

```
iptv_check -i eth0 224.0.252.136:7272
```
This example send the IGMP multicast join only to network interface eth0.

```
iptv_check -d -l logfile.txt 224.0.252.136:7272
```
This example runs the program as a daemon and write the log to logfile.txt

