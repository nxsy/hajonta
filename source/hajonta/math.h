#pragma once

struct v2
{
    float x;
    float y;
};

struct v3
{
    float x;
    float y;
    float z;
};

struct triangle2
{
    v2 p0;
    v2 p1;
    v2 p2;
};

struct triangle3
{
    v3 p0;
    v3 p1;
    v3 p2;
};

struct rectangle2
{
    v2 position;
    v2 dimension;
};

struct line2
{
    v2 position;
    v2 direction;
};

union v4
{
    struct {
        union {
            float x;
            float r;
        };
        union {
            float y;
            float g;
        };
        union {
            float z;
            float b;
        };
        union {
            float w;
            float a;
        };
    };
    float E[4];
};

struct m4
{
    v4 cols[4];
};

