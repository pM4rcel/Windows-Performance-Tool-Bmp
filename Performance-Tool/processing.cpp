#include "processing.h"

#include <cassert>
#include <chrono>
#include <tchar.h>
#include <AclAPI.h>

namespace{
	constexpr SIZE_T info_size = 1024ull;

	constexpr LPCTSTR compressions[] = {
		_T("None"),
		_T("RLE 8-bit/pixel"),
		_T("RLE 4-bit/pixel"),
		_T("OS22XBITMAPHEADER Huffman 1D"),
		_T("OS22xBITMAPHEADER RLE-24"),
		_T("PNG"),
		_T("RGBA bit field masks")
	};

	constexpr TCHAR default_compression[] = _T("Unknown");
}

void replace(TCHAR* str, const TCHAR to_replace, const TCHAR with) {
	TCHAR* found_ptr = _tcschr(str, to_replace);
	while (found_ptr) {
		*found_ptr = with;

		found_ptr = _tcschr(found_ptr + 1, to_replace);
	}
}

std::unique_ptr<TCHAR[]> build_path(const LPCTSTR format, const TCHAR* name, const TCHAR* operation) {
	auto raw_path = std::make_unique<TCHAR[]>(MAX_PATH);

	_sntprintf_s(raw_path.get(), MAX_PATH, _TRUNCATE, format, name, operation);

	return raw_path;
}

std::unique_ptr<TCHAR[]> build_path(const LPCTSTR format, const TCHAR* name, const TCHAR* operation, const double& duration) {
	auto raw_path = std::make_unique<TCHAR[]>(MAX_PATH);

	_sntprintf_s(raw_path.get(), MAX_PATH, _TRUNCATE, format, name, operation, duration);

	return raw_path;
}

void apply_sec(const TCHAR* path){
	HANDLE token_handle;

	OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token_handle);

	DWORD buffer_size = 0ul;

	GetTokenInformation(token_handle, TokenUser, NULL, 0, &buffer_size);

	const auto token_user_ptr = reinterpret_cast<PTOKEN_USER>(new BYTE[buffer_size]);

	GetTokenInformation(token_handle, TokenUser, token_user_ptr, buffer_size, &buffer_size);

	EXPLICIT_ACCESS access{};

	access.grfAccessPermissions = GENERIC_READ;
	access.grfAccessMode = SET_ACCESS;
	access.grfInheritance = NO_INHERITANCE;
	access.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	access.Trustee.TrusteeType = TRUSTEE_IS_USER;
	access.Trustee.ptstrName = static_cast<LPTSTR>(token_user_ptr->User.Sid);

	PACL new_acl{};
	SetEntriesInAcl(1, &access, nullptr, &new_acl);

	SetNamedSecurityInfo(const_cast<TCHAR*>(path), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, new_acl, nullptr);

	if (!new_acl)
		LocalFree(new_acl);

	delete[] token_user_ptr;
	CloseHandle(token_handle);
}

TCHAR* get_file_name(const TCHAR* path, const LPCTSTR extension){
	const SIZE_T path_length = _tcslen(path) - _tcslen(_tcsrchr(path, _T('\\')) + 1);
	const SIZE_T name_length = _tcslen(path + path_length) - _tcslen(extension);

	const auto name = new TCHAR[name_length + 1]{};
	_tcsncpy_s(name, name_length + 1, _tcsrchr(path, _T('\\')) + 1, name_length);

	return name;
}

