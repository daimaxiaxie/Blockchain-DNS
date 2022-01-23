//
// Created by lenovo on 2022/1/17.
//

#include "dns.h"

void Flag::serialization(short data) {
    char *bytes = (char *) &data;
    QR = *bytes & 0x80;
    Opcode[0] = *bytes & 0x40;
    Opcode[1] = *bytes & 0x20;
    Opcode[2] = *bytes & 0x10;
    Opcode[3] = *bytes & 0x08;
    AA = *bytes & 0x04;
    TC = *bytes & 0x02;
    RD = *bytes & 0x01;

    ++bytes;
    RA = *bytes & 0x80;
    Placeholder[0] = *bytes & 0x40;
    Placeholder[1] = *bytes & 0x20;
    Placeholder[2] = *bytes & 0x10;
    Rcode[0] = *bytes & 0x08;
    Rcode[1] = *bytes & 0x04;
    Rcode[2] = *bytes & 0x02;
    Rcode[3] = *bytes & 0x01;
}

short Flag::deserialization() {
    short res = 0;
    char *byte = (char *) &res;
    *byte = QR << 7 | Opcode[0] << 6 | Opcode[1] << 5 | Opcode[2] << 4 | Opcode[3] << 3 | AA << 2 | TC << 1 | RD;
    ++byte;
    *byte = RA << 7 | Placeholder[0] << 6 | Placeholder[1] << 5 | Placeholder[2] << 4 | Rcode[0] << 3 | Rcode[1] << 2 |
            Rcode[2] << 1 | Rcode[3];
    return 0;
}

void Flag::clear() {
    QR = 0;
    Opcode[0] = 0x00;
    Opcode[1] = 0x00;
    Opcode[2] = 0x00;
    Opcode[3] = 0x00;
    AA = 0x00;
    TC = 0x00;
    RD = 0x00;
    RA = 0x00;
    Placeholder[0] = 0x00;
    Placeholder[1] = 0x00;
    Placeholder[2] = 0x00;
    Rcode[0] = 0x00;
    Rcode[1] = 0x00;
    Rcode[2] = 0x00;
    Rcode[3] = 0x00;
}


void Header::clear() {
    identification = 0;
    questions = 0;
    answer_rrs = 0;
    authority_rrs = 0;
    additional_rrs = 0;
    flags.clear();
}

void Question::clear() {
    query_type = 0;
    query_class = 0;
    if (query_name != nullptr) {
        free(query_name);
    }
    query_name = nullptr;
}

void Answer::clear() {
    if (name != nullptr) {
        free(name);
    }
    name = nullptr;
    if (rd != nullptr) {
        free(rd);
    }
    rd = nullptr;
    type = 0;
    cls = 0;
    ttl = 0;
    rd_len = 0;
}

Query::Query() {
    header = {0};
    questions = nullptr;
    answer = nullptr;
    authority = nullptr;
    additional = nullptr;
}

void Query::clear() {
    header.clear();
    if (questions != nullptr) {
        questions->clear();
        delete[] questions;
    }
    questions = nullptr;
    if (answer != nullptr) {
        answer->clear();
        delete[] answer;
    }
    answer = nullptr;
    if (authority != nullptr) {
        authority->clear();
        delete[] authority;
    }
    authority = nullptr;
    if (additional != nullptr) {
        additional->clear();
        delete[] additional;
    }
    additional = nullptr;
}


DNS::DNS() : query(nullptr) {

}

DNS::~DNS() {
    //clear();
}

Result DNS::parse(char *buf, long len) {
    if (len < 12) {
        return Error;
    }

    //raw_data(buf, len);

    void *ptr = malloc(sizeof(Query));
    if (ptr == nullptr) {
        printf("jemalloc malloc Query error!\n");
        return InternalError;
    }
    query = new(ptr) Query;

    parse_header(buf);
    if (query->header.flags.TC && len < 512) {
        return Unfinished;
    }

    query->questions = new Question[query->header.questions];
    int offset = 12;
    if (!parse_questions(buf, len, &offset)) {
        return Error;
    }

    if (query->header.flags.QR) {
        query->answer = new Answer[query->header.answer_rrs];
        if (!parse_a_a_a(buf, len, query->answer, query->header.answer_rrs, &offset)) {
            return Error;
        }
        query->authority = new Answer[query->header.authority_rrs];
        if (!parse_a_a_a(buf, len, query->authority, query->header.authority_rrs, &offset)) {
            return Error;
        }
        query->additional = new Answer[query->header.additional_rrs];
        if (!parse_a_a_a(buf, len, query->additional, query->header.additional_rrs, &offset)) {
            return Error;
        }
    }

    //print();

    return Success;
}

