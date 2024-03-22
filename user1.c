#include "msocket.h"

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

    char buff[1024];
    int status;
    
    memset(buff, 0, sizeof(buff));
    sprintf(buff, "This is HELLO from user 1.");
    while((status = m_sendto(sockfd, buff, strlen(buff)+1, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr))) < 0){
        if(errno==ENOBUFS)continue;
        else{
            printf("Send failed from user 1\n");
            exit(EXIT_FAILURE);
        }
    }

    memset(buff, 0, sizeof(buff));
    int addrlen = sizeof(dest_addr);
    while((status = m_recvfrom(sockfd, buff, sizeof(buff), 0, (struct sockaddr*)&dest_addr, (socklen_t *)&addrlen)) < 0){
        if(errno == ENOMSG)continue;
        else{
            printf("Receive failed for user 1\n");
            exit(EXIT_FAILURE);
        }
    }
    printf("Received in user 1: %s\n", buff);

    
    while(1);
    //m_close(sockfd);

    return 0;

}