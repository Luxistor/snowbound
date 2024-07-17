#include <memory.h>
#include <math.h>
#include <stdlib.h>

#include "sb_math.h"
#include "sb_common.h"

uint32_t sb_color3_as_u32(sb_color3 color)
{
  return (uint32_t) color.r << 16 | (uint32_t)  color.g << 8 | (uint32_t)  color.b;
}

sb_color3 sb_color3_lerp(sb_color3 a, sb_color3 b, float t)
{
  return (sb_color3) {
		.r = sb_lerp(a.r, b.r, t),
		.g = sb_lerp(a.g, b.g, t),
		.b = sb_lerp(a.b, b.b, t),
	};
}

float sb_lerp(float a, float b, float t)
{
	return a + (b-a)*t;
}

int sb_random_int(int min, int max)
{
   return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

// returns random float [-1,1]
float sb_random_float(void)
{
    return -1+2*( (float) rand() )/RAND_MAX;
}

size_t sb_round_up(size_t n, size_t mult)
{
    size_t remainder = n % mult;
    if (remainder == 0)
        return n;
    return n + mult - remainder;
}

float sb_clamp(float d, float min, float max)
{
  const float t = d < min ? min : d;
  return t > max ? max : t;
}

float sb_rad(float deg)
{
	return deg  * SB_PI / 180.0f;
}

bool sb_is_power_of_two(size_t n)
{
	return ((n & (n - 1)) == 0);
}

uintptr_t sb_align_forward_power_of_two(uintptr_t ptr, size_t align)
{
	size_t modulo = ptr & (align - 1);
	if (modulo != 0)
		return ptr + align - modulo;
	return ptr;
}


sb_vec2 sb_vec2_lerp(sb_vec2 a, sb_vec2 b, float t)
{
	return (sb_vec2) {
		.x = sb_lerp(a.x, b.x, t),
		.y = sb_lerp(a.y, b.y, t),
	};
}

sb_ivec2 sb_ivec2_sub(sb_ivec2 lhs, sb_ivec2 rhs)
{
  return (sb_ivec2) {
		.x = lhs.x - rhs.x,
		.y = lhs.y - rhs.y,
	};
}

sb_ivec2 sb_ivec2_mul_ivec2(sb_ivec2 lhs, sb_ivec2 rhs)
{
  return (sb_ivec2) {
    .x = lhs.x*rhs.y,
    .y = lhs.y*rhs.y
  };
}

float sb_ivec2_magnitude(sb_ivec2 in)
{
  return sqrtf(pow(in.x, 2) + pow(in.y, 2));
}

sb_ivec2 sb_ivec2_add(sb_ivec2 lhs, sb_ivec2 rhs)
{
	return (sb_ivec2) {
		.x = lhs.x + rhs.x,
		.y = lhs.y + rhs.y,
	};
}

bool sb_ivec2_eq(sb_ivec2 lhs, sb_ivec2 rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y;
}


sb_vec3 sb_vec3_mul_f32(sb_vec3 vec3, float n)
{
	return (sb_vec3) {
		.x = vec3.x * n,
		.y = vec3.y * n,
		.z = vec3.z * n
	};
}

sb_vec3 sb_vec3_add(sb_vec3 lhs, sb_vec3 rhs)
{
	return (sb_vec3) {
		.x = lhs.x + rhs.x,
		.y = lhs.y + rhs.y,
		.z = lhs.z + rhs.z,
	};
}

sb_vec3 sb_vec3_sub(sb_vec3 lhs, sb_vec3 rhs)
{
  return (sb_vec3){
    .x = lhs.x - rhs.x,
    .y = lhs.y - rhs.y,
    .z = lhs.z - rhs.z
  };
}

float sb_vec3_dot(sb_vec3 lhs, sb_vec3 rhs)
{
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

sb_vec3 sb_vec3_cross(sb_vec3 lhs, sb_vec3 rhs)
{
  return (sb_vec3) { 
      .x = lhs.y * rhs.z - lhs.z * rhs.y,
      .y = lhs.z * rhs.x - lhs.x * rhs.z,
      .z = lhs.x * rhs.y - lhs.y * rhs.x 
    };
}

float sb_vec3_magnitude(sb_vec3 in)
{
    return sqrtf(powf(in.x, 2) + powf(in.y, 2) + powf(in.z, 2));
}

sb_vec3 sb_vec3_normalize(sb_vec3 in)
{
  return sb_vec3_mul_f32(in, 1.0/sb_vec3_magnitude(in));
}

sb_vec3 sb_vec3_lerp(sb_vec3 a, sb_vec3 b, float t)
{
	return (sb_vec3) {
		.x = sb_lerp(a.x, b.x, t),
		.y = sb_lerp(a.y, b.y, t),
		.z = sb_lerp(a.z, b.z, t),
	};
}

void sb_vec3_print(sb_vec3 in)
{
	printf("VEC3:\n{x: %f, y: %f, z:%f}\n", in.x, in.y, in.z);
}

sb_vec4 sb_make_vec3_vec4(sb_vec3 in, float n)
{
  return (sb_vec4) {in.x, in.y, in.z, n};
}


void sb_mat4_mul_mat4(sb_mat4 lhs, sb_mat4 rhs, sb_mat4 out)
{
  float a00 = lhs[0][0], a01 = lhs[0][1], a02 = lhs[0][2], a03 = lhs[0][3],
        a10 = lhs[1][0], a11 = lhs[1][1], a12 = lhs[1][2], a13 = lhs[1][3],
        a20 = lhs[2][0], a21 = lhs[2][1], a22 = lhs[2][2], a23 = lhs[2][3],
        a30 = lhs[3][0], a31 = lhs[3][1], a32 = lhs[3][2], a33 = lhs[3][3],

        b00 = rhs[0][0], b01 = rhs[0][1], b02 = rhs[0][2], b03 = rhs[0][3],
        b10 = rhs[1][0], b11 = rhs[1][1], b12 = rhs[1][2], b13 = rhs[1][3],
        b20 = rhs[2][0], b21 = rhs[2][1], b22 = rhs[2][2], b23 = rhs[2][3],
        b30 = rhs[3][0], b31 = rhs[3][1], b32 = rhs[3][2], b33 = rhs[3][3];

  out[0][0] = a00 * b00 + a10 * b01 + a20 * b02 + a30 * b03;
  out[0][1] = a01 * b00 + a11 * b01 + a21 * b02 + a31 * b03;
  out[0][2] = a02 * b00 + a12 * b01 + a22 * b02 + a32 * b03;
  out[0][3] = a03 * b00 + a13 * b01 + a23 * b02 + a33 * b03;
  out[1][0] = a00 * b10 + a10 * b11 + a20 * b12 + a30 * b13;
  out[1][1] = a01 * b10 + a11 * b11 + a21 * b12 + a31 * b13;
  out[1][2] = a02 * b10 + a12 * b11 + a22 * b12 + a32 * b13;
  out[1][3] = a03 * b10 + a13 * b11 + a23 * b12 + a33 * b13;
  out[2][0] = a00 * b20 + a10 * b21 + a20 * b22 + a30 * b23;
  out[2][1] = a01 * b20 + a11 * b21 + a21 * b22 + a31 * b23;
  out[2][2] = a02 * b20 + a12 * b21 + a22 * b22 + a32 * b23;
  out[2][3] = a03 * b20 + a13 * b21 + a23 * b22 + a33 * b23;
  out[3][0] = a00 * b30 + a10 * b31 + a20 * b32 + a30 * b33;
  out[3][1] = a01 * b30 + a11 * b31 + a21 * b32 + a31 * b33;
  out[3][2] = a02 * b30 + a12 * b31 + a22 * b32 + a32 * b33;
  out[3][3] = a03 * b30 + a13 * b31 + a23 * b32 + a33 * b33;
};

sb_vec4 sb_vec4_mul_f32(sb_vec4 vec4,float n)
{
  	return (sb_vec4) {
      .x = vec4.x * n,
      .y = vec4.y * n,
      .z = vec4.z * n,
      .w = vec4.w * n,
	};
}

float sb_vec4_magnitude(sb_vec4 in)
{
    return sqrtf(powf(in.x, 2.0) + powf(in.y, 2.0) + powf(in.z, 2.0) + powf(in.w, 2.0));
}

sb_vec4 sb_vec4_normalize(sb_vec4 in)
{
    return sb_vec4_mul_f32(in, 1/sb_vec4_magnitude(in));
}

sb_vec4 sb_mat4_mul_vec4(sb_mat4 m, sb_vec4 v)
{
  sb_vec4 res;
  res.x = m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z + m[3][0] * v.w;
  res.y = m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1] * v.w;
  res.z = m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2] * v.w;
  res.w = m[0][3] * v.x + m[1][3] * v.y + m[2][3] * v.z + m[3][3] * v.w;
  return res;
}

