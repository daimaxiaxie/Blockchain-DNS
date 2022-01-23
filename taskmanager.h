//
// Created by lenovo on 2022/1/18.
//

#ifndef BLOCKCHAIN_DNS_THREADPOOL_H
#define BLOCKCHAIN_DNS_THREADPOOL_H

#include <condition_variable>
#include <sys/epoll.h>
#include <chrono>
#include <future>
#include <mutex>
#include <queue>
#include <list>

#include "blockchainmanager.h"
#include "forward.h"
#include "dns.h"

enum State {
    Receiving,
    Parsing,
    Querying,
    Forwarding,
    Sending,
    Discard,
    Finish,
};

struct Residual {
    int fd;
    char *buf;
    long send_size;
    long sent_len;

    void clear();
};

struct Task {
    State state;
    long timestamp;
    int fd;
    char *receive_buf;
    ssize_t receive_size;
    unsigned long receive_buf_size;
    Query *query;

    ResponseType responseType;

    char *send_buf;
    unsigned long send_size;
    unsigned long send_buf_size;

    char *buf;
    unsigned int buf_len;

    void clear_without_send();
};

class TaskManager {
public:
    TaskManager(int min, int max, int blockchain, std::string blockchain_ip, bool forward, int forward_threads);

    ~TaskManager();

    void create_thread();

    void add_task(Task task);

    void exit();

    void set_epoll(int fd);

    void set_buf(int size);

    void set_domain(std::string domain);

    void add_forward_dns(std::string &dns);

private:
    bool task_handler(Task &task, DNS &dns);

    bool receive_handler(Task &task, DNS &dns);

    bool parse_handler(Task &task, DNS &dns);

    bool query_handler(Task &task, DNS &dns);

    bool forward_handler(Task &task);

    bool send_handler(Task &task, DNS &dns);

    bool finish_handler(Task &task);

    bool discard_handler(Task &task);

    bool send_buf_init(Task &task);

    int ep;
    int send_buf_size;

private:
    std::list<std::thread> threads;
    std::queue<Task> tasks;
    std::mutex mtx;
    std::condition_variable cond;
    std::atomic_int active;

    bool is_forward;

    BlockchainManager blockchainManager;
    std::string blockchain_domain;
    Cache &cache;
    Forward forward;
};

bool endwith(char *str, std::string &param);

size_t validlength(char *str);

#endif //BLOCKCHAIN_DNS_THREADPOOL_H
