#include "worley_noise.h"

#include "core/core_string_names.h"

WorleyNoise::WorleyNoise() {

	seed = 0;
	points = 25;
	persistence = 1;
	octaves = 3;
	period = 64;
	inverted = true;
	cellsPerAxis = 8;

	_init_seeds();
}

WorleyNoise::~WorleyNoise() {
}

void WorleyNoise::_init_seeds() {
	context.randbase = RandomPCG(seed);
	_init_points();
}

void WorleyNoise::_init_points() {
	float cell_size = 1.0 / cellsPerAxis;
	for (int x = 0; x < cellsPerAxis; ++x) {
		for (int y = 0; y < cellsPerAxis; ++y) {
			Vector2 randomOffset = Vector2(context.randbase.randf(), context.randbase.randf());
			Vector2 position = (Vector2(x, y) + randomOffset) * cell_size;
			int index = x + cellsPerAxis * y;
			context.sampled_points[index] = position;
			// print_line(vformat("idx: %d, pos: %f, %f", index, position.x, position.y));
		}
	}
}

void WorleyNoise::set_seed(int p_seed) {
	if (seed == p_seed)
		return;

	seed = p_seed;
	_init_seeds();
	emit_changed();
}

int WorleyNoise::get_seed() const {
	return seed;
}

void WorleyNoise::set_points(int p_points) {
	if (points == p_points)
		return;

	points = p_points;
	_init_seeds();
	emit_changed();
}

int WorleyNoise::get_points() const {
	return points;
}

void WorleyNoise::set_inverted(bool p_inverted) {
	if (inverted == p_inverted) return;
	inverted = p_inverted;
	_init_seeds();
	emit_changed();
}

bool WorleyNoise::get_inverted() const {
	return inverted;
}

void WorleyNoise::set_cells_per_axis(int p_cells) {
	if (cellsPerAxis == p_cells) return;
	cellsPerAxis = p_cells;
	_init_seeds();
	emit_changed();
}

int WorleyNoise::get_cells_per_axis() const {
	return cellsPerAxis;
}

void WorleyNoise::set_period(float p_period) {
	if (p_period == period) return;
	period = p_period;
	emit_changed();
}

void WorleyNoise::set_persistence(float p_persistence) {
	if (p_persistence == persistence) return;
	persistence = p_persistence;
	emit_changed();
}

// TODO deal with non equal width/height
Ref<Image> WorleyNoise::get_image(int p_width, int p_height) const {

	PoolVector<uint8_t> data;
	PoolVector<float> vals;
	data.resize(p_width * p_height);
	vals.resize(p_width * p_height);

	PoolVector<uint8_t>::Write wd8 = data.write();
	PoolVector<float>::Write va = vals.write();

	// for normalization
	float max_v = std::numeric_limits<float>::min();

	for (int i = 0; i < p_height; i++) {
		for (int j = 0; j < p_width; j++) {
			float v = get_noise_2d(i / (p_height * 1.0), j / (p_width * 1.0));
			max_v = std::max(v, max_v);
			va[(i * p_width + j)] = v;
		}
	}

	for (int i = 0; i < p_height; i++) {
		for (int j = 0; j < p_width; j++) {
			float n = (va[(i * p_width + j)] / max_v) * persistence;
			n = inverted ? (255.0 - (n * 255.0)) : (n * 255.0);
			wd8[(i * p_width + j)] = uint8_t(CLAMP(n, 0, 255));
		}
	}

	//print_line(vformat("max value: %f", max_v));

	Ref<Image> image = memnew(Image(p_width, p_height, false, Image::FORMAT_L8, data));
	return image;
}

Ref<Image> WorleyNoise::get_seamless_image(int p_size) const {

	PoolVector<uint8_t> data;
	PoolVector<float> vals;
	data.resize(p_size * p_size);
	vals.resize(p_size * p_size);

	PoolVector<uint8_t>::Write wd8 = data.write();
	PoolVector<float>::Write va = vals.write();

	// for normalization
	float max_v = std::numeric_limits<float>::min();

	for (int i = 0; i < p_size; i++) {
		for (int j = 0; j < p_size; j++) {
			float v = get_noise_2d(i / (p_size * 1.0), j / (p_size * 1.0), true);
			max_v = std::max(v, max_v);
			va[(i * p_size + j)] = v;
		}
	}

	for (int i = 0; i < p_size; i++) {
		for (int j = 0; j < p_size; j++) {
			float n = (va[(i * p_size + j)] / max_v) * persistence;
			n = inverted ? (255.0 - (n * 255.0)) : (n * 255.0);
			wd8[(i * p_size + j)] = uint8_t(CLAMP(n, 0, 255));
		}
	}

	//print_line(vformat("max value: %f", max_v));

	Ref<Image> image = memnew(Image(p_size, p_size, false, Image::FORMAT_L8, data));
	return image;
}

void WorleyNoise::set_point_list(Vector<Vector2> point_list) {
	if (point_list.size() == 0)
		return;

	Vector2 sampled_points[MAX_SAMPLED_POINTS];
	points = point_list.size();
	for (int i = 0; i < points; i++)
		context.sampled_points[i] = point_list[i];
	emit_changed();
}

