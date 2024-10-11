/**************************************************************************/
/*  tm_user_gdscript_highlighter.h                                        */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef TM_USER_GDSCRIPT_HIGHLIGHTER_H
#define TM_USER_GDSCRIPT_HIGHLIGHTER_H

#include "scene/gui/text_edit.h"
#include "scene/resources/syntax_highlighter.h"

class TMUserGDScriptSyntaxHighlighter : public SyntaxHighlighter {
	GDCLASS(TMUserGDScriptSyntaxHighlighter, SyntaxHighlighter)

private:
	struct ColorRegion {
		Color color;
		String start_key;
		String end_key;
		bool line_only = false;
	};
	Vector<ColorRegion> color_regions;
	HashMap<int, int> color_region_cache;

	HashMap<StringName, Color> class_names;
	HashMap<StringName, Color> reserved_keywords;
	HashMap<StringName, Color> member_keywords;
	HashSet<StringName> global_functions;

	enum Type {
		NONE,
		REGION,
		NODE_PATH,
		NODE_REF,
		ANNOTATION,
		STRING_NAME,
		SYMBOL,
		NUMBER,
		FUNCTION,
		SIGNAL,
		KEYWORD,
		MEMBER,
		IDENTIFIER,
		TYPE,
	};

	// Colors.
	Color font_color = Color(0.8025, 0.81, 0.8225);
	Color annotation_color = Color(1.0, 0.7, 0.45);
	Color comment_color = Color(font_color, 0.5);
	Color doc_comment_color = Color(0.6, 0.7, 0.8, 0.8);
	Color function_color = Color(0.34, 0.7, 1.0);
	Color function_definition_color = Color(0.4, 0.9, 1.0);
	Color global_function_color = Color(0.64, 0.64, 0.96);
	Color keyword_color = Color(1.0, 0.44, 0.52);
	Color keyword_control_flow_color = Color(1.0, 0.55, 0.8);
	Color member_variable_color = Color(0.736, 0.88, 1.0);
	Color node_path_color = Color(0.72, 0.77, 0.49);
	Color node_ref_color = Color(0.39, 0.76, 0.35);
	Color number_color = Color(0.63, 1.0, 0.88);
	Color string_color = Color(1.0, 0.93, 0.63);
	Color string_name_color = Color(1.0, 0.76, 0.65);
	Color symbol_color = Color(0.67, 0.79, 1.0);
	Color type_base_color = Color(0.26, 1.0, 0.76);
	Color type_engine_color = Color(0.56, 1, 0.86);
	Color type_user_color = Color(0.78, 1, 0.93);

	enum CommentMarkerLevel {
		COMMENT_MARKER_CRITICAL,
		COMMENT_MARKER_WARNING,
		COMMENT_MARKER_NOTICE,
		COMMENT_MARKER_MAX,
	};
	Color comment_marker_colors[COMMENT_MARKER_MAX] = {
		Color(0.77, 0.35, 0.35),
		Color(0.72, 0.61, 0.48),
		Color(0.56, 0.67, 0.51),
	};
	HashMap<String, CommentMarkerLevel> comment_markers;

	void add_color_region(const String &p_start_key, const String &p_end_key, const Color &p_color, bool p_line_only = false);

public:
	virtual void _update_cache() override;
	virtual Dictionary _get_line_syntax_highlighting_impl(int p_line) override;
};

#endif // TM_USER_GDSCRIPT_HIGHLIGHTER_H
