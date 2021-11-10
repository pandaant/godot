#ifndef WORLEY_NOISE_H
#define WORLEY_NOISE_H

#include "core/image.h"
#include "core/math/geometry.h"
#include "core/math/random_pcg.h"
#include "core/reference.h"
#include "scene/resources/texture.h"
#include <algorithm>

#include "thirdparty/misc/open-simplex-noise.h"

class WorleyNoise : public Resource {
	GDCLASS(WorleyNoise, Resource);
	OBJ_SAVE_TYPE(WorleyNoise);

	static const int MAX_SAMPLED_POINTS = 2025;

	struct wn_context {
		RandomPCG randbase;
		Vector2 sampled_points[MAX_SAMPLED_POINTS];
	};

	wn_context context;

	int seed;
	int points; // number of points to place

	float persistence; // Controls details, value in [0,1]. Higher increases grain, lower increases smoothness.
	int octaves; // Number of noise layers
	float period; // Distance above which we start to see similarities. The higher, the longer "hills" will be on a terrain.
	bool inverted;
	int cellsPerAxis;

	Vector2 offsets[9] = {
		Vector2(-1, -1),
		Vector2(0, -1),
		Vector2(1, -1),
		Vector2(-1, 0),
		Vector2(0, 0),
		Vector2(1, 0),
		Vector2(-1, 1),
		Vector2(0, 1),
		Vector2(1, 1),
	};

public:
	WorleyNoise();
	~WorleyNoise();

	void _init_seeds();
	void _init_points();

	void set_points(int points);
	int get_points() const;

	void set_seed(int seed);
	int get_seed() const;

	void set_period(float p_period);
	float get_period() const { return period; }

	void set_persistence(float p_persistence);
	float get_persistence() const { return persistence; }

	void set_inverted(bool p_inverted);
	bool get_inverted() const;

	void set_cells_per_axis(int p_cells);
	int get_cells_per_axis() const;

	Ref<Image> get_image(int p_width, int p_height) const;
	Ref<Image> get_seamless_image(int p_size) const;

	float get_min_distance(float x, float y, bool seamless = false) const;

	void set_point_list(Vector<Vector2> points);

	float get_noise_1d(float x) const;
	float get_noise_2d(float x, float y, bool seamless = false) const;
	float get_noise_3d(float x, float y, float z) const;
	float get_noise_4d(float x, float y, float z, float w) const;

	// Convenience

	_FORCE_INLINE_ float get_noise_2dv(const Vector2 &v) const { return get_noise_2d(v.x, v.y); }
	_FORCE_INLINE_ float get_noise_3dv(const Vector3 &v) const { return get_noise_3d(v.x, v.y, v.z); }

protected:
	static void _bind_methods();
};

#endif // WORLEY_NOISE_H
