#include "processing.h"
#include "sequential.h"

#include "tchar.h"

#include <chrono>

void seq_strategy(const TCHAR* path, const BYTE* source, const BITMAPFILEHEADER& bmp_stats, double& duration) {
	static constexpr TCHAR output_format_raw[] = _T("C:\\Facultate\\CSSO\\Week6\\rezultate\\secvential\\%s_%s.bmp");
	static constexpr TCHAR output_format_done[] = _T("C:\\Facultate\\CSSO\\Week6\\rezultate\\secvential\\%s_%s_%f.bmp");

	const TCHAR* basename = get_file_name(path);

	const auto gray_file_name = build_path(output_format_raw, basename, _T("gray-scale")), reversed_file_name =
		build_path(output_format_raw, basename, _T("reversed"));

	const auto file_gray_scaled_handle = CreateFile(gray_file_name.get(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	const auto file_reversed_handle = CreateFile(reversed_file_name.get(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (file_gray_scaled_handle == INVALID_HANDLE_VALUE) {
		delete basename;
		return;
	}

	if (file_reversed_handle == INVALID_HANDLE_VALUE) {
		delete basename;
		CloseHandle(file_gray_scaled_handle);
		return;
	}

	WriteFile(file_gray_scaled_handle, source, 54, nullptr, nullptr);
	WriteFile(file_reversed_handle, source, 54, nullptr, nullptr);

	SIZE_T offset = bmp_stats.bfOffBits;

	const auto gray_scaled = std::make_unique<BYTE[]>(WORK_BUFFER_SIZE), reversed = std::make_unique<BYTE[]>(WORK_BUFFER_SIZE);

	const auto start = std::chrono::high_resolution_clock::now();

	while (offset < bmp_stats.bfSize) {
		const SIZE_T chunk = offset + WORK_BUFFER_SIZE > bmp_stats.bfSize ? bmp_stats.bfSize - offset : WORK_BUFFER_SIZE;

		transform(chunk, source + offset, gray_scaled.get(), reversed.get());

		WriteFile(file_gray_scaled_handle, gray_scaled.get(), static_cast<DWORD>(chunk), nullptr, nullptr);
		WriteFile(file_reversed_handle, reversed.get(), static_cast<DWORD>(chunk), nullptr, nullptr);

		offset += WORK_BUFFER_SIZE;
	}

	duration = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();

	CloseHandle(file_gray_scaled_handle);
	CloseHandle(file_reversed_handle);

	const auto done_gray_file = build_path(output_format_done, basename, _T("gray-scale"), duration), done_reverse_file =
		build_path(output_format_done, basename, _T("reverse"), duration);

	MoveFile(gray_file_name.get(), done_gray_file.get());
	MoveFile(reversed_file_name.get(), done_reverse_file.get());

	apply_sec(done_gray_file.get());
	apply_sec(done_reverse_file.get());

	delete basename;
}