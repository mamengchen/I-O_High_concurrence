#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

void Usage() {
    printf( "usage: ./client [ip] [port]\n" );
}

int main( int argc, char * argv[] ) {
    if ( argc != 3 ) {
        Usage();
        return -1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &addr.sin_addr );
    addr.sin_port = htons( port );

    int sockfd = socket( AF_INET, SOCK_STREAM, 0 );
    assert( sockfd >= 0 );
    int ret = connect( sockfd, (struct sockaddr*)&addr, sizeof(addr) );
    if ( ret < 0 ) {
         perror( "connect" );
         return 1;
    }

    for (;;) {
        printf( "> " );
        fflush( stdout );
        char buf[1024] = { 0 };
        read( 0, buf, sizeof(buf)-1 );

        ssize_t write_size = write( sockfd, buf, strlen(buf) );
        assert(write_size >= 0);

        ssize_t read_size = read( sockfd, buf, sizeof(buf)-1 );
        assert(read_size >= 0);

        if (read_size == 0){
            printf( "对端关闭" );
            break;
        }
        printf("server say# %s", buf);
    }
    close(sockfd);
    return 0;
}
