#include <math.h>
#include <stdint.h>
#include <string.h>

#ifndef harray_count
#define harray_count(array) (sizeof(array) / sizeof((array)[0]))
#endif

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

float
v2cross(v2 p, v2 q)
{
    return p.x * q.y - p.y * q.x;
}

v2
v2projection(v2 q, v2 p)
{
    /*
     * Projection of vector p onto q
     */
    return v2mul(q, v2dot(p, q) / v2dot(q, q));
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
#define _P(x, ...) printf("%s(%d) : " x "\n", file, line, __VA_ARGS__)
        _P("TEST FAILED: %s", msg);
        _P("EQUAL: x: %d, y: %d, z: %d", left.x == right.x, left.y == right.y, left.z == right.z);
        _P("EXPECT: x: %.8f, y: %.8f, z: %.8f", left.x, left.y, left.z);
        _P("GOT: x: %.8f, y: %.8f, z: %.8f", right.x, right.y, right.z);
#undef _P
        return false;
    }
    return true;
}

static bool
assertEqual(v2 left, v2 right, char *msg, char *file, int line)
{
    if (!(
        (left.x == right.x) &&
        (left.y == right.y) &&
        1))
    {
#define _P(x, ...) printf("%s(%d) : " x "\n", file, line, __VA_ARGS__)
        _P("TEST FAILED: %s", msg);
        _P("EQUAL: x: %d, y: %d", left.x == right.x, left.y == right.y);
        _P("EXPECT: x: %.8f, y: %.8f", left.x, left.y);
        _P("GOT: x: %.8f, y: %.8f", right.x, right.y);
#undef _P
        return false;
    }
    return true;
}

static bool
assertEqual(float left, float right, char *msg, char *file, int line)
{
#define _P(x, ...) printf("%s(%d) : " x "\n", file, line, __VA_ARGS__)
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
v2unittests()
{
    {
        v2 a = {0.5f, 0.866f};
        v2 b = {1, 0};
        v2 a1 = {0.5, 0};
        T( a1, (v2projection(b, a)) );
    }

    {
        v2 a = {-4, 1};
        v2 b = {1, 2};
        v2 p_of_a_onto_b = {-0.4f, -0.8f};
        v2 p_of_b_onto_a = {8.0f/17, -2.0f/17};
        T( p_of_a_onto_b, (v2projection(b, a)) );
        T( p_of_b_onto_a, (v2projection(a, b)) );
    }

    {
        v2 a = { 2, 1};
        v2 b = {-3, 4};
        v2 p_of_a_onto_b = {0.24f, -0.32f};
        T( p_of_a_onto_b, (v2projection(b, a)) );
    }

    return true;
}

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
    T( 5 * sqrtf(70.0f) , v3length(cross_result) );
    T( 0, (v3dot(cross_result, cross_a)));
    T( 0, (v3dot(cross_result, cross_b)));

    v3 i = {1, 0, 0};
    v3 j = {0, 1, 0};
    v3 k = {0, 0, 1};

    T( k, (v3cross(i, j)) );
    T( i, (v3cross(j, k)) );
    T( j, (v3cross(k, i)) );

    T( (v3mul(k,-1)), (v3cross(j, i)) );
    T( (v3mul(i,-1)), (v3cross(k, j)) );
    T( (v3mul(j,-1)), (v3cross(i, k)) );

    T( (v3{0,0,0}), (v3cross(i, i)) );

    return true;
}

struct triangle2
{
    v2 p0;
    v2 p1;
    v2 p2;
};

bool
point_in_triangle(v2 point, triangle2 tri)
{
    float s = tri.p0.y * tri.p2.x - tri.p0.x * tri.p2.y + (tri.p2.y - tri.p0.y) * point.x + (tri.p0.x - tri.p2.x) * point.y;
    float t = tri.p0.x * tri.p1.y - tri.p0.y * tri.p1.x + (tri.p0.y - tri.p1.y) * point.x + (tri.p1.x - tri.p0.x) * point.y;

    if ((s < 0) != (t < 0))
        return false;

    float a = -tri.p1.y * tri.p2.x + tri.p0.y * (tri.p2.x - tri.p1.x) + tri.p0.x * (tri.p1.y - tri.p2.y) + tri.p1.x * tri.p2.y;
    if (a < 0.0)
    {
        s = -s;
        t = -t;
        a = -a;
    }
    return (s > 0) && (t > 0) && ((s + t) < a);
}

struct line2
{
    v2 position;
    v2 direction;
};

bool
line_intersect(line2 ppr, line2 qqs, v2 *intersect_point)
{
    /*
    t = (q − p) × s / (r × s)
    u = (q − p) × r / (r × s)
    */
    v2 p = ppr.position;
    v2 r = ppr.direction;
    v2 q = qqs.position;
    v2 s = qqs.direction;

    v2 q_minus_p = v2sub(q, p);
    float r_cross_s = v2cross(r, s);
    float t = v2cross(q_minus_p, s) / r_cross_s;
    float u = v2cross(q_minus_p, r) / r_cross_s;

    *intersect_point = v2add(p, v2mul(r, t));
    if ((t >= 0) && (t <= 1) && (u >= 0) && (u <= 1))
    {
        return true;
    }
    return false;
}

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

