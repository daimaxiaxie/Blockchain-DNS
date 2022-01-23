//
// Created by lenovo on 2022/1/19.
//

#ifndef BLOCKCHAIN_DNS_BLOCKCHAINMANAGER_H
#define BLOCKCHAIN_DNS_BLOCKCHAINMANAGER_H

#include <condition_variable>
#include <shared_mutex>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <list>
#include <map>

#include "blockchain/Blockchain.h"
#include "cache/Cache.h"

unsigned long long getKey(int &fd, long &timestamp);

struct Request {
    unsigned long long key;
    unsigned short type;
    char *domain;
    unsigned long domain_len;
};

class BlockchainManager {
public:
    BlockchainManager(int num, std::string ip);

    ~BlockchainManager();

    void create_thread();

    void exit();

    bool is_processing(unsigned long long key);

    void add_task(Request request);

private:
    std::list<std::thread> threads;
    std::queue<Request> tasks;
    std::mutex mtx;
    std::condition_variable cond;
    std::atomic_bool quit;

    char *addr;
    Cache &cache;
    std::map<unsigned long long, int> processing;
    std::shared_mutex rw_mtx;
};


#endif //BLOCKCHAIN_DNS_BLOCKCHAINMANAGER_H
