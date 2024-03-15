
#ifndef TM_FILE_UTIL_H
#define TM_FILE_UTIL_H

#include "core/object/class_db.h"

class AudioStream;
class ImageTexture;
class Node;
class Texture2D;

class TMFileUtil : public Object {
	GDCLASS(TMFileUtil, Object);

protected:
	static TMFileUtil *singleton;
	static void _bind_methods();

public:
	// Misc.
	static String clean_string_for_file_path(const String &p_input);
	static Ref<ImageTexture> convert_image_bytes_to_texture(const PackedByteArray &p_bytes, const String &p_format);
	static Variant parse_json_from_string(const String &p_json_text_string);

	// Texture conversion to bytes.
	static PackedByteArray get_exr_data(const Ref<Texture2D> p_texture);
	static PackedByteArray get_exr_data_at_path(const String &p_path);
	static PackedByteArray get_webp_data(const Ref<Texture2D> p_texture);
	static PackedByteArray get_webp_data_at_path(const String &p_path);

	// 3D model file loading.
	static PackedByteArray convert_gltf_files_to_glb_data(const String &p_path);
	static Node *load_gltf_file_as_node(const String &p_path, const bool p_discard_textures);

	// Resource file loading.
	static PackedByteArray load_file_bytes(const String &p_path);
	static Ref<AudioStream> load_audio(const String &p_path);
	static Ref<ImageTexture> load_image(const String &p_path);
	static Variant load_json_file(const String &p_path);

	TMFileUtil();
	~TMFileUtil();
};

#endif // TM_FILE_UTIL_H
