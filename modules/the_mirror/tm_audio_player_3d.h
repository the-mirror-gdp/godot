
#ifndef TM_AUDIO_PLAYER_3D_H
#define TM_AUDIO_PLAYER_3D_H

#include "scene/3d/audio_stream_player_3d.h"

class TMAudioPlayer3D : public AudioStreamPlayer3D {
	GDCLASS(TMAudioPlayer3D, AudioStreamPlayer3D);

	bool _loop_audio = false;
	bool _is_spatial = true;
	real_t _spatial_range = 0.0;
	real_t _spatial_max_volume_db = 3.52182518111362484162578;

protected:
	static void _bind_methods();
	void _notification(int p_what);
	void _on_audio_finished();

public:
	void setup_audio(Ref<AudioStream> p_stream);

	real_t get_base_volume_percentage() const;
	void set_base_volume_percentage(const real_t p_volume_percentage);

	bool get_loop_audio() const { return _loop_audio; }
	void set_loop_audio(const bool p_loop_audio) { _loop_audio = p_loop_audio; }

	bool get_is_spatial() const { return _is_spatial; }
	void set_is_spatial(const bool p_is_spatial);

	real_t get_spatial_range() const { return _spatial_range; }
	void set_spatial_range(const real_t p_spatial_range);

	real_t get_spatial_max_volume_percentage() const;
	void set_spatial_max_volume_percentage(const real_t p_volume_percentage);
};

#endif // TM_AUDIO_PLAYER_3D_H
