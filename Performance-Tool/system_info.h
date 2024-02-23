#pragma once

#include <windows.h>
#include <memory>


LPTSTR get_current_user_sid();

std::unique_ptr<TCHAR[]> get_system_sids();

std::unique_ptr<TCHAR[]> get_ht_info();

std::unique_ptr<TCHAR[]> get_cpu_sets(DWORD&);

std::unique_ptr<TCHAR[]> get_numa_stats();
