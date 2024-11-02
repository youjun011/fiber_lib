#include "ioscheduler.h"
#include "hook.h"
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>

static int sock_listen_fd = -1;

void test_accept();

void test_accept()
{
    // std::cout<<"test_accept"<<std::endl;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t len = sizeof(addr);
    int fd = accept(sock_listen_fd, (struct sockaddr *)&addr, &len);
    // std::cout<<"accept sucess fd = "<<fd<<std::endl;
    if (fd < 0)
    {
        std::cout << "fd = " << fd << ", accept failed" << std::endl;
    }
    else
    {
        fcntl(fd, F_SETFL, O_NONBLOCK);
        sylar::IOManager::GetThis()->addEvent(fd, sylar::IOManager::READ, [fd]() {
    char buffer[4096];
    int ret = recv(fd, buffer, sizeof(buffer), 0);
    if (ret > 0)
    {
        // 简单处理 HTTP 请求，不做详细解析
        const char *response = "HTTP/1.1 200 OK\r\n"
                               "Content-Length: 13\r\n"
                               "Content-Type: text/plain\r\n"
                               "\r\n"
                               "Hello, World!";
        ssize_t total_sent = 0;
        size_t response_len = strlen(response);

        // 发送响应
        while (total_sent < response_len)
        {
            ssize_t sent = send(fd, response + total_sent, response_len - total_sent, 0);
            if (sent > 0)
            {
                total_sent += sent;
            }
            else if (sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                // 发送缓冲区满，等待套接字可写
                sylar::IOManager::GetThis()->addEvent(fd, sylar::IOManager::WRITE, [fd, response, response_len, total_sent]() mutable {
                    ssize_t sent = send(fd, response + total_sent, response_len - total_sent, 0);
                    if (sent > 0)
                    {
                        total_sent += sent;
                        if (total_sent >= response_len)
                        {
                            // 发送完成，关闭连接
                            close(fd);
                        }
                    }
                    else
                    {
                        // 发送出错，关闭连接
                        close(fd);
                    }
                });
                return;
            }
            else
            {
                // 发送出错，关闭连接
                close(fd);
                return;
            }
        }
        // 响应发送完成，关闭连接
        close(fd);
    }
    else
    {
        // 接收出错或客户端关闭连接，关闭套接字
        close(fd);
    }
});
    }
    // 重新注册监听套接字的读事件
    sylar::IOManager::GetThis()->addEvent(sock_listen_fd, sylar::IOManager::READ, test_accept);
}

void test_iomanager()
{
    // int portno = 8080;
    int portno=9527;
    struct sockaddr_in server_addr;
    // 设置套接字
    sock_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_listen_fd < 0)
    {
        std::cout << "Error creating socket." << std::endl;
        return;
    }
    int yes = 1;
    setsockopt(sock_listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portno);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock_listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cout << "Error binding socket." << std::endl;
        return;
    }
    if (listen(sock_listen_fd, 1024) < 0)
    {
        std::cout << "Error listening socket." << std::endl;
        return;
    }
    std::cout << "Echo server listening on port: " << portno << std::endl;
    fcntl(sock_listen_fd, F_SETFL, O_NONBLOCK);

    // 注册监听套接字的读事件
    sylar::IOManager::GetThis()->addEvent(sock_listen_fd, sylar::IOManager::READ, test_accept);
}

int main(int argc, char *argv[])
{
    sylar::IOManager iom;
    iom.scheduleLock(test_iomanager);
    return 0;
}
