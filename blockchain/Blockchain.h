//
// Created by lenovo on 2021/12/30.
//

#ifndef BLOCKCHAIN_DNS_BLOCKCHAIN_H
#define BLOCKCHAIN_DNS_BLOCKCHAIN_H

#include <iostream>

#include <curl/curl.h>
#include <jsoncpp/json/json.h>
#include <algorithm>
#include <xxhash.h>
#include <cstring>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <cmath>
#include <bitset>
#include <vector>

#include "scale.h"

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata);

std::string xxhashAsU8a(const char *data, int len, int BitLength);

struct Record {
    unsigned char type;
    char ip[48];
    unsigned int ttl;
    unsigned long ip_len;
};


class Blockchain {
public:
    Blockchain();

    ~Blockchain();

    bool connect(char *ip);

    std::vector<Record> query(char *domain, unsigned short type);

private:
    CURL *curl;
};


#endif //BLOCKCHAIN_DNS_BLOCKCHAIN_H
