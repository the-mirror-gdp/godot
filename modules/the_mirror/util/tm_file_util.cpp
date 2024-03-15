#include "tm_file_util.h"

#include "modules/modules_enabled.gen.h"

#include "core/io/json.h"
#include "modules/gltf/gltf_document.h"
#include "scene/resources/audio_stream_wav.h"
#include "scene/resources/image_texture.h"
#include "servers/audio/audio_stream.h"

#ifdef MODULE_MINIMP3_ENABLED
#include "modules/minimp3/audio_stream_mp3.h"
#endif // MODULE_MINIMP3_ENABLED
#ifdef MODULE_VORBIS_ENABLED
#include "modules/vorbis/audio_stream_ogg_vorbis.h"
#endif // MODULE_VORBIS_ENABLED

#define OPEN_FILE_FOR_READING_OR_ERROR(m_path, m_retval)                                                                          \
	Error macro_open_file_error;                                                                                                  \
	const Ref<FileAccess> file = FileAccess::open(m_path, FileAccess::READ, &macro_open_file_error);                              \
	if (macro_open_file_error) {                                                                                                  \
		if (macro_open_file_error == ERR_FILE_NOT_FOUND) {                                                                        \
			ERR_PRINT("TMFileUtil: File does not exist at path: " + m_path);                                                      \
		} else if (macro_open_file_error == ERR_FILE_CANT_OPEN) {                                                                 \
			ERR_PRINT("TMFileUtil: Can't open file at path: " + m_path);                                                          \
		} else {                                                                                                                  \
			ERR_PRINT("TMFileUtil: Unable to read file " + m_path + " from disk, unknown error: " + itos(macro_open_file_error)); \
		}                                                                                                                         \
		return m_retval;                                                                                                          \
	}

// Misc.

String TMFileUtil::clean_string_for_file_path(const String &p_input) {
	int last_replace = 0;
	String cleaned = "";
	for (int i = 0; i < p_input.length(); i++) {
		char32_t character = p_input[i];
		switch (character) {
			case '"':
			case '$':
			case '%':
			case '&':
			case '\'':
			case '*':
			case ':':
			case ';':
			case '<':
			case '=':
			case '>':
			case '?':
			case '~': {
				cleaned += p_input.substr(last_replace, i - last_replace);
				last_replace = i + 1;
			} break;
			case '.':
			case '/':
			case '@':
			case '\\':
			case '|': {
				cleaned += p_input.substr(last_replace, i - last_replace) + "_";
				last_replace = i + 1;
			} break;
			default: {
			} break;
		}
	}
	cleaned += p_input.substr(last_replace);
	return cleaned;
}

Ref<ImageTexture> TMFileUtil::convert_image_bytes_to_texture(const PackedByteArray &p_bytes, const String &p_format) {
	Ref<Image> image;
	image.instantiate();
	Error err = Error::OK;
	if (p_format == "webp") {
		err = image->load_webp_from_buffer(p_bytes);
	} else if (p_format == "png") {
		err = image->load_png_from_buffer(p_bytes);
	} else if (p_format == "jpg") {
		err = image->load_jpg_from_buffer(p_bytes);
	} else {
		ERR_PRINT("TMFileUtil: Unsupported image format: " + p_format);
		return Ref<ImageTexture>();
	}
	if (!image->has_mipmaps()) {
		err = image->generate_mipmaps();
		if (err != Error::OK) {
			ERR_PRINT("TMFileUtil: Mipmap generation error: " + itos(err));
		}
	}
	return ImageTexture::create_from_image(image);
}

bool _looks_like_json(const String &p_value) {
	const String value = p_value.strip_edges();
	const bool has_braces = value.begins_with("{") && value.ends_with("}");
	const bool has_brackets = value.begins_with("[") && value.ends_with("]");
	return has_braces || has_brackets;
}

Variant TMFileUtil::parse_json_from_string(const String &p_json_text_string) {
	ERR_FAIL_COND_V_MSG(!_looks_like_json(p_json_text_string), Variant(), "TMFileUtil: String does not look like JSON: " + p_json_text_string);
	Ref<JSON> json;
	json.instantiate();
	Error err = json->parse(p_json_text_string);
	if (err != Error::OK) {
		ERR_PRINT("TMFileUtil: Failed to parse string as JSON: " + p_json_text_string);
	}
	return json->get_data();
}