std::unique_ptr<TCHAR[]> bitmap_stats_stringify(const BITMAPFILEHEADER* bmp_file_header, const BITMAPINFOHEADER* bmp_metadata_header){
	static constexpr TCHAR bmp_file_stats_format[] = _T(
		"File Header:\r\n"
		"\tType: %hu\r\n"
		"\tSize: %luMB\r\n"
		"\tReserved 1: %hu\r\n"
		"\tReserved 2: %hu\r\n"
		"\tOffset: %lu\r\n\r\n"
	);

	static constexpr TCHAR bmp_metadata_format[] = _T(
		"Bitmap Header:\r\n"
		"\tHeader Size: %lu\r\n"
		"\tWidth: %lu\r\n"
		"\tHeight: %lu\r\n"
		"\tColor Planes: %hu\r\n"
		"\tBits Per Pixel: %hu\r\n"
		"\tCompression Method: %s\r\n"
		"\tBitmap Size: %lu\r\n"
		"\tHorizontal Resolution: %lu\r\n"
		"\tVertical Resolution: %lu\r\n"
		"\tNumber of Colors: %lu\r\n"
		"\tNumber of Important Colors: %lu\r\n"
	);

	auto stats = std::make_unique<TCHAR[]>(info_size);
	const auto stringify = new TCHAR[info_size]{};

	_sntprintf_s(stringify, info_size, _TRUNCATE, bmp_file_stats_format,
		bmp_file_header->bfType,
		bmp_file_header->bfSize / 1048576,
		bmp_file_header->bfReserved1,
		bmp_file_header->bfReserved2,
		bmp_file_header->bfOffBits
	);

	_tcscat_s(stats.get(), info_size, stringify);

	_sntprintf_s(stringify, info_size, _TRUNCATE, bmp_metadata_format,
		bmp_metadata_header->biSize,
		bmp_metadata_header->biWidth,
		bmp_metadata_header->biHeight,
		bmp_metadata_header->biPlanes,
		bmp_metadata_header->biBitCount,
		bmp_metadata_header->biCompression < 7 ? compressions[bmp_metadata_header->biCompression] : default_compression,
		bmp_metadata_header->biSizeImage,
		bmp_metadata_header->biXPelsPerMeter,
		bmp_metadata_header->biYPelsPerMeter,
		bmp_metadata_header->biClrUsed,
		bmp_metadata_header->biClrImportant
	);

	_tcscat_s(stats.get(), info_size, stringify);

	delete[] stringify;

	return stats;
}


BYTE* load_bmp(const TCHAR* path, BYTE* file_stats, BYTE* bitmap_stats) {
	assert(_tcscmp(_tcsrchr(path, _T('.')), _T(".bmp")) == 0);

	const HANDLE bmp_file_handle = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	BYTE* buffer = nullptr;

	if (bmp_file_handle != INVALID_HANDLE_VALUE) {

		// extract metadata about bmp file
		BY_HANDLE_FILE_INFORMATION bmp_stats;
		GetFileInformationByHandle(bmp_file_handle, &bmp_stats);

		// extract size in bytes by merging fileSizeHigh and fileSizeLow
		SIZE_T bmp_size = (static_cast<SIZE_T>(bmp_stats.nFileSizeHigh) << 32) | bmp_stats.nFileSizeLow;

		if (
			const auto bmp_mapping = CreateFileMapping(bmp_file_handle, nullptr, PAGE_WRITECOPY, bmp_stats.nFileSizeHigh, bmp_stats.nFileSizeLow, _tcsrchr(path, '\\') + 1);
			bmp_mapping
			) {
			buffer = static_cast<BYTE*>(MapViewOfFile(bmp_mapping, FILE_MAP_COPY, 0, 0, 0));

			CloseHandle(bmp_mapping);

			if(buffer){
				if(file_stats)
					memcpy(file_stats, buffer, sizeof(BITMAPFILEHEADER));
				if (bitmap_stats)
					memcpy(bitmap_stats, buffer + sizeof(BITMAPFILEHEADER), sizeof(BITMAPINFOHEADER));
			}
		}

		CloseHandle(bmp_file_handle);
	}

	return buffer;
}


void transform(const SIZE_T size, const BYTE* source, BYTE* gray_scaled, BYTE* reversed) {
	for (SIZE_T index = 0; index < size; index += 4) {
		const BYTE rgb[] = {source[index], source[index + 1], source[index + 2]};

		gray_scaled[index] = gray_scaled[index + 1] = gray_scaled[index + 2] = 0.299 * rgb[0] + 0.587 * rgb[1] + 0.114 * rgb[2];
		gray_scaled[index + 3] = source[index + 3];
		

		reversed[index] = 0xFF - source[index];
		reversed[index + 1] = 0xFF - source[index + 1];
		reversed[index + 2] = 0xFF - source[index + 2];
		reversed[index + 3] = source[index + 3];
	}
}

