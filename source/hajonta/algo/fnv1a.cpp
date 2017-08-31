#pragma once

#define FNV1_32A_INIT ((uint32_t)0x811c9dc5)
#define FNV_32_PRIME ((uint32_t)0x01000193)

uint32_t
fnv1a_32(uint8_t *buf, uint32_t buf_size, uint32_t hval = FNV1_32A_INIT)
{
    uint8_t *end = buf + buf_size;
    for (uint8_t *pos = buf; pos < end; ++pos)
    {
        hval ^= *pos;
        hval *= FNV_32_PRIME;
    }
    return hval;
}

uint32_t
fnv1a_32_s(const char *str, uint32_t hval = FNV1_32A_INIT)
{
    unsigned char *c = (unsigned char *)str;
    while (*c)
    {
        hval ^= *c++;
        hval *= FNV_32_PRIME;
    }
    return hval;
}

#define TINYFNV1A(bits) uint32_t fnv1a_##bits(uint8_t *buf, uint32_t buf_size) { uint32_t hash; hash = fnv1a_32(buf, buf_size); hash = ((hash >> bits) ^ hash) & ((1<<bits) - 1); return hash; }
TINYFNV1A(6)
TINYFNV1A(10)

static bool
assertEqual(uint32_t left, uint32_t right, const char *msg, const char *file, int line)
{
#define _P(x, ...) printf("%s(%d) : " x "\n", file, line, __VA_ARGS__)
    if (left != right)
    {
        _P("TEST FAILED: %s", msg);
        _P("EXPECT: %u, GOT: %u", left, right);
        return false;
    }
    return true;
#undef _P
}

#define QUOTE(x) #x
#define T(x,y) {bool _unit_result = assertEqual(x, y, QUOTE(x) "        " QUOTE(y), __FILE__, __LINE__); if (!_unit_result) return false;};

bool
fnv1a_unittests()
{
    {
        const char *s = "";
        uint32_t hval = FNV1_32A_INIT;
        T(hval, fnv1a_32_s(s));
    }

    {
        const char *s = "+!=yG";
        uint32_t hval = 0;
        T(hval, fnv1a_32_s(s));
    }

    {
        const char *s = "68m* ";
        uint32_t hval = 0;
        T(hval, fnv1a_32_s(s));
    }

    {
        const char *s = "eSN.1";
        uint32_t hval = 0;
        T(hval, fnv1a_32_s(s));
    }

    {
        const char *s = "3pjNqM";
        uint32_t hval = 0;
        T(hval, fnv1a_32_s(s));
    }

#define ASSET_ID_S(_asset_name) (fnv1a_32_s(_asset_name))
#define ASSET_ID(_asset_name) (fnv1a_32((uint8_t *)_asset_name, (uint32_t)strlen(_asset_name)));
    {
        uint32_t hval = 0x1a47e90b;
        uint32_t ai = ASSET_ID("abc");
        uint32_t ais = ASSET_ID_S("abc");
        T(hval, ais);
        T(hval, ai);
    }

    return true;
}
