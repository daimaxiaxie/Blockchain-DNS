//
// Created by lenovo on 2022/1/13.
//

#include "scale.h"

bool isBig() {
    short x = 0x1;
    char *p = (char *) &x;
    if (*p == 1) {
        //Little Endian
        return false;
    }
    return true;
}

void shortswap(char *x) {
    std::swap(x[0], x[1]);
}

void intswap(char *x) {
    std::swap(x[0], x[3]);
    std::swap(x[1], x[2]);
}

std::string byte_to_hex(unsigned char data) {
    std::string res;
    int h = data / 16, l = data % 16;
    res += h < 10 ? ('0' + h) : ('a' + h - 10);
    res += l < 10 ? ('0' + l) : ('a' + l - 10);
    return res;
}

std::string encode_int8_to_fixed_int_scale(int8_t data) {
    /*
    std::stringstream ss;
    ss << std::hex << std::setw(2) << std::setfill('0') << unsigned(data);
    return ss.str();
     */
    return encode_uint8_to_fixed_int_scale(data);
}

std::string encode_uint8_to_fixed_int_scale(uint8_t data) {
    return byte_to_hex(data);
}

std::string encode_int16_to_fixed_int_scale(int16_t data) {
    if (isBig()) {
        shortswap((char *) &data);
    }
    char *p = (char *) &data;
    return byte_to_hex(p[0]) + byte_to_hex(p[1]);
}

std::string encode_uint16_to_fixed_int_scale(uint16_t data) {
    if (isBig()) {
        shortswap((char *) &data);
    }
    char *p = (char *) &data;
    return byte_to_hex(p[0]) + byte_to_hex(p[1]);
}

std::string encode_uint32_to_fixed_int_scale(uint32_t data) {
    if (isBig()) {
        intswap((char *) &data);
    }
    char *p = (char *) &data;
    std::stringstream ss;
    ss << byte_to_hex(p[0]) << byte_to_hex(p[1]) << byte_to_hex(p[2]) << byte_to_hex(p[3]);
    return ss.str();
}

std::string encode_uint8_to_compact(uint8_t data) {
    //0-63
    return byte_to_hex(data << 2);
}

std::string encode_uint16_to_compact(uint16_t data) {
    if (isBig()) {
        shortswap((char *) &data);
    }
    data = (data << 2) | 0x01;
    char *p = (char *) &data;
    return byte_to_hex(p[0]) + byte_to_hex(p[1]);
}

std::string encode_uint32_to_compact(uint32_t data) {
    if (isBig()) {
        intswap((char *) &data);
    }
    data = (data << 2) | 0x02;
    char *p = (char *) &data;
    return byte_to_hex(p[0]) + byte_to_hex(p[1]) + byte_to_hex(p[2]) + byte_to_hex(p[3]);
}

std::string encode_vec_u8_to_vectors(std::vector<uint8_t> &data) {
    std::string res;
    if (data.size() < 64) {
        res += encode_uint8_to_compact(data.size());
    } else if (data.size() < 16384) {
        res += encode_uint16_to_compact(data.size());
    } else if (data.size() < 1073741824) {
        res += encode_uint32_to_compact(data.size());
    }
    for (auto x: data) {
        res += byte_to_hex(x);
    }
    return res;
}

std::string encode_vec_u16_to_vectors(std::vector<uint16_t> &data) {
    std::string res;
    if (data.size() < 64) {
        res += encode_uint8_to_compact(data.size());
    } else if (data.size() < 16384) {
        res += encode_uint16_to_compact(data.size());
    } else if (data.size() < 1073741824) {
        res += encode_uint32_to_compact(data.size());
    }
    for (auto x: data) {
        res += encode_uint16_to_fixed_int_scale(x);
    }
    return res;
}

std::string encode_str_to_strings(std::string data) {
    std::vector<uint8_t> vec;
    vec.assign(data.begin(), data.end());
    return encode_vec_u8_to_vectors(vec);
}

std::string encode_str_to_u8_32(std::string data) {
    std::vector<uint8_t> vec;
    vec.assign(data.begin(), data.end());
    for (int i = vec.size(); i < 32; ++i) {
        vec.push_back(0);
    }
    return encode_vec_u8_to_vectors(vec);
}

std::string encode_str_to_u8_32_with_blake2(std::string data) {
    std::vector<uint8_t> vec;
    vec.assign(data.begin(), data.end());
    for (int i = vec.size(); i < 32; ++i) {
        vec.push_back(0);
    }

    std::string encode;
    uint8_t byte[34] = {0};
    unsigned int len = 32;
    for (int i = 0; i < len; ++i) {
        encode += byte_to_hex(vec[i]);
        byte[i] = vec[i];
    }
    uint8_t buf[16] = {0};
    blake2(buf, byte, nullptr, 16, len, 0);
    std::string b2;
    for (int i = 0; i < 16; ++i) {
        b2 += byte_to_hex(buf[i]);
    }
    return b2 + encode;
}

std::pair<short, uint32_t> decode_compact(std::string data) {
    if (data.size() < 2) {
        return {0, 0};
    }
    std::stringstream ss;
    ss << std::hex << data[0] << data[1];
    uint8_t buf[4] = {0};
    short count = 0;
    ss >> count;
    ss.clear();
    count &= 0x03;
    if (count == 0x00) {
        ss << std::hex << data[0] << data[1];
        ss >> count;
        return {1, count >> 2};
    } else if (count == 0x01) {
        if (data.size() < 4) {
            printf("decode compact integer data invalid\n");
            return {0, 0};
        }
        ss << std::hex << data[2] << data[3] << data[0] << data[1];
        ss >> count;
        return {2, count >> 2};
    } else if (count == 0x02) {
        if (data.size() < 8) {
            printf("decode compact integer data invalid\n");
            return {0, 0};
        }
        int res = 0;
        ss << std::hex << data[6] << data[7] << data[4] << data[5] << data[2] << data[3] << data[0] << data[1];
        ss >> res;
        return {4, res >> 2};
    } else {
        printf("decode compact integer over 32\n");
    }
    return {0, 0};
}

uint8_t decode_fixed_to_u8(std::string data) {
    if (data.size() < 2) {
        printf("decode fixed integer data invalid\n");
        return 0;
    }
    uint8_t res = (data[0] > '9' ? (data[0] - 'a' + 10) : (data[0] - '0')) * 16;
    res += (data[1] > '9' ? (data[1] - 'a' + 10) : (data[1] - '0'));
    return res;
}

uint32_t decode_fixed_to_u32(std::string data) {
    if (data.size() < 8) {
        printf("decode fixed integer data invalid\n");
        return 0;
    }
    std::stringstream ss;
    ss << std::hex << data[6] << data[7] << data[4] << data[5] << data[2] << data[3] << data[0] << data[1];
    uint32_t res;
    ss >> res;
    return res;
}

std::pair<short, std::string> decode_vec_u8_to_string(std::string data) {
    auto num = decode_compact(data);
    if (num.first == 0 || data.size() < num.second * 2) {
        return {0, ""};
    }
    data = data.substr(num.first * 2);
    std::string res;
    std::stringstream ss;

    for (int i = 0; i < num.second; ++i) {
        ss << std::hex << data[i * 2] << data[i * 2 + 1];
        short ch;
        ss >> ch;
        res.push_back(ch);
        ss.clear();
    }
    return {num.first + num.second, res};
};