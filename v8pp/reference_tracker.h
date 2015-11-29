#pragma once

template<typename T>
struct ref_debug;
#include <list>
#include <algorithm>


/*
	should probably be contained within a macro
		- so class is only added when reference tracking is enabled
*/

struct ref_static
{
	static long ref_count;
	static std::list<std::pair<void*, std::string>> instances;
};
//#define DEBUG_REFERENCES
struct ref_counter : ref_static
{
#ifdef DEBUG_REFERENCES
	ref_counter(const ref_counter &other)
	{
		ref_count++;
	}
	ref_counter()
	{
		ref_count++;
	}
	~ref_counter()
	{
		ref_count--;
	}
#endif
};

template<typename T>
struct ref_debug : ref_static
{
#ifdef DEBUG_REFERENCES
	ref_debug(const ref_debug &other){ ref_count++; instances.emplace_back(std::make_pair((void*)this, typeid(T).name())); }
	ref_debug(){ ref_count++; instances.emplace_back(std::make_pair((void*)this, typeid(T).name())); }
	~ref_debug()
	{
		ref_count--;

		auto finder = std::find_if(instances.begin(), instances.end(), [&](const std::pair<void*, std::string>& e)
		{
			if (e.first == (void*)this)
			{
				if (e.second.compare(typeid(T).name()) == 0)
					return true;
			}
			return false;
		});
		if (finder != instances.end())
			instances.erase(finder);
		else
			std::cerr << "Somehow could not find ref_debug instance. " << std::endl;

	};
#endif
};