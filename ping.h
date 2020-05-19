#ifndef __PING_H__
#define __PING_H__

#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>

// ICMP default length of data
#define ICMP_DATA_LEN 56
// ICMP default length of package head		
#define ICMP_HEAD_LEN 8			
// ICMP package total length
#define ICMP_LEN  (ICMP_DATA_LEN + ICMP_HEAD_LEN)
// length of sending buffer
#define SEND_BUFFER_SIZE 128	
// length of receiving buffer	
#define RECV_BUFFER_SIZE 128	
// number of packages to send
#define DEFAULT_SEND_NUM 100 	
// max time for waiting	for response
#define MAX_WAIT_TIME 3

// host information
extern struct hostent *g_host;
extern int g_sockfd;
extern int g_num_sent;
extern char *g_ip_addr;
extern int g_is_max_ttl_set;
extern int g_max_ttl;
extern int g_ipv4_mode;

// sends out the ICMP pacage in certain socket
void SendPacket(int sockfd, struct sockaddr_in *dest_addr, int seq);

// blocks the current process to wait for receiving a package in a socket
int ReceivePacket(int sock_icmp, struct sockaddr_in *dest_addr);

// computes the checksum of a ICMP package
u_int16_t ComputeChecksum(struct icmp *icmp_package);

// sets the field of a ICMP package. It is constructed from global variable g_send_buffer
void SetICMP(u_int16_t seq);

// computes the difference between two timevals, in microseconds
double ComputeRtt(struct timeval *send_time, struct timeval *recv_time);

// unpacks a received package and checks if it is a response to a package sent
int Unpack(struct timeval *receive_time);

// prints out the overall statistics of the ping request, and closes the socket
void PrintStatistics(int signal);

#endif	//__PING_H__
