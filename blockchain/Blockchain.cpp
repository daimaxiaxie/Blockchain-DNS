//
// Created by lenovo on 2021/12/30.
//

#include "Blockchain.h"


size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    std::string data(ptr, size * nmemb);
    *((std::stringstream *) userdata) << data;
    return size * nmemb;
}

std::string xxhashAsU8a(const char *data, int len, int BitLength) {
    int rounds = std::ceil(BitLength / 64.0);
    std::string res;
    char buf[17] = {0};
    for (int seed = 0; seed < rounds; ++seed) {
        XXH64_hash_t hash = XXH64(data, len, seed);
        //printf("0x%016llx\n", hash);// debug
        char *point = (char *) &hash;
        std::reverse(point, point + 8);
        sprintf(buf, "%016lx", hash);
        res.append(buf, 16);
    }
    return res;
}

Blockchain::Blockchain() : curl(nullptr) {
    CURL *handle = curl_easy_init();
    if (handle) {
        curl = handle;
    } else {
        printf("Blockchain init error\n");
    }
}

Blockchain::~Blockchain() {
    if (curl) {
        curl_easy_cleanup(curl);
        curl = nullptr;
    }
}

bool Blockchain::connect(char *ip) {
    curl_easy_setopt(curl, CURLOPT_URL, ip);
    curl_slist *list = nullptr;
    list = curl_slist_append(list, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

    Json::Value query;
    query["id"] = 1;
    query["jsonrpc"] = "2.0";
    query["method"] = "rpc_methods";
    std::string str = query.toStyledString();
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, str.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, str.size());

    std::stringstream ss;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ss);

    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);// Debug
    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        //printf("%s", ss.str().c_str());
        if (ss.str().find("state_getStorage") == std::string::npos) {
            return false;
        }
        return true;
    } else {
        printf("Blockchain connect error : %d\n", res);
    }
    curl_slist_free_all(list);
    return false;
}

std::vector<Record> Blockchain::query(char *domain, unsigned short type) {

    const char *module = "TemplateModule";
    const char *function = "Records";
    const char *key = domain;

    std::string param = "0x" + xxhashAsU8a(module, 14, 128) + xxhashAsU8a(function, 7, 128);
    param += encode_str_to_u8_32_with_blake2(key);
    std::cout << std::endl << param << std::endl;
    Json::Value query;
    query["id"] = 1;
    query["jsonrpc"] = "2.0";
    query["method"] = "state_getStorage";
    query["params"].append(param);
    std::string str = query.toStyledString();
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, str.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, str.size());

    std::stringstream ss;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ss);
    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        //printf("%s", ss.str().c_str());
        Json::Reader reader;
        query.clear();
        reader.parse(ss.str(), query);
        if (query["result"].empty() || query["result"].isNull()) {
            return {};
        }
        std::string res = query["result"].asString();
        std::cout << res << std::endl;
        if (res == "null") {
            return {};
        }
        res = res.substr(2);
        auto num = decode_compact(res);
        res = res.substr(num.first * 2);
        std::vector<Record> vec;
        for (int i = 0; i < num.second; ++i) {
            vec.emplace_back();
            vec.back().type = decode_fixed_to_u8(res);
            //std::cout << "rr_type : " << unsigned(result->type) << std::endl;
            res = res.substr(2);
            auto ip = decode_vec_u8_to_string(res);
            if (ip.second.size() > 46) {
                vec.pop_back();
                continue;
            } else {
                vec.back().ip_len = ip.second.size();
            }
            std::memcpy(vec.back().ip, ip.second.c_str(), ip.second.size());
            vec.back().ip[vec.back().ip_len] = '\0';
            std::cout << "ip : " << ip.second << std::endl;
            res = res.substr(ip.first * 2);
            vec.back().ttl = decode_fixed_to_u32(res);
            //std::cout << "ttl : " << result->ttl << std::endl;
            res = res.substr(8);
        }
        //printf("Blockchain query over\n");
        return vec;
    } else {
        printf("Blockchain query error : %d\n", res);
        return {};
    }
}



