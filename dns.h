//
// Created by lenovo on 2022/1/17.
//

#ifndef BLOCKCHAIN_DNS_DNS_H
#define BLOCKCHAIN_DNS_DNS_H

#include <jemalloc/jemalloc.h>
#include <arpa/inet.h>
#include <cstring>


#include <iostream>
#include <iomanip>


// 2 bytes
class Flag {
public:
    void serialization(short data);

    short deserialization();

    void clear();

public:
    bool QR;
    bool Opcode[4];
    bool AA;
    bool TC;
    bool RD;
    bool RA;
    bool Placeholder[3];
    bool Rcode[4];
};

// 12 bytes
struct Header {
    unsigned short identification;
    Flag flags;
    unsigned short questions;
    unsigned short answer_rrs;
    unsigned short authority_rrs;
    unsigned short additional_rrs;

    void clear();
};

struct Question {
    char *query_name;
    unsigned short query_type;
    unsigned short query_class;

    void clear();
};

//Answer Authority Additional
struct Answer {
    char *name;
    unsigned short type;
    unsigned short cls;//class
    unsigned int ttl;
    unsigned short rd_len;
    char *rd;

    void clear();
};

struct Query {
    Header header;
    Question *questions;
    Answer *answer;
    Answer *authority;
    Answer *additional;

    Query();

    void clear();
};

enum Result {
    Success,
    Error,
    Unfinished,
    InternalError,
};

enum ResponseType {
    Normal,
    FormatError,
    ServerError,
    NameError,
    NotImplemented,
    Refused,
    Ignore,
};

class DNS {
public:
    DNS();

    ~DNS();

    Result parse(char *buf, long len);

    unsigned short query_type();

    short questions();

    void print();

    size_t make_header(char *buf, long max, ResponseType type);

    size_t
    add_answer(char *buf, long offset, long max, int question_index, short type, std::string &ip, unsigned int ttl);

    void clear();

    Query *save();

    bool resume(Query *q);

private:
    bool parse_header(char *buf);

    bool parse_questions(char *buf, int len, int *offset);

    bool parse_a_a_a(char *buf, int len, Answer *A, int count, int *offset);

    char *parse_name(char *buf, int max, int *offset);

    char *parse_name_ptr(char *buf, int len, int *offset);

    int make_name(char *name, char *buf, int len);

    int ipv4_to_rdata(std::string *ip, char *buf, int len);

    int ipv6_to_rdata(std::string *ip, char *buf, int len);

    int raw_to_rdata(std::string *ip, char *buf, int len);

private:
    Query *query;
};

void print_str_endl(char *str);

void raw_data(char *buf, int len);

void raw_data_default(char *buf, int len);

std::string byte_to_hex(unsigned char data);

#endif //BLOCKCHAIN_DNS_DNS_H
