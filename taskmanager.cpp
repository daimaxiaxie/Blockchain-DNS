//
// Created by lenovo on 2022/1/18.
//

#include "taskmanager.h"


void Residual::clear() {
    if (buf != nullptr) {
        free(buf);
    }
}

void Task::clear_without_send() {
    if (receive_buf != nullptr) {
        free(receive_buf);
    }
    if (buf != nullptr) {
        free(buf);
    }
    if (query != nullptr) {
        query->clear();
        free(query);
    }
}


TaskManager::TaskManager(int min, int max, int blockchain, std::string blockchain_ip, bool forward, int forward_threads)
        : active(0),
          send_buf_size(
                  512),
          blockchainManager(
                  blockchain,
                  blockchain_ip),
          is_forward(
                  forward),
          cache(Cache::GetCache()) {
    while (min > 0) {
        create_thread();
        --min;
    }
    if (forward && forward_threads > 0) {
        this->forward.init(forward_threads);
    }
}

TaskManager::~TaskManager() {
    exit();
}

void TaskManager::create_thread() {
    threads.emplace_back([this] {
        int id = this->active.load();
        while (!this->active.compare_exchange_weak(id, id + 1)) {

        }
        ++id;
        std::unique_lock<std::mutex> lock(this->mtx, std::defer_lock);
        bool run_task = false;
        Task task;
        DNS dns;
        while (id <= this->active.load()) {
            lock.lock();
            this->cond.wait(lock, [this, id]() { return !this->tasks.empty() || id > this->active.load(); });
            run_task = false;
            if (!this->tasks.empty() && id <= this->active.load()) {
                task = std::move(this->tasks.front());
                this->tasks.pop();
                run_task = true;
            }
            lock.unlock();
            if (run_task) {
                task_handler(task, dns);
                if (task.state != Finish && task.state != Discard) {
                    add_task(task);
                }
            }
        }
    });
}

void TaskManager::add_task(Task task) {
    {
        std::lock_guard<std::mutex> lock(this->mtx);
        tasks.push(task);
    }
    this->cond.notify_all();
}

void TaskManager::exit() {
    if (active.load() > 0) {
        active.store(0);
        this->cond.notify_all();
    }
}

void TaskManager::set_epoll(int fd) {
    this->ep = fd;
}


void TaskManager::set_buf(int size) {
    send_buf_size = size;
}

void TaskManager::set_domain(std::string domain) {
    blockchain_domain = domain;
}

void TaskManager::add_forward_dns(std::string &dns) {
    forward.add_dns(dns);
}


bool TaskManager::task_handler(Task &task, DNS &dns) {
    bool repeat = true;
    while (repeat) {
        switch (task.state) {
            case Receiving:
                repeat = receive_handler(task, dns);
                break;
            case Parsing:
                repeat = parse_handler(task, dns);
                break;
            case Querying:
                repeat = query_handler(task, dns);
                break;
            case Forwarding:
                repeat = forward_handler(task);
                break;
            case Sending:
                repeat = send_handler(task, dns);
                break;
            case Finish:
                finish_handler(task);
                return false;
            default:
                discard_handler(task);
                return false;
        }
    }

    return true;
}

bool TaskManager::receive_handler(Task &task, DNS &dns) {
    switch (dns.parse(task.receive_buf, task.receive_size)) {
        case Success:
            task.query = dns.save();
            task.responseType = Normal;
            task.state = Parsing;
            send_buf_init(task);
            break;
        case Error:
            task.responseType = FormatError;
            task.state = Sending;
            send_buf_init(task);
            break;
        case Unfinished:
            //add epoll
            task.state = Discard;
            break;
        case InternalError:
            task.state = Discard;
            break;
        default:
            task.state = Discard;
            break;
    }
    //printf("receive handler\n");
    return true;
}