// Texture conversion to bytes.

PackedByteArray TMFileUtil::get_exr_data(const Ref<Texture2D> p_texture) {
	ERR_FAIL_COND_V(p_texture.is_null(), PackedByteArray());
	Ref<Image> image = p_texture->get_image();
	PackedByteArray data = image->save_exr_to_buffer();
	return data;
}

PackedByteArray TMFileUtil::get_exr_data_at_path(const String &p_path) {
	PackedByteArray data;
	if (p_path.get_extension().to_lower() != "exr") {
		// We do not support exporting other image types as exr.
		return data;
	}
	Ref<Image> image;
	image.instantiate();
	Error error = image->load(p_path);
	if (error == Error::OK) {
		// Now that we confirmed that file is a correct image we load it
		// again from path, as it can be compressed with smaller size.
		data = FileAccess::get_file_as_bytes(p_path);
	}
	return data;
}

PackedByteArray TMFileUtil::get_webp_data(const Ref<Texture2D> p_texture) {
	ERR_FAIL_COND_V(p_texture.is_null(), PackedByteArray());
	Ref<Image> image = p_texture->get_image();
	PackedByteArray data = image->save_webp_to_buffer();
	return data;
}

PackedByteArray TMFileUtil::get_webp_data_at_path(const String &p_path) {
	PackedByteArray data;
	Ref<Texture2D> texture = load_image(p_path);
	if (texture.is_valid()) {
		if (p_path.get_extension().to_lower() == "webp") {
			// Now that we confirmed that file is a correct image we load it
			// again from path, as it can be compressed with smaller size.
			data = FileAccess::get_file_as_bytes(p_path);
		} else {
			data = get_webp_data(texture);
		}
	}
	return data;
}

// 3D model file loading.

PackedByteArray TMFileUtil::convert_gltf_files_to_glb_data(const String &p_path) {
	Ref<GLTFDocument> gltf_document;
	gltf_document.instantiate();
	Ref<GLTFState> gltf_state;
	gltf_state.instantiate();
	Error err = gltf_document->append_from_file(p_path, gltf_state, 8);
	ERR_FAIL_COND_V_MSG(err != Error::OK, PackedByteArray(), "TMFileUtil: Failed to load GLTF file from disk, error: " + itos(err));
	return gltf_document->generate_buffer(gltf_state);
}

Node *TMFileUtil::load_gltf_file_as_node(const String &p_path, const bool p_discard_textures) {
	Ref<GLTFDocument> gltf_document;
	gltf_document.instantiate();
	Ref<GLTFState> gltf_state;
	gltf_state.instantiate();
	if (p_discard_textures) {
		gltf_state->set_handle_binary_image(GLTFState::HANDLE_BINARY_DISCARD_TEXTURES);
	}
	Error err = gltf_document->append_from_file(p_path, gltf_state, 8);
	if (err != Error::OK) {
		ERR_PRINT("TMFileUtil: Failed to load GLTF file from disk, error: " + itos(err) + " " + p_path);
		return nullptr;
	}
	Node *node = gltf_document->generate_scene(gltf_state);
	if (node == nullptr) {
		ERR_PRINT("TMFileUtil: Failed to generate a Godot scene from GLTF data, error: " + itos(err) + " " + p_path);
		return nullptr;
	}
	// Disallow importing a model with an empty root node name.
	if (node->get_name() == StringName()) {
		ERR_PRINT("TMFileUtil: Scene root name missing in your GLTF file. Either the root node or the scene must have a valid name. We recommend exporting the GLTF file from the Godot editor using Single Root mode, which is best for The Mirror. " + p_path);
		return nullptr;
	}
	return node;
}

// Resource file loading.

PackedByteArray TMFileUtil::load_file_bytes(const String &p_path) {
	OPEN_FILE_FOR_READING_OR_ERROR(p_path, PackedByteArray());
	return file->get_buffer(file->get_length());
}

