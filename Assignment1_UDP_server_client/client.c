#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#define MAXBUFF 10000

int main(int argc,char*argv[]){

    int sockid;
    struct sockaddr_in cliaddr,servaddr;


    char *filename;  //filename passed as a command line argument
    if(argc>1){
        filename=argv[1];
    }
    else{
        filename="data.txt";
    }

    sockid=socket(AF_INET,SOCK_DGRAM,0);  // create socket
    if(sockid<0){
        printf("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&cliaddr,0,sizeof(cliaddr));   // clean the addresses
    memset(&servaddr,0,sizeof(servaddr));

    // set the address
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(8181);
    
    // convert ip and set it
    int err= inet_aton("127.0.0.1",&servaddr.sin_addr);
    if(err==0){
        printf("Conversion of ip to integer failed\n");
        exit(EXIT_FAILURE);
   }


    //send  filename to server and check if message sent properly
    if(sendto(sockid,(const char *)filename,strlen(filename),0,( struct sockaddr*) &servaddr,sizeof(servaddr))!=strlen(filename)) //soxkaddr vs sockaddr_in ?
    {
        perror("Couldnt send the message properly\n");
        exit(EXIT_FAILURE);
    }

    char buffer[MAXBUFF];
    int len=sizeof(servaddr);

    int n=recvfrom(sockid,buffer,MAXBUFF-1,0,( struct sockaddr*)&servaddr,&len);
    {
        // server sends response on recieving filename, if it is not found then it send NOTFOUND : filename else it send a message FOUND
        if(strcmp(buffer,"FOUND")!=0){
            printf("FILE <%s> not found\n",filename);
            exit(EXIT_FAILURE);
        }

    }
    // if file is present the server sends hello and client recieves it
     n=recvfrom(sockid,buffer,MAXBUFF-1,0,( struct sockaddr*)&servaddr,&len);
    if(n<0){
        perror("Error in recieving message from server\n");
        exit(EXIT_FAILURE);
    }

    // check if HELLO is returned

    if(strcmp(buffer,"HELLO\n")!=0){
        printf("HELLO not found at the start of the document, exiting\n");
        exit(EXIT_FAILURE);
    }
    else printf("HELLO\n");  // prints in terminal
    
    FILE* outputfile=fopen("ouput.txt","w");  // if hello is present then open a new file
    char last[]="END";
    int i=1;
    char num[1000];
    fflush(outputfile);  //flush any buffer

    for(;;){

        sprintf(num,"%d",i);
        i++;
        char word[50]="WORD"; 
        char* curword=strcat(word,num);  //check if correct string

        printf("%s : ",word);

        // n = sendto(sockid, buffer, strlen(buffer), 0, (struct sockaddr *)&cliaddr, len);
        n=sendto(sockid,(const char *)curword,strlen(curword),0,(const struct sockaddr*)&servaddr,sizeof(servaddr));
        if(n!=strlen(curword)){
            perror("Couldn't send the message properly\n");
            exit(EXIT_FAILURE);
        }

        // n = recvfrom(sockid, buffer, MAXBUFF - 1, 0, (struct sockaddr *)&cliaddr, &len);

        n=recvfrom(sockid,buffer,MAXBUFF-1,0,( struct sockaddr*)&servaddr,&len);

        if(n<0){
            perror("Error in recieving the message");
            exit(EXIT_FAILURE);
        } 


        //  CHECK
        buffer[n]='\0';  //append null to end string
        printf("%s\n",buffer);

        fprintf(outputfile,"%s\n",buffer);//  print in output file
        fflush(outputfile);// flush the buffer to print everything in the output file


        if(strcmp(buffer,last)==0){
            fclose(outputfile);  // close and break if END is recieved
            break;
        }

    }


    exit(EXIT_SUCCESS);



// blocking non blocking 

}
