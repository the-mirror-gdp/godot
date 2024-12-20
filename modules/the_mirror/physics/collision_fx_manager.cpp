#include "collision_fx_manager.h"

#include "core/config/project_settings.h"
#include "core/string/print_string.h"
#include "modules/jolt/j_contact_listener.h"
#include "modules/jolt/j_utils.h"
#include "modules/jolt/jolt.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/Collision/ContactListener.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/Collision/EstimateCollisionResponse.h"
#include "scene/3d/audio_stream_player_3d.h"
#include "scene/main/node.h"
#include "servers/audio/audio_stream.h"
#include "servers/audio_server.h"

#include "collision_fx_manager.h"
#include "tm_scene_sync.h"

#define KMH_TO_MS(v) (v / 3.6)

CollisionFxManager *CollisionFxManager::singleton = nullptr;

CollisionFxManager::CollisionFxManager() {
	// You can't have more than 1 CollisionSoundManager
	CRASH_COND(singleton != nullptr);
	singleton = this;

	base_volume_multiplier = GLOBAL_DEF("mirror/base_volume_multiplier", base_volume_multiplier);
	min_speed_kmh_volume_multiplier = GLOBAL_DEF("mirror/min_speed_kmh_volume_multiplier", min_speed_kmh_volume_multiplier);
	max_speed_kmh_volume_multiplier = GLOBAL_DEF("mirror/max_speed_kmh_volume_multiplier", max_speed_kmh_volume_multiplier);
	random_pitch = GLOBAL_DEF("mirror/random_pitch", random_pitch);
	max_mass_pitch_multiplier = GLOBAL_DEF("mirror/max_mass_pitch_multiplier", max_mass_pitch_multiplier);
	mass_pitch_multiplier = GLOBAL_DEF("mirror/mass_pitch_multiplier", mass_pitch_multiplier);
	attenuation_range_min = GLOBAL_DEF("mirror/attenuation_range_min", attenuation_range_min);
	attenuation_range_max = GLOBAL_DEF("mirror/attenuation_range_max", attenuation_range_max);
	audio_streams_count = GLOBAL_DEF("mirror/audio_streams_count", audio_streams_count);

	Jolt::singleton()->get_contact_listener(0)->contact_added_listeners.push_back(CollisionFxManager::__notify_impact);
}

CollisionFxManager::~CollisionFxManager() {
	singleton = nullptr;
}

void CollisionFxManager::__notify_impact(
		const JPH::Body &body_a,
		const JPH::Body &body_b,
		const JPH::ContactManifold &manifold,
		JPH::ContactSettings &settings) {
	singleton->notify_impact(
			body_a,
			body_b,
			manifold,
			settings);
}

void CollisionFxManager::notify_impact(
		const JPH::Body &body_a,
		const JPH::Body &body_b,
		const JPH::ContactManifold &manifold,
		JPH::ContactSettings &settings) {
	if (TMSceneSync::singleton()->is_server()) {
		// Nothing to do on the server.
		return;
	}

	const JPH::Body *body_1 = &body_a;
	const JPH::Body *body_2 = &body_b;
	if (body_a.GetID().GetIndex() > body_b.GetID().GetIndex()) {
		body_1 = &body_b;
		body_2 = &body_a;
	}

	if (!should_emit_a_sound(*body_1, *body_2)) {
		// Nothing to do.
		return;
	}

	SoundInfo sound_info;
	get_sound_prop_based_on_the_impact_intensity(
			*body_1,
			*body_2,
			manifold,
			settings,
			sound_info);

	try_play_impact_sound(
			*body_1,
			*body_2,
			sound_info);
}

bool CollisionFxManager::should_emit_a_sound(
		const JPH::Body &body_1,
		const JPH::Body &body_2) const {
	if (body_1.IsSensor() || body_2.IsSensor()) {
		return false;
	} else {
		return true;
	}
}

