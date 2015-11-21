#ifndef __HANDLE_H__
#define __HANDLE_H__

#include <sys/select.h>
#include <sys/time.h>

class handle {
public:
    static handle * Instance();
    void log();

    handle() {}
    virtual ~handle() {}

    int decode(char *in_stream, int buffer_length, char *out_stream);
    int encode(char *in_stream, int buffer_length, char *out_stream);

    int parse_header(char *header, struct sockaddr_in &server_addr, bool *is_tunnel);

    int repack_header(bool is_tunnel, char *old_header, char *new_header);
    int handle_http_header(char *in_stream, int buffer_length, char *out_stream, int &send_length, struct sockaddr_in &client);

private:
    static handle *instance;

};

#endif