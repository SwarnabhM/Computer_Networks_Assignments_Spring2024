#include "msocket.h"
#define BLOCK_SIZE 1024

int main(int argc, char *argv[]){

    if(argc<5){
        printf("Please provide 'source IP, source port, destination IP and destination port (respectively)' as command line arguments\n");
        exit(0);
    }

    char source_IP[INET_ADDRSTRLEN], dest_IP[INET_ADDRSTRLEN];
    strcpy(source_IP, argv[1]);
    strcpy(dest_IP, argv[3]);
    unsigned short source_port = atoi(argv[2]);
    unsigned short dest_port = atoi(argv[4]);

    int sockfd = m_socket(AF_INET, SOCK_MTP, 0);
    if(sockfd<0){
        printf("Socket creation failed!\n");
        exit(EXIT_FAILURE);
    }

    int bind_status = m_bind(sockfd, inet_addr(source_IP), htons(source_port), inet_addr(dest_IP), htons(dest_port));
    if(bind_status<0){
        printf("Socket bind failed.\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(dest_IP);
    dest_addr.sin_port = htons(dest_port);

    int file_descriptor = open("content.txt", O_RDONLY);
    if(file_descriptor == -1) {
        printf("Error opening the file.\n");
        exit(EXIT_FAILURE);
    }

    char buff[BLOCK_SIZE];
    ssize_t bytes_read, bytes_sent;
    // Send file contents in blocks of size 1024 bytes/characters
    memset(buff, 0, sizeof(buff));
    while ((bytes_read = read(file_descriptor, buff, BLOCK_SIZE-1)) > 0) {
        while((bytes_sent = m_sendto(sockfd, buff, strlen(buff), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr))) < 0){
            if(errno==ENOBUFS)continue;
            else{
                printf("Send failed from user 1\n");
                close(file_descriptor);
                exit(EXIT_FAILURE);
            }
        }
        memset(buff, 0, sizeof(buff));
    }

    // Send termination block
    char termination_block[] = "\r\n.\r\n";
    while((bytes_sent = m_sendto(sockfd, termination_block, strlen(termination_block), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr))) < 0){
        if(errno==ENOBUFS)continue;
        else{
            printf("Send failed from user 1\n");
            close(file_descriptor);
            exit(EXIT_FAILURE);
        }
    }

    close(file_descriptor);

    
    while(1);
    //m_close(sockfd);

    return 0;

}