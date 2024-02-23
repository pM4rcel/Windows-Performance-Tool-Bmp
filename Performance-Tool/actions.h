#pragma once

#include <memory>
#include <windows.h>

std::unique_ptr<TCHAR[]> get_file(HWND);

void log_data(const HWND&, const TCHAR*);
const std::unique_ptr<TCHAR[]>& log_data(const HWND&, const std::unique_ptr<TCHAR[]>&);

void write_data_in(const TCHAR*, const std::unique_ptr<TCHAR[]>&, const DWORD);

//void show_image_in(const HWND&, const HBITMAP&);

void load_image(const TCHAR*, const HWND&, HBITMAP&);
