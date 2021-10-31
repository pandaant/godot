/* register_types.cpp */

#include "register_types.h"

#include "core/class_db.h"
#include "worley_noise.h"
#include "worley_noise_texture.h"

void register_worley_types() {
    ClassDB::register_class<WorleyNoise>();
	ClassDB::register_class<WorleyNoiseTexture>();
}

void unregister_worley_types() {
   // Nothing to do here in this example.
}