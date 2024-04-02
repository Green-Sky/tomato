#pragma once

#include <solanaceae/file/file2.hpp>

#include <memory>

#include <zstd.h>

// zstd compression wrapper over another file
// WARNING: only supports sequential writes
struct File2ZSTDW : public File2I {
	File2I& _real_file;

	// TODO: hide this detail?
	std::unique_ptr<ZSTD_CCtx, decltype(&ZSTD_freeCCtx)> _cctx{ZSTD_createCCtx(), &ZSTD_freeCCtx};

	File2ZSTDW(File2I& real);
	virtual ~File2ZSTDW(void);

	bool isGood(void) override;

	// for simplicity and potential future seekability each write is its own frame
	bool write(const ByteSpan data, int64_t pos = -1) override;
	std::variant<ByteSpan, std::vector<uint8_t>> read(uint64_t size, int64_t pos = -1) override;
};

// zstd decompression wrapper over another file
// WARNING: only supports sequential reads
// TODO: add seeking support (use frames)
struct File2ZSTDR : public File2I {
	File2I& _real_file;

	// TODO: hide this detail?
	std::unique_ptr<ZSTD_DCtx, decltype(&ZSTD_freeDCtx)> _dctx{ZSTD_createDCtx(), &ZSTD_freeDCtx};
	std::vector<uint8_t> _in_buffer;
	std::vector<uint8_t> _out_buffer;
	std::vector<uint8_t> _decompressed_buffer; // retains decompressed unread data between read() calls
	ZSTD_inBuffer _z_input{nullptr, 0, 0};

	File2ZSTDR(File2I& real);
	virtual ~File2ZSTDR(void) {}

	bool isGood(void) override;

	bool write(const ByteSpan data, int64_t pos = -1) override;
	std::variant<ByteSpan, std::vector<uint8_t>> read(uint64_t size, int64_t pos = -1) override;

	private:
		bool feedInput(std::variant<ByteSpan, std::vector<uint8_t>>&& read_buff);
};