Ref<AudioStream> TMFileUtil::load_audio(const String &p_path) {
	OPEN_FILE_FOR_READING_OR_ERROR(p_path, Ref<AudioStream>());
	PackedByteArray audio_bytes = file->get_buffer(file->get_length());
	const String filename = p_path.get_file();
	const String extension = filename.get_extension().to_lower();
	Ref<AudioStream> audio_stream;
	if (extension == "wav") {
		Ref<AudioStreamWAV> audio_stream_wav;
		audio_stream_wav.instantiate();
		// TODO: This only works for 16-bit PCM WAV files.
		audio_stream_wav->set_data(audio_bytes);
#ifdef MODULE_MINIMP3_ENABLED
	} else if (extension == "mp3") {
		Ref<AudioStreamMP3> audio_stream_mp3;
		audio_stream_mp3.instantiate();
		audio_stream_mp3->set_data(audio_bytes);
#endif // MODULE_MINIMP3_ENABLED
#ifdef MODULE_VORBIS_ENABLED
	} else if (extension == "ogg") {
		Ref<AudioStreamOggVorbis> audio_stream_ogg = AudioStreamOggVorbis::load_from_buffer(audio_bytes);
		audio_stream = audio_stream_ogg;
#endif // MODULE_VORBIS_ENABLED
	} else {
		ERR_PRINT("TMFileUtil: Unsupported audio file type: " + extension);
		return audio_stream;
	}
	ERR_FAIL_COND_V_MSG(audio_stream.is_null(), audio_stream, "TMFileUtil: Failed to load audio file: " + p_path);
	audio_stream->set_name(filename);
	return audio_stream;
}

Ref<ImageTexture> TMFileUtil::load_image(const String &p_path) {
	OPEN_FILE_FOR_READING_OR_ERROR(p_path, Ref<ImageTexture>());
	PackedByteArray image_bytes = file->get_buffer(file->get_length());
	const String extension = p_path.get_extension().to_lower();
	return convert_image_bytes_to_texture(image_bytes, extension);
}

Variant TMFileUtil::load_json_file(const String &p_path) {
	OPEN_FILE_FOR_READING_OR_ERROR(p_path, Variant());
	const String json_string = file->get_as_text();
	return JSON::parse_string(json_string);
}

// Boring boilerplate and bindings.

TMFileUtil *TMFileUtil::singleton = nullptr;

TMFileUtil::TMFileUtil() {
	singleton = this;
}

TMFileUtil::~TMFileUtil() {
	singleton = nullptr;
}

void TMFileUtil::_bind_methods() {
	// Misc.
	ClassDB::bind_static_method("TMFileUtil", D_METHOD("clean_string_for_file_path", "input"), &TMFileUtil::clean_string_for_file_path);
	ClassDB::bind_static_method("TMFileUtil", D_METHOD("convert_image_bytes_to_texture", "bytes", "format"), &TMFileUtil::convert_image_bytes_to_texture);
	ClassDB::bind_static_method("TMFileUtil", D_METHOD("parse_json_from_string", "json_text_string"), &TMFileUtil::parse_json_from_string);

	// Texture conversion to bytes.
	ClassDB::bind_static_method("TMFileUtil", D_METHOD("get_exr_data", "texture"), &TMFileUtil::get_exr_data);
	ClassDB::bind_static_method("TMFileUtil", D_METHOD("get_exr_data_at_path", "path"), &TMFileUtil::get_exr_data_at_path);
	ClassDB::bind_static_method("TMFileUtil", D_METHOD("get_webp_data", "texture"), &TMFileUtil::get_webp_data);
	ClassDB::bind_static_method("TMFileUtil", D_METHOD("get_webp_data_at_path", "path"), &TMFileUtil::get_webp_data_at_path);

	// 3D model file loading.
	ClassDB::bind_static_method("TMFileUtil", D_METHOD("convert_gltf_files_to_glb_data", "path"), &TMFileUtil::convert_gltf_files_to_glb_data);
	ClassDB::bind_static_method("TMFileUtil", D_METHOD("load_gltf_file_as_node", "path", "discard_textures"), &TMFileUtil::load_gltf_file_as_node);

	// Resource file loading.
	ClassDB::bind_static_method("TMFileUtil", D_METHOD("load_file_bytes", "path"), &TMFileUtil::load_file_bytes);
	ClassDB::bind_static_method("TMFileUtil", D_METHOD("load_audio", "path"), &TMFileUtil::load_audio);
	ClassDB::bind_static_method("TMFileUtil", D_METHOD("load_image", "path"), &TMFileUtil::load_image);
	ClassDB::bind_static_method("TMFileUtil", D_METHOD("load_json_file", "path"), &TMFileUtil::load_json_file);
}