void sb_mat4_print(sb_mat4 mat4)
{
	printf("MAT4:\n{{%f, %f, %f, %f},\n{%f, %f, %f, %f},\n {%f, %f, %f, %f},\n{%f, %f, %f, %f}}\n",
			mat4[0][0], mat4[0][1], mat4[0][2], mat4[0][3],
			mat4[1][0], mat4[1][1], mat4[1][2], mat4[1][3],
			mat4[2][0], mat4[2][1], mat4[2][2], mat4[2][3],
			mat4[3][0], mat4[3][1], mat4[3][2], mat4[3][3]);
}

void sb_mat4_translate(sb_mat4 lhs, sb_vec3 rhs)
{
	lhs[3][0] += rhs.x;
	lhs[3][1] += rhs.y;
	lhs[3][2] += rhs.z;
}

void sb_mat4_from_position(sb_mat4 out, sb_vec3 position)
{
	sb_mat4_identity(out);
  sb_mat4_set_position(out, position);
}

void sb_mat4_set_position(sb_mat4 out, sb_vec3 position)
{
  out[3][0] = position.x;
	out[3][1] = position.y;
	out[3][2] = position.z;
}

void sb_mat4_identity(sb_mat4 out)
{
	memset(out, 0, sizeof(float)*16);

	out[0][0] = 1;
	out[1][1] = 1;
	out[2][2] = 1;
	out[3][3] = 1;
}

