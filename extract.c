/*
 * 从逻辑地址中提取页号和页内偏移 (Lab05 "How to Begin")。
 * 地址只取低 16 位: 高 8 位为页号, 低 8 位为页内偏移。
 */
#include <stdio.h>

int main(void) {
    unsigned int values[] = {1, 256, 32768, 32769, 128, 65534, 33153};

    for (size_t i = 0; i < sizeof values / sizeof values[0]; i++) {
        unsigned int addr = values[i] & 0xFFFF;

        printf("%u: page number = %u, offset = %u\n", values[i], addr >> 8,
               addr & 0xFF);
    }
    return 0;
}
