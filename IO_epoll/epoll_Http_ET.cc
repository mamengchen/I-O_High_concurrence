//这是一个epoll实现的服务端的demo

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <sys/epoll.h>
#include <fcntl.h>

class buffer{
public:
    int sock;
    char request[4096];
    int size;

public:
    buffer(int sock_):sock(sock_), size(0)
    {}
};


void SetNonBlock(int sock)
{
    int fl = fcntl( sock, F_GETFL );
    fcntl(sock, F_SETFL, O_NONBLOCK | fl);
}

int StartUp(int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        std::cerr << "socket error! " << std::endl;
        exit(1);
    }
    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(port);

    if (bind(sock, (struct sockaddr*)&local, sizeof(local)) < 0)
    {
        std::cerr << "绑定失败 !!" << std::endl;
        exit(2);
    }

    listen(sock, 5);

    //设置成非阻塞
    SetNonBlock(sock);

    return sock;
}

//添加
void EpollEventsAdd(int epfd, int sock, uint32_t epoll_in)
{
    struct epoll_event ev;
    ev.events = epoll_in;
    ev.data.ptr = new buffer(sock);
    epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev);
}

//删除
void EpollEventsDel(int epfd, int sock)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, sock, NULL);
}

//修改事件
void EpollEventEx(int epfd, int sock, uint32_t event)
{
    struct epoll_event ev;
    ev.events = event;
    ev.data.fd = sock;
    epoll_ctl(epfd, EPOLL_CTL_MOD, sock, &ev);
}


void MyAccept( int epfd, int  listen_sock )
{
    volatile bool quit = false;
    while (!quit)
    {
        struct sockaddr_in peer;
        socklen_t len = sizeof(peer);
        int sock = accept(listen_sock, (struct sockaddr*)&peer, &len);
        if (sock < 0)
        {
            std::cerr << "accept done.... " << std::endl;
            quit = true;
            continue;
        }
        SetNonBlock(sock);
        EpollEventsAdd(epfd, sock, EPOLLIN | EPOLLET);
    }
}

int MyRecv( buffer* buf )
{
    int sock = buf->sock;
    int &total = buf->size;
    char *rq = buf->request;
    ssize_t s = 0;
    while ( s = recv(sock, rq+total, 128, 0) > 0)
    {
        std::cout << rq << std::endl;
        total += s;
    }
    rq[total] = 0;

    if ( s == 0 )
    {
        return 0;
    }
    return 1;
}


void HandlerEvents(int epfd, struct epoll_event revs[], int num, int listen_sock)
{
    for (auto i = 0; i < num; i++)
    {
        uint32_t revent = revs[i].events;
        buffer* bf = (buffer*)(revs[i].data.ptr);
        int fd = bf->sock;
        char *rq = bf->request;
        
        if (revent & EPOLLIN)
        {
            

            if (fd == listen_sock)
            {
                MyAccept(epfd, listen_sock);

            } else {
                //普通读事件
                //这里需要定制协议
                
                int res = MyRecv(bf);
                if (res == 0)
                {
                    delete bf;
                    EpollEventsDel(epfd, fd);
                    close(fd);
                }
                    
                    
                /*
                char buf[10240];
                ssize_t s = recv(fd, buf, sizeof(buf)-1, 0);
                if (s > 0)
                {
                    std::cout << "************************" << std::endl;
                    std::cout << buf << std::endl;
                    std::cout << "************************" << std::endl;

                    EpollEventEx(epfd, fd, EPOLLOUT);
                } else if (s == 0) {
                    EpollEventsDel(epfd, fd);
                    close(fd);
                } else {
                    EpollEventsDel(epfd, fd);
                    close(fd);
                }
                */
            }
        } else if (revent & EPOLLOUT) {
            //普通写事件
            std::string echo_http = "HTTP/1.0 200 OK\r\n\r\n<html><h2>hello</h2></html>";
            send(fd, echo_http.c_str(), echo_http.size(), 0);
            close(fd);
            EpollEventsDel(epfd, fd);
        }
    }
}

int main()
{
    int listen_sock = StartUp(8888);
    int epfd = epoll_create(256);
    if (epfd < 0)
    {
        std::cerr << "epoll error" << std::endl;
        exit(3);
    }

    EpollEventsAdd(epfd, listen_sock, EPOLLIN | EPOLLET);
    struct epoll_event revs[64];
    while (1)
    {
        int timeout = 1000;
        int num = epoll_wait(epfd, revs, 64, timeout);
        switch(num)
        {
        case -1:
            std::cerr << "epoll_wait error! " << std::endl;
            break;
        case 0:
            std::cout << "time out..." << std::endl;
            break;
        default:
            HandlerEvents(epfd, revs, num, listen_sock);
            break;
        }
    }
    return 0;
}
