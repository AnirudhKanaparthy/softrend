#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "vec.h"

#define RGFW_IMPLEMENTATION
#include "RGFW.h"

#define ARRAY_LEN(arr) (sizeof(arr)/sizeof(*(arr)))
#define CONST_PI 3.14159265359f
#define RAD(degrees) ((degrees)*(CONST_PI/180.0f))
#define DEG(radians) ((radians)*(180.0f/CONST_PI))

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} RGBA;

bool output_ppm(const char *filepath, const RGBA *image, int width, int height) {
    FILE *fp = fopen(filepath, "wb");
    if (!fp) return false;

    // Netpbm header
    fprintf(fp, "P6\n");
    fprintf(fp, "%d %d\n", width, height);
    fprintf(fp, "255\n");

    // Netpbm raster
    for (int i = 0; i < width * height; ++i) {
        fputc(image[i].r, fp);
        fputc(image[i].g, fp);
        fputc(image[i].b, fp);
    }
    return true;
}

inline bool baryi(Vec2i a, Vec2i b, Vec2i c, Vec2i p, Vec3f *uvw) {
    float det = a.x * (b.y - c.y) - b.x * (a.y - c.y) + c.x * (a.y - b.y);
    if (det == 0)
        return false;

    uvw->u = p.x * (b.y - c.y) - b.x * (p.y - c.y) + c.x * (p.y - b.y);
    uvw->v = a.x * (p.y - c.y) - p.x * (a.y - c.y) + c.x * (a.y - p.y);

    uvw->u /= det;
    uvw->v /= det;

    uvw->w = 1.0f - uvw->u - uvw->v;

    return (
        uvw->u >= 0.0f && uvw->u <= 1.0f &&
        uvw->v >= 0.0f && uvw->v <= 1.0f &&
        uvw->w >= 0.0f && uvw->w <= 1.0f);
}

bool render_clear(RGBA *image, int width, int height, RGBA color) {
    // TODO: Validation
    for (int i = 0; i < width * height; ++i) {
        image[i] = color;
    }
}

bool render_rect(RGBA *image, int width, int height, Vec2i pos, Vec2i dim, RGBA color) {
    // TODO: Validation
    for (int j = pos.y; j < pos.y + dim.y; ++j) {
        for (int i = pos.x; i < pos.x + dim.x; ++i) {
            image[j * width + i] = color;
        }
    }
    return true;
}

bool render_tri(RGBA *image, float* zbuf, int width, int height,
    Vec2i a, Vec2i b, Vec2i c,
    float za, float zb, float zc,
    RGBA color
) {
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            Vec3f uvw;
            if (!baryi(a, b, c, (Vec2i){i, j}, &uvw))
                continue;

            // Update depth info and update color accordingly
            float z = uvw.u * za + uvw.v * zb + uvw.w * zc;
            int idx = j * width + i;
            if(z > zbuf[idx]) {
                zbuf[idx] = z;
                image[idx] = color;
            }
        }
    }
}

inline Vec4f vec4f_zero() {
    Vec4f v;
    memset(v.c, 0, sizeof(v.c));
    return v;
}

inline Mat4f mat4f_zero() {
    Mat4f mat;
    memset(mat.c, 0, sizeof(mat.c));
    return mat;
}

inline Mat4f mat4f_indentity() {
    Mat4f mat = mat4f_zero();
    mat.x0 = 1.0f;
    mat.y1 = 1.0f;
    mat.z2 = 1.0f;
    mat.w3 = 1.0f;
    return mat;
}

Vec4f mat4f_mulv(Mat4f mat, Vec4f vec) {
    Vec4f res = vec4f_zero();
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            res.c[i] += mat.c[i*4+j] * vec.c[j];
        }
    }
    return res;
}

Mat4f mat4f_mulm(Mat4f a, Mat4f b) {
    Mat4f res = mat4f_zero();
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            for(int k = 0; k < 4; ++k) {
                res.c[i*4+j] += a.c[i*4+k] * b.c[k*4+j];
            }
        }
    }
    return res;
}

inline Mat4f mat4f_translate(Vec3f vec) {
    Mat4f mat = mat4f_indentity();
    mat.x3 = vec.x;
    mat.y3 = vec.y;
    mat.z3 = vec.z;
    return mat;
}

inline Mat4f mat4f_yaw(float angle) {
    Mat4f mat = mat4f_indentity();
    
    float c = cosf(angle);
    float s = sinf(angle);

    mat.y1 =  c;
    mat.y2 = -s;
    mat.z1 =  s;
    mat.z2 =  c;

    return mat;
}

inline Mat4f mat4f_pitch(float angle) {
    Mat4f mat = mat4f_indentity();
    
    float c = cosf(angle);
    float s = sinf(angle);

    mat.x0 =  c;
    mat.x2 =  s;
    mat.z0 = -s;
    mat.z2 =  c;

    return mat;
}

