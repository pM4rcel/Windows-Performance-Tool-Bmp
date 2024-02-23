#pragma once

#include <windows.h>
#include <tchar.h>

namespace env{
	constexpr LPCTSTR dir_structure[] = {
		_T("C:\\Facultate\\CSSO\\Week6"),
		_T("C:\\Facultate\\CSSO\\Week6\\date"),
		_T("C:\\Facultate\\CSSO\\Week6\\rezultate"),
		_T("C:\\Facultate\\CSSO\\Week6\\rezultate\\secvential"),
		_T("C:\\Facultate\\CSSO\\Week6\\rezultate\\static"),
		_T("C:\\Facultate\\CSSO\\Week6\\rezultate\\dinamic"),
	};

	constexpr LPCTSTR tests[] = {
		_T("default"),
		_T("static"),
		_T("dynamic")
	};

	constexpr DWORD message_size = 512ul;
	inline TCHAR message[message_size];

	constexpr TCHAR system_info_file_path[] = _T("C:\\Facultate\\CSSO\\Week6\\info.txt");

	constexpr TCHAR bmp_file_default_format[] = _T("C:\\Faculate\\CSSO\\Week6\\rezultate\\secvential\\%s_%s_%s.bmp");
	constexpr TCHAR bmp_file_static_format[] = _T("C:\\Faculate\\CSSO\\Week6\\rezultate\\static\\%s_%s_%s.bmp");
	constexpr TCHAR bmp_file_dynamic_format[] = _T("C:\\Faculate\\CSSO\\Week6\\rezultate\\dinamic\\%s_%s_%s.bmp");

	void init();
}