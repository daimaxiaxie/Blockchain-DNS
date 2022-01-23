//
// Created by lenovo on 2022/1/16.
//

#include "config.h"

Config::Config() : blockchain_node("http://127.0.0.1:9933"), read_timeout(100), blockchain_domain("default"),
                   forward(false), tasks_min(get_nprocs()), tasks_max(get_nprocs()), recv_buf(512), send_buf(512),
                   blockchain_threads(get_nprocs() * 2), forward_threads(get_nprocs() * 2),thread_adjust_threshold(32) {

}

Config::Config(const char *file) : Config() {
    std::fstream in(file, std::ios::in|std::ios::binary);
    if (!in.is_open()) {
        perror("Config file : ");
        return;
    }
    Json::Reader reader;
    Json::Value root;
    if (reader.parse(in, root)) {
        if (!root["blockchain"].isNull() && !root["blockchain"].empty()) {
            blockchain_node = root["blockchain"].asString();
        }
        if (!root["read_timeout"].isNull() && !root["read_timeout"].empty()) {
            read_timeout = root["read_timeout"].asInt();
            if (read_timeout < 2) {
                read_timeout = 2;
            }
        }
        if (!root["blockchain_domain"].isNull() && !root["blockchain_domain"].empty()) {
            blockchain_domain = root["blockchain_domain"].asString();
        }
        if (!root["forward"].isNull() && !root["forward"].empty()) {
            forward = root["forward"].asBool();
            if (forward) {
                if (!root["forward_dns"].isNull() && !root["forward_dns"].empty() && root["forward_dns"].size() > 0) {
                    for (int i = 0; i < root["forward_dns"].size(); ++i) {
                        forward_dns.push_back(root["forward_dns"][i].asString());
                    }
                } else {
                    forward = false;
                }
                if (!root["forward_threads"].isNull() && !root["forward_threads"].empty() &&
                    root["forward_threads"].asInt() > 0) {
                    forward_threads = root["forward_threads"].asInt();
                }
            }
        }
        if (!root["thread_min"].isNull() && !root["thread_min"].empty() && !root["thread_max"].isNull() &&
            !root["thread_max"].empty()) {
            if (root["thread_min"].asInt() <= root["thread_max"].asInt()) {
                tasks_min = root["thread_min"].asInt();
                tasks_max = root["thread_max"].asInt();
            }
        }
        if (!root["receive_buf_size"].isNull() && !root["receive_buf_size"].empty()) {
            if (root["receive_buf_size"].asInt() >= 64 && root["receive_buf_size"].asInt() % 64 == 0) {
                recv_buf = root["receive_buf_size"].asInt();
            }
        }
        if (!root["send_buf_size"].isNull() && !root["send_buf_size"].empty()) {
            if (root["send_buf_size"].asInt() >= 64 && root["send_buf_size"].asInt() % 64 == 0) {
                send_buf = root["send_buf_size"].asInt();
            }
        }
        if (!root["blockchain_threads"].isNull() && !root["blockchain_threads"].empty()) {
            if (root["blockchain_threads"].asInt() > 0) {
                blockchain_threads = root["blockchain_threads"].asInt();
            }
        }
        if (!root["adjust_threshold"].isNull() && !root["adjust_threshold"].empty()) {
            thread_adjust_threshold = root["adjust_threshold"].asUInt();
        }
    }
    in.close();
}

Config::~Config() {

}


std::string Config::Blockchain() {
    return blockchain_node;
}

int Config::ReadTimeout() {
    return read_timeout;
}

std::string Config::Domain() {
    return blockchain_domain;
}

bool Config::Forward() {
    return forward;
}

int Config::TaskMin() {
    return tasks_min;
}

int Config::TaskMax() {
    return tasks_max;
}

int Config::SendBuf() {
    return send_buf;
}

int Config::RecvBuf() {
    return recv_buf;
}

int Config::BlockchainThreads() {
    return blockchain_threads;
}

std::vector<std::string> &Config::ForwardDNS() {
    return forward_dns;
}

int Config::ForwardThreads() {
    return forward_threads;
}

unsigned int Config::AdjustThreshold() {
    return thread_adjust_threshold;
}

bool string_copy_to_char(std::string &src, char *dest, int len) {
    return string_copy_to_char(std::move(src), dest, len);
}

bool string_copy_to_char(std::string &&src, char *dest, int len) {
    if (src.size() < len) {
        src.copy(dest, src.size(), 0);
        return true;
    } else {
        printf("string length over destination");
    }
    return false;
}


