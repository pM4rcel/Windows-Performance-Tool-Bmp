#include "dynamic.h"
#include "processing.h"
#include "actions.h"

#include <chrono>

#include "Mediator.h"

struct DynamicWorkerParam{
	HANDLE gray, reverse;
	Mediator* mediator;
};

DWORD worker(LPVOID params) {
	const auto data = static_cast<DynamicWorkerParam*>(params);

	const auto gray_buffer = std::make_unique<BYTE[]>(WORK_BUFFER_SIZE), reverse_buffer = std::make_unique<BYTE[]>(WORK_BUFFER_SIZE);

	SIZE_T chunk_size, file_offset;
	BYTE* chunk = nullptr;
	while (data->mediator->get_next_chunk(chunk_size, file_offset, chunk)) {
		SetFilePointer(data->reverse, static_cast<LONG>(file_offset), nullptr, FILE_BEGIN);
		SetFilePointer(data->gray, static_cast<LONG>(file_offset), nullptr, FILE_BEGIN);

		SIZE_T buffer_offset = 0;
		while (buffer_offset < chunk_size) {
			const SIZE_T buffer_chunk = buffer_offset + WORK_BUFFER_SIZE > chunk_size ? chunk_size - buffer_offset : WORK_BUFFER_SIZE;

			transform(buffer_chunk, chunk + buffer_offset, gray_buffer.get(), reverse_buffer.get());

			WriteFile(data->gray, gray_buffer.get(), static_cast<DWORD>(buffer_chunk), nullptr, nullptr);
			WriteFile(data->reverse, reverse_buffer.get(), static_cast<DWORD>(buffer_chunk), nullptr, nullptr);

			buffer_offset += buffer_chunk;
		}

	}

	CloseHandle(data->reverse);
	CloseHandle(data->gray);

	delete data;

	return EXIT_SUCCESS;
}

void dynamic_strategy(const TCHAR* path, const BYTE* source, const BITMAPFILEHEADER& bmp_stats, double& duration, DWORD& cpus, HWND window_handle){
	static constexpr TCHAR output_format_raw[] = _T("C:\\Facultate\\CSSO\\Week6\\rezultate\\dinamic\\%s_%s.bmp");
	static constexpr TCHAR output_format_done[] = _T("C:\\Facultate\\CSSO\\Week6\\rezultate\\dinamic\\%s_%s_%f.bmp");
	static constexpr TCHAR mutex_name[] = _T("DynamicBMPProcsesorMutex");


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

	SetFilePointer(file_reversed_handle, static_cast<LONG>(bmp_stats.bfSize), nullptr, FILE_BEGIN);
	SetEndOfFile(file_reversed_handle);

	SetFilePointer(file_gray_scaled_handle, static_cast<LONG>(bmp_stats.bfSize), nullptr, FILE_BEGIN);
	SetEndOfFile(file_gray_scaled_handle);

	CloseHandle(file_gray_scaled_handle);
	CloseHandle(file_reversed_handle);

	const DWORD workers = cpus << 1;

	Mediator mediator(source, mutex_name, bmp_stats, workers);

	const auto threads_handles = std::make_unique<HANDLE[]>(workers);

	TCHAR message[MAX_PATH]{};

	const auto start = std::chrono::high_resolution_clock::now();

	for (DWORD index = 0; index < workers; index++) {
		DWORD thread_id;

		const auto parameters = new DynamicWorkerParam{
			CreateFile(gray_file_name.get(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr),
			CreateFile(reversed_file_name.get(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr),
			&mediator
		};

		if (threads_handles.get()[index] = CreateThread(nullptr, 0, worker, parameters, 0, &thread_id);
			threads_handles.get()[index]){
			_sntprintf_s(message, MAX_PATH, _TRUNCATE, _T("Started Thread: %lu\r\n"), thread_id);
			log_data(window_handle, message);
		}
		else{
			_sntprintf_s(message, MAX_PATH, _TRUNCATE, _T("Failed to start thread at iteration: %lu\r\n"), index);
			log_data(window_handle, message);
		}
	}

	WaitForMultipleObjects(workers, threads_handles.get(), TRUE, INFINITE);

	duration = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();

	const auto done_gray_file = build_path(output_format_done, basename, _T("gray-scale"), duration), done_reverse_file =
		build_path(output_format_done, basename, _T("reverse"), duration);

	MoveFile(gray_file_name.get(), done_gray_file.get());
	MoveFile(reversed_file_name.get(), done_reverse_file.get());

	apply_sec(done_gray_file.get());
	apply_sec(done_reverse_file.get());

	delete basename;
}