unsigned short DNS::query_type() {
    return query->questions->query_type;
}

short DNS::questions() {
    return query->header.questions;
}

size_t DNS::make_header(char *buf, long max, ResponseType type) {
    if (max < 12) { // 12 + 16
        return 0;
    }

    *((short *) buf) = htons(query->header.identification);
    short flag = query->header.flags.deserialization();
    char *byte = (char *) &flag;
    *(buf + 2) = *byte;
    *(buf + 3) = *(byte + 1);
    *(buf + 2) = *(buf + 2) | 0x80 & 0xfd;
    *(buf + 3) = *(buf + 3) & 0x80;
    switch (type) {
        case Normal:
            break;
        case FormatError:
            *(buf + 3) = *(buf + 3) | 0x01;
            break;
        case ServerError:
            *(buf + 3) = *(buf + 3) | 0x02;
            break;
        case NameError:
            *(buf + 3) = *(buf + 3) | 0x03;
            break;
        case NotImplemented:
            *(buf + 3) = *(buf + 3) | 0x04;
            break;
        case Refused:
            *(buf + 3) = *(buf + 3) | 0x05;
            break;
        default:
            break;
    }

    *((short *) (buf + 4)) = 0;
    *((short *) (buf + 6)) = htons(query->header.answer_rrs);
    *((short *) (buf + 8)) = 0;
    *((short *) (buf + 10)) = 0;

    return 12;
}

size_t
DNS::add_answer(char *buf, long offset, long max, int question_index, short type, std::string &ip, unsigned int ttl) {
    if (offset < 12) {
        offset = 12;
    }
    if (max - offset < 16) {
        return 0;
    }

    size_t len = offset;
    len += make_name(query->questions[question_index].query_name, buf + len, max - len);
    if (max - len < 12) {
        return 0;
    }
    *((short *) (buf + len)) = htons(type);
    *((short *) (buf + len + 2)) = htons(1);
    *((int *) (buf + len + 4)) = htonl(ttl);

    unsigned short rd_len = 0;
    switch (type) {
        case 1: // A
            rd_len = ipv4_to_rdata(&ip, buf + len + 10, max - len - 10);
            break;
        case 28: // AAAA
            rd_len = ipv6_to_rdata(&ip, buf + len + 10, max - len - 10);
            break;
        default:
            rd_len = raw_to_rdata(&ip, buf + len + 10, max - len - 10);
            break;
    }

    *((short *) (buf + len + 8)) = htons(rd_len);
    len += 10 + rd_len;

    ++query->header.answer_rrs;

    return len;
}

void DNS::clear() {
    if (query != nullptr) {
        query->clear();
        free(query);
    }
}

Query *DNS::save() {
    return query;
}

bool DNS::resume(Query *q) {
    if (q == nullptr) {
        return false;
    }
    query = q;
    return true;
}


bool DNS::parse_header(char *buf) {
    query->header.identification = ntohs(*((short *) buf));
    query->header.flags.serialization(*((short *) (buf + 2)));
    query->header.questions = ntohs(*((short *) (buf + 4)));
    query->header.answer_rrs = ntohs(*((short *) (buf + 6)));
    query->header.authority_rrs = ntohs(*((short *) (buf + 8)));
    query->header.additional_rrs = ntohs(*((short *) (buf + 10)));
    return true;
}

bool DNS::parse_questions(char *buf, int len, int *offset) {
    for (int i = 0; i < query->header.questions; ++i) {
        query->questions[i].query_name = parse_name(buf, len, offset);
        if (query->questions[i].query_name == nullptr) {
            return false;
        } else {
            query->questions[i].query_type = ntohs(*((uint16_t *) (buf + *offset)));
            query->questions[i].query_class = ntohs(*((uint16_t *) (buf + *offset + 2)));
            *offset += 4;
        }
    }
    return true;
}

bool DNS::parse_a_a_a(char *buf, int len, Answer *A, int count, int *offset) {
    for (int i = 0; i < count; ++i) {
        A->name = parse_name_ptr(buf, len, offset);
        A->type = htons(*((uint16_t *) (buf + *offset)));
        A->cls = htons(*((uint16_t *) (buf + *offset + 2)));
        A->ttl = htonl(*((uint16_t *) (buf + *offset + 4)));
        A->rd_len = htons(*((uint16_t *) (buf + *offset + 8)));
        A->rd = (char *) malloc(A->rd_len);
        if (A->rd == nullptr) {
            return false;
        }
        std::memcpy(A->rd, buf + *offset + 10, A->rd_len);
        *offset += 10 + A->rd_len;
    }
    return true;
}

