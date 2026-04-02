#ifndef __VEC_H_DEFINED
#define __VEC_H_DEFINED

typedef union {
    struct { float x, y; };
    float c[2];
} Vec2f;

typedef union {
    struct { float x, y, z; };
    struct { Vec2f xy; float __pad1; };
    struct { float __pad2; Vec2f yz; };
    struct { float u, v, w; };
    float c[3];
} Vec3f;

typedef union {
    struct { float x, y, z, w; };
    struct { Vec2f xy; Vec2f zw; };
    struct { float __pad1; Vec2f yz; float __pad2; };
    struct { Vec3f xyz; float __pad3; };
    struct { float __pad4; Vec3f yzw; };
    float c[4];
} Vec4f;

typedef union {
    struct { int x, y; };
    int c[2];
} Vec2i;

typedef union {
    struct { int x, y, z; };
    struct { Vec2f xy; int __pad1; };
    struct { int __pad2; Vec2f yz; };
    struct { int u, v, w; };
    int c[3];
} Vec3i;

typedef union {
    struct { int x, y, z, w; };
    struct { Vec2f xy; Vec2f zw; };
    struct { int __pad1; Vec2f yz; int __pad2; };
    struct { Vec3f xyz; int __pad3; };
    struct { int __pad4; Vec3f yzw; };
    int c[4];
} Vec4i;

typedef union {
    struct { float x0, x1, x2, x3; float __pad1[4*3]; };
    struct { float __pad2[4*1]; float y0, y1, y2, y3; float __pa3[4*2]; };
    struct { float __pad4[4*2]; float z0, z1, z2, z3; float __pad5[4*1]; };
    struct { float __pad6[4*3]; float w0, w1, w2, w3; };
    float c[4*4];
} Mat4f;

#endif // __VEC_H_DEFINED