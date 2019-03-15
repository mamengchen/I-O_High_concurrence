#pragma once

#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>


#define DEFAULT_FD -1

class Server {
private:
    int listen_sock;
    int port;
    int *fd_array;
    int fd_num;
    int size;
public:
    Server(const int &port_)
        :port(port_)
    {}

    void InitServer()
    {
        fd_num = sizeof(fd_set)*8;
        fd_array = new int[fd_num];
        
        for (auto i = 0; i < fd_num; ++i)
        {
            fd_array[i] = DEFAULT_FD;
        }
        
        listen_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_sock < 0)
        {
            std::cerr << "套接字失败" << std::endl;
            exit(2);
        }
        
        int opt = 1;
        setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        struct sockaddr_in local;
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(INADDR_ANY);
        local.sin_port = htons(port);

        if (bind(listen_sock, (struct sockaddr*)&local, sizeof(local)) < 0)
        {
            std::cerr << "绑定失败" << std::endl;
            exit(3);
        }

        if (listen(listen_sock, 5) < 0)
        {
            std::cerr << "没有链接到来" << std::endl; 
        }
    }

    int AddAllfd(int fds[], fd_set &rfds)
    {
        int max = fds[0];
        for (auto i = 0; i < size; ++i)
        {
            if (fds[i] == DEFAULT_FD)
            {
                continue;
            }
            FD_SET(fds[i], &rfds);
            if (max < fds[i])
            {
                max = fds[i];
            }
        }
        return max;
    }

    bool ADDfd(int fd, int fds[])
    {
        std::cout << "文件描述符添加成功: " << fd << std::endl;
        if (size < fd_num)
        {
            fds[size] = fd;
            size++;
            std::cout << "size: " << size << " cap: " << fd_num << std::endl;
            return true;
        }
        return false;
    }

    void DelFd(int index, int fds[])
    {
        std::cout << "delete fd: " << fds[index] << std::endl;
        fds[index] = fds[size-1];
        fds[size-1] = DEFAULT_FD;
        size--;
        std::cout << "size: " << size << " cap: " << fd_num << std::endl;
    }

    void Start()
    {
        fd_array[0] = listen_sock;
        size = 1;
        while(1)
        {
            fd_set rfds;
            FD_ZERO(&rfds);

            int maxfd = AddAllfd(fd_array, rfds);
            struct timeval timeout = {5, 0};

            switch(select(maxfd+1, &rfds, NULL, NULL, &timeout))
            {
            case -1:
                std::cerr << "select 失败" << std::endl;
                break;
            case 0:
                std::cerr << "超时" << std::endl;
                break;
            default:
                {
                    for (auto i = 0; i < size; ++i)
                    {
                        if (i == 0 && FD_ISSET(fd_array[i], &rfds))
                        {
                            std::cout << "获得一个新链接...." << std::endl;
                            struct sockaddr_in peer;
                            socklen_t len = sizeof(peer);
                            int sock = accept(listen_sock, (struct sockaddr*)&peer, &len);
                            if (sock < 0)
                            {
                                std::cerr << "accept error" << std::endl;
                                continue;
                            }
                            if (!ADDfd(sock, fd_array))
                            {
                                std::cout << "socket full!" << std::endl;
                                close(sock);
                            }
                        } else if (FD_ISSET(fd_array[i], &rfds)) {
                            char buf[10240];
                            ssize_t s = recv(fd_array[i], buf, sizeof(buf)-1, 0);

                            if (s > 0)
                            {
                                buf[s] = 0;
                                std::cout << buf << std::endl;
                                const std::string echo_http = "HTTP/1.0 200 OK\r\n\r\n<html>hello</html>\r\n";
                                send(fd_array[i], echo_http.c_str(), echo_http.size(), 0);
                                close(fd_array[i]);
                                DelFd(i, fd_array);
                                std::cout << "echo http response!!!" << std::endl;

                            } else if (s == 0) {
                                std::cout << "客户端关闭" << std::endl;
                                close(fd_array[i]);
                                DelFd(i, fd_array);
                            } else {
                                std::cerr << "读异常" << std::endl;
                            }
                        }
                    }
                }
                break;
            }
        }
    }
    ~Server()
    {}
};

