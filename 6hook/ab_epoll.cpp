#include <iostream>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>

#define MAX_EVENTS 1024
#define READ_BUFFER_SIZE 4096

// set to non-block so that we could have EAGAIN and EWOULDBLOCK as expected.
// accept / send / recv could block.
void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char *argv[]) {
    int port = 9527, opt = 1, listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket error");
        return -1;
    }
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(port);

    if (bind(listen_fd, (sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind error");
        close(listen_fd);
        return -1;
    }
    if (listen(listen_fd, 1024) == -1) {
        perror("listen error");
        close(listen_fd);
        return -1;
    }
    set_nonblocking(listen_fd);
    int epoll_fd = epoll_create(1);
    if (epoll_fd == -1) {
        perror("epoll_create error");
        close(listen_fd);
        return -1;
    }

    epoll_event event;
    event.events  = EPOLLIN | EPOLLET;
    event.data.fd = listen_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) == -1) {
        perror("epoll_ctl error");
        close(listen_fd);
        close(epoll_fd);
        return -1;
    }
    std::vector<epoll_event> events(MAX_EVENTS);
    while (true) {
        int n = epoll_wait(epoll_fd, events.data(), MAX_EVENTS, -1);
        if (n == -1) {
            perror("epoll_wait error");
            break;
        }

        for (int i = 0; i < n; ++i) {
            int fd      = events[i].data.fd;
            uint32_t ev = events[i].events;

            if (fd == listen_fd) {
                while (true) {
                    sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int conn_fd = accept(listen_fd, (sockaddr *)&client_addr, &client_len);
                    if (conn_fd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // All connections are done.
                            break;
                        } else {
                            perror("accept error");
                            break;
                        }
                    }

                    set_nonblocking(conn_fd);

                    epoll_event conn_event;
                    conn_event.events  = EPOLLIN | EPOLLET;
                    conn_event.data.fd = conn_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &conn_event) == -1) {
                        perror("epoll_ctl add conn_fd error");
                        close(conn_fd);
                        continue;
                    }
                }
            } else if (ev & EPOLLIN) {
                // Incoming read events.
                char buffer[READ_BUFFER_SIZE];
                while (true) {
                    ssize_t count = read(fd, buffer, sizeof(buffer));
                    if (count == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // all data is done reading.
                            break;
                        } else {
                            perror("read error");
                            close(fd);
                            break;
                        }
                    } else if (count == 0) {
                        close(fd);
                        break;
                    } else {
                        // now have read data, prepare to send response.
                        const char *response = "HTTP/1.1 200 OK\r\n"
                                               "Content-Length: 13\r\n"
                                               "Content-Type: text/plain\r\n"
                                               "\r\n"
                                               "Hello, World!";
                        size_t response_len = strlen(response);
                        ssize_t sent        = 0;

                        while (sent < (ssize_t)response_len) {
                            ssize_t n = write(fd, response + sent, response_len - sent);
                            if (n == -1) {
                                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                    // 发送缓冲区满，等待可写事件
                                    epoll_event write_event;
                                    write_event.events  = EPOLLOUT | EPOLLET;
                                    write_event.data.fd = fd;
                                    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &write_event) == -1) {
                                        perror("epoll_ctl mod fd error");
                                        close(fd);
                                    }
                                    break;
                                } else {
                                    perror("write error");
                                    close(fd);
                                    break;
                                }
                            } else {
                                sent += n;
                            }
                        }

                        if (sent >= (ssize_t)response_len) {
                            close(fd);
                        }
                        break;
                    }
                }
            } else if (ev & EPOLLOUT) {
                // Focus on read event..
                close(fd);
            } else {
                close(fd);
            }
        }
    }

    close(listen_fd);
    close(epoll_fd);
    return 0;
}
