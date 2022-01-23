//
// Created by lenovo on 2021/12/30.
//

#ifndef BLOCKCHAIN_DNS_CACHE_H
#define BLOCKCHAIN_DNS_CACHE_H

#include <jemalloc/jemalloc.h>
#include <shared_mutex>
#include <string>
#include <vector>
#include <queue>
#include <tuple>
#include <map>

#include "list.h"

struct Expire {
    unsigned long timestamp;
    unsigned int ttl;

    unsigned long time();
};

struct Storage {
    std::string key;
    Expire expire;
    std::vector<std::tuple<unsigned short, std::string, unsigned int>> records;
};

class Cache {
private:
    Cache();

public:
    ~Cache();

    static Cache& GetCache();

    bool put(std::string domain, unsigned short type, std::string ip, unsigned int ttl);

    bool exist(std::string domain);

    bool exist(std::string domain, unsigned short type);

    std::vector<std::tuple<unsigned short, std::string, unsigned int>> get(std::string domain);

    std::vector<std::tuple<unsigned short, std::string, unsigned int>> get(std::string domain, unsigned short type);

    void erase(std::string &&key);

    void set_max(int m);

    void set_clear_num(long num);

private:
    void order();

    bool check_expire(std::string &key);

private:
    std::shared_mutex rw_mtx;
    std::map<std::string, Node<Storage> *> cache;

    List<Storage> lru;
    int max;
    long clear_num;
};


#endif //BLOCKCHAIN_DNS_CACHE_H
