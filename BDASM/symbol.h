#pragma once

#include <cstdint>
#include <map>
#include <vector>
#include <mutex>

#include "traits.h"

namespace symbol_flag
{
	typedef uint32_t type;
	constexpr type none = 0;

	// Indicates that the address is relative to
	// These are used to key into the map and find symbols.
	// After that they dont matter.
	//
	constexpr type base = (1 << 0);

	// import_lookup_key uses this to mask to only the values used in the key.
	//
	constexpr type key_mask = (1 << 0);


	// This symbol was placed and given an updated rva
	//
	constexpr type placed = (1 << 1);


	// The type descriptor for debug purposes i think...
	// Will probably remove this later
	//
	constexpr type type_export = (1 << 2);
	constexpr type type_export_data = (1 << 3);
	constexpr type type_export_function = (1 << 4);
	constexpr type type_export_mask = (type_export | type_export_data | type_export_function);

	constexpr type type_import = (1 << 5);
	constexpr type type_import_data = (1 << 6);
	constexpr type type_import_routine = (1 << 7);
	constexpr type type_import_mask = (type_import | type_import_data | type_import_routine);


	constexpr type mask = 0xffffffff;
};

// These symbols are basically an intermediate between two things. For example an instruction and another isntruction
// One instruction sets the rva once its placed in its final destination, and the other reads the rva to calculate a needed delta 
//
class symbol_t
{
public:

	// Some rva that represents where the symbol isa. Set once its placed.
	//
	uint64_t address;

	// 
	//
	symbol_flag::type flags;

	explicit symbol_t(symbol_flag::type flags = 0, uint64_t raw_address = 0)
		: flags(flags), address(raw_address) {}

	explicit symbol_t(symbol_t const& to_copy)
		: flags(to_copy.flags), address(to_copy.address) {}

	finline symbol_t& set_flag(symbol_flag::type value)
	{
		flags |= value;
		return *this;
	}
	finline symbol_t& remove_flag(symbol_flag::type value)
	{
		flags &= ~value;
		return *this;
	}
	finline symbol_t& set_flag_abs(symbol_flag::type new_flags)
	{
		flags = new_flags;
		return *this;
	}
	finline symbol_t& set_address(uint64_t new_addr)
	{
		address = new_addr;
		return *this;
	}
};

#define make_symbol_map_key(macro_flag, macro_addr)															\
static_cast<uint64_t>(																						\
		(																									\
			((static_cast<uint64_t>(macro_flag) & static_cast<uint64_t>(symbol_flag::key_mask)) << 32)		\
			|																								\
			static_cast<uint64_t>(macro_addr & 0xffffffff)													\
		)																									\
	)
//class symbol_table_t
//{
//	// Lookup table to support lookups of static things
//	// imports, functions, rvas inside binary, anything
//	//
//	std::map<uint64_t, uint32_t> m_lookup_table;
//
//	//
//	std::vector<symbol_t> m_entries;
//
//public:
//	explicit symbol_table_t(uint32_t base_table_size = 3000)
//	{
//		m_entries.emplace_back(symbol_flag::none, 0);
//		m_entries.reserve(base_table_size);
//	}
//	explicit symbol_table_t(symbol_table_t const& table)
//	{
//		m_entries.insert(m_entries.begin(), table.m_entries.begin(), table.m_entries.end());
//		m_lookup_table.insert(table.m_lookup_table.begin(), table.m_lookup_table.end());
//	}
//
//	// For looking up symbols that are inside the original binary.
//	// Ex: auto import_sym_idx = get_symbol_index_for_rva(symbol_flag::base, import_addr);
//	//
//	ndiscard uint32_t get_symbol_index_for_rva(symbol_flag::type flags, uint64_t address)
//	{
//		uint64_t map_key = make_symbol_map_key(flags, address);
//		auto it = m_lookup_table.find(map_key);
//		if (it == m_lookup_table.end())
//		{
//			uint32_t index = static_cast<uint32_t>(m_entries.size());
//			m_entries.emplace_back(flags, address);
//			m_lookup_table[map_key] = index;
//			return index;
//		}
//		return it->second;
//	}
//
//	// For symbols created after the fact, these dont get an entry in the lookup table because
//	// they are not within the original binary. => dont have an rva.
//	//
//	ndiscard uint32_t get_arbitrary_symbol_index(symbol_flag::type flags = symbol_flag::none)
//	{
//		m_entries.emplace_back(flags, 0);
//		return static_cast<uint32_t>(m_entries.size()) - 1;
//	}
//
//	ndiscard finline symbol_t& get_symbol(uint32_t symbol_index)
//	{
//		return m_entries[symbol_index];
//	}
//	finline void set_sym_addr_and_placed(uint32_t symbol_index, uint64_t address)
//	{
//		m_entries[symbol_index].set_flag(symbol_flag::placed).set_address(address);
//		/*m_entries[symbol_index].address = address;
//		m_entries[symbol_index].flags |= symbol_flag::placed;*/
//	}
//};


class symbol_table_t
{
	const uint32_t m_image_size;

	symbol_t* m_image_table;

	std::vector<symbol_t> m_arbitrary_table;

	// This is ONLY needed when allocating in the arbitrary_table 
	// So now the multithreading can be used easily! :D
	std::mutex m_lock;
public:

	symbol_table_t(uint32_t image_size, uint32_t arbitrary_table_start_size = 1000)
		: m_image_size(image_size)
	{
		m_image_table = new symbol_t[image_size];
		m_arbitrary_table.reserve(arbitrary_table_start_size);
	}
	ndiscard uint32_t get_symbol_index_for_rva(symbol_flag::type flags, uint64_t address)
	{
		if (address < m_image_size)
		{
			m_image_table[address].set_flag_abs(flags).set_address(address);
			return address;
		}
		else
		{
			m_lock.lock();
			auto sym_idx =  get_arbitrary_symbol_index(flags);
			m_lock.unlock();
		}
	}

	// For symbols created after the fact, these dont get an entry in the lookup table because
	// they are not within the original binary. => dont have an rva.
	//
	ndiscard uint32_t get_arbitrary_symbol_index(symbol_flag::type flags = symbol_flag::none)
	{
		m_arbitrary_table.emplace_back(flags, 0);
		return static_cast<uint32_t>(m_arbitrary_table.size()) - 1;
	}

	ndiscard finline symbol_t& get_symbol(uint32_t symbol_index)
	{
		if (symbol_index < m_image_size)
			return m_image_table[symbol_index];
		else
			return m_arbitrary_table[symbol_index];
	}
	finline void set_sym_addr_and_placed(uint32_t symbol_index, uint64_t address)
	{
		if (symbol_index < m_image_size)
			m_image_table[symbol_index].set_flag(symbol_flag::placed).set_address(address);
		else
			m_arbitrary_table[symbol_index].set_flag(symbol_flag::placed).set_address(address);
	}
};