void CollisionFxManager::get_sound_prop_based_on_the_impact_intensity(
		const JPH::Body &body_1,
		const JPH::Body &body_2,
		const JPH::ContactManifold &manifold,
		JPH::ContactSettings &settings,
		SoundInfo &r_sound_info) {
	r_sound_info.volume = 0.0f;
	r_sound_info.pitch_scale = 1.0f;
	r_sound_info.attenuation_radius = 0.0f;
	r_sound_info.location = Vector3();

	const bool use_impact_force = false;
	if (use_impact_force) {
		// Estimate the contact impulses. Note that these won't be 100% accurate
		// unless you set the friction of the bodies to 0 (EstimateCollisionResponse ignores friction)
		JPH::CollisionEstimationResult res;
		JPH::EstimateCollisionResponse(
				body_1,
				body_2,
				manifold,
				res,
				settings.mCombinedFriction,
				settings.mCombinedRestitution,
				1.0,
				// `NumIterations` No need to iterate more, this algorithm must be easy to process.
				1);

		float max_impulse = 0.0;
		for (const auto &impulse : res.mImpulses) {
			// impulse.mContactImpulse: Kg m / s (AKA Newton per Second [Ns])
			max_impulse = MAX(impulse.mContactImpulse, max_impulse);
		}

		r_sound_info.volume = MIN(max_impulse / 2000.0f, 1.0f);
		r_sound_info.attenuation_radius = Math::lerp(10.0f, 30.0f, r_sound_info.volume);
		r_sound_info.location = convert_r(manifold.GetWorldSpaceContactPointOn1(0));
	} else {
		// Fetch the volume based on the object velocity.
		float speed = 0.0f;
		float mass = 0.0f;
		if (body_1.IsDynamic()) {
			speed = ABS(body_1.GetLinearVelocity().Dot(manifold.mWorldSpaceNormal));
			mass = 1.0f / body_1.GetMotionPropertiesUnchecked()->GetInverseMass();
		}
		if (body_2.IsDynamic()) {
			speed = MAX(speed, ABS(body_2.GetLinearVelocity().Dot(manifold.mWorldSpaceNormal)));
			mass = MAX(mass, 1.0f / body_2.GetMotionPropertiesUnchecked()->GetInverseMass());
		}

		// High precision is not needed here so perform all calculations with 32-bit floats.
		r_sound_info.volume = (speed - KMH_TO_MS(min_speed_kmh_volume_multiplier)) / KMH_TO_MS(max_speed_kmh_volume_multiplier);
		r_sound_info.volume = CLAMP(r_sound_info.volume, 0.0f, 1.0f);
		r_sound_info.pitch_scale -= (Math::randf() * random_pitch) - (random_pitch * 0.5f);
		r_sound_info.pitch_scale -= (MIN(mass / max_mass_pitch_multiplier, 1.0f) * mass_pitch_multiplier) - (mass_pitch_multiplier * 0.5f);
		r_sound_info.attenuation_radius = Math::lerp(attenuation_range_min, attenuation_range_max, r_sound_info.volume);
		r_sound_info.location = convert_r(manifold.GetWorldSpaceContactPointOn1(0));
	}
}

void CollisionFxManager::fetch_sound_to_play_for(
		const JPH::Body &body_1,
		const JPH::Body &body_2,
		String &r_sound) {
	// TODO fetch the sound based on the material used by these bodies.
	r_sound = "res://audio/audio_collision_gen-003-fast.wav";
}

void CollisionFxManager::try_play_impact_sound(
		const JPH::Body &body_1,
		const JPH::Body &body_2,
		const SoundInfo &p_sound_info) {
	if (p_sound_info.volume <= 0.0 || p_sound_info.attenuation_radius <= 0.0) {
		// No sound to play.
		return;
	}

	if (is_sound_playing_for(body_1.GetID().GetIndex(), body_2.GetID().GetIndex())) {
		// There is a playing sound already
		return;
	}

	const uint64_t now = OS::get_singleton()->get_ticks_msec();

	QueuedSound qs;
	qs.body_1_id = body_1.GetID().GetIndex();
	qs.body_2_id = body_2.GetID().GetIndex();
	fetch_sound_to_play_for(body_1, body_2, qs.sound);
	qs.sound_info = p_sound_info;
	qs.timestamp = now;

	sound_queue.push_back(qs);
}

