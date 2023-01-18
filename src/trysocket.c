#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

int sockfd, newsockfd, portno, clilen, n;
struct sockaddr_in serv_addr, cli_addr;
struct hostent *server;
char cmd_c[10];

int main(int argc, char const *argv[])
{
    printf("server mode\n");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd <0){
        perror("error in opening socket");
    }
    //get port number 
    printf("portnumber to use:");
    scanf("%d", &portno);
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(portno);
    serv_addr.sin_addr.s_addr=INADDR_ANY;
    //bind
    if(bind(sockfd, (struct sockadrr *)&serv_addr, sizeof(serv_addr))<0){
        perror("error in bind");
    }

    //listen
    listen(sockfd,5); 

    //accept
    clilen=sizeof(cli_addr);
    newsockfd = accept(sockfd,(struct sockadrr *)&cli_addr, &clilen);
    if(newsockfd <0){
        perror("error in accept socket");
    }
    while(1){
        bzero(cmd_c, strlen(cmd_c));
        read(newsockfd, cmd_c, sizeof(cmd_c));
        printf("%s\n", cmd_c);
    }
}

