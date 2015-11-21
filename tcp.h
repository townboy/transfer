#ifndef __TCP_H__
#define __TCP_H__

#include <sys/select.h>
#include <sys/time.h>

#define SERVER_PORT 1080
#define CLIENT_PORT 1080

#define MAX_FD 1024
#define BUFFER_SIZE 1024* 128       // 64 * 1024 B = 64 KB

enum SocketType {
    socket_listen = 0,
    socket_client = 1,
    socket_server = 2,
};

enum ClientStatus {
    not_connect = 0,
    connected = 1,
};

struct SocketInfo {
    int fd;
    SocketType type;
    ClientStatus client_status;
    int peer_fd;
};

class tcp {
public:
    static tcp * Instance();

    int make_server_socket(int port = CLIENT_PORT);
    void loop();
    void set_client(bool is_client) {
        this->is_client = is_client;
    }

private:
    tcp();
    void print_timestamp();
    int process_listen_socket(int fd);
    int process_client_socket(int fd);
    int process_server_socket(int fd);

    int make_socket(struct sockaddr_in &server_addr);

    int send_tunnel_ok(int fd);
    bool is_buffer_header(char *buffer);
    
    int ok_write(int fd, char *buffer, int length);
    int ok_close(int fd);

    static tcp * instance;

    fd_set attention;
    SocketInfo connection[MAX_FD];

    bool is_client;
};

#endif
