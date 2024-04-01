#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <netdb.h>
#include <ifaddrs.h>

#include "ec_config.h"

int initialize_network(int *sockfd_p, int port, int ip_offset)
{
    struct sockaddr_in server_addr;
    int ip_addr_start = STORAGENODES_START_IP_ADDR;
    int sockfd;

    /* Create socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        ERROR_RETURN_VALUE("Fail create socket");

    /* Create sockaddr */
    char ip_addr[16];
    sprintf(ip_addr, "%s%d", IP_PREFIX, ip_addr_start + ip_offset);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip_addr, &server_addr.sin_addr) <= 0)
        ERROR_RETURN_VALUE("Invalid IP address");

    /* Connect data datanode of ip_addr */
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Error connecting to %s\n", ip_addr);
        return EC_ERROR;
    }
    *sockfd_p = sockfd;
    return EC_OK;
}

int server_initialize_network(int *server_fd_p, int port)
{
    int server_fd;
    struct sockaddr_in server_addr;

    /* create socket */
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        ERROR_RETURN_VALUE(" Fail create server socket");

    /* can reuse */
    int reuse = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    /* bind address and port */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        ERROR_RETURN_VALUE("Fail bind socket");

    /* listen socket */
    if (listen(server_fd, EC_N) == -1)
        ERROR_RETURN_VALUE("Fail listen socket");

    *server_fd_p = server_fd;
    return EC_OK;
}

int get_local_ip_lastnum(int *lastnum_p)
{
    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *ifa = NULL;
    void *tmpAddrPtr = NULL;

    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr->sa_family == AF_INET)
        { // IPv4 address
            tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            if (strncmp(addressBuffer, IP_PREFIX, 10) == 0)
            {
                char *lastNumStr = strrchr(addressBuffer, '.') + 1;
                *lastnum_p = atoi(lastNumStr);
                return EC_OK;
            }
        }
    }

    if (ifAddrStruct != NULL)
        freeifaddrs(ifAddrStruct);
    return EC_ERROR;
}

void *Send(int sockfd, const void *buf, size_t len, const char *str)
{
    if (send(sockfd, buf, len, 0) < 0)
        ERROR_RETURN_NULL(str);
    return NULL;
}
void *Recv(int sockfd, void *buf, size_t len, char *str)
{
    if (recv(sockfd, buf, len, MSG_WAITALL) < 0)
        ERROR_RETURN_NULL(str);
    return NULL;
}

void *Send_Response(int sockfd)
{
    int error_response = 1;
    if (send(sockfd, &error_response, sizeof(error_response), 0) < 0)
        ERROR_RETURN_NULL("send response");
    return NULL;
}
void *Recv_Response(int sockfd)
{
    int error_response = 0;
    if (recv(sockfd, &error_response, sizeof(error_response), MSG_WAITALL) < 0)
        ERROR_RETURN_NULL("recv response case 1");
    if (error_response == 0)
        ERROR_RETURN_NULL("recv response case 2");
    return NULL;
}

/* pthread func: Used to send to or receive from multiple nodes */
void *send_metadata_chunk(void *arg)
{
    metadata_t *metadata = (metadata_t *)arg;
    Send(metadata->sockfd, metadata, sizeof(metadata_t), "send metadata");
    Send(metadata->sockfd, metadata->data, CHUNK_SIZE, "send chunk");
    Recv_Response(metadata->sockfd);
    return NULL;
}

void *recv_metadata_chunk(void *arg)
{
    metadata_t *metadata = (metadata_t *)arg;
    Send(metadata->sockfd, metadata, sizeof(metadata_t), "send metadata");
    Recv(metadata->sockfd, metadata->data, CHUNK_SIZE, "recv chunk");
    Send_Response(metadata->sockfd);
    return NULL;
}

void *send_metadata(void *arg)
{
    metadata_t *metadata = (metadata_t *)arg;
    Send(metadata->sockfd, metadata, sizeof(metadata_t), "send metadata");
    Recv_Response(metadata->sockfd);
    return NULL;
}

void *recv_chunk(void *arg)
{
    metadata_t *metadata = (metadata_t *)arg;
    Recv(metadata->sockfd, metadata->data, CHUNK_SIZE, "recv chunk");
    Send_Response(metadata->sockfd);
    return NULL;
}

void *send_chunk(void *arg)
{
    metadata_t *metadata = (metadata_t *)arg;
    Send(metadata->sockfd, metadata->data, CHUNK_SIZE, "send chunk");
    Recv_Response(metadata->sockfd);
    return NULL;
}