BOOL compare_bitmaps(const TCHAR* st_path, const TCHAR* nd_path){
	// In D:\\ I have more space
	static constexpr TCHAR result_bmp_filename_format[] = _T("D:\\%s-%s.bmp");

	const HANDLE st_bmp_file_handle = CreateFile(st_path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (st_bmp_file_handle == INVALID_HANDLE_VALUE)
		return FALSE;

	const HANDLE nd_bmp_file_handle = CreateFile(nd_path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (nd_bmp_file_handle == INVALID_HANDLE_VALUE) {
		CloseHandle(st_bmp_file_handle);
		return FALSE;
	}

	BYTE st_bmp_metadata[54]{}, nd_bmp_metadata[54]{};

	const auto* st_file_stats = reinterpret_cast<BITMAPFILEHEADER*>(st_bmp_metadata), *nd_file_stats = reinterpret_cast<BITMAPFILEHEADER*>(nd_bmp_metadata);
	const auto* st_bmp_stats = reinterpret_cast<BITMAPINFOHEADER*>(st_bmp_metadata + sizeof(BITMAPFILEHEADER)), *nd_bmp_stats = reinterpret_cast<BITMAPINFOHEADER*>(st_bmp_metadata + sizeof(BITMAPFILEHEADER));
	 
	ReadFile(st_bmp_file_handle, st_bmp_metadata, sizeof(st_bmp_metadata), nullptr, nullptr);
	ReadFile(nd_bmp_file_handle, nd_bmp_metadata, sizeof(nd_bmp_metadata), nullptr, nullptr);

	DWORD size = st_file_stats->bfSize, offset = st_file_stats->bfOffBits;

	TCHAR* st_bmp_name = get_file_name(st_path), * nd_bmp_name = get_file_name(nd_path);

	replace(st_bmp_name, _T('.'), _T('_'));
	replace(nd_bmp_name, _T('.'), _T('_'));

	TCHAR new_name[MAX_PATH];
	

	_sntprintf_s(new_name, MAX_PATH, _TRUNCATE, result_bmp_filename_format, st_bmp_name, nd_bmp_name);

	delete[] st_bmp_name;
	delete[] nd_bmp_name;

	const HANDLE comparison_result_file_handle = CreateFile(new_name, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	if(comparison_result_file_handle == INVALID_HANDLE_VALUE){
		CloseHandle(st_bmp_file_handle);
		CloseHandle(nd_bmp_file_handle);
		return FALSE;
	}

	// Write the first file header data and place the second file to the same offset
	WriteFile(comparison_result_file_handle, st_bmp_metadata, 54, nullptr, nullptr);
	SetFilePointer(nd_bmp_file_handle, 54, nullptr, FILE_BEGIN);

	const auto st_buffer = std::make_unique<BYTE[]>(0x4000), nd_buffer = std::make_unique<BYTE[]>(0x4000);
	DWORD st_read_bytes, nd_read_bytes;

	while(ReadFile(st_bmp_file_handle, st_buffer.get(), 0x4000, &st_read_bytes, nullptr) && st_read_bytes > 0){

		ReadFile(nd_bmp_file_handle, nd_buffer.get(), 0x4000, &nd_read_bytes, nullptr);

		for(DWORD index = 0; index < st_read_bytes; index += 4){
			st_buffer[index] -= nd_buffer[index];
			st_buffer[index + 1] -= nd_buffer[index + 1];
			st_buffer[index + 2] -= nd_buffer[index + 2];
		}

		WriteFile(comparison_result_file_handle, st_buffer.get(), st_read_bytes, nullptr, nullptr);
	}

	CloseHandle(st_bmp_file_handle);
	CloseHandle(nd_bmp_file_handle);
	CloseHandle(comparison_result_file_handle);

	return TRUE;
}


