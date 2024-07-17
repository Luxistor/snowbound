#define WAVE_COUNT 32
#define INCREMENT 0.1176f

float hash(float n) { return fract(sin(n) * 1e4); }

float noise(float x) {
    float i = floor(x);
    float f = fract(x);
    float u = f * f * (3.0 - 2.0 * f);
    return mix(hash(i), hash(i + 1.0), u);
}

#define GET_DIRECTION(seed) normalize( vec2(cos((seed)), sin((seed))) )

struct wave_t
{
	float seed;
	float amplitude;
	float frequency;
	vec2 direction;
};

const wave_t start_wave = wave_t(0.28f, 0.28f, 3.14f/6.0f, GET_DIRECTION(3.14f/6.0f));

wave_t next_wave(wave_t wave)
{
    wave_t ret;
	ret.amplitude = wave.amplitude*0.82;
	ret.frequency = wave.frequency*1.18;
	ret.seed = wave.seed+3.14f/6.0f;
	ret.direction = GET_DIRECTION(wave.seed);
    return ret;
}

float wave_y_value(wave_t wave, float x, float z, float time)
{
	float xz = dot(wave.direction, vec2(x,z));
	return noise(xz*wave.frequency + time)*wave.amplitude;
}

struct wave_result_t
{
    float dydx;
    float dydz;
    float y;
};

wave_result_t get_wave_data(vec4 world_position, float time)
{			
    float dydx = 0;
	float dydz = 0;

	wave_t last_wave = start_wave;

	float y = 0;

	for(int i = 0; i < WAVE_COUNT; i++)
	{
        // central difference normals approximate the partial derivatives of the noise function since it's not differentiable
		float x_forward  = wave_y_value(last_wave, world_position.x + INCREMENT, world_position.z, time);
		float x_backward = wave_y_value(last_wave, world_position.x - INCREMENT, world_position.z, time);

		float last_dydx  = (x_forward - x_backward) / (2.0*INCREMENT);

		float z_forward  = wave_y_value(last_wave, world_position.x, world_position.z + INCREMENT, time);
		float z_backward = wave_y_value(last_wave, world_position.x, world_position.z - INCREMENT, time);

		float last_dydz  = (z_forward - z_backward) / (2.0*INCREMENT);

		wave_t wave = next_wave(last_wave);

        // we apply the last waves partial derivatives to give the illusion the waves "push" eachother around
		y += wave_y_value(wave, world_position.x+last_dydx, world_position.z+last_dydz, time);

		dydx += last_dydx;
		dydz += last_dydz;

		last_wave = wave;
	}

    return wave_result_t(dydx, dydz, y);
}


