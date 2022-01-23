//
// Created by lenovo on 2022/1/16.
//

#include "server.h"

Server::Server(Config *config) : config(config) {
    //Epoll init
    epoll_fd = epoll_create(512);
    if (epoll_fd < 0) {
        perror("Epoll create failed : ");
        exit(0);
    }
}

Server::~Server() {
    close(epoll_fd);
    if (listen_fd > 0) {
        close(listen_fd);
    }
}

bool Server::init() {
    listen_fd = create_socket();
    if (listen_fd < 0) {
        exit(0);
    }

    timeval time;
    time.tv_sec = 0;
    time.tv_usec = config->ReadTimeout();
    setsockopt(listen_fd, SOL_SOCKET, SO_RCVTIMEO, &time, sizeof(time));

    epoll_event event;
    event.data.fd = listen_fd;
    //event.events = EPOLLIN;
    event.events = EPOLLIN | EPOLLEXCLUSIVE;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event);
    return true;
}

void Server::run() {
    epoll_event events[16];
    TaskManager taskManager(config->TaskMin(), config->TaskMax(), config->BlockchainThreads(), config->Blockchain(),
                            config->Forward(), config->ForwardThreads());
    if (config->Forward()) {
        for (auto &ip: config->ForwardDNS()) {
            taskManager.add_forward_dns(ip);
        }
    }
    unsigned long buf_size = config->RecvBuf();
    taskManager.set_epoll(epoll_fd);
    taskManager.set_buf(config->SendBuf());
    taskManager.set_domain(config->Domain());

    while (!EXIT) {
        int fds = epoll_wait(epoll_fd, events, 16, -1);
        if (fds < 0) {
            perror("Epoll wait failed : ");
            continue;
        }
        for (int i = 0; i < fds; ++i) {
            if (events[i].data.fd == listen_fd) {
                sockaddr_in addr;
                char *buf = (char *) malloc(buf_size);
                if (buf == nullptr) {
                    printf("jemalloc malloc receive buf error\n");
                    continue;
                }
                std::memset(buf, 0, buf_size);
                long timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                socklen_t addr_len = sizeof(addr);
                ssize_t size = recvfrom(listen_fd, buf, 512, 0, (sockaddr *) &addr, &addr_len);
                if (size <= 0) {
                    continue;
                }
                int fd = create_socket(true);
                if (connect(fd, (sockaddr *) &addr, addr_len) == 0) {
                    printf("receive : %ld\n", size);
                    taskManager.add_task({Receiving, timestamp, fd, buf, size, buf_size, nullptr, Normal, nullptr, 0, 0,
                                          nullptr, 0});
                } else {
                    perror("connect error : ");
                }
            } else if ((events[i].events && EPOLLOUT) || (events[i].events && EPOLLERR)) {
                Residual *residual = (Residual *) events[i].data.ptr;
                if (residual == nullptr) {
                    continue;
                } else if (residual->sent_len > 0 || residual->sent_len < residual->send_size) {
                    ssize_t len = send(residual->fd, residual->buf + residual->sent_len,
                                       residual->send_size - residual->sent_len, 0);
                    if (len > 0) {
                        residual->sent_len += len;
                    } else {
                        residual->sent_len = residual->send_size;
                    }
                }
                if (residual->sent_len >= residual->send_size) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, residual->fd, &events[i]);
                    printf("close fd : %d\n", residual->fd);
                    close(residual->fd);
                    residual->clear();
                    free(residual);
                }
            }
        }
    }
    taskManager.exit();
}

int Server::create_socket(bool noblock) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0); // 0 auto select protocol(IPPROTO_TCP、IPPTOTO_UDP)
    int port = 53;
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    socklen_t on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
    if (noblock) {
        fcntl(fd, F_SETFL, O_NONBLOCK);
    }

    //0 : success, -1 : error, set errno
    if (bind(fd, (sockaddr *) &addr, sizeof(addr)) != 0) {
        std::cout << "Can't bind socket! ( " << errno << " ) : " << strerror(errno) << std::endl;
        return -1;
    }
    return fd;
}
