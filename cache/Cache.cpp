//
// Created by lenovo on 2021/12/30.
//

#include "Cache.h"


unsigned long Expire::time() {
    return timestamp + ttl;
}


Cache::Cache() : max(102400), clear_num(16) {

}

Cache::~Cache() {

}


Cache &Cache::GetCache() {
    static Cache cache;
    return cache;
}

bool Cache::put(std::string domain, unsigned short type, std::string ip, unsigned int ttl) {
    unsigned long timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    //printf("put record to cache\n");
    {
        std::lock_guard<std::shared_mutex> lock(this->rw_mtx);
        if (cache.count(domain) == 0) {
            cache[domain] = lru.push_front({domain, {timestamp, ttl}});
        }
        if (cache[domain] == nullptr) {
            cache.erase(domain);
            return false;
        }
        for (auto &record: cache[domain]->data.records) {
            if (std::get<0>(record) == type && std::get<1>(record) == ip) {
                return true;
            }
        }
        if (timestamp + ttl > cache[domain]->data.expire.time()) {
            cache[domain]->data.expire.timestamp = timestamp;
            cache[domain]->data.expire.ttl = ttl;
        }
        cache[domain]->data.records.emplace_back(std::make_tuple(type, ip, ttl));
        lru.move_to_front(cache[domain]);
    }
    {
        order();
    }
    printf("put record into cache\n");
    return true;
}

bool Cache::exist(std::string domain) {
    check_expire(domain);
    std::shared_lock<std::shared_mutex> rwlock(rw_mtx);
    return cache.count(domain) == 1;
}

bool Cache::exist(std::string domain, unsigned short type) {
    if (exist(domain)) {
        std::shared_lock<std::shared_mutex> rwlock(rw_mtx);
        for (auto &record: cache[domain]->data.records) {
            if (std::get<0>(record) == type) {
                return true;
            }
        }
    }
    return false;
}

std::vector<std::tuple<unsigned short, std::string, unsigned int>> Cache::get(std::string domain) {
    if (exist(domain)) {
        std::unique_lock<std::shared_mutex> lock(rw_mtx);
        lru.move_to_front(cache[domain]);
        lock.unlock();

        std::shared_lock<std::shared_mutex> rwlock(rw_mtx);
        return cache[domain]->data.records;
    }
    return {};
}

std::vector<std::tuple<unsigned short, std::string, unsigned int>> Cache::get(std::string domain, unsigned short type) {
    if (exist(domain)) {
        std::unique_lock<std::shared_mutex> lock(rw_mtx);
        lru.move_to_front(cache[domain]);
        lock.unlock();

        std::vector<std::tuple<unsigned short, std::string, unsigned int>> res;
        std::shared_lock<std::shared_mutex> rwlock(rw_mtx);
        for (auto &record: cache[domain]->data.records) {
            if (std::get<0>(record) == type) {
                res.push_back(record);
            }
        }
        return res;
    }
    return {};
}

void Cache::erase(std::string &&key) {
    if (exist(key)) {
        std::lock_guard<std::shared_mutex> lock(this->rw_mtx);
        lru.erase(cache[key]);
        cache.erase(key);
    }
}

void Cache::set_max(int m) {
    max = m;
}

void Cache::order() {
    if (max + clear_num < cache.size()) {
        printf("ordering\n");
        long num = clear_num > 0 ? clear_num : (cache.size() - max);
        std::lock_guard<std::shared_mutex> lock(this->rw_mtx);
        for (int i = 0; i < num; ++i) {
            cache.erase(lru.back().key);
            lru.pop_back();
        }
    }
}

bool Cache::check_expire(std::string &key) {
    unsigned long now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    std::lock_guard<std::shared_mutex> lock(this->rw_mtx);
    if (cache.count(key) == 1) {
        if (cache[key]->data.expire.time() < now) {
            lru.erase(cache[key]);
            cache.erase(key);
        }
        return true;
    }
    return false;
}

void Cache::set_clear_num(long num) {
    clear_num = num > 0 ? num : 0;
}

