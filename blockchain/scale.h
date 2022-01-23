//
// Created by lenovo on 2022/1/13.
//

#ifndef BLOCKCHAIN_DNS_SCALE_H
#define BLOCKCHAIN_DNS_SCALE_H

#include <iostream>

#include <sstream>
#include <string>
#include <vector>

#include "blake/blake2.h"

bool isBig();

void shortswap(char *x);

void intswap(char *x);

std::string byte_to_hex(unsigned char data);

std::string encode_int8_to_fixed_int_scale(int8_t data);

std::string encode_uint8_to_fixed_int_scale(uint8_t data);

std::string encode_int16_to_fixed_int_scale(int16_t data);

std::string encode_uint16_to_fixed_int_scale(uint16_t data);

std::string encode_uint32_to_fixed_int_scale(uint32_t data);

std::string encode_uint8_to_compact(uint8_t data);

std::string encode_uint16_to_compact(uint16_t data);

std::string encode_uint32_to_compact(uint32_t data);

std::string encode_vec_u8_to_vectors(std::vector<uint8_t> &data);

std::string encode_vec_u16_to_vectors(std::vector<uint16_t> &data);

std::string encode_str_to_strings(std::string data);

std::string encode_str_to_u8_32(std::string data);

std::string encode_str_to_u8_32_with_blake2(std::string data);

//Decode

std::pair<short, uint32_t> decode_compact(std::string data);

uint8_t decode_fixed_to_u8(std::string data);

uint32_t decode_fixed_to_u32(std::string data);

std::pair<short, std::string> decode_vec_u8_to_string(std::string data);

#endif //BLOCKCHAIN_DNS_SCALE_H
