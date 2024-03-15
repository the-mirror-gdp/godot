#include "tm_audio_player_3d.h"

#include "servers/audio/audio_stream.h"

void TMAudioPlayer3D::setup_audio(Ref<AudioStream> p_stream) {
	if (unlikely(p_stream->has_loop())) {
		WARN_PRINT("The AudioStream itself is looped, so TMAudioPlayer3D's loop property will be ignored.");
	}
	set_stream(p_stream);
	if (is_autoplay_enabled()) {
		play();
	}
}

real_t TMAudioPlayer3D::get_base_volume_percentage() const {
	return Math::db_to_linear(get_volume_db()) * 100.0;
}

void TMAudioPlayer3D::set_base_volume_percentage(const real_t p_volume_percentage) {
	set_volume_db(Math::linear_to_db(p_volume_percentage / 100.0));
}

void TMAudioPlayer3D::set_is_spatial(const bool p_is_spatial) {
	_is_spatial = p_is_spatial;
	if (_is_spatial) {
		set_attenuation_model(ATTENUATION_INVERSE_DISTANCE);
		set_panning_strength(1.0);
		set_max_distance(_spatial_range);
		set_max_db(_spatial_max_volume_db);
	} else {
		set_attenuation_model(ATTENUATION_DISABLED);
		set_panning_strength(0.0);
		set_max_distance(0.0); // Zero max distance means infinite.
		set_max_db(get_volume_db());
	}
}

void TMAudioPlayer3D::set_spatial_range(const real_t p_spatial_range) {
	_spatial_range = p_spatial_range;
	if (_is_spatial) {
		set_max_distance(_spatial_range);
	}
}

real_t TMAudioPlayer3D::get_spatial_max_volume_percentage() const {
	return Math::db_to_linear(_spatial_max_volume_db) * 100.0;
}

void TMAudioPlayer3D::set_spatial_max_volume_percentage(const real_t p_volume_percentage) {
	_spatial_max_volume_db = Math::linear_to_db(p_volume_percentage / 100.0);
	if (_is_spatial) {
		set_max_db(_spatial_max_volume_db);
	}
}

void TMAudioPlayer3D::_on_audio_finished() {
	if (_loop_audio) {
		play();
	}
}

void TMAudioPlayer3D::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_POSTINITIALIZE: {
			connect("finished", callable_mp(this, &TMAudioPlayer3D::_on_audio_finished));
		}
	}
}

void TMAudioPlayer3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("setup_audio", "audio_stream"), &TMAudioPlayer3D::setup_audio);
	ClassDB::bind_method(D_METHOD("get_base_volume_percentage"), &TMAudioPlayer3D::get_base_volume_percentage);
	ClassDB::bind_method(D_METHOD("set_base_volume_percentage", "volume_percentage"), &TMAudioPlayer3D::set_base_volume_percentage);
	ClassDB::bind_method(D_METHOD("get_loop_audio"), &TMAudioPlayer3D::get_loop_audio);
	ClassDB::bind_method(D_METHOD("set_loop_audio", "loop_audio"), &TMAudioPlayer3D::set_loop_audio);
	ClassDB::bind_method(D_METHOD("get_is_spatial"), &TMAudioPlayer3D::get_is_spatial);
	ClassDB::bind_method(D_METHOD("set_is_spatial", "is_spatial"), &TMAudioPlayer3D::set_is_spatial);
	ClassDB::bind_method(D_METHOD("get_spatial_range"), &TMAudioPlayer3D::get_spatial_range);
	ClassDB::bind_method(D_METHOD("set_spatial_range", "spatial_range"), &TMAudioPlayer3D::set_spatial_range);
	ClassDB::bind_method(D_METHOD("get_spatial_max_volume_percentage"), &TMAudioPlayer3D::get_spatial_max_volume_percentage);
	ClassDB::bind_method(D_METHOD("set_spatial_max_volume_percentage", "volume_percentage"), &TMAudioPlayer3D::set_spatial_max_volume_percentage);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "base_volume_percentage"), "set_base_volume_percentage", "get_base_volume_percentage");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "loop_audio"), "set_loop_audio", "get_loop_audio");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_spatial"), "set_is_spatial", "get_is_spatial");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "spatial_range"), "set_spatial_range", "get_spatial_range");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "spatial_max_volume_percentage"), "set_spatial_max_volume_percentage", "get_spatial_max_volume_percentage");
}