// SOURCE: https://github.com/recp/cglm/blob/master/include/cglm/euler.h
void sb_mat4_from_angles(sb_mat4 out, sb_vec3 angles)
{
  float sx = sinf(angles.x);
  float sy = sinf(angles.y);
  float sz = sinf(angles.z);
  float cx = cosf(angles.x);
  float cy = cosf(angles.y);
  float cz = cosf(angles.z);

  float czsx = cz * sx;
  float cxcz = cx * cz;
  float sysz = sy * sz;

  out[0][0] =  cy * cz;
  out[0][1] =  czsx * sy + cx * sz;
  out[0][2] = -cxcz * sy + sx * sz;
  out[1][0] = -cy * sz;
  out[1][1] =  cxcz - sx * sysz;
  out[1][2] =  czsx + cx * sysz;
  out[2][0] =  sy;
  out[2][1] = -cy * sx;
  out[2][2] =  cx * cy;
  out[0][3] =  0.0f;
  out[1][3] =  0.0f;
  out[2][3] =  0.0f;
  out[3][0] =  0.0f;
  out[3][1] =  0.0f;
  out[3][2] =  0.0f;
  out[3][3] =  1.0f;
}

void sb_mat4_get_rotation_matrix(sb_mat4 in, sb_mat4 out)
{
	sb_mat4_copy(in, out);
	out[3][0] = 0;
	out[3][1] = 0;
	out[3][2] = 0;
	out[3][3] = 1;
	out[0][3] = 0;
	out[1][3] = 0;
	out[2][3] = 0;
}

