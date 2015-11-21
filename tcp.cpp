#include "tcp.h"
#include "handle.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>


tcp* tcp::instance = NULL;

tcp* tcp::Instance() {
    if (NULL == instance)
        instance = new tcp();
    return instance;
}

tcp::tcp() {
    FD_ZERO(&attention);
}

void tcp::print_timestamp() {

    time_t now;
    struct tm *timenow;
    char strtemp[255];

    time(&now);
    timenow = localtime(&now);
    std::cout << asctime(timenow);
}

int tcp::make_server_socket(int port /* = 1080 */) {
    int server_fd;
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return -1;
	}

    struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl( INADDR_ANY );
	addr.sin_port = htons(port);
	
	int iFlag = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&iFlag, sizeof(iFlag));
	
	if (-1 == bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)))
		return -1;

	if (-1 == listen(server_fd, MAX_FD))
		return -1;

    connection[server_fd].fd = server_fd;
    connection[server_fd].type = socket_listen;

    FD_SET(server_fd, &attention);
    return server_fd;
}

int tcp::make_socket(struct sockaddr_in &server_addr) {
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    std::cout << "start connection" << std::endl;
    if (-1 == connect(fd, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        std::cout << "host parser error, can not connect" << std::endl;
        return -1;
    }
    std::cout << "connect remote server success" << std::endl;

    FD_SET(fd, &attention);
    return fd;
}

int tcp::process_listen_socket(int fd) {
	struct sockaddr_in client;
    socklen_t client_length = sizeof(client);

    std::cout << "listen_socket reveive new connection" << std::endl;
    int new_fd = accept(fd, (struct sockaddr*)&client, &client_length);
    std::cout << "accept into fd = " << new_fd << std::endl;

    connection[new_fd].fd = new_fd;
    connection[new_fd].type = socket_client;
    connection[new_fd].client_status = not_connect;

    FD_SET(new_fd, &attention);

    return 0;
}

bool tcp::is_buffer_header(char *buffer) {

    return false;
}

int tcp::ok_write(int fd, char *buffer, int length) {
    int already_send = 0;
    while (already_send != length) {
        int sended = write(fd, buffer + already_send, length - already_send);
        already_send += sended;
    }
    return 0;
}

int tcp::ok_close(int fd) {
    std::cout << "close fd = " << fd << std::endl;
    FD_CLR(fd, &attention);
    shutdown(fd, SHUT_RDWR);
    close(fd);
    return 0;
}

int tcp::process_client_socket(int fd) {
    char buffer[BUFFER_SIZE + 10];
    char new_header[BUFFER_SIZE + 10];
    struct sockaddr_in server_address;

    std::cout << "client_socket receive data fd = " << fd << std::endl;

    int receive_length = read(fd, buffer, BUFFER_SIZE);
    int send_length = 0;
    // peer close the fd
    if (0 >= receive_length) {
        std::cout << "close " << fd << std::endl;
        FD_CLR(fd, &attention);
        close(fd);
        return 0;
    }

    buffer[receive_length] = '\0';
    std::cout << "receive_length is " << receive_length << std::endl << buffer << std::endl;

    int result_header = handle::Instance()->handle_http_header(buffer, receive_length, new_header, send_length, server_address);

    std::cout << "result_header = " << result_header << std::endl;

    // connect to server
    // if (-1 != connection[fd].client_status) {
    if (-1 != result_header) {

        int server_fd = make_socket(server_address);
        std::cout << "server_fd = " << server_fd << std::endl;

        connection[fd].peer_fd = server_fd;
        connection[server_fd].fd = server_fd;
        connection[server_fd].peer_fd = fd;
        connection[server_fd].type = socket_server;

        connection[fd].client_status = connected;
    }

    if (1 == result_header)
        send_tunnel_ok(fd);
    
    //transfer data
    std::cout << "send server_fd = " << connection[fd].peer_fd << " header to server length = " << send_length << std::endl;
    std::cout << "header content is  = " << std::endl << new_header << std::endl;
    
    ok_write(connection[fd].peer_fd, new_header, send_length);

    std::cout << "write to server success!" << std::endl << std::endl;

    return 0;
}

int tcp::send_tunnel_ok(int fd) {
    char buffer[50] = "HTTP/1.1 200 Connection Established\r\n\r\n";
    write(fd, buffer, strlen(buffer));
    return 0;
}

int tcp::process_server_socket(int fd) {

    char buffer[BUFFER_SIZE + 10];
    int receive_length;

    std::cout << "server_socket receive data fd = " << fd << std::endl;

    print_timestamp();
    std::cout << "start read" << std::endl << std::endl;
    receive_length = read(fd, buffer, BUFFER_SIZE);
    print_timestamp();
    std::cout << "end read" << std::endl << std::endl;

    if (0 < receive_length) {
        buffer[receive_length] = '\0';
        //std::cout << "receive length = " << receive_length << std::endl << buffer << std::endl;
        //response the content to web browser

        std::cout << "receive length = " << receive_length << std::endl << std::endl;

        write(connection[fd].peer_fd, buffer, receive_length);
    }

    if (receive_length < 0)
        std::cout << strerror(errno) << std::endl;

    if (receive_length <= 0) {
        /*
        std::cout << "close fd " << fd << std::endl;
        std::cout << "close peer fd" << connection[fd].peer_fd << std::endl;
        FD_CLR(fd, &attention);
        FD_CLR(connection[fd].peer_fd, &attention);

        shutdown(fd, SHUT_RDWR);
        shutdown(connection[fd].peer_fd, SHUT_RDWR);

        close(fd);
        close(connection[fd].peer_fd);
        */
        ok_close(fd);
        ok_close(connection[fd].peer_fd);
    }

    return 0;
}

void tcp::loop() {
    int handle_result;

	while (true) {
        fd_set read_set, write_set;
        memcpy(&read_set, &attention, sizeof(fd_set));
        memcpy(&write_set, &attention, sizeof(fd_set));
        
        print_timestamp();
        std::cout << "------------------------------start select-----------------------" << std::endl;
		int ret = select(MAX_FD, &read_set, NULL, NULL, NULL);

        print_timestamp();
        std::cout << "------------------------------end select-------------------------" << std::endl;

		for (int i = 0; i < MAX_FD; i++) {
			if (!FD_ISSET(i, &read_set))
                continue;

            // listen socket
            switch (connection[i].type)
            {
            case socket_listen:
                handle_result = process_listen_socket(i);
                break;

            case socket_client:
                handle_result = process_client_socket(i);
                break;

            //回复给客户端，然后关闭两边的fd
            case socket_server:
                handle_result = process_server_socket(i);
                break;
            }

            if (-1 == handle_result)
                std::cout << "handle " << i << " fd " << "fail" << std::endl;
		}
	}

}