#include <math.h>

struct v2
{
    float x;
    float y;
};

v2
v2add(v2 left, v2 right)
{
    v2 result = {left.x + right.x, left.y + right.y};
    return result;
}

v2
v2sub(v2 left, v2 right)
{
    v2 result = {left.x - right.x, left.y - right.y};
    return result;
}

v2
v2mul(v2 v, float multiplier)
{
    v2 result = {v.x * multiplier, v.y * multiplier};
    return result;
}

v2
v2div(v2 v, float divisor)
{
    v2 result = {v.x / divisor, v.y / divisor};
    return result;
}

float
v2length(v2 v)
{
    float result = sqrtf(v.x * v.x + v.y * v.y);
    return result;
}

v2
v2normalize(v2 v)
{
    float length = v2length(v);
    if (length == 0)
    {
        return v;
    }

    v2 result = {v.x / length, v.y / length};
    return result;
}

float
v2dot(v2 left, v2 right)
{
    float result = (left.x * right.x + left.y * right.y);
    return result;
}

struct v3
{
    float x;
    float y;
    float z;
};

v3
v3add(v3 left, v3 right)
{
    v3 result = {left.x + right.x, left.y + right.y, left.z + right.z};
    return result;
}

v3
v3sub(v3 left, v3 right)
{
    v3 result = {left.x - right.x, left.y - right.y, left.z - right.z};
    return result;
}

v3
v3mul(v3 v, float multiplier)
{
    v3 result = {v.x * multiplier, v.y * multiplier, v.z * multiplier};
    return result;
}

v3
v3div(v3 v, float divisor)
{
    v3 result = {v.x / divisor, v.y / divisor, v.z / divisor};
    return result;
}

float
v3length(v3 v)
{
    float result = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    return result;
}

v3
v3normalize(v3 v)
{
    float length = v3length(v);
    if (length == 0)
    {
        return v;
    }

    v3 result = {v.x / length, v.y / length, v.z / length};
    return result;
}

float
v3dot(v3 left, v3 right)
{
    float result = (left.x * right.x + left.y * right.y + left.z * right.z);
    return result;
}

v3
v3cross(v3 p, v3 q)
{
    v3 result = {
        p.y * q.z - p.z * q.y,
        -(p.x * q.z - p.z * q.x),
        p.x * q.y - p.y * q.x
    };

    return result;
}

static bool
assertEqual(v3 left, v3 right, char *msg, char *file, int line)
{
    if (!(
        (left.x == right.x) &&
        (left.y == right.y) &&
        (left.z == right.z) &&
        1))
    {
#define _P(x, ...) printf("%s(%d) : "x, file, line, __VA_ARGS__)
        _P("TEST FAILED: %s\n", msg);
        _P("EQUAL: x: %d, y: %d, z: %d\n", left.x == right.x, left.y == right.y, left.z == right.z);
        _P("EXPECT: x: %.8f, y: %.8f, z: %.8f\n", left.x, left.y, left.z);
        _P("GOT: x: %.8f, y: %.8f, z: %.8f\n", right.x, right.y, right.z);
#undef _P
        return false;
    }
    return true;
}

static bool
assertEqual(float left, float right, char *msg, char *file, int line)
{
#define _P(x, ...) printf("%s(%d) : "x"\n", file, line, __VA_ARGS__)
    if (left != right)
    {
        _P("TEST FAILED: %s", msg);
        _P("EXPECT: %.8f, GOT: %.8f", left, right);
        return false;
    }
    return true;
#undef _P
}

#define QUOTE(x) #x
#define T(x,y) {bool _unit_result = assertEqual(x, y, QUOTE(x) "        " QUOTE(y), __FILE__, __LINE__); if (!_unit_result) return false;};

bool
v3unittests()
{
    v3 cross_a = {3, -3, 1};
    v3 cross_b = {4, 9, 2};
    v3 cross_result = {-15, -2, 39};

    T( (v3{ 7, 6, 3}), (v3add(cross_a, cross_b)) );
    T( (v3{-1, -12, -1}), (v3sub(cross_a, cross_b)) );
    T( (v3{ 6, -6, 2}), (v3mul(cross_a, 2)) );
    T( (v3{ 2, 4.5, 1}), (v3div(cross_b, 2)) );
    T( cross_result, (v3cross(cross_a, cross_b)) );
    T( -13, (v3dot(cross_a, cross_b)) );
    T( 5 * sqrt(70.0) , v3length(cross_result) );

    return true;
}
