#include "env.h"

#include <stdexcept>
#include <format>

void env::init(){
	for (const auto& path : env::dir_structure)
		if (!CreateDirectory(path, nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
			throw std::runtime_error(std::format("[Error C{}] No Permission to create the tree structure!", GetLastError()));

	if (const HANDLE file_handle = CreateFile(env::system_info_file_path, GENERIC_READ, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr); file_handle == INVALID_HANDLE_VALUE)
			throw std::runtime_error(std::format("[Error C{}] No Permission to create the tree structure!", GetLastError()));
		else
			CloseHandle(file_handle);
}
