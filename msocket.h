#ifndef MSOCKET_H
#define MSOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>


// Define constants for semaphores
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

// Define the MTP socket type
#define SOCK_MTP 100
// Define the value of parameter T
#define T 5
// Define the maximum number of active MTP sockets
#define MAX_MTP_SOCKETS 25
//max msg size
#define MAX_MSG_SIZE 1024

#define SENDER_MSG_BUFFER 10
#define RECEIVER_MSG_BUFFER 5


// Define the structure for a data message header
typedef struct {
    char msg_type; //'D' for data, 'A' for acknowledgment message
    int seq_no;
}MessageHeader;

typedef struct {
    MessageHeader header;
    char msg[MAX_MSG_SIZE];
} Message;

typedef struct {
    int ack_no;
    time_t time;
    int sent;
    Message message;
}send_msg;

typedef struct {
    int ack_no;
    char message[MAX_MSG_SIZE];
}recv_msg;

typedef struct {
    int window_size;
    int window_start_index;
    int last_seq_no;
    send_msg send_buff[SENDER_MSG_BUFFER];
} swnd;

typedef struct {
    int window_size;
    int index_to_read;
    int next_seq_no;
    int index_to_write;
    int nospace;
    recv_msg recv_buff[RECEIVER_MSG_BUFFER];
} rwnd;

// Define the structure for an MTP socket entry in shared memory
typedef struct {
    int socket_alloted;
    pid_t process_id;
    int udp_socket_id;
    struct sockaddr_in destination_addr;
    swnd send_window;
    rwnd recv_window;
} MTPSocketEntry;

// Structure to hold socket creation and binding information
typedef struct {
    int sock_id;
    unsigned long IP;
    unsigned short port;
    int errno_val;
} SOCK_INFO;


// Function to open an MTP socket
int m_socket(int domain, int type, int protocol);

// Function to bind an MTP socket
int m_bind(int sockfd, unsigned long src_ip, unsigned short src_port, unsigned long dest_ip, unsigned short dest_port);

// Function to send data over an MTP socket
ssize_t m_sendto(int sockfd, const void *buf, size_t len, int flags,
                 const struct sockaddr *dest_addr, socklen_t addrlen);

// Function to receive data from an MTP socket
ssize_t m_recvfrom(int sockfd, void *buf, size_t len, int flags,
                   struct sockaddr *src_addr, socklen_t *addrlen);

// Function to close an MTP socket
int m_close(int sockfd);

#endif /* MSOCKET_H */
