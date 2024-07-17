#ifndef SB_MATH_H
#define SB_MATH_H

#include <stdint.h>
#include <stdbool.h>

#define SB_PI 3.1415927F
#define SB_MAT4_IDENTITY_INIT  {{1.0f, 0.0f, 0.0f, 0.0f},                    \
                                 {0.0f, 1.0f, 0.0f, 0.0f},                    \
                                 {0.0f, 0.0f, 1.0f, 0.0f},                    \
                                 {0.0f, 0.0f, 0.0f, 1.0f}}

#define KB(x) ((size_t) (x) << 10)
#define MB(x) ((size_t) (x) << 20)
#define GB(x) ((size_t) (x) << 30)

typedef struct
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} sb_color3;

uint32_t sb_color3_as_u32(sb_color3 color);
sb_color3 sb_color3_lerp(sb_color3 a, sb_color3 b, float t);

#define SB_BLUE (sb_color3) {0,0,255}
#define SB_RED (sb_color3) {255,0,0}
#define SB_LIGHT_BLUE (sb_color3) {0, 195, 255}
#define SB_WHITE (sb_color3) {255,255,255}
#define SB_BLACK (sb_color3) {0,0,0}
#define SB_GREEN (sb_color3) {0,255,0}
#define SB_DARK_GREEN (sb_color3) {46, 139, 58}
#define SB_LIME (sb_color3) {191, 214, 65}
#define SB_YELLOW (sb_color3) {255,239,123}
#define SB_PURPLE (sb_color3) {151, 3, 175}
#define SB_BROWN (sb_color3) {141, 111, 100}
#define SB_ROSE (sb_color3) {239, 195, 202}
#define SB_ORANGE (sb_color3) {254, 153, 0}

float sb_lerp(float a, float b, float t);
int sb_random_int(int min, int max);
float sb_random_float(void);
uintptr_t sb_align_forward_power_of_two(uintptr_t ptr, size_t align);
float sb_clamp(float d, float min, float max);
float sb_rad(float deg);
bool sb_is_power_of_two(size_t n);
size_t sb_round_up(size_t n, size_t mult);

typedef struct
{
    float x, y, z;
} sb_vec3;

sb_vec3 sb_vec3_mul_f32(sb_vec3 vec3, float n);
sb_vec3 sb_vec3_add(sb_vec3 lhs, sb_vec3 rhs);
sb_vec3 sb_vec3_sub(sb_vec3 lhs, sb_vec3 rhs);
float sb_vec3_dot(sb_vec3 lhs, sb_vec3 rhs);
sb_vec3 sb_vec3_cross(sb_vec3 lhs, sb_vec3 rhs);
float sb_vec3_magnitude(sb_vec3 in);
sb_vec3 sb_vec3_normalize(sb_vec3 in);
sb_vec3 sb_vec3_lerp(sb_vec3 a, sb_vec3 b, float t);
void sb_vec3_print(sb_vec3 in);

typedef struct
{
    int32_t x,y;
} sb_ivec2;

sb_ivec2 sb_ivec2_add(sb_ivec2 lhs, sb_ivec2 rhs);
sb_ivec2 sb_ivec2_sub(sb_ivec2 lhs, sb_ivec2 rhs);
sb_ivec2 sb_ivec2_mul_ivec2(sb_ivec2 lhs, sb_ivec2 rhs);
float sb_ivec2_magnitude(sb_ivec2 in);
bool sb_ivec2_eq(sb_ivec2 lhs, sb_ivec2 rhs);

typedef struct
{
    float x, y;
} sb_vec2;

sb_vec2 sb_vec2_lerp(sb_vec2 a, sb_vec2 b, float t);

typedef struct
{
    float x, y, z, w;
} sb_vec4;

sb_vec4 sb_make_vec3_vec4(sb_vec3 in, float n);
sb_vec4 sb_vec4_mul_f32(sb_vec4 vec4,float n);
float sb_vec4_magnitude(sb_vec4 in);
sb_vec4 sb_vec4_normalize(sb_vec4 in);

typedef float sb_mat4[4][4];

void sb_mat4_mul_mat4(sb_mat4 lhs, sb_mat4 rhs, sb_mat4 out);
sb_vec4 sb_mat4_mul_vec4(sb_mat4 lhs, sb_vec4 rhs);
void sb_mat4_print(sb_mat4 in);
void sb_mat4_identity(sb_mat4 out);
void sb_mat4_translate(sb_mat4 lhs, sb_vec3 rhs);
void sb_mat4_from_position(sb_mat4 out, sb_vec3 position);
void sb_mat4_set_position(sb_mat4 out, sb_vec3 position);
void sb_mat4_from_angles(sb_mat4 out, sb_vec3 angles);
void sb_mat4_get_rotation_matrix(sb_mat4 in, sb_mat4 out);
void sb_mat4_inv(sb_mat4 in, sb_mat4 out);
void sb_mat4_scale_by_vec3(sb_mat4 in, sb_vec3 scale);
void sb_mat4_scale(sb_mat4 in, float scale);
void sb_mat4_scale_p(sb_mat4 in, float scale) ;
sb_vec3 sb_mat4_get_angles(sb_mat4 in);
sb_vec3 sb_mat4_get_position(sb_mat4 in);
void sb_mat4_transpose_to(sb_mat4 in, sb_mat4 out);
void sb_mat4_transpose(sb_mat4 in);
void sb_mat4_copy(sb_mat4 src, sb_mat4 dst);
void sb_mat4_look_at(sb_vec3 eye, sb_vec3 center, sb_vec3 up, sb_mat4 out);
void sb_mat4_projection_ortho(sb_mat4 out, float left, float right, float top, float bottom, float near, float far);
void sb_mat4_projection_perpective(sb_mat4 out, float aspect_ratio, float fov, float near, float far);

#endif
