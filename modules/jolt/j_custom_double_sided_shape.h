// Adapted version of: https://github.com/godot-jolt

#pragma once

#include "j_custom_decorated_shape.h"

class JoltCustomDoubleSidedShapeSettings final : public JoltCustomDecoratedShapeSettings {
public:
	ShapeResult Create() const override;
};

class JoltCustomDoubleSidedShape final : public JoltCustomDecoratedShape {
public:
	static void register_type();

	JoltCustomDoubleSidedShape() :
			JoltCustomDecoratedShape(JoltCustomShapeSubType::DOUBLE_SIDED) {}

	JoltCustomDoubleSidedShape(
			const JoltCustomDoubleSidedShapeSettings &p_settings,
			ShapeResult &p_result) :
			JoltCustomDecoratedShape(
					JoltCustomShapeSubType::DOUBLE_SIDED,
					p_settings,
					p_result) {
		if (!p_result.HasError()) {
			p_result.Set(this);
		}
	}
};