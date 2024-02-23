#include "system_info.h"

#include <tchar.h>
#include <sddl.h>


namespace {
	constexpr size_t info_size = 2048ull;
	constexpr size_t entity_stringify_size = 512ull;

	constexpr LPCTSTR relation_as_string[] = {
		_T("RelationProcessorCore"),
		_T("RelationNumaNode"),
		_T("RelationCache"),
		_T("RelationProcessorPackage"),
		_T("RelationGroup"),
		_T("RelationProcessorDie"),
		_T("RelationNumaNodeEx"),
		_T("RelationProcessorModule")
	};
}


LPTSTR get_current_user_sid() {
	DWORD user_token_size = 0;
	HANDLE token_handle;

	LPTSTR sid_stringify = nullptr;

	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token_handle)) {
		GetTokenInformation(token_handle, TokenUser, nullptr, NULL, &user_token_size);

		if (user_token_size > 0) {
			const auto user_token_ptr = std::make_unique<BYTE[]>(user_token_size);

			if (GetTokenInformation(token_handle, TokenUser, user_token_ptr.get(), user_token_size, &user_token_size))
				ConvertSidToStringSid(reinterpret_cast<PTOKEN_USER>(user_token_ptr.get())->User.Sid, &sid_stringify);
		}

		CloseHandle(token_handle);
	}

	return sid_stringify;
}


LPTSTR get_sid_of(const WELL_KNOWN_SID_TYPE sid_type) {
	DWORD sid_size = SECURITY_MAX_SID_SIZE;

	const auto sid_raw = LocalAlloc(LPTR, sid_size);

	TCHAR* sid_stringify = nullptr;

	if (CreateWellKnownSid(sid_type, nullptr, sid_raw, &sid_size))
		ConvertSidToStringSid(sid_raw, &sid_stringify);

	return sid_stringify;
}


std::unique_ptr<TCHAR[]> get_system_sids() {
	static constexpr auto sids_info_format_string = _T(
		"SIDs:\r\n"
		"\tCurrent User SID:\t\t%s\r\n"
		"\tEveryone Group SID:\t%s\r\n"
		"\tAdmin Group SID:\t\t%s\r\n\r\n"
	);

	auto info = std::make_unique<TCHAR[]>(info_size);

	LPTSTR sids[] = { get_current_user_sid(), get_sid_of(WinWorldSid), get_sid_of(WinBuiltinAdministratorsSid) };

	_sntprintf_s(info.get(), info_size, _TRUNCATE, sids_info_format_string, sids[0], sids[1], sids[2]);

	for (const auto& sid_string : sids)
		LocalFree(sid_string);

	return info;
}


std::unique_ptr<TCHAR[]> get_ht_info() {
	static constexpr TCHAR processor_info_format[] = _T(
		"\t\tProcessor: %lu\r\n"
		"\t\tProcessor Mask: %llu\r\n"
		"\t\tRelationship: %s\r\n\r\n"
	);

	DWORD buffer_size = 0ul;

	auto info = std::make_unique<TCHAR[]>(info_size);

	_tcscat_s(info.get(), info_size, _T("HT API:\r\n"));

	GetLogicalProcessorInformation(nullptr, &buffer_size);

	if (buffer_size > 0) {
		const auto buffer_raw = std::make_unique<BYTE[]>(buffer_size);

		const auto slp_buffer = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION>(buffer_raw.get());
		
		if (GetLogicalProcessorInformation(slp_buffer, &buffer_size)) {

			TCHAR current_processor_data[entity_stringify_size] = {};
			const DWORD total = buffer_size / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
			for (DWORD index = 0ul; index < total; index++) {
				const auto relation = slp_buffer[index].Relationship;

				_sntprintf_s(current_processor_data, entity_stringify_size, processor_info_format,
					index,
					slp_buffer[index].ProcessorMask,
					relation != RelationAll ? relation_as_string[relation] : _T("RelationAll")
				);

				_tcscat_s(info.get(), info_size, current_processor_data);
			}

			_sntprintf_s(current_processor_data, entity_stringify_size, _T("\t\tTotal Number of Nodes: %lu\r\n\r\n"), total);
			_tcscat_s(info.get(), info_size, current_processor_data);
		}
	}

	return info;
}

