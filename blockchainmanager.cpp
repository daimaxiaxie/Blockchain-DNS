//
// Created by lenovo on 2022/1/19.
//

#include "blockchainmanager.h"

unsigned long long getKey(int &fd, long &timestamp) {
    return (0x00000000ffffffff & timestamp) | ((unsigned long long) fd << 32);
}

BlockchainManager::BlockchainManager(int num, std::string ip) : quit(false), cache(Cache::GetCache()) {
    addr = new char[ip.size() + 1];
    addr[ip.size()] = '\0';
    ip.copy(addr, ip.size());

    while (num > 0) {
        create_thread();
        --num;
    }
}

BlockchainManager::~BlockchainManager() {

}

void BlockchainManager::create_thread() {
    threads.emplace_back([this] {
        std::unique_lock<std::mutex> lock(this->mtx, std::defer_lock);
        Request task;
        bool run_task = false;
        Blockchain blockchain;
        if (!blockchain.connect(addr)) {
            //this->threads.pop_back();
            //return;
        }

        while (!quit.load()) {
            lock.lock();
            this->cond.wait(lock, [this]() { return !this->tasks.empty() || this->quit.load(); });
            run_task = false;
            if (!this->tasks.empty() && !this->quit.load()) {
                task = std::move(this->tasks.front());
                this->tasks.pop();
                run_task = true;
            }
            lock.unlock();
            //printf("BlockchainManager thread wake up\n");
            if (run_task) {
                //printf("BlockchainManager run task\n");
                if (task.domain != nullptr) {
                    std::vector<Record> &&res = blockchain.query(task.domain, task.type);
                    //printf("BlockchainManager add to Cache\n");
                    if (!res.empty()) {
                        for (auto &record: res) {
                            cache.put(task.domain, record.type, record.ip, record.ttl);
                        }
                    }
                    free(task.domain);
                }
                {
                    std::lock_guard<std::shared_mutex> rwlock(rw_mtx);
                    --processing[task.key];
                    if (processing[task.key] <= 0) {
                        processing.erase(task.key);
                    }

                }
            }
        }
        this->threads.pop_back();
    });
    threads.back().detach();
}

void BlockchainManager::exit() {
    quit.store(true);
    cond.notify_all();
}

bool BlockchainManager::is_processing(unsigned long long key) {
    std::shared_lock<std::shared_mutex> rwlock(rw_mtx);
    return processing.count(key) == 1;
}

void BlockchainManager::add_task(Request request) {
    Request req = request;
    req.domain = (char *) malloc(req.domain_len + 1);
    if (req.domain == nullptr) {
        return;
    }
    {
        std::lock_guard<std::shared_mutex> rwlock(rw_mtx);
        ++processing[request.key];
    }
    std::memcpy(req.domain, request.domain, request.domain_len);
    req.domain[req.domain_len] = '\0';
    {
        std::lock_guard<std::mutex> lock(this->mtx);
        tasks.push(req);
    }
    printf("BlockchainManager add task\n");
    this->cond.notify_all();
}
