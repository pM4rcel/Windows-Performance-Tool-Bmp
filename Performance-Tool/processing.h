#pragma once

#include <windows.h>
#include <memory>
#include <tchar.h>

#define WORK_BUFFER_SIZE 0x4000

std::unique_ptr<TCHAR[]> bitmap_stats_stringify(const BITMAPFILEHEADER*, const BITMAPINFOHEADER*);

void transform(const SIZE_T, const BYTE*, BYTE*, BYTE*);

TCHAR* get_file_name(const TCHAR* path, const LPCTSTR extension = _T(".bmp"));

BYTE* load_bmp(const TCHAR*, BYTE* = nullptr, BYTE* = nullptr);

void apply_sec(const TCHAR*);

std::unique_ptr<TCHAR[]> build_path(const LPCTSTR, const TCHAR*, const TCHAR*);

std::unique_ptr<TCHAR[]> build_path(const LPCTSTR, const TCHAR*, const TCHAR*, const double&);

BOOL compare_bitmaps(const TCHAR*, const TCHAR*);
