#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <iostream>
#include <stack>
#include <string.h>

int main(int argc, char *argv[]){
    std::cout<<"开始测试\n";
    struct sockaddr_in addr; //maybe sockaddr_un;
    memset( &addr,0,sizeof(addr) );
    addr.sin_addr.s_addr=inet_addr("0.0.0.1");
    addr.sin_port=htons(8080);
    addr.sin_family=AF_INET;
    int client1=socket(PF_INET,SOCK_STREAM,0);
    std::cout<<"开始测试1\n";
    int ret=connect(client1,(sockaddr*)&addr,sizeof(sockaddr_in));
    std::cout<<"开始测试2\n";
    char buff[1024]="";
    ret=send(client1,"111",4,0);
    if(ret<=0){
        std::cout<<"send error\n";
        return -1;
    }
    ret=recv(client1,buff,1024,0);
    if(ret<=0){
        std::cout<<"recv error\n";
        return -2;
    }
    std::cout<<buff<<std::endl;
    return 0;
}