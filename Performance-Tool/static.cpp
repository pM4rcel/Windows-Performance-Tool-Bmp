#include "static.h"
#include "processing.h"
#include "actions.h"
#include <chrono>

struct WorkerParams{
	HANDLE gray, reverse;
	BYTE* source;
	SIZE_T pointer;
	SIZE_T size;
};


DWORD static_worker(LPVOID params) {
	const auto data = static_cast<WorkerParams*>(params);

	const auto gray_buffer = std::make_unique<BYTE[]>(WORK_BUFFER_SIZE), reverse_buffer = std::make_unique<BYTE[]>(WORK_BUFFER_SIZE);

	DWORD offset = 0ul;

	SetFilePointer(data->reverse, static_cast<LONG>(data->pointer), nullptr, FILE_BEGIN);
	SetFilePointer(data->gray, static_cast<LONG>(data->pointer), nullptr, FILE_BEGIN);
	while (offset < data->size) {
		const SIZE_T chunk = offset + WORK_BUFFER_SIZE > data->size ? data->size - offset : WORK_BUFFER_SIZE;

		transform(chunk, data->source + offset, gray_buffer.get(), reverse_buffer.get());

		WriteFile(data->gray, gray_buffer.get(), static_cast<DWORD>(chunk), nullptr, nullptr);
		WriteFile(data->reverse, reverse_buffer.get(), static_cast<DWORD>(chunk), nullptr, nullptr);

		offset += WORK_BUFFER_SIZE;
	}

	CloseHandle(data->reverse);
	CloseHandle(data->gray);

	delete data;

	return EXIT_SUCCESS;
}

void static_strategy(const TCHAR* path, const BYTE* source, const BITMAPFILEHEADER& bmp_stats, double& duration, DWORD& cpus, HWND window_handle){
	static constexpr TCHAR output_format_raw[] = _T("C:\\Facultate\\CSSO\\Week6\\rezultate\\static\\%s_%s.bmp");
	static constexpr TCHAR output_format_done[] = _T("C:\\Facultate\\CSSO\\Week6\\rezultate\\static\\%s_%s_%f.bmp");


	const TCHAR* basename = get_file_name(path);

	const auto gray_file_name = build_path(output_format_raw, basename, _T("gray-scale")), reversed_file_name =
		build_path(output_format_raw, basename, _T("reversed"));

	const auto file_gray_scaled_handle = CreateFile(gray_file_name.get(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	const auto file_reversed_handle = CreateFile(reversed_file_name.get(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

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

	SetFilePointer(file_reversed_handle, bmp_stats.bfSize, nullptr, FILE_BEGIN);
	SetEndOfFile(file_reversed_handle);

	SetFilePointer(file_gray_scaled_handle, bmp_stats.bfSize, nullptr, FILE_BEGIN);
	SetEndOfFile(file_gray_scaled_handle);

	CloseHandle(file_gray_scaled_handle);
	CloseHandle(file_reversed_handle);

	DWORD offset = bmp_stats.bfOffBits;
	const DWORD worker_number = cpus << 1;

	const DWORD total_chunks = (bmp_stats.bfSize - bmp_stats.bfOffBits) / WORK_BUFFER_SIZE;
	const auto chunks_per_worker = static_cast<DWORD>(floor(total_chunks / static_cast<double>(worker_number)));

	const auto threads_handles = new HANDLE[worker_number];

	TCHAR message[MAX_PATH]{};

	SIZE_T chunk = static_cast<SIZE_T>(chunks_per_worker) * WORK_BUFFER_SIZE;

	const auto start = std::chrono::high_resolution_clock::now();

	for(DWORD index = 0; index < worker_number; index++){
		DWORD thread_id;

		if (index == worker_number - 1)
			chunk = bmp_stats.bfSize - offset;

		const auto parameters = new WorkerParams{
			CreateFile(gray_file_name.get(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr),
			CreateFile(reversed_file_name.get(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr),
			const_cast<BYTE*>(source + offset), offset,chunk };
		
		if(threads_handles[index] = CreateThread(nullptr, 0, static_worker, parameters, 0, &thread_id);
			threads_handles[index])
		{
			_sntprintf_s(message, MAX_PATH, _TRUNCATE, _T("Started Thread: %lu\r\n"), thread_id);
			log_data(window_handle, message);
		}else
		{
			_sntprintf_s(message, MAX_PATH, _TRUNCATE, _T("Failed to start thread at iteration: %lu\r\n"), index);
			log_data(window_handle, message);
		}


		offset += chunk;
	}

	WaitForMultipleObjects(worker_number, threads_handles, TRUE, INFINITE);

	duration = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();

	const auto done_gray_file = build_path(output_format_done, basename, _T("gray-scale"), duration), done_reverse_file =
		build_path(output_format_done, basename, _T("reverse"), duration);

	MoveFile(gray_file_name.get(), done_gray_file.get());
	MoveFile(reversed_file_name.get(), done_reverse_file.get());

	apply_sec(done_gray_file.get());
	apply_sec(done_reverse_file.get());

	delete basename;
	delete[] threads_handles;
}