v4
v4add(v4 left, v4 right)
{
    v4 result = {left.x + right.x, left.y + right.y, left.z + right.z, left.w + right.w};
    return result;
}

v4
v4sub(v4 left, v4 right)
{
    v4 result = {left.x - right.x, left.y - right.y, left.z - right.z, left.w - right.w};
    return result;
}

v4
v4mul(v4 v, float multiplier)
{
    v4 result = {v.x * multiplier, v.y * multiplier, v.z * multiplier, v.w * multiplier};
    return result;
}

v4
v4div(v4 v, float divisor)
{
    v4 result = {v.x / divisor, v.y / divisor, v.z / divisor, v.w / divisor};
    return result;
}

float
v4length(v4 v)
{
    float result = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
    return result;
}

v4
v4normalize(v4 v)
{
    float length = v4length(v);
    if (length == 0)
    {
        return v;
    }

    v4 result = {v.x / length, v.y / length, v.z / length, v.w / length};
    return result;
}

float
v4dot(v4 left, v4 right)
{
    float result = (left.x * right.x + left.y * right.y + left.z * right.z + left.w * right.w);
    return result;
}


struct m4
{
    v4 cols[4];
};

m4
m4identity()
{
    m4 result = {};
    result.cols[0].E[0] = 1;
    result.cols[1].E[1] = 1;
    result.cols[2].E[2] = 1;
    result.cols[3].E[3] = 1;
    return result;
};

void
m4copy(m4 *dest, const m4 source)
{
    for (uint32_t col = 0;
            col < harray_count(dest[0].cols);
            ++col)
    {
        *(dest->cols + col) = source.cols[col];
    }
};

void
m4transpose(m4 *matrix)
{
    for (uint32_t col = 0;
            col < 4;
            ++col)
    {
        for (uint32_t row = 0;
                row < col+1;
                ++row)
        {
            float t = matrix->cols[col].E[row];
            matrix->cols[col].E[row] = matrix->cols[row].E[col];
            matrix->cols[row].E[col] = t;
        }
    }
};

m4
m4transposed(const m4 matrix)
{
    m4 result;
    m4copy(&result, matrix);
    m4transpose(&result);
    return result;
}

m4
m4mul(const m4 matrix, float multiplier)
{
    m4 result = {};

    for (uint32_t col = 0;
            col < harray_count(matrix.cols);
            ++col)
    {
        for (uint32_t row = 0;
                row < harray_count(matrix.cols[0].E);
                ++row)
        {
            result.cols[col].E[row] = matrix.cols[col].E[row] * multiplier;
        }
    }
    return result;
}

bool
m4identical(const m4 left, const m4 right)
{
    for (uint32_t col = 0;
            col < harray_count(left.cols);
            ++col)
    {
        const v4 *lc = left.cols + col;
        const v4 *rc = right.cols + col;
        for (uint32_t row = 0;
                row < harray_count(left.cols[0].E);
                ++row)
        {
            if(lc->E[row] != rc->E[row])
            {
                return false;
            }
        }
    }
    return true;
}

m4
m4add(const m4 left, const m4 right)
{
    m4 result = {};

    for (uint32_t col = 0;
            col < harray_count(left.cols);
            ++col)
    {
        for (uint32_t row = 0;
                row < harray_count(left.cols[0].E);
                ++row)
        {
            result.cols[col].E[row] = left.cols[col].E[row] + right.cols[col].E[row];
        }
    }

    return result;
}

m4
m4sub(const m4 left, const m4 right)
{
    m4 result = {};

    for (uint32_t col = 0;
            col < harray_count(left.cols);
            ++col)
    {
        for (uint32_t row = 0;
                row < harray_count(left.cols[0].E);
                ++row)
        {
            result.cols[col].E[row] = left.cols[col].E[row] - right.cols[col].E[row];
        }
    }

    return result;
}

m4
m4mul(const m4 left, const m4 right)
{
    m4 result = {};
    uint32_t ncols = harray_count(left.cols);
    uint32_t nrows = harray_count(left.cols[0].E);
    for (uint32_t col = 0;
            col < ncols;
            ++col)
    {
        for (uint32_t row = 0;
                row < nrows;
                ++row)
        {
            v4 lrow = {
                left.cols[0].E[row],
                left.cols[1].E[row],
                left.cols[2].E[row],
                left.cols[3].E[row],
            };
            result.cols[col].E[row] = v4dot(lrow, right.cols[col]);
        }
    }
    return result;
}

void
m4sprint(char *msg, uint32_t msg_size, const m4 left)
{
    sprintf(msg, "{");
    uint32_t ncols = harray_count(left.cols);
    uint32_t nrows = harray_count(left.cols[0].E);

    for (uint32_t col = 0;
            col < ncols;
            ++col)
    {
        sprintf(msg+strlen(msg), "{");

        const v4 *lc = left.cols + col;
        for (uint32_t row = 0;
                row < nrows;
                ++row)
        {
            sprintf(msg+strlen(msg), "%0.4f,", lc->E[row]);
        }

        sprintf(msg+strlen(msg), "},");
    }
    sprintf(msg+strlen(msg), "}");
}

