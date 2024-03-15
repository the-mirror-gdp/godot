/**************************************************************************/
/*  app_protocol.cpp                                                      */
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

#include "app_protocol.h"
#include "core/object/object.h"
#include "thirdparty/ipc/ipc.h"

#include "core/config/project_settings.h"
#include "core/os/memory.h"

AppProtocol *AppProtocol::singleton = nullptr;

AppProtocol::AppProtocol() {
	singleton = this;
}

AppProtocol::~AppProtocol() {
	singleton = nullptr; // we only ever have one
}

void AppProtocol::_bind_methods() {
	ADD_SIGNAL(MethodInfo("on_url_received", PropertyInfo(Variant::STRING, "url")));
	ClassDB::bind_method(D_METHOD("poll_server"), &AppProtocol::poll_server);
	ClassDB::bind_method(D_METHOD("start_protocol_handling"), &AppProtocol::start_protocol_handling);
}

void AppProtocol::initialize() {
	if (singleton == nullptr) {
		new AppProtocol();
	}
}
void AppProtocol::finalize() {
	if (singleton != nullptr) {
		delete singleton;
	}
}

AppProtocol *AppProtocol::get_singleton() {
	return singleton;
}

static String cached_socket_path = "";
String AppProtocol::get_socket_path() {
	if (cached_socket_path == "") {
		ProjectSettings *projectSettings = ProjectSettings::get_singleton();
		Variant var = projectSettings->get("app_protocol/socket_name");
		if (var.get_type() != Variant::STRING) {
			print_error("Invalid IPC socket using default");
			cached_socket_path = "godotapp.sock";
		} else {
			cached_socket_path = var;
		}

		cached_socket_path = projectSettings->globalize_path("user://" + cached_socket_path);
	}
	return cached_socket_path;
}

/* Required for when main startup has to check for the settings */
static bool registered_project_settings = false;

/* Register project settings is called by engine but potentially also by main()
 * Hence the static bool */
void AppProtocol::register_project_settings() {
	if (!registered_project_settings) {
		GLOBAL_DEF("app_protocol/enable_app_protocol", false);
		GLOBAL_DEF("app_protocol/protocol_name", "godotapp");
		GLOBAL_DEF("app_protocol/socket_name", "godotapp.sock");
		registered_project_settings = true;
	}
}

bool AppProtocol::is_server_already_running() {
	// connection will be refused if it is - connection will be closed instantly.
	IPCClient client;
	char str[] = "client_init";
	return client.setup_one_shot(AppProtocol::get_socket_path().ascii(), str, sizeof(str));
}

void AppProtocol::start_protocol_handling() {
	ProjectSettings *projectSettings = ProjectSettings::get_singleton();
	if (!(bool)projectSettings->get("app_protocol/enable_app_protocol")) {
		print_error("AppProtocol must be enabled in project settings to start the IPC server");
		return;
	}

	const String protocol_name = projectSettings->get("app_protocol/protocol_name");
	print_verbose("Enabled Protocol Handling for protocol: " + protocol_name);
	// editor is not the server
	if (Engine::get_singleton()->is_editor_hint()) {
		print_error("Editor scripts cannot use the URL handler as it would block users using it.");
		return;
	}

	if (protocol_name.is_empty()) {
		print_error("Protocol name is empty.");
		return;
	}

	if (is_server_already_running()) {
		print_error("You can't run two client protocol handler servers.");
		return;
	}

	// If a server is already registered there is no way to register another protocol until it closes.
	if (this->server == nullptr) {
		OS::get_singleton()->print("Starting IPC server at path: %s\n", AppProtocol::get_socket_path().ascii().get_data());
		OS::get_singleton()->print("Protocol used: %s\n", protocol_name.ascii().get_data());
		this->server = memnew(IPCServer);
		this->server->setup(AppProtocol::get_socket_path().ascii().get_data());
		this->server->add_receive_callback(&AppProtocol::on_server_get_message);
		CompiledPlatform.register_protocol_handler(protocol_name);
	} else {
		print_error("Server is already running, you can't start it twice in a client.");
	}
}

/* Kill server - normally done if OS error or if the app would crash without killing it */
void AppProtocol::kill_server() {
	if (server != nullptr) {
		print_error("Killed server to prevent crash its likely you are running two clients normally and can ignore this error");
		// kill AppProtocol server
		memdelete(server);
		server = nullptr;
	}
}

void AppProtocol::poll_server() {
	if (server) {
		server->poll_update();
	}
}

bool AppProtocol::is_server_running_locally() {
	return get_singleton() && get_singleton()->server;
}

bool AppProtocol::try_client_ipc_connection_deeplink(const char *p_deeplink_uri) {
	IPCClient client;
	ProjectSettings *projectSettings = ProjectSettings::get_singleton();
	const String protocol_name = projectSettings->get("app_protocol/protocol_name");
	OS::get_singleton()->print("Opening IPC client link at socket path: %s\n", AppProtocol::get_socket_path().ascii().get_data());
	OS::get_singleton()->print("Protocol used: %s\n", protocol_name.ascii().get_data());
	// Could be running in another game instance if it is we pass and close down.
	// If our server is up we just close down the game, so this will return false if the server is down, and true if it is up.
	return client.setup_one_shot(AppProtocol::get_socket_path().ascii().get_data(), p_deeplink_uri, strlen(p_deeplink_uri));
}

void AppProtocol::on_server_get_message(const char *p_str, size_t strlen) {
	const String str = p_str;
	// Not passed to the app - used for checking if server is already up.
	if (str.contains("client_init"))
		return;
	get_singleton()->emit_signal(SNAME("on_url_received"), str);
	print_error("Got message from client: " + String(p_str));
}

void AppProtocol::on_os_get_arguments(const List<String> &args) {
	String str;
	for (const String &s : args) {
		str += s;
	}
	get_singleton()->emit_signal(SNAME("on_url_received"), str);
}

bool ProtocolPlatformImplementation::validate_protocol(const String &p_protocol) {
#ifdef TOOLS_ENABLED
	// This warning should be shared between platforms, so putting it here.
	// However, it's not technically related to validation.
	WARN_PRINT("Registering protocols in the editor likely won't work as expected, since it will point to the editor binary. Consider only doing this in exported projects.");
#endif
	// https://datatracker.ietf.org/doc/html/rfc3986#section-6.2.2.1
	// Protocols can't be empty, must be lowercase, must start with a letter,
	// can only contain letters, numbers, '+', '-', and '.' characters.
	if (p_protocol.is_empty()) {
		return false;
	}
	if (p_protocol[0] > 'z' || p_protocol[0] < 'a') {
		ERR_PRINT("Invalid protocol character: " + String::chr(p_protocol[0]) + ". Protocols must start with a lowercase letter.");
		return false;
	}
	for (int i = 1; i < p_protocol.length(); i++) {
		char32_t c = p_protocol[i];
		if (c != '+' && c != '-' && c != '.' && !(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9')) {
			ERR_PRINT("Invalid protocol character: " + String::chr(c) + ". Protocols must be lowercase, must start with a letter, can only contain letters, numbers, '+', '-', and '.' characters.");
			return false;
		}
	}
	return true;
}
