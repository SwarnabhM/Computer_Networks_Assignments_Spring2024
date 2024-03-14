#include "msocket.h"

int main(){

    int sockfd = m_socket(AF_INET, SOCK_MTP, 0);
    printf("socket created %d\n", sockfd);


    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    dest_addr.sin_port = htons(10010);
    int err = m_bind(sockfd, htonl(INADDR_ANY), htons(10020), inet_addr("127.0.0.1"), htons(10010));
    printf("bind err %d\n", err);

    // sleep(5);

    char buff[1024];
    for(int i=0; i<24; i++){
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        int addrlen = sizeof(client_addr);
        memset(buff, 0, sizeof(buff));
        while(1){
            sleep(1);
            int recvs = m_recvfrom(sockfd, buff, sizeof(buff), 0, (struct sockaddr *)&client_addr, &addrlen);
            if(recvs!=0){
                printf("recvs:%d\n", recvs);
                if(recvs>0){
                    printf("%s\n", buff);
                    break;
                }
                else{
                    if (errno = ENOMSG)printf("no msg\n");
                }
            }
        }
        memset(buff, 0, sizeof(buff));
        sprintf(buff, "this is message no. %d from user 2", i);
        int sends = m_sendto(sockfd, buff, strlen(buff)+1, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        printf("sent: %d\n", sends);
    }

    while(1);

    return 0;

}