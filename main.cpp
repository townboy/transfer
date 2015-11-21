#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>

#include "tcp.h"

int launch() {
    std::cout << "start launch" << std::endl << std::endl;

    if (-1 == tcp::Instance()->make_server_socket()) {
        std::cout << "make server socket error" << std::endl;
        return -1;
    }
    tcp::Instance()->loop();
    return 0;
}

int main(int argc, char ** argv) {

    int opt_result;
    bool is_client = true;

    while (-1 != (opt_result = getopt(argc, argv, "cs")))
    {
        switch (opt_result)
        {
        case 'c':
            is_client = true;
            break;
        case 's':
            is_client = false;
            break;
        default:
            std::cout << "invalid arg" << std::endl;
            break;
        }
    }

    tcp::Instance()->set_client(is_client);
    launch();

	return 0;
}