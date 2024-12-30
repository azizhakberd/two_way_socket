#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <netdb.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include "./promise.h"
#include "./queue.h"
#include "./dynamicPtr.h"
#include <fcntl.h>
#include <poll.h>

char* IPself = "10.147.17.235";
char* IPserv;
int PORTself;
int PORTserv;

typedef struct
{
	int c_sock;
	int s_sock;
	int length;
	char* ip;
} socketParams;

dynamicPtr streamRequestToServer(dynamicPtr args);
dynamicPtr streamResponseToRequester(dynamicPtr args);
dynamicPtr createAsyncChannel(dynamicPtr args);

void setNonBlock(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);

	if (flags == -1)
	{
		perror("fcntl get");
		exit(EXIT_FAILURE);
	}

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		perror("fcntl set");
		exit(EXIT_FAILURE);
	}
}

int connectNonBlocking(int sockfd, struct sockaddr* addr, socklen_t addrlen)
{
	int ret = connect(sockfd, addr, addrlen);

	if (ret == 0)
	{
		return 0;
	}

	if (ret == -1 && errno == EINPROGRESS)
	{
		return 1;
	}

	perror("connect");
	return -1;
}

dynamicPtr waitForExitCode(dynamicPtr args)
{
	int* socket = args.data;
	char* buffer = calloc(3, 1);
	size_t bufsize = 32;
	size_t length = 0;

	while ((length != 2)&&(buffer[0] != 'q' || buffer[1] != '\n'))
	{
		length = getline(&buffer, &bufsize, stdin);
	}

	close(*socket);
	exit(0);

	return args;
}


int main(int argc, char *argv[])
{
	struct sockaddr_in serv_addr;
    int sock_file_descriptor;
    int client_sock;
    socklen_t clength;

    int terminate = 0;
    Promise* waitForTerminate = createPromise(waitForExitCode, dp(&sock_file_descriptor));
    invoke(waitForTerminate);

    if (argc < 5)
    {
        perror("Usage: <socket ip> <socket port> <server ip> <server port>\n\n");
        exit(1);
    }

    IPself = argv[1];
    PORTself = atoi(argv[2]);
    IPserv = argv[3];
    PORTserv = atoi(argv[4]);
    
    sock_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);

    if (sock_file_descriptor < 0)
    {
        perror("Unable to create socket\n\n");
    }

    printf("Socket created\n\n");

    setNonBlock(sock_file_descriptor);

    serv_addr.sin_addr.s_addr = inet_addr(IPself);
    serv_addr.sin_port = htons(PORTself);
    serv_addr.sin_family = AF_INET;

    if(bind(sock_file_descriptor, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
    {
    	perror("Unable to bind socket\n\n");
    	return 1;
    }

    printf("Bind Successful\n\n");

    if(listen(sock_file_descriptor, 100))
    {
    	perror("Unable to listen\n\n");
    	return 1;
    }
    printf("Listening on host %s:%d\n\n", IPself, PORTself);
    clength = sizeof(struct sockaddr_in);

    struct pollfd pfd[1];
    pfd[0].fd = sock_file_descriptor;
    pfd[0].events = POLLIN;

    while(1)
    {	
    	int poll_ret = poll(pfd, 1, 0);

    	if (poll_ret == 0)
    	{
    		// no connections
    		continue;
    	}

    	if (poll_ret < 0)
    	{
    		perror("poll error");
    		exit(EXIT_FAILURE);
    	}
    	if((client_sock = accept(sock_file_descriptor, (struct sockaddr *)&serv_addr, (socklen_t*) &clength)) >= 0)
    	{
    		// handle connection
    		setNonBlock(client_sock);
    		struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&serv_addr;
			struct in_addr ipAddr = pV4Addr->sin_addr;
			char str[INET_ADDRSTRLEN];
			inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN );
			socketParams* params = malloc(sizeof(socketParams));
			params->c_sock = client_sock;
			params->ip = str;
			Promise* channelRequest = createPromise(createAsyncChannel, dp(params));
			invoke(channelRequest);
   		}
    }

    return 0;


}

dynamicPtr createAsyncChannel(dynamicPtr args)
{
	socketParams* params = args.data;
	params->length = 1;

	struct sockaddr_in server_sock_addr, client_addr;
    socklen_t client_adrr_len = sizeof(client_addr);
    int portnum;
    int sock_file_descriptor;

    portnum = PORTserv;

    sock_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_file_descriptor < 0)
    {
    	perror("Unable to create socket\n\n");
    }

    setNonBlock(sock_file_descriptor);

    server_sock_addr.sin_family = AF_INET;
    server_sock_addr.sin_port = htons(portnum);
    server_sock_addr.sin_addr.s_addr = inet_addr(IPserv);

    int status = connectNonBlocking(sock_file_descriptor, (struct sockaddr*)&server_sock_addr, sizeof(server_sock_addr));

    if (status == -1)
    {
    	close(sock_file_descriptor);
    	exit(EXIT_FAILURE);
    }

    if (status == 1)
    {
    	struct pollfd pfd[1];
    	pfd[0].fd = sock_file_descriptor;
    	pfd[0].events = POLLOUT;
    	while(1)
    	{
    		int ret = poll(pfd, 1, 0);
    		if (ret == -1)
    		{
    			perror("server poll");
    			exit(EXIT_FAILURE);
    		}

    		if (ret == 0)
    			continue;

    		break;
    	}
    }

    if(getsockname(sock_file_descriptor, (struct sockaddr *) &client_addr, &client_adrr_len) < 0)
	{	
    	perror("Unable to get sockname\n\n");
    	params->length = -1;
    }

	params->s_sock = sock_file_descriptor;

	Promise* c2s = createPromise(streamRequestToServer, args);
	Promise* s2c = createPromise(streamResponseToRequester, args);
	invoke(c2s);
	invoke(s2c);

   	return args;
}