inline Mat4f mat4f_roll(float angle) {
    Mat4f mat = mat4f_indentity();
    
    float c = cosf(angle);
    float s = sinf(angle);

    mat.x0 =  c;
    mat.x1 = -s;
    mat.y0 =  s;
    mat.y1 =  c;

    return mat;
}

inline Mat4f mat4f_rotate(Vec3f ypr) {
    Mat4f yaw   = mat4f_yaw(ypr.x);
    Mat4f pitch = mat4f_pitch(ypr.y);
    Mat4f roll  = mat4f_roll(ypr.z);
    return mat4f_mulm(roll, mat4f_mulm(pitch, yaw));
}

void mat4f_print(Mat4f mat) {
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            int idx = i*4+j;
            printf("%s%.3f ", mat.c[idx] >= 0.0f ? " " : "", mat.c[idx]);
        }
        printf("\n");
    }
}

void mat4f_printfu(Mat4f mat) {
    for(int i = 0; i < 4; ++i) {
        if(i == 0)
            printf("⎡");
        else if(i == 3)
            printf("⎣");
        else
            printf("⎜");

        for(int j = 0; j < 4; ++j) {
            int idx = i*4+j;
            printf("%s%.3f ", mat.c[idx] >= 0.0f ? " " : "", mat.c[idx]);
        }

        if(i == 0)
            printf("⎤\n");
        else if(i == 3)
            printf("⎦\n");
        else
            printf("⎥\n");
    }
}

void vec4f_print(Vec4f v) {
    for(int i = 0; i < 4; ++i) {
        printf("%s%.3f\n", v.c[i] >= 0.0f ? " " : "", v.c[i]);
    }
}

void vec3f_print(Vec3f v) {
    for(int i = 0; i < 3; ++i) {
        printf("%s%.3f\n", v.c[i] >= 0.0f ? " " : "", v.c[i]);
    }
}

void vec2f_print(Vec2f v) {
    for(int i = 0; i < 2; ++i) {
        printf("%s%.3f\n", v.c[i] >= 0.0f ? " " : "", v.c[i]);
    }
}

inline Vec4f vec4f_vec3f(Vec3f v, float w) {
    Vec4f res = (Vec4f){.xyz=v};
    res.w = w;
    return res;
}

inline Vec2i norm_to_screen(int width, int height, Vec2f xy) {
    float cx = (xy.x + 1.0f) / 2.0f;
    float cy = (xy.y + 1.0f) / 2.0f;
    return (Vec2i){
        .x=width*cx,
        .y=height*(1.0f-cy),
    };
}

inline Vec2f project(Vec3f v) {
    float z = v.z;
    return (Vec2f){
        .x=v.x/z,
        .y=v.y/z,
    };
}

void rgfw_keyfunc(const RGFW_event* event) {
    if (event->key.value == RGFW_keyEscape) {
        RGFW_window_setShouldClose(event->common.win, 1);
    }
}

