//
// Created by lenovo on 2022/1/16.
//

#ifndef BLOCKCHAIN_DNS_CONFIG_H
#define BLOCKCHAIN_DNS_CONFIG_H

#include <jsoncpp/json/json.h>
#include <sys/sysinfo.h>
#include <fstream>

class Config {
public:
    Config();

    Config(const char *file);

    ~Config();

    std::string Blockchain();

    int ReadTimeout();

    std::string Domain();

    bool Forward();

    int TaskMin();

    int TaskMax();

    int RecvBuf();

    int SendBuf();

    int BlockchainThreads();

    int ForwardThreads();

    std::vector<std::string>& ForwardDNS();

private:
    std::string blockchain_node;
    int read_timeout;
    std::string blockchain_domain;
    bool forward;
    int tasks_min;
    int tasks_max;
    int recv_buf;
    int send_buf;
    int blockchain_threads;
    int forward_threads;
    std::vector<std::string> forward_dns;
};


bool string_copy_to_char(std::string &src, char *dest, int len);

bool string_copy_to_char(std::string &&src, char *dest, int len);

#endif //BLOCKCHAIN_DNS_CONFIG_H
