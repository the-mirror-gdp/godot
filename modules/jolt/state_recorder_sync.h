
#ifndef STATE_RECORDER_SYNC_H
#define STATE_RECORDER_SYNC_H

#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Jolt.h"

#include "Jolt/Physics/StateRecorder.h"
#include "core/templates/local_vector.h"

class StateRecorderSync final : public JPH::StateRecorder {
public:
	StateRecorderSync() = default;
	StateRecorderSync(StateRecorderSync &&p_recorder) :
			StateRecorder(p_recorder), data(std::move(p_recorder.data)) {}

	virtual void WriteBytes(const void *inData, size_t inNumBytes) override;
	virtual void ReadBytes(void *outData, size_t inNumBytes) override;

	virtual bool IsEOF() const override;
	virtual bool IsFailed() const override;

	bool IsEqual(const StateRecorderSync &p_other) const;

	void BeginRead();

	void Clear();

	void set_data(const Vector<uint8_t> &p_data);
	const LocalVector<uint8_t> &get_data() const;

private:
	bool failed = false;
	int64_t read_offset = 0;
	LocalVector<uint8_t> data;
};

#endif // STATE_RECORDER_SYNC_H
