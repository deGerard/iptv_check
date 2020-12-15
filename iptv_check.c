/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   iptv_check.c
 * Author: gerard
 *
 * Created on April 21, 2019, 11:43 AM
 */

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "rtp.h"
#include "log_print.h"

int runprogram = 1;
int runasdaemon = 0;
FILE *logfile_fp = NULL;

/**
 * Get ip address for the given interface
 * @param addr
 * @param ethdev
 * @return 
 */
int get_ip_address(in_addr_t *addr, const char *ethdev) 
{
    int fd;
    struct ifreq ifr;
    struct in_addr ipaddr;
    int ret;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return -1;
    }
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, ethdev, IFNAMSIZ - 1);
    ret = ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);
    if (ret != 0) {
        return ret;
    }
    ipaddr = ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr;
    if (addr) {
        memcpy(addr, &ipaddr, sizeof(in_addr_t));
    }
    return 0;
}


/**
 * signal callback handler
*  This function handles SIGINT (ctrl-c) and SIGTERM
 * @param signum
 */
static void signal_callback_handler(int signum) {
    runprogram = 0;
}

static int print_help(void)
{
    printf ("iptv_check [options] <ip-address>:<port-number>\n");
    exit(0);
}

int main(int argc, const char **argv) 
{
    struct sockaddr_in si_peer;
    struct sockaddr_in udp_localaddress; // my addressinformation
    struct sockaddr_in udp_peeraddress;
    struct ip_mreq group;
    socklen_t udp_peeraddress_len;
    int sock;
    char buffer[2048];
    int buflen;
    int c;
    char *interface = NULL;
    char *logfile = NULL;
    int cntr;
    char *ipaddress = NULL;
    char *portnumberstr = NULL;
    int portnumber = 0;
    in_addr_t interface_addr;
    pid_t pid, sid;

    while ((c = getopt(argc, (char * const*) argv, "di:l:")) != -1) {
        switch (c) {
            case 'd':
                runasdaemon = 1;
                break;
            case 'i':
                interface = optarg;
                break;
            case 'l':
                logfile = optarg;
                break;
            default:
                break;
        }
    }
    
    if (interface) {
        if (get_ip_address(&interface_addr, interface) != 0) {
            printf ("Can't find open interface %s\n", interface);
            exit(1);
        }
    }

    if (optind >= argc) {
        printf ("No ip address and portnumber\n");
        print_help();
    }
    
    /* Split ip address and port number on the ':' */
    portnumberstr = (char*)argv[optind];
    ipaddress = strsep(&portnumberstr, ":");
    if (!ipaddress || !portnumberstr) {
        print_help();
        exit(0);
    }
    portnumber = atoi(portnumberstr);
    if (portnumber < 0 || portnumber > 65535) {
        printf ("Invalid port number %d\n", portnumber);
        exit(0);
    }
    
    if (logfile) {
        if (log_open(logfile) != 0) {
            printf("Can't open logfile %s\n", logfile);
            exit(0);
        }
    }
    
    if (runasdaemon) {
            // Fork off the parent process
            pid = fork();
            if (pid < 0) {
                    exit(EXIT_FAILURE);
            }
            /* If we got a good PID, then
            we can exit the parent process. */
            if (pid > 0) {
                    printf ("iptv_check PID=%u\n",pid);  /* the only allowed printf */
                    sleep(1);
                    exit(EXIT_SUCCESS);
            }

            /* Change the file mode mask */
            umask(0);

            /* Open any logs here */

            /* Create a new SID for the child process */
            sid = setsid();
            if (sid < 0) {
                    /* Log any failure */
                    exit(EXIT_FAILURE);
            }

            /* Close out the standard file descriptors */
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
    }    
    
    
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        log_printf("Can't create socket\n");
        exit(1);
    }
    memset(&si_peer, 0, sizeof (si_peer));
    si_peer.sin_family = AF_INET;
    si_peer.sin_port = htons(portnumber);

    signal(SIGINT, signal_callback_handler);
    signal(SIGTERM, signal_callback_handler);
    
    memset(&udp_localaddress, 0, sizeof (struct sockaddr_in));
    udp_localaddress.sin_family = AF_INET;
    udp_localaddress.sin_addr.s_addr = INADDR_ANY;
    udp_localaddress.sin_port = htons(portnumber);

    if (bind(sock, (struct sockaddr *) &udp_localaddress, sizeof (struct sockaddr)) == -1) {
        log_printf("bind");
        return -1;
    }

    group.imr_multiaddr.s_addr = inet_addr(ipaddress);
    group.imr_interface.s_addr = INADDR_ANY;
    if (interface) {
        setsockopt(sock,SOL_SOCKET,SO_BINDTODEVICE, interface, strlen(interface));
        group.imr_interface.s_addr = interface_addr;
#if 0
        if (get_ip_address(&group.imr_interface.s_addr, interface) != 0) {
            printf ("Can't find open interface %s\n", interface);
            exit(1);
        }
#endif
    }
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &group, sizeof (group)) < 0) {
        log_printf("Adding multicast group error");
        close(sock);
        exit(1);
    }
    else {
        log_printf("Adding multicast group...OK.\n");
    }
    while (runprogram) {
        udp_peeraddress_len = sizeof (struct sockaddr_in);
        buflen = recvfrom(sock, buffer, sizeof (buffer), 0, (struct sockaddr *) &udp_peeraddress, &udp_peeraddress_len);
        if (buflen > 0) {
            rtp_in(buffer, buflen);
        }
    }
    
    if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *) &group, sizeof (group)) < 0) {
        log_printf("Drop multicast group error");
    }
    else {
        log_printf("Drop multicast group...OK.\n");
    }
    close(sock);
    log_close();
}