char *DNS::parse_name(char *buf, int max, int *offset) {
    if (max < 2) {
        return nullptr;
    }
    std::string name;
    char *cur = buf + *offset;
    while (*cur != 0x00 && *offset < max) {
        char len = *cur;
        name.append(cur + 1, len);
        name.append(".");
        *offset += len + 1;
        cur += len + 1;
    }
    if (name.back() == '.') {
        name.pop_back();
    }

    if (*cur == 0x00) {
        ++*offset;
    } else {
        return nullptr;
    }

    char *res = (char *) malloc(name.size() + 1);
    if (res == nullptr) {
        return nullptr;
    }
    name.copy(res, name.size());
    res[name.size()] = '\0';
    return res;
}

char *DNS::parse_name_ptr(char *buf, int len, int *offset) {
    char *cur = buf + *offset;
    std::string name;
    while (*cur != 0x00 && (*cur & 0xc0) != 0xc0 && *offset < len) {//pointer : 11000000
        char l = *cur;
        name.append(cur + 1, l);
        name.append(".");
        *offset += l + 1;
        cur += l + 1;
    }

    if (name.back() == '.') {
        name.pop_back();
    }

    if (*cur == 0x00) {
        ++*offset;
    }

    char *res = nullptr;
    if ((*cur & 0xc0) == 0xc0) {
        unsigned short p = *(short *) cur;
        p &= 0x3fff;
        int pos = htons(p);
        char *ptr = parse_name_ptr(buf, len, &pos);
        int size = name.size() + pos - htons(*cur);
        res = (char *) malloc(size + 1);
        if (res == nullptr) {
            return nullptr;
        }
        name.copy(res, name.size());
        std::memcpy(res + name.size(), ptr, pos - htons(*cur));
        res[size] = '\0';
        *offset += 2;
    } else {
        res = (char *) malloc(name.size() + 1);
        if (res == nullptr) {
            return nullptr;
        }
        name.copy(res, name.size());
        res[name.size()] = '\0';
    }
    return res;
}

void DNS::print() {
    std::cout << "+--------" << "+--------" << "+--------" << "+--------" << "+" << std::endl;
    std::cout << "|" << std::setw(17) << query->header.identification;
    std::cout << "|" << query->header.flags.QR;
    std::cout << query->header.flags.Opcode[0] << query->header.flags.Opcode[1] << query->header.flags.Opcode[2]
              << query->header.flags.Opcode[3];
    std::cout << query->header.flags.AA << query->header.flags.TC << query->header.flags.RD << " "
              << query->header.flags.RA
              << "000";
    std::cout << query->header.flags.Rcode[0] << query->header.flags.Rcode[1] << query->header.flags.Rcode[2]
              << query->header.flags.Rcode[3] << "|" << std::endl;
    std::cout << "+--------" << "+--------" << "+--------" << "+--------" << "+" << std::endl;
    std::cout << "|" << std::setw(17) << query->header.questions << "|" << std::setw(17) << query->header.answer_rrs
              << "|" << std::endl;
    std::cout << "+--------" << "+--------" << "+--------" << "+--------" << "+" << std::endl;
    std::cout << "|" << std::setw(17) << query->header.authority_rrs << "|" << std::setw(17)
              << query->header.additional_rrs << "|"
              << std::endl;
    std::cout << "+--------" << "+--------" << "+--------" << "+--------" << "+" << std::endl;
    if (query->questions != nullptr) {
        print_str_endl(query->questions->query_name);
        std::cout << "+--------" << "+--------" << "+--------" << "+--------" << "+" << std::endl;
        std::cout << "|" << std::setw(17) << query->questions->query_type << "|" << std::setw(17)
                  << query->questions->query_class << "|"
                  << std::endl;
    } else {
        std::cout << "|        " << "         " << "         " << "         " << "|" << std::endl;
    }
    std::cout << "+--------" << "+--------" << "+--------" << "+--------" << "+" << std::endl;
    if (query->answer != nullptr) {
        print_str_endl(query->answer->name);
        std::cout << "+--------" << "+--------" << "+--------" << "+--------" << "+" << std::endl;
        std::cout << "|" << std::setw(17) << query->answer->type << "|" << std::setw(17) << query->answer->cls << "|"
                  << std::endl;
        std::cout << "+--------" << "+--------" << "+--------" << "+--------" << "+" << std::endl;
        std::cout << "|" << std::setw(35) << query->answer->ttl << "|" << std::endl;
        std::cout << "+--------" << "+--------" << "+--------" << "+--------" << "+" << std::endl;
        std::cout << "|" << std::setw(17) << query->answer->rd_len << "| 0x";
        raw_data_default(query->answer->rd, 6);
        std::cout << "..|" << std::endl;
    } else {
        std::cout << "|        " << "         " << "         " << "         " << "|" << std::endl;
    }
    std::cout << "+--------" << "+--------" << "+--------" << "+--------" << "+" << std::endl;
    if (query->authority != nullptr) {
        print_str_endl(query->authority->name);
        std::cout << "+--------" << "+--------" << "+--------" << "+--------" << "+" << std::endl;
        std::cout << "|" << std::setw(17) << query->authority->type << "|" << std::setw(17) << query->authority->cls
                  << "|"
                  << std::endl;
        std::cout << "+--------" << "+--------" << "+--------" << "+--------" << "+" << std::endl;
        std::cout << "|" << std::setw(35) << query->authority->ttl << "|" << std::endl;
        std::cout << "+--------" << "+--------" << "+--------" << "+--------" << "+" << std::endl;
        std::cout << "|" << std::setw(17) << query->authority->rd_len << "| 0x";
        raw_data_default(query->authority->rd, 6);
        std::cout << "..|" << std::endl;
    } else {
        std::cout << "|        " << "         " << "         " << "         " << "|" << std::endl;
    }
    std::cout << "+--------" << "+--------" << "+--------" << "+--------" << "+" << std::endl;
    if (query->additional != nullptr) {
        print_str_endl(query->additional->name);
        std::cout << "+--------" << "+--------" << "+--------" << "+--------" << "+" << std::endl;
        std::cout << "|" << std::setw(17) << query->additional->type << "|" << std::setw(17) << query->additional->cls
                  << "|"
                  << std::endl;
        std::cout << "+--------" << "+--------" << "+--------" << "+--------" << "+" << std::endl;
        std::cout << "|" << std::setw(35) << query->additional->ttl << "|" << std::endl;
        std::cout << "+--------" << "+--------" << "+--------" << "+--------" << "+" << std::endl;
        std::cout << "|" << std::setw(17) << query->additional->rd_len << "| 0x";
        raw_data_default(query->additional->rd, 6);
        std::cout << "..|" << std::endl;
    } else {
        std::cout << "|        " << "         " << "         " << "         " << "|" << std::endl;
    }
    std::cout << "+--------" << "+--------" << "+--------" << "+--------" << "+" << std::endl;
}