int main() {
    int width = 1024;
    int height = 1024;
    int target_fps = 120;

    RGBA background = (RGBA){24, 2, 12, 255};

    RGBA* image = malloc(width * height * sizeof(RGBA));

    // Coordinates are normalized local coordinates
    Vec3f vertices[] = {
        // Front (z = +0.5)
        (Vec3f){-0.5f, -0.5f,  0.5f},
        (Vec3f){-0.5f,  0.5f,  0.5f},
        (Vec3f){ 0.5f, -0.5f,  0.5f},
        (Vec3f){ 0.5f,  0.5f,  0.5f},
        (Vec3f){-0.5f,  0.5f,  0.5f},
        (Vec3f){ 0.5f, -0.5f,  0.5f},

        // Back (z = -0.5)
        (Vec3f){ 0.5f, -0.5f, -0.5f},
        (Vec3f){ 0.5f,  0.5f, -0.5f},
        (Vec3f){-0.5f, -0.5f, -0.5f},
        (Vec3f){-0.5f,  0.5f, -0.5f},
        (Vec3f){ 0.5f,  0.5f, -0.5f},
        (Vec3f){-0.5f, -0.5f, -0.5f},

        // Left (x = -0.5)
        (Vec3f){-0.5f, -0.5f, -0.5f},
        (Vec3f){-0.5f,  0.5f, -0.5f},
        (Vec3f){-0.5f, -0.5f,  0.5f},
        (Vec3f){-0.5f,  0.5f,  0.5f},
        (Vec3f){-0.5f,  0.5f, -0.5f},
        (Vec3f){-0.5f, -0.5f,  0.5f},

        // Right (x = +0.5)
        (Vec3f){ 0.5f, -0.5f,  0.5f},
        (Vec3f){ 0.5f,  0.5f,  0.5f},
        (Vec3f){ 0.5f, -0.5f, -0.5f},
        (Vec3f){ 0.5f,  0.5f, -0.5f},
        (Vec3f){ 0.5f,  0.5f,  0.5f},
        (Vec3f){ 0.5f, -0.5f, -0.5f},

        // Top (y = +0.5)
        (Vec3f){-0.5f,  0.5f,  0.5f},
        (Vec3f){-0.5f,  0.5f, -0.5f},
        (Vec3f){ 0.5f,  0.5f,  0.5f},
        (Vec3f){ 0.5f,  0.5f, -0.5f},
        (Vec3f){-0.5f,  0.5f, -0.5f},
        (Vec3f){ 0.5f,  0.5f,  0.5f},

        // Bottom (y = -0.5)
        (Vec3f){-0.5f, -0.5f, -0.5f},
        (Vec3f){-0.5f, -0.5f,  0.5f},
        (Vec3f){ 0.5f, -0.5f, -0.5f},
        (Vec3f){ 0.5f, -0.5f,  0.5f},
        (Vec3f){-0.5f, -0.5f,  0.5f},
        (Vec3f){ 0.5f, -0.5f, -0.5f},
    };

    RGBA colors[] = {
        (RGBA){229, 255, 222, 255},  // Front
        (RGBA){187, 203, 203, 255},  // Back
        (RGBA){149, 144, 168, 255},  // Left
        (RGBA){99,  75,  102, 255},  // Right
        (RGBA){212, 170, 170, 255},  // Top
        (RGBA){104, 125, 125, 255},  // Bottom
    };

    RGFW_window *win = RGFW_createWindow(
        "Software Renderer",
        0, 0, width, height,
        RGFW_windowCenter | RGFW_windowNoResize
    );
    RGFW_surface* surface = RGFW_createSurface((unsigned char*)image, width, height, RGFW_formatRGBA8);

    RGFW_setEventCallback(RGFW_keyPressed, rgfw_keyfunc);

    float* zbuf = malloc(width*height*sizeof(float));
    float del = 0.0f;

    Vec3f rotated_vertices[ARRAY_LEN(vertices)];
    Vec3f translated_vertices[ARRAY_LEN(vertices)];
    Vec2f projected_vertices[ARRAY_LEN(vertices)];
    Vec2i screen_vertices[ARRAY_LEN(vertices)];
    
    clock_t prev_time = clock();
    while (RGFW_window_shouldClose(win) == RGFW_FALSE) {
        RGFW_pollEvents();

        render_clear(image, width, height, background);

        // Rotate
        Mat4f rotate = mat4f_rotate((Vec3f){.x=RAD(del), .y=RAD(del), .z=RAD(del)});
        del += 1.0f;
        for(int i = 0; i < ARRAY_LEN(vertices); ++i) {
            Vec4f vec = vec4f_vec3f(vertices[i], 1.0f);
            rotated_vertices[i] = mat4f_mulv(rotate, vec).xyz;
        }

        // Translate
        Mat4f translate = mat4f_translate((Vec3f){.x=0.0f, .y=0.0f, .z=-3.0f});
        for(int i = 0; i < ARRAY_LEN(vertices); ++i) {
            Vec4f vec = vec4f_vec3f(rotated_vertices[i], 1.0f);
            translated_vertices[i] = mat4f_mulv(translate, vec).xyz;
        }
        
        // Project
        for(int i = 0; i < ARRAY_LEN(vertices); ++i) {
            projected_vertices[i] = project(translated_vertices[i]);
        }

        // Screen Coordinates
        for(int i = 0; i < ARRAY_LEN(vertices); ++i) {
            screen_vertices[i] = norm_to_screen(width, height, projected_vertices[i]);
        }

        for(int i = 0; i < width*height; ++i) { zbuf[i] = -9999.9f; }

        // Render
        render_clear(image, width, height, background);
        for(int i = 0; i < (ARRAY_LEN(vertices)/3); ++i) {
            Vec2i a = screen_vertices[i*3+0];
            Vec2i b = screen_vertices[i*3+1];
            Vec2i c = screen_vertices[i*3+2];
            float za = translated_vertices[i*3+0].z;
            float zb = translated_vertices[i*3+1].z;
            float zc = translated_vertices[i*3+2].z;
            render_tri(image, zbuf, width, height, a, b, c, za, zb, zc, colors[i/2]);
        }
        RGFW_window_blitSurface(win, surface);

        clock_t now = clock();
        double delta_time = now - prev_time;
        prev_time = now;

        int to_sleep = 1000000.0/target_fps - (delta_time*1000000 / CLOCKS_PER_SEC);
        if(to_sleep > 0) { usleep(to_sleep); }
    }
    RGFW_window_close(win);

    return 0;
}
