#include "msocket.h"

int main(){

    int sockfd = m_socket(AF_INET, SOCK_MTP, 0);
    printf("socket created %d\n", sockfd);


    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    dest_addr.sin_port = htons(10020);
    int err = m_bind(sockfd, htonl(INADDR_ANY), htons(10010), inet_addr("127.0.0.1"), htons(10020));
    printf("bind err %d\n", err);

    // sleep(5);

    char buff[1024];
    for(int i=0; i<32; i=i+8){
        for(int j=0; j<8; j++){
            memset(buff, 0, sizeof(buff));
            sprintf(buff, "this is message no. %d from user 1", i+j);
            int sends = m_sendto(sockfd, buff, strlen(buff)+1, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            printf("sent: %d\n", sends);
        }

        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        int addrlen = sizeof(client_addr);
        memset(buff, 0, sizeof(buff));
        int cnt = 0;
        while(1){
            sleep(1);
            int recvs = m_recvfrom(sockfd, buff, sizeof(buff), 0, (struct sockaddr *)&client_addr, &addrlen);
            if(recvs!=0){
                printf("recvs:%d\n", recvs);
                if(recvs>0){
                    cnt++;
                    printf("%s\n", buff);
                    if(cnt==8)break;
                }
                else{
                    if(errno == ENOMSG) printf("nomsg\n");
                    if(errno == EBADF) printf("bad file\n");
                }
            }
        }
    }


    
    while(1);

    return 0;

}