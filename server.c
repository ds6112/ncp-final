#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<signal.h>
#include<unistd.h>
#define MAXBUF 8196

char * parse(char request[MAXBUF]);
void indexhtml(int clientfd);
void webstream(int clientfd);

int main(int argc, char** argv){
    
    struct sockaddr_in sockserv,sockclient;
    int socketfd,clientfd;
    socklen_t clientsocklen;
    char request[MAXBUF];
    signal(SIGPIPE,SIG_IGN);

    socketfd = socket(AF_INET,SOCK_STREAM,0);
    printf("Socket Created: %s\n",strerror(errno));
    int opt=1;
    bzero(&sockserv,sizeof(sockserv));
    sockserv.sin_family = AF_INET;
    sockserv.sin_addr.s_addr = INADDR_ANY;
    sockserv.sin_port = htons(8001);
    
    //Reusable Sockets
    setsockopt(socketfd,SOL_SOCKET,SO_REUSEADDR,(const char *)&opt,sizeof(int));
    
    bind(socketfd,(struct sockaddr *)&sockserv,sizeof(sockserv));
    printf("Socket Bind: %s\n",strerror(errno));
    listen(socketfd,10);
    printf("Socket Listen: %s\n",strerror(errno));
    
    while (1){
        clientfd = accept(socketfd,(struct sockaddr*)&sockclient,&clientsocklen);
        
        memset(request,0, MAXBUF);
        read(clientfd,request,MAXBUF);
        /*Do not handle any non GET requests*/
        if(strncmp(request, "GET", 3)!=0){
            close(clientfd);
            continue;
        }
        printf("%s\n\n",request);

        char *address;
        address = (char *)malloc(sizeof(parse(request)));
        memset(address, 0, sizeof(address));
        address = parse(request);
        if ((!strncmp(address,"/index.html", strlen(address)))&&strlen(address)==11){
            indexhtml(clientfd);
        }
        else if ((!strncmp(address, "/", strlen(address)))&&strlen(address)==1){
            indexhtml(clientfd);
        }
        else if ((!strncmp(address, "/out.ogg", strlen(address)))&&strlen(address)==8){
            webstream(clientfd);
        }

        close(clientfd);
    }
    return 0;   
}

char * parse(char request[MAXBUF]){
    char * target;
    target = strtok (request," ");
    target = strtok (NULL, " ");
    return target;
}

void indexhtml(int clientfd){
    char ihtml[MAXBUF];
    memset(ihtml, 0, MAXBUF);
    strcpy(ihtml,"HTTP/1.0 200 OK\r\nContent-type: text/html\r\n\r\n\r\n");
    send(clientfd, ihtml, strlen(ihtml), 0);
    int filefd, filesize;
    filefd = open("index.html", O_RDONLY);
    memset(ihtml, 0, MAXBUF);
    filesize = read(filefd, ihtml, MAXBUF);
    send(clientfd, ihtml, strlen(ihtml), 0);
}

void webstream(int clientfd){
    remove("out.ogg");
    char video[MAXBUF];
    pid_t record;
    int bytesread;
    int bytessent;
    int filesize;
    int closecounter = 0;
    char * argv[11];
    int videostream;

    memset(video, 0, MAXBUF);
    strcpy(video,"HTTP/1.0 200 OK\r\n\r\nConnection: close\r\nCache-control: public\r\nContent-type: video/ogg\r\n\r\n\r\n");
    send(clientfd, video, strlen(video), 0);
    record = fork();
    if (record == 0){
        argv[0]="./ffmpeg";
        argv[1]= "-f";
        argv[2]= "video4linux2";
        argv[3]= "-r";
        argv[4]="30";
        argv[5]= "-s";
        argv[6]="640x480";
        argv[7]="-i";
        argv[8]="/dev/video0";
        argv[9]="out.ogg";
        argv[10]=(char*)0;
        sleep(1);
        execvp(argv[0],argv);
    }
    else{
        sleep(5);
        errno = 0;
        while((videostream = open("out.ogg",O_RDONLY))==-1){
            ;
        }
        
        sleep(1);
        while(1){
            bytesread = read(videostream,video,1024);
            if(strcmp(strerror(errno), "Broken pipe")==0){   
                close(clientfd);
                close(videostream);
                break;
            }
            filesize = filesize + bytesread;
            if(bytesread <= 0){
                sleep(1);
                closecounter++;
                if (closecounter == 1000000){
                    close(clientfd);
                    close(videostream);
                    break;
                }
            }
            else {
                closecounter = 0;
                if(strcmp(strerror(errno), "Broken pipe")==0){
                    close(clientfd);
                    close(videostream);
                    break;
                }
                errno = 0;
                bytessent = send(clientfd, video,bytesread,0);
                printf("Read %d bytes from the file:Total %i\n",bytesread,filesize);
                printf("Data written = %d bytes: %s\n", bytessent,strerror(errno));
            }   
        }
        kill(record,SIGTERM);
    }
}