#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAXBUFF 10000

int main()
{

    int sockid;

    struct sockaddr_in cliaddr, servaddr;
    char buffer[MAXBUFF];

    // what the heck is AF_NET ;)

    sockid = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockid < 0)
    {
        printf("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(servaddr));

    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8181);
    // cliaddr.sin_zero=;  use of this field?


    // bind the socket to the port
    if (bind(sockid, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        printf("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // print in terminal on starting te server
    printf("Server is now live \n");

    int len = sizeof(cliaddr);
    int n;
    // recieve the filename from the client
    if ((n = recvfrom(sockid, buffer, MAXBUFF - 1, 0, ( struct sockaddr *)&cliaddr, &len)) < 0)
    { 
        perror("Recieve failed\n");
        exit(EXIT_FAILURE);
    }
    buffer[n]= '\0';
  
    FILE *filepointer;
    filepointer = fopen(buffer, "r");  // open the file
    if (filepointer == NULL)
    {
        // if file not found send the error message NOTFOUND : filename to the client 
        printf("The reqired file:%s not found\n", buffer);
        char errmsg[]="NOTFOUND :";
        char* tmmp=strdup(strcat(errmsg,buffer));
        sendto(sockid, errmsg, strlen(errmsg), 0, ( struct sockaddr *)&cliaddr, len);
        printf("%s\n",errmsg);
        exit(EXIT_FAILURE);
    }
    else{
        char* errmsg="FOUND";
        sendto(sockid, errmsg, strlen(errmsg), 0, ( struct sockaddr *)&cliaddr, len);
    }




    // start reading the file

    char hello[] = "HELLO\n";

    fgets(buffer, MAXBUFF, filepointer);  // read HELLO from the file 
    // printf("Buffer is %s", buffer);
    if (strcmp(hello, buffer) != 0)
    {
        printf("HELLO not found at the start of the file provide correct file\n");
        sendto(sockid, buffer, strlen(buffer), 0, ( struct sockaddr *)&cliaddr, len);
        exit(EXIT_FAILURE);
    }

    //correct till here


    len=sizeof(cliaddr);
    if (sendto(sockid, buffer, strlen(buffer), 0, ( struct sockaddr *)&cliaddr, len) != strlen(buffer))
    {
        printf("Couldn't send the message\n");
        exit(EXIT_FAILURE);
    }

    char last[] = "END";



    for (;;)
    {

        // send the words one by one until END is found if END is found close the server 
        n = recvfrom(sockid, buffer, MAXBUFF - 1, 0, (struct sockaddr *)&cliaddr, &len);
        if (n < 0)
        {
            printf("failed to recieve message");
        }
        fscanf(filepointer, "%s", buffer);
        buffer[strlen(buffer)]='\0';


        n = sendto(sockid, buffer, strlen(buffer), 0, (struct sockaddr *)&cliaddr, len);
        if (n < strlen(buffer))
        {
            printf("Failed to send the word\n");
            exit(EXIT_FAILURE);
        }
        if (strcmp(buffer, last) == 0)
        {
            fclose(filepointer);
            break;
        }
    }    
    // this is not a good tranfer protocol since this is unreliable and also we are not sure wether everything gets sent

    exit(EXIT_SUCCESS);
}