/// This file is an utility to avoid write the same code over and over to expose
/// the variables in Godot.
/// To use it you can include this into your header, and use `SETGET` to define
/// the variable you want to expose:
/// ```
/// #include "scene/3d/node_3d.h"
/// #include "godot_expose_utils.h"
///
/// class TestNode : public Node {
/// 	GDCLASS(TestNode, Node);
//
/// 	/* Exposed members */
/// 	SETGET(real_t, variable_1, 0.2);
/// 	SETGET(bool, variable_2, false);
/// 	SETGET(Transform, variable_3, Transform);
/// }
/// ```
///
/// Into the cpp file you can define and expose the variables.
/// NOTE that is needed define the __CLASS__ macro with the name of the class:
/// ```
/// #include "test_node.cpp"
///
/// #define __CLASS__ TestNode
///
/// SETGET_IMPL(real_t, variable_1);
/// SETGET_IMPL(bool, variable_2);
/// SETGET_IMPL(Transform, variable_3);
///
/// void TestNode::_bind_methods() {
/// 	EXPOSE(FLOAT, variable_1, PROPERTY_HINT_RANGE, "0,1,0.01");
/// 	EXPOSE_NO_HINT(BOOL, variable_2);
/// 	EXPOSE_NO_HINT(TRANSFORM, variable_3);
/// }
///
/// ```
///
/// If you want to contribute, please write a comment, I'll update the code.
///
/// License: MIT.
///

#ifndef GODOT_EXPOSE_UTILS_H
#define GODOT_EXPOSE_UTILS_H

/// This macro can be used, in the header, to define public variable with
/// public setter and getter functions.
/// You can implment those using the macro: `SETGET_IMPL`.
#define SETGET_PUB(__TYPE__, __VAR_NAME__, __DEF__) \
	__TYPE__ __VAR_NAME__ = __DEF__;                \
                                                    \
	void set_##__VAR_NAME__(__TYPE__ p_val);        \
	__TYPE__ get_##__VAR_NAME__() const;

/// This macro can be used, in the header, to define a private variable with
/// public setter and getter functions.
/// You can implment those using the macro: `SETGET_IMPL`.
#define SETGET(__TYPE__, __VAR_NAME__, __DEF__) \
private:                                        \
	__TYPE__ __VAR_NAME__ = __DEF__;            \
                                                \
public:                                         \
	void set_##__VAR_NAME__(__TYPE__ p_val);    \
	__TYPE__ get_##__VAR_NAME__() const;        \
                                                \
private:

/// This macro can be used in the cpp file to implement the set and get of variable
///
/// Please define the macro `__CLASS__` with the class you want to use in the cpp
/// file:
/// ```
/// #define __CLASS__ TestNode
/// ```
#define SETGET_IMPL(__TYPE__, __VAR_NAME__)              \
	void __CLASS__::set_##__VAR_NAME__(__TYPE__ p_val) { \
		__VAR_NAME__ = p_val;                            \
	}                                                    \
                                                         \
	__TYPE__ __CLASS__::get_##__VAR_NAME__() const {     \
		return __VAR_NAME__;                             \
	}

/// This macro can be used into the function `_bind_methods()` to expose the
/// specified variable to editor.
///
/// Please define the macro `__CLASS__` with the class you want to use in the cpp
/// file:
/// ```
/// #define __CLASS__ TestNode
/// ```
#define EXPOSE(__TYPE__, __VAR_NAME__, __HINT_MODE__, __HINT__)                                          \
	ClassDB::bind_method(D_METHOD("set_" #__VAR_NAME__, #__VAR_NAME__), &__CLASS__::set_##__VAR_NAME__); \
	ClassDB::bind_method(D_METHOD("get_" #__VAR_NAME__), &__CLASS__::get_##__VAR_NAME__);                \
	ADD_PROPERTY(PropertyInfo(Variant::__TYPE__, #__VAR_NAME__, __HINT_MODE__, __HINT__), "set_" #__VAR_NAME__, "get_" #__VAR_NAME__);

#define EXPOSE_NO_HINT(__TYPE__, __VAR_NAME__)                                                           \
	ClassDB::bind_method(D_METHOD("set_" #__VAR_NAME__, #__VAR_NAME__), &__CLASS__::set_##__VAR_NAME__); \
	ClassDB::bind_method(D_METHOD("get_" #__VAR_NAME__), &__CLASS__::get_##__VAR_NAME__);                \
	ADD_PROPERTY(PropertyInfo(Variant::__TYPE__, #__VAR_NAME__), "set_" #__VAR_NAME__, "get_" #__VAR_NAME__);

#endif // GODOT_EXPOSE_UTILS_H