float WorleyNoise::get_noise_1d(float x) const {

	return get_noise_2d(x, 1.0);
}

float WorleyNoise::get_min_distance(float x, float y, bool seamless) const {
	//bool seamless = false;
	Vector2 samplePos(x, y);
	Vector2 cellId = (samplePos * cellsPerAxis).floor();
	float minSqrDst = 1.0;

	for (int cellOffsetIndex = 0; cellOffsetIndex < 9; ++cellOffsetIndex) {
		Vector2 adjId = cellId + offsets[cellOffsetIndex];

		if ((adjId.x == -1 || adjId.y == -1 || adjId.x == cellsPerAxis || adjId.y == cellsPerAxis)) {
			if (!seamless)
				continue;

			Vector2 wrappedId = Vector2((int(adjId.x) + cellsPerAxis) % cellsPerAxis, (int(adjId.y) + cellsPerAxis) % cellsPerAxis);
			int adjCellIndex = wrappedId.x + cellsPerAxis * wrappedId.y;
			Vector2 wrappedPoint = context.sampled_points[adjCellIndex];

			for (int wrapOffsetIndex = 0; wrapOffsetIndex < 9; ++wrapOffsetIndex) {
				Vector2 sampleOffset = (samplePos - (wrappedPoint + offsets[wrapOffsetIndex]));
				minSqrDst = std::min(minSqrDst, sampleOffset.length_squared());
			}
		} else {
			int adjCellIndex = adjId.x + cellsPerAxis * adjId.y;
			Vector2 sampleOffset = samplePos - context.sampled_points[adjCellIndex];
			minSqrDst = std::min(minSqrDst, sampleOffset.length_squared());
		}
	}

	return sqrt(minSqrDst);
}

float WorleyNoise::get_noise_2d(float x, float y, bool seamless) const {
	float amp = 1.0;
	float max = 1.0;
	float sum = get_min_distance(x, y, seamless);
	return sum; // / max;
}

float WorleyNoise::get_noise_3d(float x, float y, float z) const {
	return 0;
}

void WorleyNoise::_bind_methods() {

	ClassDB::bind_method(D_METHOD("get_seed"), &WorleyNoise::get_seed);
	ClassDB::bind_method(D_METHOD("set_seed", "seed"), &WorleyNoise::set_seed);

	ClassDB::bind_method(D_METHOD("get_points"), &WorleyNoise::get_points);
	ClassDB::bind_method(D_METHOD("set_points", "points"), &WorleyNoise::set_points);

	ClassDB::bind_method(D_METHOD("set_period", "period"), &WorleyNoise::set_period);
	ClassDB::bind_method(D_METHOD("get_period"), &WorleyNoise::get_period);

	ClassDB::bind_method(D_METHOD("set_persistence", "persistence"), &WorleyNoise::set_persistence);
	ClassDB::bind_method(D_METHOD("get_persistence"), &WorleyNoise::get_persistence);

	ClassDB::bind_method(D_METHOD("set_inverted", "inverted"), &WorleyNoise::set_inverted);
	ClassDB::bind_method(D_METHOD("get_inverted"), &WorleyNoise::get_inverted);

	ClassDB::bind_method(D_METHOD("set_cells_per_axis", "cellsPerAxis"), &WorleyNoise::set_cells_per_axis);
	ClassDB::bind_method(D_METHOD("get_cells_per_axis"), &WorleyNoise::get_cells_per_axis);

	ClassDB::bind_method(D_METHOD("get_image", "width", "height"), &WorleyNoise::get_image);
	ClassDB::bind_method(D_METHOD("get_seamless_image", "size"), &WorleyNoise::get_seamless_image);

	ClassDB::bind_method(D_METHOD("set_point_list", "points"), &WorleyNoise::set_point_list);

	ClassDB::bind_method(D_METHOD("get_noise_1d", "x"), &WorleyNoise::get_noise_1d);
	ClassDB::bind_method(D_METHOD("get_noise_2d", "x", "y"), &WorleyNoise::get_noise_2d);
	ClassDB::bind_method(D_METHOD("get_noise_3d", "x", "y", "z"), &WorleyNoise::get_noise_3d);

	ClassDB::bind_method(D_METHOD("get_noise_2dv", "pos"), &WorleyNoise::get_noise_2dv);
	ClassDB::bind_method(D_METHOD("get_noise_3dv", "pos"), &WorleyNoise::get_noise_3dv);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "seed"), "set_seed", "get_seed");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "points", PROPERTY_HINT_RANGE, vformat("1,%d", MAX_SAMPLED_POINTS)), "set_points", "get_points");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "period", PROPERTY_HINT_RANGE, "1,1024"), "set_period", "get_period");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "inverted"), "set_inverted", "get_inverted");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "persistence", PROPERTY_HINT_RANGE, "1,5.0,0.01"), "set_persistence", "get_persistence");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "cellsPerAxis", PROPERTY_HINT_RANGE, "1,45"), "set_cells_per_axis", "get_cells_per_axis");
}