// SOURCE: https://github.com/recp/cglm/blob/master/include/cglm/euler.h
sb_vec3 sb_mat4_get_angles(sb_mat4 in)
{
	if (in[2][0] < 1.0f)
	{
    	if (in[2][0] > -1.0f)
		{
			return (sb_vec3) {
				.x = atan2f(-in[2][1], in[2][2]),
				.y = asinf(in[2][0]),
				.z = atan2f(-in[1][0], in[0][0])
			};
    	}
       // not a unique solution
		return (sb_vec3) {
			.x = -atan2f(in[0][1], in[1][1]),
			.y = -SB_PI/2.0f,
			.z = 0,
		};
    }

  	// m20 == +1
	return (sb_vec3) {
		.x = atan2f(in[0][1], in[1][1]),
		.y = SB_PI/2.0f,
		.z = 0,
	};

}

sb_vec3 sb_mat4_get_position(sb_mat4 in)
{
	return (sb_vec3){
		.x = in[3][0],
		.y = in[3][1],
		.z = in[3][2]
	};
}

void sb_mat4_transpose_to(sb_mat4 in, sb_mat4 out)
{
  out[0][0] = in[0][0]; out[1][0] = in[0][1];
  out[0][1] = in[1][0]; out[1][1] = in[1][1];
  out[0][2] = in[2][0]; out[1][2] = in[2][1];
  out[0][3] = in[3][0]; out[1][3] = in[3][1];
  out[2][0] = in[0][2]; out[3][0] = in[0][3];
  out[2][1] = in[1][2]; out[3][1] = in[1][3];
  out[2][2] = in[2][2]; out[3][2] = in[2][3];
  out[2][3] = in[3][2]; out[3][3] = in[3][3];
}

void sb_mat4_transpose(sb_mat4 in)
{
	sb_mat4 transposed;
	sb_mat4_transpose_to(in, transposed);
	sb_mat4_copy(transposed, in);
}


void sb_mat4_scale(sb_mat4 in, float scale)
{
  in[0][0] *= scale; in[0][1] *= scale; in[0][2] *= scale; in[0][3] *= scale;
  in[1][0] *= scale; in[1][1] *= scale; in[1][2] *= scale; in[1][3] *= scale;
  in[2][0] *= scale; in[2][1] *= scale; in[2][2] *= scale; in[2][3] *= scale;
}

void sb_mat4_scale_p(sb_mat4 in, float scale)
{
  sb_mat4_scale(in, scale);
  in[3][0] *= scale; in[3][1] *= scale; in[3][2] *= scale; in[3][3] *= scale;
}


// SOURCE: https://github.com/recp/cglm/blob/master/include/cglm/mat4.h
void sb_mat4_inv(sb_mat4 in, sb_mat4 out)
{
  float t[6];
  float det;
  float a = in[0][0], b = in[0][1], c = in[0][2], d = in[0][3],
        e = in[1][0], f = in[1][1], g = in[1][2], h = in[1][3],
        i = in[2][0], j = in[2][1], k = in[2][2], l = in[2][3],
        m = in[3][0], n = in[3][1], o = in[3][2], p = in[3][3];

  t[0] = k * p - o * l; t[1] = j * p - n * l; t[2] = j * o - n * k;
  t[3] = i * p - m * l; t[4] = i * o - m * k; t[5] = i * n - m * j;

  out[0][0] =  f * t[0] - g * t[1] + h * t[2];
  out[1][0] =-(e * t[0] - g * t[3] + h * t[4]);
  out[2][0] =  e * t[1] - f * t[3] + h * t[5];
  out[3][0] =-(e * t[2] - f * t[4] + g * t[5]);

  out[0][1] =-(b * t[0] - c * t[1] + d * t[2]);
  out[1][1] =  a * t[0] - c * t[3] + d * t[4];
  out[2][1] =-(a * t[1] - b * t[3] + d * t[5]);
  out[3][1] =  a * t[2] - b * t[4] + c * t[5];

  t[0] = g * p - o * h; t[1] = f * p - n * h; t[2] = f * o - n * g;
  t[3] = e * p - m * h; t[4] = e * o - m * g; t[5] = e * n - m * f;

  out[0][2] =  b * t[0] - c * t[1] + d * t[2];
  out[1][2] =-(a * t[0] - c * t[3] + d * t[4]);
  out[2][2] =  a * t[1] - b * t[3] + d * t[5];
  out[3][2] =-(a * t[2] - b * t[4] + c * t[5]);

  t[0] = g * l - k * h; t[1] = f * l - j * h; t[2] = f * k - j * g;
  t[3] = e * l - i * h; t[4] = e * k - i * g; t[5] = e * j - i * f;

  out[0][3] =-(b * t[0] - c * t[1] + d * t[2]);
  out[1][3] =  a * t[0] - c * t[3] + d * t[4];
  out[2][3] =-(a * t[1] - b * t[3] + d * t[5]);
  out[3][3] =  a * t[2] - b * t[4] + c * t[5];

  det = 1.0f / (a * out[0][0] + b * out[1][0]
              + c * out[2][0] + d * out[3][0]);

  sb_mat4_scale_p(out, det);
}

