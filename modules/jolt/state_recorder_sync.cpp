#include "state_recorder_sync.h"

#include "modules/network_synchronizer/core/core.h"
#include "modules/network_synchronizer/core/scene_synchronizer_debugger.h"

void StateRecorderSync::WriteBytes(const void *inData, size_t inNumBytes) {
	const uint32_t start = data.size();
	data.resize(data.size() + inNumBytes);
	memcpy(data.ptr() + start, inData, inNumBytes);
}

void StateRecorderSync::ReadBytes(void *outData, size_t inNumBytes) {
	// Verify bounds.
	const size_t next_offset = read_offset + inNumBytes;
	if (next_offset > data.size()) {
		SceneSynchronizerDebugger::singleton()->print(NS::ERROR, "[FATAL] StateRecorderSync is trying to read more bytes than it contains.");
		failed = true;
		return;
	}

	// Read
	if (IsValidating()) {
		// Read data in temporary buffer to compare with current value
		void *tmp_data = JPH_STACK_ALLOC(inNumBytes);
		memcpy(tmp_data, data.ptr() + read_offset, inNumBytes);
		if (memcmp(tmp_data, outData, inNumBytes) != 0) {
			SceneSynchronizerDebugger::singleton()->print(NS::ERROR, "[FATAL] StateRecorderSync missmatch detected.");
			failed = true;
		}
	} else {
		memcpy(outData, data.ptr() + read_offset, inNumBytes);
	}
	read_offset = next_offset;
}

bool StateRecorderSync::IsEOF() const {
	return read_offset >= data.size();
}
bool StateRecorderSync::IsFailed() const {
	return failed;
}

bool StateRecorderSync::IsEqual(const StateRecorderSync &p_other) const {
	if (data.size() != p_other.data.size()) {
		return false;
	}
	return memcmp(data.ptr(), p_other.data.ptr(), data.size()) == 0;
}

void StateRecorderSync::BeginRead() {
	read_offset = 0;
}

void StateRecorderSync::Clear() {
	failed = false;
	read_offset = 0;
	data.clear();
}

void StateRecorderSync::set_data(const Vector<uint8_t> &p_data) {
	failed = false;
	read_offset = 0;
	data = p_data;
}

const LocalVector<uint8_t> &StateRecorderSync::get_data() const {
	return data;
}