void print_str_endl(char *str) {
    int count = 0;
    while (str[count] != '\0' && count < 256) {
        if (count == 0 || count % 35 == 0) {
            std::cout << '|';
        }
        std::cout << str[count];
        ++count;
        if (count % 35 == 0) {
            std::cout << '|' << std::endl;
        }
    }
    while (count % 35 != 0) {
        std::cout << ' ';
        ++count;
    }
    std::cout << '|' << std::endl;
}

void raw_data(char *buf, int len) {
    for (int i = 0; i < len; ++i) {
        std::cout << byte_to_hex(buf[i]) << " ";
        if (i > 0 && (i + 1) % 4 == 0) {
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
}

void raw_data_default(char *buf, int len) {
    for (int i = 0; i < len; ++i) {
        std::cout << byte_to_hex(buf[i]);
    }
}

int DNS::make_name(char *name, char *buf, int len) {
    int name_len = strlen(name);
    if (len < name_len) {
        return 0;
    }
    int offset = 0;
    int count = 0;
    int last = 0;
    while (*(name + count) != '\0') {
        if (*(name + count) == '.') {
            buf[offset] = count - last;
            std::memcpy(buf + offset + 1, name + last, buf[offset]);
            last = count + 1;
            offset += buf[offset] + 1;
        }
        ++count;
    }

    if (*(name + count) == '\0') {
        buf[offset] = count - last;
        std::memcpy(buf + offset + 1, name + last, buf[offset]);
        offset += buf[offset] + 1;
    }

    buf[offset] = 0x00;
    ++offset;
    return offset;
}

int DNS::ipv4_to_rdata(std::string *ip, char *buf, int len) {
    if (len < 4) {
        return 0;
    }
    uint32_t _ip = inet_addr(ip->c_str());
    std::memcpy(buf, &_ip, 4);
    return 4;
}

int DNS::ipv6_to_rdata(std::string *ip, char *buf, int len) {
    if (len < 16) {
        return 0;
    }
    inet_pton(AF_INET6, ip->c_str(), buf);
    return 16;
}

int DNS::raw_to_rdata(std::string *ip, char *buf, int len) {
    if (len < ip->size()) {
        return 0;
    }
    std::memcpy(buf, ip->c_str(), ip->size());
    return 0;
}


std::string byte_to_hex(unsigned char data) {
    std::string res;
    int h = data / 16, l = data % 16;
    res += h < 10 ? ('0' + h) : ('a' + h - 10);
    res += l < 10 ? ('0' + l) : ('a' + l - 10);
    return res;
}