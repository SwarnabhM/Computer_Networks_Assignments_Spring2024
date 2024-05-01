#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

typedef struct sockaddr_in sockaddr_in;
#define MAXLEN 100

int main()
{
	int			sockfd ;
    int k,t;
    sockaddr_in	serv_addr;

	int i;
	char buf[MAXLEN];
	char filename[MAXLEN];


	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(1);
	}
// set server address
	serv_addr.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(20000);


	
	int fd;

while(1){

    
    while (1){
        
        // get filename
        printf("Provide the file name\n");
        scanf("%s",filename);
        fd=open(filename,O_RDONLY);
        if(fd==-1){
            perror("invalid filename provided\n");
            continue;
        }
        break;
    }
    // get k
        printf("Give the value of k\n");
        scanf("%d",&k);                                    

        // create socket
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("Unable to create socket\n");
            exit(1);
        }

        // connect 
        if ((connect(sockfd, (struct sockaddr *) &serv_addr,
                            sizeof(serv_addr))) < 0) {
            perror("Unable to connect to server\n");
            close(sockfd);
            close(fd);
            exit(1);
        }
        sprintf(buf,"%d",k);
        
        // send(sockfd, buf, strlen(buf) + 1, 0);
        send(sockfd, &k, sizeof(int), 0);

        ssize_t chunksize,sentsize,sizrecv,writesize;

        // send file 
        while((chunksize=read(fd,buf,MAXLEN))>0){
            sentsize=send(sockfd,buf,chunksize,0);
            if(sentsize!=chunksize){
                perror("error during sending message to server\n");
                close(fd); 
                // ****************************************
                exit(1);
            }
        }
        // close(fd1);
        close(fd);

        if(chunksize==-1){
            perror("Error while reading the source file\n");
            exit(1);
        }

        

        shutdown(sockfd,SHUT_WR);

        strcat(filename,".enc");
        fd=open(filename,O_RDWR|O_CREAT|O_TRUNC,0666);
        
        // receive the encrypted file back
        while ((sizrecv = recv(sockfd, buf, MAXLEN, 0)) > 0){

                writesize = write(fd, buf, sizrecv);
                /*******************************/ // check the part below
                if (writesize != sizrecv)
                {
                    perror("Error while writing file\n");
                    close(fd);
                    exit(1);
                }
            }

        close(fd);
            if (sizrecv == -1)
            {
                perror("Error while recieving data from server\n");
                close(sockfd);
                exit(1);
                    // do something here
            }

        
        close(sockfd);

            printf("File Encryption Completed, press 1 to encrypt another file else press 0\n\n");
            scanf("%d",&t);
            if(t!=1){
                break;
            }

}

exit(0);




}