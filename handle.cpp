#include "handle.h"

#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>

handle* handle::instance = NULL;

handle* handle::Instance() {
    if (NULL == instance)
        instance = new handle();
    return instance;
}

void handle::log() {
    struct timeval now;
    gettimeofday(&now, NULL);


}

int handle::parse_header(char *header, struct sockaddr_in &server_addr, bool *is_tunnel) {

    int port = 443;
    char str_port[6];
    char hostname[100];
    //处理https的请求
    if (strstr(header, "CONNECT"))
    {
        *is_tunnel = true;

        char *position_host_start = strchr(header, ' ');
        char *position_host_end = strchr(position_host_start + 1, ' ');
        char *position_colon = strchr(header, ':');

        //exist port information
        if (position_colon && position_colon < position_host_end)
        {
            int length_port = position_host_end - position_colon - 1;
            strncpy(str_port, position_colon + 1, length_port);
            port = atoi(str_port);
            str_port[length_port] = '\0';

            position_host_end = position_colon;
        }

        int hostname_length = position_host_end - position_host_start - 1;
        strncpy(hostname, position_host_start + 1, hostname_length);
        hostname[hostname_length] = '\0';

        std::cout << port << " " << hostname << std::endl;

        struct hostent *address_by_name = gethostbyname(hostname);
        memcpy(&server_addr.sin_addr.s_addr, address_by_name->h_addr, address_by_name->h_length);

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);

        return 0;
    }

    *is_tunnel = false;
    //处理http请求
    char *position_host = strstr(header, "Host:");
    if (!position_host)
        return -1;

    char *position_lineend = strstr(position_host, "\r\n");
    char *position_port = strchr(position_host + 5, ':');
    
    //exist port information
    port = 80;
    if (position_port && position_port < position_lineend)
    {
        int port_length = int(position_lineend - position_port - 1);
        strncpy(str_port, position_port + 1, port_length);
        str_port[port_length] = '\0';

        port = atoi(str_port);

        position_lineend = position_port;
    }

    //get host name
    int hostname_length = int(position_lineend - (position_host + 6));
    strncpy(hostname, position_host + 6, hostname_length);
    hostname[hostname_length] = '\0';
    std::cout << port << " " << hostname << std::endl;

    struct hostent *address_by_name = gethostbyname(hostname);
    memcpy(&server_addr.sin_addr.s_addr, address_by_name->h_addr, address_by_name->h_length);

    // TODO
    //jd ip
    //server_addr.sin_addr.s_addr = inet_addr("111.206.227.118");
    //baidu ip
    //addr.sin_addr.s_addr = inet_addr("220.181.57.217");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    return 0;
}

int handle::repack_header(bool is_tunnel, char *old_header, char *new_header) {

    //handle https
    if (true == is_tunnel)
    {
        char *position_lineend = strstr(old_header, "\r\n");
        char *position_end = strchr(old_header, '\0');

        int length_copy = int(position_end - position_lineend - 2);
        memcpy(new_header, position_lineend + 2, length_copy);
        new_header[length_copy] = '\0';

        std::cout << new_header << std::endl;
        return 0;
    }

    char *position_http = strstr(old_header, "http://");
    char *position_path = strchr(position_http + 7, '/');
    char *position_lineend = strstr(old_header, "HTTP/");
    char *position_end = strchr(old_header, '\0');

   
    // GET http://www.baidu.com/abc.html HTTP/1.1\r\n
    if (position_path && (position_path < position_lineend)) {

        int length_first_Get = int(position_http - old_header);
        int length_later_path = position_end - position_path;

        memcpy( new_header, old_header, length_first_Get );    // (GET )
        memcpy( new_header + length_first_Get, position_path, length_later_path ); // (/abc.html ...)
        new_header[length_first_Get + length_later_path] = '\0';
    }
    // GET http://www.baidu.com HTTP/1.1\r\n
    else {
        
        int length_first_Get = int(position_http - old_header);
        int length_later_HTTP = position_end - position_http;
        int length_total = length_first_Get + 2 + length_later_HTTP;

        memcpy( new_header, old_header, length_first_Get );    // (GET )
        new_header[length_first_Get] = '/';
        new_header[length_first_Get + 1] = ' ';

        memcpy( new_header + length_first_Get + 2, position_http, sizeof(position_http));
        new_header[length_total] = '\0';
    }

    std::cout << new_header << std::endl;

    return 0;
}

/*  return 
-1 buffer not contain http header
0 correct and not need send tunnel ok
1 correct and need send tunnel ok
*/
int handle::handle_http_header(char *in_stream, int buffer_length, char *out_stream, int &send_length, struct sockaddr_in &server_address) {

    bool is_tunnel = false;
    int parse_ret = parse_header(in_stream, server_address, &is_tunnel);

    // this buffer is http or https header
    if (0 == parse_ret) {

        std::cout << "parser success" << std::endl;

        // http
        if (false == is_tunnel) {
            repack_header(is_tunnel, in_stream, out_stream);
            send_length = strlen(out_stream);
            return 0;
        }
        //response connect success information to web browser
        else if (true == is_tunnel) {
            out_stream[0] = '\0';
            send_length = 0;
            return 1;
        }
    }

    //just simply transfer data
    else if (-1 == parse_ret) {
        memcpy(out_stream, in_stream, buffer_length);
        out_stream[buffer_length] = '\0';
        send_length = buffer_length;
        return -1;
    }

    return 0;
}

int handle::encode(char *in_stream, int buffer_length, char *out_stream) {


    return 0;
}

int handle::decode(char *in_stream, int buffer_length, char *out_stream) {


    return 0;
}