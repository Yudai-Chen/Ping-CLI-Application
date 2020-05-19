#include "ping.h"

#define WAIT_TIME 5

// a buffer used for save package to send temprorily
char g_send_buffer[SEND_BUFFER_SIZE];
// a buffer used for save package received temprorily
char g_receive_buffer[RECV_BUFFER_SIZE];
// number of packages received
int g_num_received = 0;
// time when the first package sent
struct timeval g_first_send_time;
// time when the last package received
struct timeval g_last_recv_time;
// min rtt till now
double g_min_rtt = 0.0;
// sum of rtt till now
double g_sum_rtt = 0.0;
// max of rtt till now
double g_max_rtt = 0.0;
// mean standard deviation of rtt
double g_mdev_rtt = 0.0;
// number of ICMP message exceeds the given max ttl
int g_num_exceeded = 0;
/*
 * This function computes the checksum of a ICMP package.
 * @param pIcmp the ICMP package
 * @return the checksum of the package
 */
u_int16_t 
ComputeChecksum(struct icmp *icmp_package)
{
	u_int16_t *data = (u_int16_t *)icmp_package;
	int len = ICMP_LEN;
	u_int32_t sum = 0;
	
	while (len > 1)
	{
		sum += *data++;
		len -= 2;
	}

	if (1 == len)
	{
		u_int16_t tmp = *data;
		tmp &= 0xff00;
		sum += tmp;
	}

	while (sum >> 16) {
		sum = (sum >> 16) + (sum & 0x0000ffff);
	}
	sum = ~sum;
	
	return sum;
}

/*
 * This function sets the fields of a ICMP package. It is constructed from global variable g_send_buffer.
 * @param the sequence number to be set
 */
void 
SetICMP(u_int16_t seq)
{
	struct icmp *pIcmp;

	pIcmp = (struct icmp*)g_send_buffer;
	pIcmp->icmp_type = ICMP_ECHO;
	pIcmp->icmp_code = 0;
	pIcmp->icmp_cksum = 0;	
	pIcmp->icmp_seq = seq;	
	// we set the icmp id to be the process ID of current process
	pIcmp->icmp_id = getpid();
	gettimeofday((struct timeval *)pIcmp->icmp_data, NULL);
	pIcmp->icmp_cksum = ComputeChecksum(pIcmp);
	
	// if it is the first ICMP to be sent, we should bookkeep its send time
	if (seq == 1) {
		g_first_send_time = *(struct timeval *)pIcmp->icmp_data;
	}
}

/*
 * This function sends out the ICMP pacage in certain socket.
 * @param sockfd the socket number to be used for listening
 * @param dest_addr the destination address of the socket
 * @param the sequence number of the package to be sent
 */
void 
SendPacket(int sockfd, struct sockaddr_in *dest_addr, int seq)
{
	SetICMP(seq);
	if (sendto(sockfd, g_send_buffer, ICMP_LEN, 0, (struct sockaddr *)dest_addr, sizeof(struct sockaddr_in)) < 0)
	{
		perror("An error occurs when sending the ICMP package by socket.");
		return;
	}
}

/*
 * This function computes the difference between two timevals, in microseconds.
 * @param send_time the timeval for saving send time
 * @param recv_time the timeval for saving receive time
 * @return the difference between send time and receive time, in microseconds
 */
double 
ComputeRtt(struct timeval *send_time, struct timeval *recv_time)
{
	long usec_diff = recv_time->tv_usec - send_time->tv_usec;
	long sec_diff = recv_time->tv_sec - send_time->tv_sec;
	if (usec_diff < 0) {
		sec_diff--;
		usec_diff += 1000000;
	}
	
	// transfer to microsecond
	return sec_diff * 1000.0 + usec_diff / 1000.0; 
}

/*
 * This function unpacks a received package and checks if it is a response to a package sent.
 * If so, print a message to the terminal. The package is saved in the global variable 
 * g_receive_buffer.
 * @param receive_time the time when the package is received
 */
int 
Unpack(struct timeval *receive_time)
{
	struct ip *ip_segment = (struct ip *)g_receive_buffer;
	struct icmp *icmp_package;
	// length of ip head segment
	int len_ip_head = ip_segment->ip_hl << 2;
	double rtt;

	// the start of the icmp package
	icmp_package = (struct icmp *)(g_receive_buffer + len_ip_head);

	// if the package is a response to a package sent, print a message and update statistics
	if ((icmp_package->icmp_type == ICMP_ECHOREPLY) && icmp_package->icmp_id == getpid())
	{
		struct timeval *send_time = (struct timeval *)icmp_package->icmp_data;
		rtt = ComputeRtt(send_time, receive_time);
	
		printf("%u bytes from %s: icmp_seq=%u ttl=%u time=%.1f ms\n",
			ntohs(ip_segment->ip_len) - len_ip_head,
			inet_ntoa(ip_segment->ip_src),
			icmp_package->icmp_seq,
			ip_segment->ip_ttl,
			rtt);
		
		// update statistics
		if (rtt < g_min_rtt || g_min_rtt == 0) {
			g_min_rtt = rtt;
		}
		if (rtt > g_max_rtt) {
			g_max_rtt = rtt;
		}
		// if a ICMP message has been transfered by more than max ttl sites, bookkeep it
		if (g_is_max_ttl_set == 1 && (64 - ip_segment->ip_ttl) > g_max_ttl) {
			g_num_exceeded++;
		}
		g_sum_rtt += rtt;
		g_mdev_rtt += rtt * rtt;
		
		return 0;
	}
		
	return -1;
}

/*
 * This function blocks the current process to wait for receiving a package in a socket.
 * @param sockfd the socket used for listening the package
 * @param dest_addr the destination address of the socket
 */
int 
ReceivePacket(int sockfd, struct sockaddr_in *dest_addr)
{
	int addr_len = sizeof(struct sockaddr_in);
	
	signal(SIGALRM, PrintStatistics);
	alarm(WAIT_TIME);
	if ((recvfrom(sockfd, g_receive_buffer, RECV_BUFFER_SIZE, 0, (struct sockaddr *)dest_addr, &addr_len)) < 0)
	{
		perror("An error occurs when receiving data from socket.");
		return 0;
	}

	struct timeval receive_time;
	gettimeofday(&receive_time, NULL);
	g_last_recv_time = receive_time;

	if (Unpack(&receive_time) == -1)
	{
		return -1; 
	}
	g_num_received++;
}

/*
 * This function prints out the overall statistics of the ping request, and closes the socket.
 * @param signal this functions is waked up by a signal
 */
void 
PrintStatistics(int signal)
{
	double tmp;
	double avg_rtt = g_sum_rtt /= g_num_received;
	tmp = g_mdev_rtt / g_num_received - avg_rtt * avg_rtt;
	g_mdev_rtt = sqrt(tmp);
	
	if (g_host != NULL) {
		printf("--- %s  ping statistics ---\n", g_host->h_name);
	}
	else {
		printf("--- %s  ping statistics ---\n", g_ip_addr);
	}
		
	printf("%d packets transmitted, %d received, %d%% packet loss, time %dms\n"
		, g_num_sent
		, g_num_received
		, (g_num_sent - g_num_received) / g_num_sent * 100
		, (int)ComputeRtt(&g_first_send_time, &g_last_recv_time));
	printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n",
		g_min_rtt, avg_rtt, g_max_rtt, g_mdev_rtt);
	if (g_is_max_ttl_set) {
		printf("%d ICMP message's ttl (from the destination site) has exceeded %d\n", g_num_exceeded, g_max_ttl);
	}
	close(g_sockfd);
	exit(0);
}