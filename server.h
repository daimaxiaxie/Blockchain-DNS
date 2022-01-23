//
// Created by lenovo on 2022/1/16.
//

#ifndef BLOCKCHAIN_DNS_SERVER_H
#define BLOCKCHAIN_DNS_SERVER_H

#include <jemalloc/jemalloc.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>

#include "taskmanager.h"
#include "config.h"
#include "global.h"


class Server {
public:
    Server(Config *config);

    ~Server();

    bool init();

    void run();

private:
    int create_socket(bool noblock = false);

private:
    Config *config;
    int epoll_fd;
    int listen_fd;
};


#endif //BLOCKCHAIN_DNS_SERVER_H
