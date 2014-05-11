#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<signal.h>
#include<unistd.h>

int main(int argc, char** argv){
    
    struct sockaddr_in sockserv,sockclient;
    int socketfd,clientfd;
    socklen_t clientsocklen;
    
    int filefd;
    int cnt;
    int finalcnt = 0;
    char buff[BUFSIZ];
    pid_t childprocess;
    signal(SIGPIPE,SIG_IGN);
    socketfd = socket(AF_INET,SOCK_STREAM,0);
    printf("Socket Created: %s\n",strerror(errno));
    int opt=1;
    int size = 0,closecnt = 0;
    
    bzero(&sockserv,sizeof(sockserv));
    sockserv.sin_family = AF_INET;
    sockserv.sin_addr.s_addr = INADDR_ANY;
    sockserv.sin_port = htons(8000);
    
    //Reusable Sockets
    setsockopt(socketfd,SOL_SOCKET,SO_REUSEADDR,(const char *)&opt,sizeof(int));
    
    bind(socketfd,(struct sockaddr *)&sockserv,sizeof(sockserv));
    printf("Socket Bind: %s\n",strerror(errno));
    listen(socketfd,10);
    printf("Socket Listen: %s\n",strerror(errno));
    
    while (1){
        remove("out.ogg");
        clientfd = accept(socketfd,(struct sockaddr*)&sockclient,&clientsocklen);
        
        memset(buff,0, BUFSIZ);
        read(clientfd,buff,BUFSIZ);
        printf("%s\n\n",buff);
        strcpy(buff,"HTTP/1.0 200 OK\r\nDate: Tue, 01 Mar 2011 06:14:58 GMT\r\nConnection: close\r\nCache-control: private\r\nContent-type: video/ogg\r\nServer: lighttpd/1.4.26\r\n\r\n");
        
        cnt = send(clientfd,buff,strlen(buff),0);
        /*./ffmpeg -f video4linux2 -r 25 -s 640x480 -i /dev/video0 out.ogg*/
        childprocess = fork();
        if (childprocess == 0){
            argc =11;
            argv[0]="./ffmpeg";
            argv[1]= "-f";
            argv[2]= "video4linux2";
            argv[3]= "-r";
            argv[4]="25";
            argv[5]= "-s";
            argv[6]="640x480";
            argv[7]="-i";
            argv[8]="/dev/video0";
            argv[9]="out.ogg";
            argv[10]=(char*)0;
            
            int returnvalue;
            returnvalue =execvp(argv[0],argv);
            printf("debug:%i\n", returnvalue);
        }
        else{
            sleep(10);
            printf("Sent this reply : %s \nwith size = %d : %s\n",buff,cnt,strerror(errno));
            errno = 0;
            filefd = open("out.ogg",O_RDONLY);
            printf("File open: %s\n",strerror(errno));
            sleep(1);
            while(1){
                cnt = read(filefd,buff,1024);
                if(strcmp(strerror(errno), "Broken pipe")==0){
                    
                    close(clientfd);
                    break;
                }
                size = size + cnt;
                printf("size: %i\n", size);
                printf("Read %d bytes 1 from the file : %s\n",cnt,strerror(errno));
                if(cnt <= 0){
                    sleep(1);
                    closecnt++;
                    if (closecnt == 10){
                        close(clientfd);
                        break;
                    }
                }
                else {
                    closecnt = 0;
                    if(strcmp(strerror(errno), "Broken pipe")==0){
                        
                        close(clientfd);
                        break;
                    }
                    errno = 0;
                    finalcnt = send(clientfd,buff,cnt,0);
                    printf("Data written 1 = %d bytes: %s\n",finalcnt,strerror(errno));
                }   
            }
            kill(childprocess,SIGTERM);
        }
    }
    return 0;   
}