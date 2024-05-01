#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/wait.h> // is this required?

/*************TAKE CARE OF ERRORS DON'T CLOSE SERVER IF ERROR ************************/

typedef struct sockaddr_in sockaddr_in;
#define MAXLEN 100-sizeof(int)

int main()
{

    int sockfd, newsockfd;
    int clilen;
    sockaddr_in serv_addr, cli_addr;
    char buffer[MAXLEN];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed\n");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(20000);

    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
    {
        printf("Unable to bind local address\n");
        close(sockfd);
        exit(1);
    }

    listen(sockfd, 10);

    while (1)
    {

        clilen = sizeof(cli_addr);
        printf("Waiting for new connection\n");
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        if (newsockfd < 0)
        {
            perror("socket creation failed during accept\n");
            // exit(0);
            continue;
        }
        else{
            printf("New client accepted and new socket created for the client\n");
        }
        pid_t pid = fork();
        if(pid<0){
            perror("error while forking\n");
            close(newsockfd);
            exit(1);
        }
        if (pid == 0)
        {

            /*CHILD PROCESS, PERFORM PARSING HERE */

            close(sockfd); // didin't understand this part read again

            /******use only system calls ***********/

            // transfer begins

            // recieve k
          
            int k;
            int si=recv(newsockfd, &k, sizeof(int), 0);
            printf("key: %d", k);
            if(si!=sizeof(int)){
                perror("error in receiving the vlaue of k\n");
                close(newsockfd);
                exit(EXIT_FAILURE);
            }
            // error check

            printf("value of k recieved is %d\n", k);
            printf("Initiating source file transfer...\n");

            ssize_t sizrecv, writesize;

            char cliIP[INET_ADDRSTRLEN];
            char cliport[10];
            char filename1[MAXLEN];
            char filename2[MAXLEN];
            inet_ntop(AF_INET, &(cli_addr.sin_addr), cliIP, INET_ADDRSTRLEN);
            sprintf(cliport, "%d", ntohs(cli_addr.sin_port));
            strcpy(filename1, cliIP);
            strcat(filename1, ".");
            strcat(filename1, cliport);
            strcat(filename1, ".txt");
            int fd1 = open(filename1, O_RDWR | O_CREAT | O_TRUNC, 0666);

            // file name should be <IP.port of client>.txt
            // recieve the file in chunks (size used by client is uk to server)
            // it copies the file on the go to a temp file
            while ((sizrecv = recv(newsockfd, buffer, MAXLEN, 0)) > 0)
            {

                writesize = write(fd1, buffer, sizrecv);
                /*******************************/ // check the part below
                if (writesize != sizrecv)
                {
                    perror("Error while writing file\n");
                    close(newsockfd);
                    exit(1);

                }
            }
            if (sizrecv == -1)
            {
                perror("Error while recieving data from server\n");
                    close(newsockfd);
                    exit(1);
            }

            printf("File recieved successfully\n");
            printf("Initiating Caeser Cipher protocol...\n");
            // new file opened <IP.port of client>.txt.enc
            strcpy(filename2, filename1);
            strcat(filename2, ".enc");
            
            close(fd1);
            fd1=open(filename1,O_RDONLY);
            
            // open();
            int fd2 = open(filename2, O_RDWR | O_CREAT | O_TRUNC, 0666);

            ssize_t sentsize, readsize;

            // Caeser cipher written in new file
            while ((readsize = read(fd1, buffer, MAXLEN)) > 0)
            {

                for (int i = 0; i < readsize; i++)
                {
                    if (buffer[i] >= 'a' && buffer[i] <= 'z')
                    {
                        buffer[i] = 'a' + ((buffer[i] - 'a' + k) + 26) % 26;
                    }
                    else if (buffer[i] >= 'A' && buffer[i] <= 'Z')
                    {
                        buffer[i] = 'A' + ((buffer[i] - 'A' + k) + 26) % 26;
                    }
                }
                printf(". ");
                writesize = write(fd2, buffer, readsize);
                if (writesize != readsize)
                {
                    perror("\nError while writing Caesar cipher\n");
                    close(newsockfd);
                    close(fd1);
                    close(fd2);
                    exit(1);
                }
            }
            close(fd1);
            close(fd2);
            if (readsize == -1)
            {
                perror("\nError while reading the source file\n");
                close(newsockfd);
                exit(1);
            }
            printf("\nCaeser Cipher Completed Succesfully\n");

            // send the file in chunks back to client
            printf("Sending encrypted data to client\n");
            fd2=open(filename2,O_RDONLY);

            while((readsize=read(fd2,buffer,MAXLEN))){

                sentsize=send(newsockfd,buffer,readsize,0);
                if(sentsize!=readsize){
                    perror("Error during sending encrypted data to client\n");
                    // check this part
                    close(newsockfd);
                    close(fd2);
                    exit(1);
                }
            }
            close(fd2);
            if(readsize==-1){
                perror("Error while reading encrypted data\n");
                // check this part
                close(newsockfd);
                exit(1);

            }
            printf("Encrypted data sent back to client successfully\n\n");
            close(newsockfd);
            exit(0);
        }


        close(newsockfd);
    }
    return 0;
}