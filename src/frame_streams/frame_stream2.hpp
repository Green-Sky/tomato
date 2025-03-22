#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

// Frames often consist of:
// - seq id // incremental sequential id, gaps in ids can be used to detect loss
// - or timestamp
// - data // the frame data
// eg:
//struct ExampleFrame {
	//int64_t seq_id {0};
	//std::vector<uint8_t> data;
//};

template<typename FrameType>
struct FrameStream2I {
	virtual ~FrameStream2I(void) {}

	// get number of available frames
	// returns -1 if unknown
	[[nodiscard]] virtual int32_t size(void) = 0;

	// get next frame
	// data sharing? -> no, data is copied for each fsr, if concurency supported
	[[nodiscard]] virtual std::optional<FrameType> pop(void) = 0;

	// returns true if there are readers (or we dont know)
	virtual bool push(const FrameType& value) = 0;
};

template<typename FrameType>
struct FrameStream2SourceI {
	virtual ~FrameStream2SourceI(void) {}
	[[nodiscard]] virtual std::shared_ptr<FrameStream2I<FrameType>> subscribe(void) = 0;
	virtual bool unsubscribe(const std::shared_ptr<FrameStream2I<FrameType>>& sub) = 0;
};

template<typename FrameType>
struct FrameStream2SinkI {
	virtual ~FrameStream2SinkI(void) {}
	[[nodiscard]] virtual std::shared_ptr<FrameStream2I<FrameType>> subscribe(void) = 0;
	virtual bool unsubscribe(const std::shared_ptr<FrameStream2I<FrameType>>& sub) = 0;
};

// typed config
// overload for your frame types

template<typename FrameType>
constexpr bool frameHasTimestamp(void) { return false; }

template<typename FrameType>
constexpr uint64_t frameGetTimestampDivision(void) = delete;

template<typename FrameType>
uint64_t frameGetTimestamp(const FrameType&) = delete;

template<typename FrameType>
constexpr bool frameHasBytes(void) { return false; }

template<typename FrameType>
uint64_t frameGetBytes(const FrameType&) = delete;

