#pragma once

#include <map>
#include <functional>
#include "v8.h"

class isolate_watcher
{
public:
	static isolate_watcher *get_value_watcher(v8::Isolate *isolate)
	{
		auto finder = watcher_per_isolate.find(isolate);
		if (finder == watcher_per_isolate.end())
			finder = watcher_per_isolate.emplace(isolate, isolate_watcher()).first;

		return &finder->second;
	}


	static void emplace_data(v8::Isolate *isolate, void *data, std::function<void()> remove_function)
	{
		isolate_watcher *e_info = get_value_watcher(isolate);
		e_info->value_map.emplace(data, remove_function);
	}

	static void remove_data(v8::Isolate *isolate, void *data)
	{
		isolate_watcher *e_info = get_value_watcher(isolate);

		auto it = e_info->value_map.find(data);
		if (it != e_info->value_map.end())
			e_info->value_map.erase(it);
	}

	static void delete_isolate_instance(v8::Isolate *isolate)
	{
		isolate_watcher *e_info = get_value_watcher(isolate);

		auto it = e_info->value_map.begin();
		for (; it != e_info->value_map.end();)
		{
			it->second();
			it = e_info->value_map.erase(it);
		}

		auto finder = watcher_per_isolate.find(isolate);
		if (finder != watcher_per_isolate.end())
			watcher_per_isolate.erase(finder);
	}
private:
	isolate_watcher(){};

	std::map<void *, std::function<void()>> value_map;

	static std::map<v8::Isolate *, isolate_watcher> watcher_per_isolate;
};