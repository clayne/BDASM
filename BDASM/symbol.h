#pragma once

#include <cstdint>
#include <map>
#include <vector>


#include <Windows.h>
namespace symbol_flag
{
	typedef uint32_t type;
	type none = 0;

	// Indicates that the address is relative to
	// These are used to key into the map and find symbols.
	// After that they dont matter.
	//
	type base = (1 << 0);

	// import_lookup_key uses this to mask to only the values used in the key.
	//
	type key_mask = (1 << 0);


	// An instruction or other isntrument set this symbol after final placement and the address is valid.
	// If this isn't set upon recompilation... something is very wrong.
	//
	type valid = (1 << 1);


	// The type descriptor for debug purposes i think...
	// Will probably remove this later
	//
	type type_export = (1 << 2);
	type type_import = (1 << 3);


	type mask = 0xffffffff;
};

// These symbols are basically an intermediate between two things. For example an instruction and another isntruction
// One instruction sets the rva once its placed in its final destination, and the other reads the rva to calculate a needed delta 
//
class symbol_t
{
public:

	// Some rva that represents where the symbol isa. Set once its placed.
	//
	uint64_t rva;

	// 
	//
	symbol_flag::type flags;

	explicit symbol_t(symbol_flag::type flags, uint64_t raw_address)
		: flags(flags), rva(raw_address) {}

	explicit symbol_t(symbol_t const& to_copy)
		: flags(to_copy.flags), rva(to_copy.rva) {}
};

#define make_symbol_map_key(macro_flag, macro_addr)															\
static_cast<uint64_t>(																						\
		(																									\
			((static_cast<uint64_t>(macro_flag) & static_cast<uint64_t>(symbol_flag::key_mask)) << 32)		\
			|																								\
			static_cast<uint64_t>(macro_addr & 0xffffffff)													\
		)																									\
	)
class symbol_table_t
{
	// Lookup table to support 
	std::map<uint64_t, uint32_t> m_lookup_table;

	//
	std::vector<symbol_t> m_entries;

public:
	explicit symbol_table_t(uint32_t base_table_size = 3000)
	{
		m_entries.reserve(base_table_size);
	}
	explicit symbol_table_t(symbol_table_t const& table)
	{
		m_entries.insert(m_entries.begin(), table.m_entries.begin(), table.m_entries.end());
		m_lookup_table.insert(table.m_lookup_table.begin(), table.m_lookup_table.end());
	}

	// For looking up symbols that are inside the original binary.
	// Ex: auto import_sym_idx = get_symbol_index_for_rva(symbol_flag::base, import_addr);
	//
	[[nodiscard]] uint32_t get_symbol_index_for_rva(symbol_flag::type flags, uint64_t address)
	{
		uint64_t map_key = make_symbol_map_key(flags, address);
		auto it = m_lookup_table.find(map_key);
		if (it == m_lookup_table.end())
		{
			uint32_t index = static_cast<uint32_t>(m_entries.size());
			m_entries.emplace_back(flags, address);
			m_lookup_table[map_key] = index;
			return index;
		}
		return it->second;
	}

	// For symbols created after the fact, these dont get an entry in the lookup table because
	// they are not within the original binary. => dont have an rva.
	//
	[[nodiscard]] uint32_t get_arbitrary_symbol_index(symbol_flag::type flags = symbol_flag::none)
	{
		m_entries.emplace_back(flags, 0);
		return static_cast<uint32_t>(m_entries.size()) - 1;
	}

	[[nodiscard]] finline symbol_t& get_symbol_by_index(uint32_t symbol_index)
	{
		return m_entries[symbol_index];
	}
	finline void set_sym_addr_and_valid(uint32_t symbol_index, uint64_t address)
	{
		m_entries[symbol_index].rva = address;
		m_entries[symbol_index].flags |= symbol_flag::valid;
	}
};
