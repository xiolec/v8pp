#include "v8pp_debug.h"

namespace v8pp
{
	v8::PropertyHandlerFlags debug::_debug_flag = v8::PropertyHandlerFlags::kAllCanRead;
	bool debug::enable_debug = false;
	void debug::set_debug_flag(v8::PropertyHandlerFlags debug_flag)
	{
		_debug_flag = debug_flag;
	}

	void window_global_callback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& info)
	{
		if (!debug::enable_debug)
			return;
		// key name		--		times found
		static std::map<std::string, int> keys_found;
		std::string object_name = v8pp::from_v8<std::string>(v8::Isolate::GetCurrent(), info.Data(), "");
		std::string key = v8pp::from_v8<std::string>(v8::Isolate::GetCurrent(), property, "");

		if (key.compare("performance") == 0)
			return;

		static std::vector<std::string> skipped_keys;
		if (skipped_keys.empty())
		{
			skipped_keys.emplace_back("getAttribute");
			skipped_keys.emplace_back("ownerDocument");
			skipped_keys.emplace_back("className");
			skipped_keys.emplace_back("parentNode");
			skipped_keys.emplace_back("nodeType");
		}

		for (size_t x = 0; x < skipped_keys.size(); x++)
		{
			if (skipped_keys[x].compare(key) == 0)
				return;
		}

		static std::string last;
		static std::string last_object_name;
		static int last_count = 0;

		if (last.compare(key) == 0)
		{
			last_count++;

			return;
		}
		else
		{
			if (last_count != 0)
				//window_instance::call_log += "Getter named property -- " + last_object_name + "." + last + " x" + std::to_string(last_count) + "\n";
				std::cout << "Getter named property -- " << last_object_name << "." << last << " x" << last_count << std::endl;


			last = key;
			last_object_name = object_name;
			last_count = 0;
		}

		std::map<std::string, int>::iterator finder = keys_found.find(key);
		if (finder == keys_found.end())
		{
			keys_found[key] = 1;

			std::cout << "Getter named property -- " << object_name << "." << key << std::endl;
			//window_instance::call_log += "Getter named property -- " + object_name + "." + key + "\n";

		}
		else
		{
			finder->second++;
			std::cout << "Getter named property -- " << object_name << "." << key << std::endl;
			//window_instance::call_log += "Getter named property -- " + object_name + "." + key + "\n";
		}
	}

	void global_setter_callback(v8::Local<v8::Name> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
	{
		if (!debug::enable_debug)
			return;
		// key name		--		times found
		static std::map<std::string, int> keys_found;
		std::string object_name = v8pp::from_v8<std::string>(v8::Isolate::GetCurrent(), info.Data(), "");
		std::string key = v8pp::from_v8<std::string>(v8::Isolate::GetCurrent(), property, "");

		if (key.compare("performance") == 0)
			return;

		static std::vector<std::string> skipped_keys;
		if (skipped_keys.empty())
		{
			skipped_keys.emplace_back("getAttribute");
			skipped_keys.emplace_back("ownerDocument");
			skipped_keys.emplace_back("className");
			skipped_keys.emplace_back("parentNode");
			skipped_keys.emplace_back("nodeType");
		}

		for (size_t x = 0; x < skipped_keys.size(); x++)
		{
			if (skipped_keys[x].compare(key) == 0)
				return;
		}

		static std::string last;
		static std::string last_object_name;
		static int last_count = 0;

		if (last.compare(key) == 0)
		{
			last_count++;

			return;
		}
		else
		{
			if (last_count != 0)
				//window_instance::call_log += "Setter named property -- " + last_object_name + "." + last + " x" + std::to_string(last_count) + "\n";
				std::cout << "Setter named property -- " << last_object_name << "." << last << " x" << last_count << std::endl;


			last = key;
			last_object_name = object_name;
			last_count = 0;
		}

		std::map<std::string, int>::iterator finder = keys_found.find(key);
		if (finder == keys_found.end())
		{
			keys_found[key] = 1;

			std::cerr << "Setter named property-- " << object_name << "." << key << std::endl;
			//window_instance::call_log += "Setter named property -- " + object_name + "." + key + "\n";
		}
		else
		{
			finder->second++;

			std::cout << "Setter named property -- " << object_name << "." << key << std::endl;
			//window_instance::call_log += "Setter named property -- " + object_name + "." + key + "\n";
		}
	}

	void global_query(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Integer>& info)
	{
		if (!debug::enable_debug)
			return;
		// key name		--		times found
		static std::map<std::string, int> keys_found;
		std::string object_name = v8pp::from_v8<std::string>(v8::Isolate::GetCurrent(), info.Data(), "");
		std::string key = v8pp::from_v8<std::string>(v8::Isolate::GetCurrent(), property, "");

		
		if (key.compare("performance") == 0)
			return;

		static std::vector<std::string> skipped_keys;
		if (skipped_keys.empty())
		{
			skipped_keys.emplace_back("getAttribute");
			skipped_keys.emplace_back("ownerDocument");
			skipped_keys.emplace_back("className");
			skipped_keys.emplace_back("parentNode");
			skipped_keys.emplace_back("nodeType");
		}

		for (size_t x = 0; x < skipped_keys.size(); x++)
		{
			if (skipped_keys[x].compare(key) == 0)
				return;
		}

		std::map<std::string, int>::iterator finder = keys_found.find(key);
		if (finder == keys_found.end())
		{
			keys_found[key] = 1;

			std::cerr << "Query named property -- " << object_name << "." << key << std::endl;
			//window_instance::call_log += "Query named property -- " + object_name + "." + key + "\n";
		}
		else
		{
			finder->second++;
			std::cout << "Query named property -- " << object_name << "." << key << std::endl;
			//window_instance::call_log += "Query named property -- " + object_name + "." + key + "\n";
		}
	}

	void global_deleter(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Boolean>& info)
	{
		if (!debug::enable_debug)
			return;
		// key name		--		times found
		static std::map<std::string, int> keys_found;
		std::string object_name = v8pp::from_v8<std::string>(v8::Isolate::GetCurrent(), info.Data(), "");
		std::string key = v8pp::from_v8<std::string>(v8::Isolate::GetCurrent(), property, "");

		if (key.compare("performance") == 0)
			return;

		std::map<std::string, int>::iterator finder = keys_found.find(key);
		if (finder == keys_found.end())
		{
			keys_found[key] = 1;

			std::cerr << "Deleter named property -- " << object_name << "." << key << std::endl;
		}
		else
		{
			finder->second++;
			std::cout << "Deleter named property -- " << object_name << "." << key << std::endl;
		}
	}

	void debug::set_debug_handler(v8::Handle<v8::ObjectTemplate> &obj_temp, const char *name, v8::Isolate *isolate)
	{
		v8::NamedPropertyHandlerConfiguration config;
		config.getter = &v8pp::window_global_callback;
		config.setter = &v8pp::global_setter_callback;
		config.query = &v8pp::global_query;
		config.deleter = &v8pp::global_deleter;
		config.flags = _debug_flag;

		config.data = v8pp::to_v8(isolate, name);
		obj_temp->SetHandler(config);
	}
}