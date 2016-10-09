#pragma once

uint32_t
fnv1a_32(uint8_t *buf, uint32_t buf_size)
{
    uint8_t *pos = buf;
    uint8_t *end = buf + buf_size;
    uint32_t hval = 0x811c9dc5;
    for (uint8_t databyte = *pos; pos < end; ++pos)
    {
        hval ^= databyte;
        hval *= 0x01000193;
    }
    return hval;
}

#define TINYFNV1A(bits) uint32_t fnv1a_##bits(uint8_t *buf, uint32_t buf_size) { uint32_t hash; hash = fnv1a_32(buf, buf_size); hash = ((hash >> bits) ^ hash) & ((1<<bits) - 1); return hash; }
TINYFNV1A(6)
TINYFNV1A(10)
