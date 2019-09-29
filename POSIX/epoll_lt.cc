#include "epoll_et_lt.h"

void addfd(int epollfd, int fd) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    int ret = epoll_ctl( epollfd, EPOLL_CTL_ADD, fd, &event );
    if ( ret < 0 ) {
        perror( "epoll_ctl" );
        return;
    }
}

int main( int argc, char * argv[] ) {
    if (argc != 3) {
        printf( "usage: %s ip_address and port_number", basename( argv[0] ) );
        return 1;
    }

    const char * ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &addr.sin_addr );
    addr.sin_port = htons(port);

    int listenfd = socket( PF_INET, SOCK_STREAM, 0 );
    assert(listenfd >= 0);
    int ret = bind( listenfd, (struct sockaddr*)&addr, sizeof(addr) );
    assert(ret != -1);
    ret = listen( listenfd, 5 );
    assert(ret != -1);
    
    int epollfd = epoll_create(5);
    assert(epollfd >= 0);
    addfd(epollfd, listenfd);
    for (;;) {
        struct epoll_event events[10];
        int size = epoll_wait( epollfd, events, sizeof(events)/sizeof(events[0]), -1 );
        if ( size < 0 ) {
            perror( "epoll_wait" );
            continue;
        }
        if ( size == 0 ) {
            printf( "epoll timeout\n" );
            continue;
        }
        for (int i = 0; i < size; i++) {
            if (!(events[i].events & EPOLLIN)) {
                continue;
            }
            if (events[i].data.fd == listenfd) {
                struct sockaddr_in client_addr;
                socklen_t len = sizeof(client_addr);
                int connfd = accept( listenfd, (struct sockaddr*)&client_addr, &len );
                if ( connfd < 0 ) {
                    perror( "accept" );
                    exit(1);
                }
                printf( "client %s:%d connect\n", ip, port );
                struct epoll_event ev;
                ev.data.fd = connfd;
                ev.events |= EPOLLIN;
                int ret = epoll_ctl( epollfd, EPOLL_CTL_ADD, connfd, &ev );
                if ( ret < 0 ) {
                    perror( "epoll_ctl" );
                    exit(2);
                }
            } else {
                char buf[1024] = {0};
                int connfd = events[i].data.fd;
                ssize_t read_size = read( connfd, buf, sizeof(buf)-1 );
                if ( read_size < 0 ) {
                    perror( "read" );
                    exit(3);
                }
                if (read_size == 0) {
                    close( connfd );
                    epoll_ctl( epollfd, EPOLL_CTL_DEL, connfd, NULL );
                    printf( "client say : goodbye\n" );
                    continue;
                }
                printf( "client say: %s", buf );
                write( connfd, buf, strlen(buf) );
            }
        }
    }
    return 0;
}
