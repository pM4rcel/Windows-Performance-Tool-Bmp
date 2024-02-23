#include "actions.h"

#include <tchar.h>


std::unique_ptr<TCHAR[]> get_file(const HWND owner) {
	auto file_name = std::make_unique<TCHAR[]>(MAX_PATH);

	OPENFILENAME open_file_name{};

	open_file_name.lStructSize = sizeof(OPENFILENAME);
	open_file_name.hwndOwner = owner;
	open_file_name.lpstrFile = file_name.get();
	open_file_name.nMaxFile = MAX_PATH;
	open_file_name.lpstrFilter = _T("Bitmap Files\0*.bmp\0All Files\0*.*\0");
	open_file_name.nFilterIndex = 1ul;
	open_file_name.lpstrFileTitle = nullptr;
	open_file_name.nMaxFileTitle = 0;
	open_file_name.lpstrInitialDir = nullptr;
	open_file_name.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	GetOpenFileName(&open_file_name);

	return file_name;
}

const std::unique_ptr<TCHAR[]>& log_data(const HWND& window_handle, const std::unique_ptr<TCHAR[]>& data){
	const auto text_length = GetWindowTextLength(window_handle);

	SendMessage(window_handle, EM_SETSEL, static_cast<WPARAM>(text_length), static_cast<LPARAM>(text_length));

	SendMessage(window_handle, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(data.get()));

	return data;
}

void log_data(const HWND& window_handle, const TCHAR* data) {
	const auto text_length = GetWindowTextLength(window_handle);

	SendMessage(window_handle, EM_SETSEL, static_cast<WPARAM>(text_length), static_cast<LPARAM>(text_length));

	SendMessage(window_handle, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(data));
}

void write_data_in(const TCHAR* file_path, const std::unique_ptr<TCHAR[]>& data, const DWORD creation_disposition = FILE_APPEND_DATA){
	if(const HANDLE file_handle = CreateFile(file_path, FILE_APPEND_DATA, 0, nullptr, creation_disposition, FILE_ATTRIBUTE_NORMAL, nullptr)){
		const auto data_size = static_cast<DWORD>(_tcslen(data.get()) * sizeof(TCHAR));
		WriteFile(file_handle, data.get(), data_size, nullptr, nullptr);
		CloseHandle(file_handle);
	}
}

//void show_image_in(const HWND& window_handle, const HBITMAP& bmp_handle) {
//	const HDC hdc = GetDC(window_handle), bmp_hdc = CreateCompatibleDC(hdc);
//
//	const auto bmp_old = static_cast<HBITMAP>(SelectObject(bmp_hdc, bmp_handle));
//
//	BITMAP bmp;
//	GetObject(bmp_handle, sizeof(BITMAP), &bmp);
//
//	RECT rect;
//	GetWindowRect(window_handle, &rect);
//
//
//	StretchBlt(hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, bmp_hdc, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
//	SelectObject(bmp_hdc, bmp_old);
//	ReleaseDC(window_handle ,hdc);
//}

void load_image(const TCHAR* path, const HWND& window, HBITMAP& bmp){
	if (bmp) {
		DeleteObject(bmp);
		bmp = nullptr;
	}
	bmp = static_cast<HBITMAP>(LoadImage(nullptr, path, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE));

	if (!bmp)
		MessageBox(window, _T("Failed to load bitmap!"), _T("Error"), MB_OK);
}
