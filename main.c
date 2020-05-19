#include "ping.h"

struct hostent * g_host = NULL;		
int g_sockfd;				
int g_num_sent = 1;
char *g_ip_addr = NULL;
int g_is_max_ttl_set = 0;
int g_max_ttl = 64;
int g_ipv4_mode = 1;

int 
main(int argc, char *argv[])
{
	struct sockaddr_in dest_addr; 

	in_addr_t inaddr;	
    int max_send_num = DEFAULT_SEND_NUM;


    int opt;

    if (argc < 2)
	{
		printf("Usage: %s [-c count] [-t max ttl] [-i interval] [hostname/IP address]\n", argv[0]);
		exit(EXIT_FAILURE);	
	}

    char input_adress[50];
    int send_interval = 1;

    // deal with command line arguments
    while((opt = getopt(argc, argv, "46c:i:t:")) != -1) {
        switch(opt) {
            case '4':
                g_ipv4_mode = 1;
                break;
            case '6':
                g_ipv4_mode = 0;
                break;
            case 'c':  
                if ((max_send_num = atoi(optarg)) < 0) {
                    perror("Option c with an invalid value.");
		            exit(EXIT_FAILURE);
                }
                break;
            case 'i':
                if ((send_interval = atoi(optarg)) < 0) {
                    perror("Option i with an invalid value.");
		            exit(EXIT_FAILURE);
                }
                break;
            case 't':
                g_is_max_ttl_set = 1;
                if ((g_max_ttl = atoi(optarg)) < 0) {
                    perror("Option t with an invalid value.");
		            exit(EXIT_FAILURE);
                }
                break;
            case '?': 
                printf("Unknown option\n");
                break;
        }
    }

    if (g_ipv4_mode == 1) {
        if ((g_sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
        {
            perror("An error occurs when setting up the socket.");
            exit(EXIT_FAILURE);
        }
    } 
    else {
        if ((g_sockfd = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) < 0)
        {
            perror("An error occurs when setting up the socket.");
            exit(EXIT_FAILURE);
        }
    }

	dest_addr.sin_family = AF_INET;

    // transfer to internet address
	if ((inaddr = inet_addr(argv[optind])) == INADDR_NONE)
	{
		// it may be a hostname, try to get its ip address
		if ((g_host = gethostbyname(argv[optind])) == NULL)
		{
			perror("An error occurs when resolving the host name.");
			exit(EXIT_FAILURE);
		}
		memcpy(&dest_addr.sin_addr, g_host->h_addr_list[0], g_host->h_length);
	}
	else
	{
		memcpy(&dest_addr.sin_addr, &inaddr, sizeof(struct in_addr));
	}

	if (g_host != NULL) 
    {
		printf("PING %s", g_host->h_name);
    }
	else 
    {
		printf("PING %s", argv[optind]);
    }

	printf("(%s) %d bytes of data.\n", inet_ntoa(dest_addr.sin_addr), ICMP_LEN);

	g_ip_addr = argv[1];
	signal(SIGINT, PrintStatistics);

    // in the loop we send ICMP messages
	while (g_num_sent <= max_send_num)
	{
		int unpack_ret;
		
		SendPacket(g_sockfd, &dest_addr, g_num_sent);
		
		unpack_ret = ReceivePacket(g_sockfd, &dest_addr);
		if (unpack_ret == -1) {
			ReceivePacket(g_sockfd, &dest_addr);
        }
			
		sleep(send_interval);
		g_num_sent++;
	}
	
	PrintStatistics(0);

	return 0;
}
