//
// Created by lenovo on 2022/1/22.
//

#ifndef BLOCKCHAIN_DNS_FORWARD_H
#define BLOCKCHAIN_DNS_FORWARD_H

#include <condition_variable>
#include <netinet/in.h>
#include <sys/socket.h>
#include <shared_mutex>
#include <arpa/inet.h>
#include <cstring>
#include <atomic>
#include <thread>
#include <vector>
#include <mutex>
#include <queue>
#include <list>
#include <map>

struct Buf {
    unsigned long long key;
    char *receive_buf;
    ssize_t receive_size;
    unsigned long receive_buf_size;
    char *send_buf;
    unsigned long *send_size;
    unsigned long send_buf_size;
};

class Forward {
public:
    Forward();

    ~Forward();

    void init(int num);

    void create_thread();

    void exit();

    bool is_processing(unsigned long long key);

    void add_task(Buf &&buf);

    void add_dns(std::string &dns);

private:
    std::list<std::thread> threads;
    std::queue<Buf> tasks;
    std::mutex mtx;
    std::condition_variable cond;
    std::atomic_bool quit;

    std::vector<std::string> dns_server;
    std::map<unsigned long long, int> processing;
    std::shared_mutex rw_mtx;
};


#endif //BLOCKCHAIN_DNS_FORWARD_H
