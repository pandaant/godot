#include "worley_noise_texture.h"

#include "core/core_string_names.h"

WorleyNoiseTexture::WorleyNoiseTexture() {
	update_queued = false;
	regen_queued = false;
	first_time = true;

	size = Vector2i(512, 512);
	seamless = false;
	as_normalmap = false;
	bump_strength = 8.0;
	flags = FLAGS_DEFAULT;

	noise = Ref<WorleyNoise>();

	texture = VS::get_singleton()->texture_create();

	_queue_update();
}

WorleyNoiseTexture::~WorleyNoiseTexture() {
	VS::get_singleton()->free(texture);
	noise_thread.wait_to_finish();
}

void WorleyNoiseTexture::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_width", "width"), &WorleyNoiseTexture::set_width);
	ClassDB::bind_method(D_METHOD("set_height", "height"), &WorleyNoiseTexture::set_height);

	ClassDB::bind_method(D_METHOD("set_noise", "noise"), &WorleyNoiseTexture::set_noise);
	ClassDB::bind_method(D_METHOD("get_noise"), &WorleyNoiseTexture::get_noise);

	ClassDB::bind_method(D_METHOD("set_seamless", "seamless"), &WorleyNoiseTexture::set_seamless);
	ClassDB::bind_method(D_METHOD("get_seamless"), &WorleyNoiseTexture::get_seamless);

	ClassDB::bind_method(D_METHOD("set_as_normalmap", "as_normalmap"), &WorleyNoiseTexture::set_as_normalmap);
	ClassDB::bind_method(D_METHOD("is_normalmap"), &WorleyNoiseTexture::is_normalmap);

	ClassDB::bind_method(D_METHOD("set_bump_strength", "bump_strength"), &WorleyNoiseTexture::set_bump_strength);
	ClassDB::bind_method(D_METHOD("get_bump_strength"), &WorleyNoiseTexture::get_bump_strength);

	ClassDB::bind_method(D_METHOD("_update_texture"), &WorleyNoiseTexture::_update_texture);
	ClassDB::bind_method(D_METHOD("_queue_update"), &WorleyNoiseTexture::_queue_update);
	ClassDB::bind_method(D_METHOD("_generate_texture"), &WorleyNoiseTexture::_generate_texture);
	ClassDB::bind_method(D_METHOD("_thread_done", "image"), &WorleyNoiseTexture::_thread_done);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "width", PROPERTY_HINT_RANGE, "1,2048,1,or_greater"), "set_width", "get_width");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "height", PROPERTY_HINT_RANGE, "1,2048,1,or_greater"), "set_height", "get_height");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "seamless"), "set_seamless", "get_seamless");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "as_normalmap"), "set_as_normalmap", "is_normalmap");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "bump_strength", PROPERTY_HINT_RANGE, "0,32,0.1,or_greater"), "set_bump_strength", "get_bump_strength");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "noise", PROPERTY_HINT_RESOURCE_TYPE, "WorleyNoise"), "set_noise", "get_noise");
}

void WorleyNoiseTexture::_validate_property(PropertyInfo &property) const {
	if (property.name == "bump_strength") {
		if (!as_normalmap) {
			property.usage = PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL;
		}
	}
}

void WorleyNoiseTexture::_set_texture_data(const Ref<Image> &p_image) {
	data = p_image;
	if (data.is_valid()) {
		VS::get_singleton()->texture_allocate(texture, size.x, size.y, 0, p_image->get_format(), VS::TEXTURE_TYPE_2D, flags);
		VS::get_singleton()->texture_set_data(texture, p_image);
	}
	emit_changed();
}

void WorleyNoiseTexture::_thread_done(const Ref<Image> &p_image) {
	_set_texture_data(p_image);
	noise_thread.wait_to_finish();
	if (regen_queued) {
		noise_thread.start(_thread_function, this);
		regen_queued = false;
	}
}

void WorleyNoiseTexture::_thread_function(void *p_ud) {
	WorleyNoiseTexture *tex = (WorleyNoiseTexture *)p_ud;
	tex->call_deferred("_thread_done", tex->_generate_texture());
}

void WorleyNoiseTexture::_queue_update() {
	if (update_queued)
		return;

	update_queued = true;
	call_deferred("_update_texture");
}

Ref<Image> WorleyNoiseTexture::_generate_texture() {

	// Prevent memdelete due to unref() on other thread.
	Ref<WorleyNoise> ref_noise = noise;

	if (ref_noise.is_null()) {
		return Ref<Image>();
	}

	Ref<Image> image;

	if (seamless) {
		image = ref_noise->get_seamless_image(size.x);
	} else {
		image = ref_noise->get_image(size.x, size.y);
	}

	if (as_normalmap) {
		image->bumpmap_to_normalmap(bump_strength);
	}

	return image;
}

void WorleyNoiseTexture::_update_texture() {
	bool use_thread = true;
	if (first_time) {
		use_thread = false;
		first_time = false;
	}
#ifdef NO_THREADS
	use_thread = false;
#endif
	if (use_thread) {

		if (!noise_thread.is_started()) {
			noise_thread.start(_thread_function, this);
			regen_queued = false;
		} else {
			regen_queued = true;
		}

	} else {
		Ref<Image> image = _generate_texture();
		_set_texture_data(image);
	}
	update_queued = false;
}

void WorleyNoiseTexture::set_noise(Ref<WorleyNoise> p_noise) {
	if (p_noise == noise)
		return;
	if (noise.is_valid()) {
		noise->disconnect(CoreStringNames::get_singleton()->changed, this, "_queue_update");
	}
	noise = p_noise;
	if (noise.is_valid()) {
		noise->connect(CoreStringNames::get_singleton()->changed, this, "_queue_update");
	}
	_queue_update();
}

Ref<WorleyNoise> WorleyNoiseTexture::get_noise() {
	return noise;
}

void WorleyNoiseTexture::set_width(int p_width) {
	if (p_width == size.x) return;
	size.x = p_width;
	_queue_update();
}

void WorleyNoiseTexture::set_height(int p_height) {
	if (p_height == size.y) return;
	size.y = p_height;
	_queue_update();
}

void WorleyNoiseTexture::set_seamless(bool p_seamless) {
	if (p_seamless == seamless) return;
	seamless = p_seamless;
	_queue_update();
}

bool WorleyNoiseTexture::get_seamless() {
	return seamless;
}

void WorleyNoiseTexture::set_as_normalmap(bool p_as_normalmap) {
	if (p_as_normalmap == as_normalmap) return;
	as_normalmap = p_as_normalmap;
	_queue_update();
	_change_notify();
}

bool WorleyNoiseTexture::is_normalmap() {
	return as_normalmap;
}

void WorleyNoiseTexture::set_bump_strength(float p_bump_strength) {
	if (p_bump_strength == bump_strength) return;
	bump_strength = p_bump_strength;
	if (as_normalmap)
		_queue_update();
}

float WorleyNoiseTexture::get_bump_strength() {

	return bump_strength;
}

int WorleyNoiseTexture::get_width() const {
	return size.x;
}

int WorleyNoiseTexture::get_height() const {
	return size.y;
}

void WorleyNoiseTexture::set_flags(uint32_t p_flags) {
	flags = p_flags;
	VS::get_singleton()->texture_set_flags(texture, flags);
}

uint32_t WorleyNoiseTexture::get_flags() const {
	return flags;
}

Ref<Image> WorleyNoiseTexture::get_data() const {
	return data;
}
