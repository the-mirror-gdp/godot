#pragma once

#include "core/templates/oa_hash_map.h"
#include "scene/main/node.h"
#include "servers/audio/audio_stream.h"
#include <deque>

namespace JPH {
class Body;
class ContactManifold;
class ContactSettings;
}; //namespace JPH

struct SoundInfo {
	float volume = 0.0f;
	float pitch_scale = 1.0f;
	float attenuation_radius = 0.0f;
	Vector3 location;
};

struct QueuedSound {
	uint32_t body_1_id;
	uint32_t body_2_id;
	String sound;
	SoundInfo sound_info;
	uint64_t timestamp;
};

struct Stream {
	class AudioStreamPlayer3D *stream = nullptr;
	uint32_t body_1_id = UINT32_MAX;
	uint32_t body_2_id = UINT32_MAX;
};

class CollisionFxManager final : public Node {
	GDCLASS(CollisionFxManager, Node);

	static CollisionFxManager *singleton;

	float base_volume_multiplier = 1.0f;
	float min_speed_kmh_volume_multiplier = 2.0f;
	float max_speed_kmh_volume_multiplier = 20.0f;
	float random_pitch = 0.2f;
	float max_mass_pitch_multiplier = 20.0f;
	float mass_pitch_multiplier = 0.4f;
	float attenuation_range_min = 20.0f;
	float attenuation_range_max = 30.0f;
	uint32_t audio_streams_count = 20;

	OAHashMap<String, Ref<AudioStream>> sounds;
	std::deque<QueuedSound> sound_queue;
	LocalVector<Stream> audio_streams;
	LocalVector<int> free_audio_streams;
	LocalVector<int> used_audio_streams;

public:
	CollisionFxManager();
	virtual ~CollisionFxManager();

	static CollisionFxManager *get_singleton() { return singleton; }

	static void __notify_impact(
			const JPH::Body &body_a,
			const JPH::Body &body_b,
			const JPH::ContactManifold &manifold,
			JPH::ContactSettings &settings);

private:
	void notify_impact(
			const JPH::Body &body_a,
			const JPH::Body &body_b,
			const JPH::ContactManifold &manifold,
			JPH::ContactSettings &settings);

	bool should_emit_a_sound(
			const JPH::Body &body_1,
			const JPH::Body &body_2) const;

	void get_sound_prop_based_on_the_impact_intensity(
			const JPH::Body &body_1,
			const JPH::Body &body_2,
			const JPH::ContactManifold &manifold,
			JPH::ContactSettings &settings,
			SoundInfo &r_sound_info);

	void fetch_sound_to_play_for(
			const JPH::Body &body_1,
			const JPH::Body &body_2,
			String &r_sound);

	void try_play_impact_sound(
			const JPH::Body &body_1,
			const JPH::Body &body_2,
			const SoundInfo &p_sound_info);

	void setup_AFX();
	// Returns true if the sounds for these two bodies is being played already.
	bool is_sound_playing_for(uint32_t body_1, uint32_t body_2) const;
	void play_sound(const QueuedSound &p_qs);
	bool can_play_a_sound() const;

	void flush_sounds();

	Ref<AudioStream> get_sound(const String &p_sound_name);

public:
	void _notification(int p_what);
};