void CollisionFxManager::setup_AFX() {
	if (audio_streams.size() >= audio_streams_count) {
		return;
	}

	for (uint32_t i = 0; i < audio_streams_count; i++) {
		AudioStreamPlayer3D *asp = memnew(AudioStreamPlayer3D);
		Stream s;
		s.stream = asp;
		audio_streams.push_back(s);
		add_child(asp);
		asp->set_owner(this);

		free_audio_streams.push_back(i);
	}
}

bool CollisionFxManager::is_sound_playing_for(uint32_t body_1, uint32_t body_2) const {
	for (int stream_index : used_audio_streams) {
		// NOTE: there is no need to check the other way around because the `body_1` is always < `body_2` at this point
		// thanks to the check we do on `notify_impact`.
		if (audio_streams[stream_index].body_1_id == body_1 && audio_streams[stream_index].body_2_id == body_2) {
			return true;
		}
	}
	return false;
}

void CollisionFxManager::play_sound(const QueuedSound &p_qs) {
	// It's safe to read from 0 because the `can_play_a_sound` prevents it from
	// playing entirely if there is not a free stream.
	const int stream_id = free_audio_streams[0];
	free_audio_streams.remove_at_unordered(0);
	used_audio_streams.push_back(stream_id);

	// Play the sound.
	audio_streams[stream_id].stream->set_position(p_qs.sound_info.location);
	audio_streams[stream_id].stream->set_stream(get_sound(p_qs.sound));
	audio_streams[stream_id].stream->set_volume_db(Math::linear_to_db(p_qs.sound_info.volume * base_volume_multiplier));
	audio_streams[stream_id].stream->set_max_db(Math::linear_to_db(p_qs.sound_info.volume * base_volume_multiplier));
	audio_streams[stream_id].stream->set_pitch_scale(p_qs.sound_info.pitch_scale);
	audio_streams[stream_id].stream->set_max_distance(p_qs.sound_info.attenuation_radius);
	audio_streams[stream_id].stream->play(0);
	audio_streams[stream_id].body_1_id = p_qs.body_1_id;
	audio_streams[stream_id].body_2_id = p_qs.body_2_id;
}

bool CollisionFxManager::can_play_a_sound() const {
	return free_audio_streams.size() > 0;
}

void CollisionFxManager::flush_sounds() {
	// Fetch the expired sounds and put these into the free queue.
	{
		LocalVector<int> just_finished_stream;
		for (int used_stream_index : used_audio_streams) {
			if (!audio_streams[used_stream_index].stream->is_playing()) {
				audio_streams[used_stream_index].body_1_id = UINT32_MAX;
				audio_streams[used_stream_index].body_2_id = UINT32_MAX;
				just_finished_stream.push_back(used_stream_index);
			}
		}
		for (int index : just_finished_stream) {
			just_finished_stream.erase(index);
			free_audio_streams.push_back(index);
		}
	}

	// Play the queued sounds.
	while (!sound_queue.empty() && can_play_a_sound()) {
		const QueuedSound &qs = sound_queue.front();
		if (!is_sound_playing_for(qs.body_1_id, qs.body_2_id)) {
			// Play the sound only if it's not yet playing.
			play_sound(qs);
		}
		sound_queue.pop_front();
	}
}

Ref<AudioStream> CollisionFxManager::get_sound(const String &p_sound_name) {
	Ref<AudioStream> *sound_ptr = sounds.lookup_ptr(p_sound_name);
	if (sound_ptr) {
		// The sound was already loaded.
		return *sound_ptr;
	}

	// Try to load the sound
	Ref<AudioStream> sound = ResourceLoader::load(p_sound_name);
	if (sound.is_valid()) {
		// Store the sound for later use.
		sounds.insert(p_sound_name, sound);
	}

	return sound;
}

void CollisionFxManager::_notification(int p_what) {
	if (NOTIFICATION_READY == p_what) {
		set_process_internal(true);
		setup_AFX();
	} else if (NOTIFICATION_INTERNAL_PROCESS == p_what) {
		flush_sounds();
	}
}
