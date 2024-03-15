#include "ipc.h"
#include "socket_implementation.h"

IPCBase::IPCBase() {}
IPCBase::~IPCBase() {}

IPCClient::IPCClient() :
		IPCBase() {}
IPCClient::~IPCClient() {
	SocketImplementation::close(data_socket);
}

IPCServer::IPCServer() :
		IPCBase() {}
IPCServer::~IPCServer() {
	SocketImplementation::close(connection_socket);
}

// called to register the only callback for when data arrives
void IPCBase::add_receive_callback(CallbackDefinition callback) {
	// func pointer to func pointer.
	activeCallback = callback;
}

bool IPCClient::setup(const char *socket_path) {
	//printf("Client Starting socket\n");


	data_socket = SocketImplementation::create_af_unix_socket(name, socket_path);
	if (data_socket == -1) {
		SocketImplementation::perror("Socket creation failed");
		return false;
	}

	if (SocketImplementation::set_non_blocking(data_socket) == -1) {
		SocketImplementation::perror("non blocking failure");
		return false;
	}

	if (SocketImplementation::connect(data_socket, (const struct sockaddr *)&name, sizeof(name)) == -1) {
#ifdef _WIN32
		if (WSAEWOULDBLOCK != WSAGetLastError()) {
			SocketImplementation::perror("connect failure");
			return false;
		}
#else
		SocketImplementation::perror("connect failure");
		return false;
#endif
	}

	return true;
}

bool IPCClient::setup_one_shot(const char *socket_path, const char *str, size_t length) {
	if (IPCClient::setup(socket_path)) {
		// We must send the data to the client requested
		int OK = SocketImplementation::send(data_socket, str, length);
		if (OK == -1) {
			SocketImplementation::perror("cant send message");
			SocketImplementation::close(data_socket);
			return false;
		}

		// clean slate on boot.
		memset(&buffer[0], 0, BufferSize);
		//printf("Waiting for read of client_init [%d] %s\n", __LINE__, __FILE__);

		/* Non blocking */
		int len = SocketImplementation::recv(data_socket, buffer, BufferSize);
		if (len == -1) {
			SocketImplementation::perror("read client socket");
			SocketImplementation::close(data_socket);
			return false;
		}

		if(len == 0)
		{
			SocketImplementation::perror("read buffer is empty!");
			return false;
		}

		buffer[BufferSize - 1] = 0;

		if (strncmp(str, buffer, strlen(str)) != 0) {
			SocketImplementation::perror("comparison buffer result wrong client");
			SocketImplementation::close(data_socket);
			return false;
		}

		SocketImplementation::close(data_socket);
		return true;
	}
	return false;
}

void IPCClient::send_message(const char *str, int n) {
	int OK = SocketImplementation::send(data_socket, str, n);
	if (OK == -1) {
		SocketImplementation::perror("write");
	}
}

bool IPCClient::poll_update() {
	return true;
}

bool IPCServer::setup(const char *socket_path) {
	SocketImplementation::unlink(socket_path);

	//printf("Setting up server connection socket\n");
	connection_socket = SocketImplementation::create_af_unix_socket(name, socket_path);
	if (connection_socket == -1) {
		SocketImplementation::perror("Socket creation failed");
		return false;
	}

	if (SocketImplementation::set_non_blocking(connection_socket) == -1) {
		SocketImplementation::perror("non blocking failure");
		return false;
	}

	//printf("trying to bind connection\n");
	int OK = SocketImplementation::bind(connection_socket, (const struct sockaddr *)&name, sizeof(name));
	if (OK == -1) {
		SocketImplementation::perror("bind");
		return false;
	}

	//printf("Starting listen logic\n");
	OK = SocketImplementation::listen(connection_socket, 8); // assume spamming of new connections
	if (OK == -1) {
		SocketImplementation::perror("listen");
		return false;
	}
	// TODO move to init code ?
#if defined(SO_NOSIGPIPE)
	// Disable SIGPIPE (should only be relevant to stream sockets, but seems to affect UDP too on iOS)
	int par = 1;
	if (setsockopt(connection_socket, SOL_SOCKET, SO_NOSIGPIPE, &par, sizeof(int)) != 0) {
		printf("Unable to turn off SIGPIPE on socket");
	}
#endif

	//printf("started listening for new connections\n");
	return true;
}

/* We process and read the buffer once per tick */
bool IPCServer::poll_update() {
	{
		int ret = SocketImplementation::poll(connection_socket);
		if (ret == -1) {
			perror("poll error");
			return false;
		} else if (ret == EAGAIN || ret == 0) {
			return true; // would block or no connections
		}
	}

	struct sockaddr_storage their_addr;
	socklen_t size = sizeof(their_addr);
	data_socket = SocketImplementation::accept(connection_socket, (struct sockaddr *)&their_addr, &size);

	// Always start with a clean slate
	memset(&buffer[0], 0, BufferSize);

	// both are checked for portability to all OS's.

	// EWOULDBLOCK & EAGAIN sometimes are the same
	// this can produce a warning which is why I used two IF's for the same thing.
	// example: error: logical 'or' of equal expressions [-Werror=logical-op]
	if (IPC_AGAIN(data_socket)) {
		return true; // not an error
	} else if (data_socket == -1) {
		SocketImplementation::perror("socket open failed");
		return false;
	} else {
		//printf("Server accepted connection\n");
	}

	if (SocketImplementation::set_non_blocking(data_socket) == -1) {
		return false;
	}
	//
	/* end server only. */
	int len = SocketImplementation::recv(data_socket, buffer, BufferSize);
	if (len == -1) {
		SocketImplementation::perror("server read");
		return false;
	} else {
		//printf("socket read success\n");
	}

	if (len == 0 || strlen(buffer) != (size_t)len || len < 0) {
		SocketImplementation::perror("invalid buffer size");
		return false;
	}

	/* Buffer must be null terminated */
	buffer[BufferSize - 1] = 0;

	int OK = SocketImplementation::send(data_socket, buffer, strlen(buffer));
	if (OK == -1) {
		SocketImplementation::perror("cant send message");
		SocketImplementation::close(data_socket);
		return false;
	}

	/* Pass data to the application hooked in */
	//printf("Received message from client: %s\n", buffer);
	if (activeCallback) {
		activeCallback(buffer, strlen(buffer));
	}

	SocketImplementation::close(data_socket);

	return true;
}