static bool
assertEqual(m4 left, m4 right, char *msg, char *file, int line)
{
    if (!m4identical(left, right))
    {
#define _P(x, ...) printf("%s(%d) : " x "\n", file, line, __VA_ARGS__)
        _P("TEST FAILED: %s", msg);
        char m4msg[10240];
        m4sprint(m4msg, sizeof(m4msg), left);
        _P("EXPECTED: %s", m4msg);
        m4sprint(m4msg, sizeof(m4msg), right);
        _P("GOT     : %s", m4msg);
#undef _P
        return false;
    }
    return true;
}

bool
m4unittests()
{
    m4 i = m4identity();
    m4 ic;
    m4copy(&ic, i);
    m4 itransposed;
    itransposed = m4transposed(i);
    T( i, ic );
    T( i, itransposed );

    ic.cols[0].E[1] = 2.0;
    itransposed = m4transposed(ic);
    T( (itransposed.cols[1].E[0]), (ic.cols[0].E[1]) );

    m4copy(&ic, i);
    m4 dic = m4mul(i, 2.0f);
    T( (dic.cols[0].E[0]), 2.0f );
    m4 dic2 = m4mul(dic, 2.0f);
    T( (dic2.cols[0].E[0]), 4.0f );
    m4 ii = m4add(i, i);
    T( ii, dic );
    m4 iiminusi = m4sub(ii, i);
    T( iiminusi, i );

    m4 m4mul1 = {};
    m4mul1.cols[0] = {1,4,2,3};
    m4mul1.cols[1] = {2,3,1,2};
    m4mul1.cols[2] = {3,2,4,1};
    m4mul1.cols[3] = {4,1,3,4};
    m4 m4mule = {};
    m4mule.cols[0] = {27,23,23,25};
    m4mule.cols[1] = {19,21,17,21};
    m4mule.cols[2] = {23,27,27,21};
    m4mule.cols[3] = {31,29,33,33};
    m4 m4mulgot = m4mul(m4mul1, m4mul1);
    T( m4mule, m4mulgot );

    m4 m4mul2 = {};
    m4mul2.cols[0] = {4,1,3,2};
    m4mul2.cols[1] = {3,2,4,3};
    m4mul2.cols[2] = {2,3,1,4};
    m4mul2.cols[3] = {1,4,2,1};

    m4 m4mule2 = {};
    m4mule2.cols[0] = {23,27,27,25};
    m4mule2.cols[1] = {21,19,23,19};
    m4mule2.cols[2] = {27,23,23,29};
    m4mule2.cols[3] = {29,31,27,27};
    m4 m4mulgot2 = m4mul(m4mul2, m4mul1);
    T( m4mule2, m4mulgot2 );

    m4 m4mul1t = m4transposed(m4mul1);

    m4 m4mul1et = {};
    m4mul1et.cols[0] = {1,2,3,4};
    m4mul1et.cols[1] = {4,3,2,1};
    m4mul1et.cols[2] = {2,1,4,3};
    m4mul1et.cols[3] = {3,2,1,4};
    T (m4mul1et, m4mul1t );

    // (FG)ᵀ = GᵀFᵀ
    m4 m4mul2t = m4transposed(m4mul2);
    m4 m4mule2t = m4transposed(m4mule2);
    m4 m4mulgot2t = m4mul(m4mul1t, m4mul2t);
    T ( m4mule2t, m4mulgot2t );

    m4 ixi = m4mul(i, i);
    T ( i, ixi );

    m4 m4mul1xi = m4mul(m4mul1, i);
    T ( m4mul1, m4mul1xi );

    return true;
}

m4
m4rotation(v3 axis, float angle)
{
    float cosa = cosf(angle);
    float sina = sinf(angle);
    axis = v3normalize(axis);

    m4 result;
    result.cols[0] = {
        cosa + ((1.0f - cosa) * axis.x * axis.x),
        (1.0f - cosa) * axis.y * axis.x - sina * axis.z,
        (1.0f - cosa) * axis.z * axis.x + sina * axis.y,
        0.0f,
    };
    result.cols[1] = {
        ((1.0f - cosa) * axis.x * axis.y) + (sina * axis.z),
        cosa + ((1.0f - cosa) * axis.y * axis.y),
        ((1.0f - cosa) * axis.z * axis.y) - (sina * axis.x),
        0.0f,
    };
    result.cols[2] = {
        (1.0f - cosa) * axis.x * axis.z - sina * axis.y,
        (1.0f - cosa) * axis.y * axis.z + sina * axis.x,
        cosa + (1.0f - cosa) * axis.z * axis.z,
        0.0f,
    };
    result.cols[3] = {0.0f, 0.0f, 0.0f, 1.0f};

    return result;
}