dynamicPtr streamRequestToServer(dynamicPtr args)
{
	socketParams* params = args.data;

	if (params->length < 0)
	{
		close(params->c_sock);
		return args;
	}

	int bufsize = 8192;
	char* buffer = calloc(bufsize, 1);
	ssize_t length = 0;
	int bufavail = bufsize;
	printf("Connection from host %s\n", params->ip);

	struct pollfd pfd[1];
	pfd[0].fd = params->c_sock;
	pfd[0].events = POLLIN;

	while (1)
    {
    	int ret = poll(pfd, 1, 0);

    	if (ret == 0)
    		continue;

    	if (ret == -1)
    	{
    		perror("c_sock error");
    		return args;
    	}

    	if (pfd[0].revents & POLLIN)
    	{
    		if (bufavail == 0) continue;

    		length = recv(params->c_sock, buffer + (bufsize - bufavail), bufavail, 0);
    		bufavail -= length;

        	if (length < 0)
        	{
         	   perror("recv failed: ");
            	break;
        	}
        	else if (length == 0)
        	{
            	break;
        	}
        	else
        	{
        		struct pollfd pfd_s[1];
        		pfd_s[0].fd = params->s_sock;
        		pfd_s[0].events = POLLOUT;

        		int rets = poll(pfd_s, 1, 0);

        		if (rets > 0)
        		{
            		if (send(params->s_sock, buffer, length, 0) > 0)
            		{
						memset(buffer, 0, bufsize);
						bufavail = bufsize;
            		}
					else
					{
						perror("Sending request failed");
						break;
					}
				} else if (rets == 0)
				{
					if (bufavail > 0)
						continue;
					else
					{
						while (1)
						{
							int rets = poll(pfd_s, 1, 0);

							if (rets == 0)
								continue;

							else if (rets == -1)
							{
								perror("poll error in c_sock");
								close(params->c_sock);
								return args;
							}

							if (send(params->s_sock, buffer, length, 0) > 0)
            				{
								memset(buffer, 0, bufsize);
								bufavail = bufsize;
								//firstChunk = 0;
            				}
							else
							{
								perror("Sending request failed");
								close(params->c_sock);
								return args;
							}

						}
					}
				}
    		}
    	}
    }

    close(params->c_sock);

    return args;
}

dynamicPtr streamResponseToRequester(dynamicPtr args)
{
	socketParams* params = args.data;

	if (params->length < 0)
	{
		printf("%s\n", "Error ocurred, closing connection");
		close(params->c_sock);
		free(params);
		return args;
	}

	int bufsize = 8192;
	char* buffer = calloc(bufsize, 1);
	ssize_t length = 0;
	int bufavail = bufsize;

	struct pollfd pfd[1];
	pfd[0].fd = params->s_sock;
	pfd[0].events = POLLIN;

	while (1)
    {
    	int ret = poll(pfd, 1, 0);

    	if (ret == 0)
    		continue;

    	if (ret == -1)
    	{
    		perror("s_sock error");
    		return args;
    	}

    	if (pfd[0].revents & POLLIN)
    	{
    		if (bufavail == 0) continue;

    		length = recv(params->s_sock, buffer + (bufsize - bufavail), bufavail, 0);
    		bufavail -= length;

        	if (length < 0)
        	{
         	   perror("recv failed: ");
            	break;
        	}
        	else if (length == 0)
        	{
            	break;
        	}
        	else
        	{
        		struct pollfd pfd_s[1];
        		pfd_s[0].fd = params->c_sock;
        		pfd_s[0].events = POLLOUT;

        		int rets = poll(pfd_s, 1, 0);

        		if (rets > 0)
        		{
            		if (send(params->c_sock, buffer, length, 0) > 0)
            		{
						memset(buffer, 0, bufsize);
						bufavail = bufsize;
            		}
					else
					{
						perror("Sending request failed");
						break;
					}
				} else if (rets == 0)
				{
					if (bufavail > 0)
						continue;
					else
					{
						while (1)
						{
							int rets = poll(pfd_s, 1, 0);

							if (rets == 0)
								continue;

							else if (rets == -1)
							{
								perror("poll error in c_sock");
								close(params->s_sock);
								return args;
							}

							if (send(params->c_sock, buffer, length, 0) > 0)
            				{
								memset(buffer, 0, bufsize);
								bufavail = bufsize;
								//firstChunk = 0;
            				}
							else
							{
								perror("Sending request failed");
								close(params->s_sock);
								return args;
							}

						}
					}
				}
    		}
    	}
    }


    close(params->s_sock);
	return args;
}