void sb_mat4_scale_by_vec3(sb_mat4 in, sb_vec3 scale)
{
    in[0][0] *= scale.x; in[0][1] *= scale.x; in[0][2] *= scale.x; in[0][3] *= scale.x;
    in[1][0] *= scale.y; in[1][1] *= scale.y; in[1][2] *= scale.y; in[1][3] *= scale.y;
    in[2][0] *= scale.z; in[2][1] *= scale.z; in[2][2] *= scale.z; in[2][3] *= scale.z;
}

void sb_mat4_copy(sb_mat4 src, sb_mat4 dst)
{
	memcpy(dst, src, sizeof(sb_mat4));
}


void sb_mat4_look_at(sb_vec3 eye, sb_vec3 center, sb_vec3 up, sb_mat4 out) {

  sb_vec3 w = sb_vec3_sub(center, eye);
  w = sb_vec3_normalize(w);

  sb_vec3 u = sb_vec3_cross(w, up);
  u = sb_vec3_normalize(u);

  sb_vec3 v = sb_vec3_cross(w, u);

  sb_mat4_identity(out);

  out[0][0] = u.x;
  out[1][0] = u.y;
  out[2][0] = u.z;
  out[0][1] = v.x;
  out[1][1] = v.y;
  out[2][1] = v.z;
  out[0][2] = w.x;
  out[1][2] = w.y;
  out[2][2] = w.z;
  out[3][0] =-sb_vec3_dot(u, eye);
  out[3][1] =-sb_vec3_dot(v, eye);
  out[3][2] =-sb_vec3_dot(w, eye);
  
}

void sb_mat4_projection_ortho(sb_mat4 out, float left, float right, float top, float bottom, float near, float far)
{
  SB_ZERO_ARRAY(out);

  float right_minus_left = right-left;
  float bottom_minus_top = bottom-top;
  float far_minus_near = far-near;

  out[0][0] = 2.0/(right_minus_left);
  out[1][1] = 2.0/(bottom_minus_top);
  out[2][2] = 1.0/(far_minus_near);

  out[3][0] = -(right+left)/(right_minus_left);
  out[3][1] = -(bottom+top)/(bottom_minus_top);
  out[3][2] = -(near)/(far_minus_near);
  out[3][3] = 1;
}

void sb_mat4_projection_perpective(sb_mat4 out, float aspect_ratio, float fov, float near, float far)
{
    SB_ZERO_ARRAY(out);

    float tan_half_fov = tanf(fov/2.0f);

    out[0][0] = 1.0f/(aspect_ratio * tan_half_fov);
    out[1][1] = 1.0f/(tan_half_fov);
    out[2][2] = far/(far-near);
    out[2][3] = 1.0f;
    out[3][2] = -(far*near) / (far - near);
}