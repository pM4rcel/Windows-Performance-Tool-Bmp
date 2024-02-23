#pragma once

#include "processing.h"
#include <windows.h>
#include <cassert>


class Mediator{
	BYTE* source_;
	SIZE_T offset_, size_;
	HANDLE mutex_;
	SIZE_T assigning_chunks;
public:
	Mediator(const BYTE* source, const TCHAR* name, const BITMAPFILEHEADER& bmp, const DWORD& workers) : source_(const_cast<BYTE*>(source)), offset_(bmp.bfOffBits), size_(bmp.bfSize) {
		mutex_ = CreateMutex(nullptr, FALSE, name);
		assert(mutex_);

		assigning_chunks = min(static_cast<SIZE_T>(floor(static_cast<double>(size_ - offset_) / static_cast<double>(WORK_BUFFER_SIZE * workers) * 0.4)), 10ull);
	}

	~Mediator(){
		CloseHandle(mutex_);
	}

	BOOL get_next_chunk(SIZE_T& size, SIZE_T& offset, BYTE*& buffer) {
		WaitForSingleObject(mutex_, INFINITE);
		const auto status = offset_ != size_;

		if (status) {
			size = offset_ + WORK_BUFFER_SIZE * assigning_chunks > size_ ? size_ - offset_ : assigning_chunks * WORK_BUFFER_SIZE;

			buffer = source_ + offset_;

			offset = offset_;

			offset_ += size;
		}

		ReleaseMutex(mutex_);

		return status;
	}
};

