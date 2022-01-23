//
// Created by lenovo on 2022/1/22.
//

#include "forward.h"

Forward::Forward() {

}

Forward::~Forward() {
    exit();

}


void Forward::init(int num) {
    while (num > 0) {
        create_thread();
        --num;
    }
}


void Forward::create_thread() {
    threads.emplace_back([this] {
        std::unique_lock<std::mutex> lock(this->mtx, std::defer_lock);
        bool run_task = false;
        Buf bufs;
        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        timeval tv;
        tv.tv_sec = 3;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        sockaddr_in addr;
        socklen_t socklen = sizeof(addr);

        while (!quit.load()) {
            lock.lock();
            this->cond.wait(lock, [this]() { return !this->tasks.empty() || this->quit.load(); });
            run_task = false;
            if (!this->tasks.empty() && !this->quit) {
                bufs = std::move(this->tasks.front());
                this->tasks.pop();
                run_task = true;
            }
            lock.unlock();
            if (run_task && bufs.send_buf != nullptr && bufs.receive_buf != nullptr) {
                for (auto &dns: dns_server) {
                    int port = 53;

                    std::memset(&addr, 0, sizeof(addr));
                    addr.sin_family = AF_INET;
                    addr.sin_port = htons(port);
                    addr.sin_addr.s_addr = inet_addr(dns.c_str());

                    if (sendto(fd, bufs.receive_buf, bufs.receive_size, 0, (sockaddr *) &addr, socklen) < 0) {
                        continue;
                    }
                    *(bufs.send_size) = recvfrom(fd, bufs.send_buf, bufs.send_buf_size, 0, (sockaddr *) &addr,
                                                 &socklen);
                    //printf("forward receive %d\n", *bufs.send_size);
                    if (*bufs.send_size < 12) {
                        std::memset(bufs.send_buf, 0, *bufs.send_size > 0 ? *bufs.send_size : bufs.send_buf_size);
                        continue;
                    } else {
                        break;
                    }
                }
                {
                    std::lock_guard<std::shared_mutex> rwlock(rw_mtx);
                    --processing[bufs.key];
                    if (processing[bufs.key] <= 0) {
                        processing.erase(bufs.key);
                    }
                }
            }
        }
    });
}

void Forward::exit() {
    quit.store(true);
    cond.notify_all();
}

bool Forward::is_processing(unsigned long long int key) {
    std::shared_lock<std::shared_mutex> rwlock(rw_mtx);
    return processing.count(key) == 1;
}

void Forward::add_task(Buf &&buf) {
    if (buf.send_buf_size < 16) {
        return;
    }
    {
        std::lock_guard<std::shared_mutex> rwlock(rw_mtx);
        ++processing[buf.key];
    }
    buf.send_buf_size -= sizeof(long);
    buf.send_size = (unsigned long *) &buf.send_buf[buf.send_buf_size - 1];
    {
        std::lock_guard<std::mutex> lock(this->mtx);
        tasks.push(buf);
    }
    this->cond.notify_all();
}

void Forward::add_dns(std::string &dns) {
    dns_server.push_back(dns);
}