std::unique_ptr<TCHAR[]> get_numa_stats() {
	static constexpr TCHAR numa_info_format[] = _T(
		"NUMA API:\r\n"
		"\tProcess Affinity Mask: %llu\r\n"
		"\tSystem Affinity Mask: %llu\r\n"
		"\r\n\tTotal NUMA Nodes: %lu\r\n\r\n"
	);


	DWORD_PTR process_affinity_mask = 0, system_affinity_mask = 0;
	DWORD total_nodes = 0ul;

	auto info = std::make_unique<TCHAR[]>(info_size);

	if (GetProcessAffinityMask(GetCurrentProcess(), &process_affinity_mask, &system_affinity_mask)) {
		UCHAR node;

		for (DWORD index = 0ul; index < sizeof(DWORD_PTR) * 8; index++)
			if (system_affinity_mask & (1ull << index))
				if (GetNumaProcessorNode(static_cast<BYTE>(index), &node))
					total_nodes = max(total_nodes, static_cast<DWORD>(node) + 1);
	}

	_sntprintf_s(info.get(), info_size, _TRUNCATE, numa_info_format,
		process_affinity_mask,
		system_affinity_mask,
		total_nodes
	);

	return info;
}


std::unique_ptr<TCHAR[]> get_cpu_sets(DWORD& total_cpus) {
	static constexpr TCHAR cpu_info_format[] = _T(
		"\t\tCPU Set ID: %lu\r\n"
		"\t\tGroup: %hu\r\n"
		"\t\tLogical Processor Index: %hu\r\n"
		"\t\tCore Index: %hu\r\n"
		"\t\tLast Level Cache Index: %hu\r\n"
		"\t\tNuma Node Index: %hu\r\n"
		"\t\tEfficiency Class: %hu\r\n\r\n"
	);

	total_cpus = 0ul;

	ULONG cpu_sets_buffer_size = 0;

	auto info = std::make_unique<TCHAR[]>(info_size);

	_tcscat_s(info.get(), info_size, _T("CPU Sets:\r\n\tDefault CPUs:\r\n"));

	TCHAR cpus_info[entity_stringify_size] = {};

	GetProcessDefaultCpuSets(GetCurrentProcess(), nullptr, 0, &cpu_sets_buffer_size);

	if (cpu_sets_buffer_size > 0) {
		const auto default_cpu_sets_raw = std::make_unique<BYTE[]>(cpu_sets_buffer_size);
		const auto default_cpus = reinterpret_cast<PULONG>(default_cpu_sets_raw.get());

		if(GetProcessDefaultCpuSets(GetCurrentProcess(), default_cpus, cpu_sets_buffer_size, &cpu_sets_buffer_size)){
			const auto length = cpu_sets_buffer_size / sizeof(PULONG);

			for(DWORD index = 0ul; index < length; index++){
				_sntprintf_s(cpus_info, entity_stringify_size, _T("\t\tProcessor ID: %lu\r\n"), default_cpus[index]);
				_tcscat_s(info.get(), info_size, cpus_info);
			}

			_sntprintf_s(cpus_info, entity_stringify_size, _T("\t\tTotal Number of Default CPUs: %zu\r\n"), length);
			_tcscat_s(info.get(), info_size, cpus_info);
		}
	}
	else
		_tcscat_s(info.get(), info_size, _T("\t\tThere are no default cpus!\r\n\r\n"));

	GetSystemCpuSetInformation(nullptr, 0, &cpu_sets_buffer_size, GetCurrentProcess(), 0);

	_tcscat_s(info.get(), info_size, _T("\tSystem CPUs:\r\n"));

	if (cpu_sets_buffer_size > 0) {
		const auto cpu_sets_raw = std::make_unique<BYTE[]>(cpu_sets_buffer_size);
		const auto cpu_sets_info = reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(cpu_sets_raw.get());

		if (GetSystemCpuSetInformation(cpu_sets_info, cpu_sets_buffer_size, &cpu_sets_buffer_size, GetCurrentProcess(), 0)) {
			total_cpus = cpu_sets_buffer_size / sizeof(SYSTEM_CPU_SET_INFORMATION);

			for (DWORD index = 0ul; index < total_cpus; index++) {
				_sntprintf_s(cpus_info, entity_stringify_size, cpu_info_format,
					cpu_sets_info[index].CpuSet.Id,
					cpu_sets_info[index].CpuSet.Group,
					cpu_sets_info[index].CpuSet.LogicalProcessorIndex,
					cpu_sets_info[index].CpuSet.CoreIndex,
					cpu_sets_info[index].CpuSet.LastLevelCacheIndex,
					cpu_sets_info[index].CpuSet.NumaNodeIndex,
					cpu_sets_info[index].CpuSet.EfficiencyClass);

				_tcscat_s(info.get(), info_size, cpus_info);
			}

			_sntprintf_s(cpus_info, entity_stringify_size, _T("\t\tTotal Number of System CPUs: %lu\r\n\r\n"), total_cpus);
			_tcscat_s(info.get(), info_size, cpus_info);
		}
	}

	return info;
}