bool TaskManager::parse_handler(Task &task, DNS &dns) {
    bool repeat = true;
    task.state = Sending;
    printf("parse handler\n");
    for (int i = 0; i < task.query->header.questions; ++i) {
        char *domain = task.query->questions[i].query_name;
        if (endwith(domain, blockchain_domain)) {
            auto &&res = cache.get({domain, validlength(domain)}, task.query->questions[i].query_type);
            if (!res.empty() && repeat) {
                dns.resume(task.query);
                for (auto &record: res) {
                    size_t offset = dns.add_answer(task.send_buf, task.send_size, task.send_buf_size, i,
                                                   std::get<0>(record),
                                                   std::get<1>(record), std::get<2>(record));
                    if (offset == 0) {
                        task.responseType = ServerError;
                        task.state = Sending;
                        return true;
                    } else {
                        task.send_size = offset;
                    }

                }
            } else {
                if (res.empty()) {
                    printf("add to query\n");
                    blockchainManager.add_task(
                            {getKey(task.fd, task.timestamp), task.query->questions[i].query_type, domain,
                             validlength(domain)});
                }
                task.state = Querying;
                repeat = false;
            }
        } else if (is_forward) {
            task.state = Forwarding;
            printf("add to forward\n");
            forward.add_task(
                    {getKey(task.fd, task.timestamp), task.receive_buf, task.receive_size, task.receive_buf_size,
                     task.send_buf, nullptr, task.send_buf_size});
            task.responseType = Ignore;
            return false;
        }
    }

    return repeat;
}

bool TaskManager::query_handler(Task &task, DNS &dns) {
    if (blockchainManager.is_processing(getKey(task.fd, task.timestamp))) {
        return false;
    }

    printf("query handler\n");
    task.state = Sending;
    for (int i = 0; i < task.query->header.questions; ++i) {
        char *domain = task.query->questions[i].query_name;
        auto &&res = cache.get({domain, validlength(domain)}, task.query->questions[i].query_type);
        if (!res.empty()) {
            dns.resume(task.query);
            for (auto &record: res) {
                size_t offset = dns.add_answer(task.send_buf, task.send_size, task.send_buf_size, i,
                                               std::get<0>(record),
                                               std::get<1>(record), std::get<2>(record));
                if (offset == 0) {
                    task.responseType = ServerError;
                    return true;
                } else {
                    task.send_size = offset;
                }

            }
        }
    }
    return true;
}

bool TaskManager::forward_handler(Task &task) {
    if (forward.is_processing(getKey(task.fd, task.timestamp))) {
        return false;
    }

    task.send_size = *((unsigned long *) &task.send_buf[task.send_buf_size - sizeof(long) - 1]);
    for (int i = 0; i < sizeof(long); ++i) {
        task.send_buf[task.send_buf_size - i - 1] = 0x00;
    }
    task.state = Sending;
    return true;
}

bool TaskManager::send_handler(Task &task, DNS &dns) {
    printf("send handler\n");
    Residual *residual = (Residual *) malloc(sizeof(Residual));
    if (residual == nullptr) {
        printf("jemalloc malloc Residual error\n");
        task.state = Discard;
        return true;
    }
    if (task.responseType != Ignore) {
        dns.resume(task.query);
        dns.make_header(task.send_buf, task.send_buf_size, task.responseType);
        task.send_size = task.send_size > 12 ? task.send_size : 12;
    }
    residual->sent_len = send(task.fd, task.send_buf, task.send_size, 0);

    residual->fd = task.fd;
    residual->buf = task.send_buf;
    residual->send_size = task.send_size;
    epoll_event event;
    event.data.ptr = residual;
    event.events = EPOLLOUT | EPOLLERR | EPOLLET;
    epoll_ctl(ep, EPOLL_CTL_ADD, task.fd, &event);
    task.state = Finish;
    return true;
}

bool TaskManager::finish_handler(Task &task) {
    task.clear_without_send();
    printf("finish\n");
    return false;
}

bool TaskManager::discard_handler(Task &task) {
    task.clear_without_send();
    if (task.send_buf != nullptr) {
        free(task.send_buf);
    }
    printf("discard\n");
    return false;
}

bool TaskManager::send_buf_init(Task &task) {
    task.send_buf_size = send_buf_size;
    task.send_buf = (char *) malloc(send_buf_size);
    if (task.send_buf == nullptr) {
        task.state = Discard;
        return false;
    }
    return true;
}

bool endwith(char *str, std::string &param) {
    int len = strlen(str) - 1;
    int size = param.size() - 1;
    if (size < len) {
        while (size >= 0) {
            if (str[len] != param[size]) {
                return false;
            }
            --size;
            --len;
        }
        return true;
    }
    return false;
}

size_t validlength(char *str) {
    size_t pos = 1;
    size_t cur = 2;
    while (str[cur] != '\0') {
        if (str[cur] == '.') {
            pos = cur;
        }
        ++cur;
    }
    return pos;
}