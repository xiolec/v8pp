#pragma once
#include "v8pp/convert.hpp"
#include <unordered_map>
#include "persistent.hpp"
#include <functional>

namespace v8pp
{
	namespace detail {
		template<typename T>
		using is_pointer_cast_allowed = std::integral_constant < bool, sizeof(T) <= sizeof(void*) >;

		typedef v8::Persistent<v8::External, v8::CopyablePersistentTraits<v8::External>> type_data_object;

		template<typename T>
		void delete_object_callback(void *obj)
		{
			delete static_cast<T*>(obj);
		};

		class external_info
		{
		public:
			static external_info *get_external_data_info(v8::Isolate *isolate)
			{
				auto finder = isolate_storage.find(isolate);
				if (finder == isolate_storage.end())
					finder = isolate_storage.emplace(isolate, external_info()).first;

				return &finder->second;
			}


			static v8::Local<v8::Value> emplace_data(v8::Isolate *isolate, void *data, std::function<void(void*)> delete_callback)
			{
				v8::Local<v8::External> ext = v8::External::New(isolate, data);

				external_info *e_info = get_external_data_info(isolate);
				e_info->objects_.emplace(data, std::make_pair(type_data_object(isolate, ext), delete_callback));

				return ext;
			}

			static void delete_isolate_instance(v8::Isolate *isolate)
			{
				external_info *e_info = get_external_data_info(isolate);

				auto it = e_info->objects_.begin();
				for (; it != e_info->objects_.end(); )
				{
					it->second.first.Reset();
					it->second.second(it->first);
					it = e_info->objects_.erase(it);
				}

				auto finder = isolate_storage.find(isolate);
				if (finder != isolate_storage.end())
					isolate_storage.erase(finder);
			}
		private:
			external_info(){};

			//external_data		object for data
			std::map<void*, std::pair<type_data_object, std::function<void(void*)>>> objects_;

			static std::map<v8::Isolate *, external_info> isolate_storage;
		};


		template<typename T>
		typename std::enable_if<!is_pointer_cast_allowed<T>::value, v8::Local<v8::Value>>::type
			set_external_data(v8::Isolate* isolate, T const& value)
		{
			T* data = new T(value);

			//v8::Local<v8::External> ext = v8::External::New(isolate, data);

			//v8::Persistent<v8::External> pext(isolate, ext);
			//pext.SetWeak(data, [](v8::WeakCallbackData<v8::External, T> const& data)
			//{
			//	T* data_ret = (T*)data.GetParameter();
			//	delete data_ret;
			//});

			return external_info::emplace_data(isolate, data, std::bind(&delete_object_callback<T>, std::placeholders::_1));
		}
	};
};

