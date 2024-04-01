#ifndef NETWORK_TRANSFER_H
#define NETWORK_TRANSFER_H
#include <stdlib.h>

int initialize_network(int *sockfd_p, int port, int ip_offset);
int server_initialize_network(int *server_fd_p, int port);
int get_local_ip_lastnum(int *lastnum_p);

void *Send(int sockfd, const void *buf, size_t len, const char *str);
void *Recv(int sockfd, void *buf, size_t len, char *str);
void *Send_Response(int sockfd);
void *Recv_Response(int sockfd);


void *send_metadata_chunk(void *arg);
void *recv_metadata_chunk(void *arg);
void *send_metadata(void *arg);
void *recv_chunk(void *arg);
void *send_chunk(void *arg);

#endif // NETWORK_TRANSFER_H
