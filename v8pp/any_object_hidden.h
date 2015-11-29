#ifndef V8PP_ANY_OBJECT_HIDDEN_H_INCLUDED
#define V8PP_ANY_OBJECT_HIDDEN_H_INCLUDED

#include "any_object.h"
#include "v8.h"
#include <v8pp/convert.hpp>
#include <v8pp/call_v8.hpp>
#include <map>

namespace v8pp
{

	/*
		Need to destroy all persistent values on isolate destruction 
		handle with any_value object?

		handle with v8pp_value object?
			- super class that collects all the objects created
			- reset all persistent values on destruction
			- value then returns empty

			- any_value then not only needs to check if the value is created but also if it still exists.

		any_value
			- should try to convert the value to a javascript object if possible?
				- needs to also have the type information then
				- needs to open up more v8pp functions
				- convert functions
	*/
	struct PersistentValue::data_object
	{
		void set_value(v8::Local<v8::Value> &value, v8::Isolate *isolate);

		v8::Local<v8::Value> get_value();

		bool isEmpty();

		data_object()
		{
			_isolate = nullptr;
		}
		~data_object();

		v8::Isolate *get_isolate();
	private:
		v8::Persistent<v8::Value> base_object;
		v8::Isolate *_isolate = nullptr;
	};

	struct ValueIsolate::data_object
	{
		data_object(v8::Local<v8::Value> &value, v8::Isolate *isolate);
		~data_object();
		v8::Local<v8::Value> _value;
		v8::Isolate *_isolate;
	};

	struct function_data
	{
		function_data(){};
		~function_data();
		void set_value(v8::Local<v8::Function> &function, v8::Isolate *isolate);

		/*template<class _Ret, typename ...Args>
		_Ret call(v8::Handle<v8::Value> &this_object, Args... args)
		{
			if (isolate_ == nullptr)
				return "";

			if (this_object.IsEmpty())
			{
				this_object = isolate_->GetCurrentContext()->Global();
			}
			v8::Local<v8::Value> result = call_v8(isolate_, v8pp::to_local(isolate_, function_), 
													this_object, args);

			return v8pp::from_v8<_Ret>(isolate_, result);
		}*/

		bool isEmpty();

		void reset();

		v8::Local<v8::Function> get_function();
		v8::Isolate *get_isolate();
	private:
		v8::Persistent<v8::Function> function_;
		v8::Isolate *isolate_ = nullptr;
	};

	template<class T> class forward_struct { };

	template<class _Ret, class... ArgTypes>
	struct forward_struct<_Ret(ArgTypes...)>
	{

		template<typename U = _Ret>
		static typename std::enable_if<!std::is_same < void, U>::value, U>::type
			function_data_forward(Function<U(ArgTypes...)> *data, v8::Local<v8::Value> &this_object, ArgTypes... args)
		{
			//return data->get_data()->call<_Ret(ArgTypes...)>(this_object, std::forward<ArgTypes>(args)...);
			function_data *fdata = data->get_data();
			v8::Isolate *isolate = fdata->get_isolate();
			if (isolate == nullptr)
				return _Ret();

			if (this_object.IsEmpty())
			{
				this_object = isolate->GetCurrentContext()->Global();
			}

			v8::Local<v8::Value> result = v8pp::call_v8<ArgTypes...>(isolate, fdata->get_function(),
				this_object, std::forward<ArgTypes>(args)...);

			return v8pp::from_v8<_Ret>(isolate, result);
		}

		template<typename U = _Ret>
		static typename std::enable_if<std::is_same <void, U>::value>::type
			function_data_forward(Function<U(ArgTypes...)> *data, v8::Local<v8::Value> &this_object, ArgTypes... args)
		{
			function_data *fdata = data->get_data();
			v8::Isolate *isolate = fdata->get_isolate();
			if (isolate == nullptr)
				return;

			if (this_object.IsEmpty())
			{
				this_object = isolate->GetCurrentContext()->Global();
			}

			v8::Local<v8::Value> result = v8pp::call_v8<ArgTypes...>(isolate, fdata->get_function(),
				this_object, std::forward<ArgTypes>(args)...);
		}
	};


	template<class _Ret, class... ArgTypes>
	_Ret forward_functional(Function<_Ret(ArgTypes...)> *data, v8::Local<v8::Value> &this_object, ArgTypes... args)
	{
		return forward_struct<_Ret(ArgTypes...)>::function_data_forward<_Ret>(data, this_object, std::forward<ArgTypes>(args)...);
	}

	class value_watcher
	{
	public:
		static value_watcher *get_value_watcher(v8::Isolate *isolate);


		static void emplace_data(v8::Isolate *isolate, PersistentValue *data);

		static void remove_data(v8::Isolate *isolate, PersistentValue *data);

		static void delete_isolate_instance(v8::Isolate *isolate);

		static void emplace_data(v8::Isolate *isolate, function_data *data);

		static void remove_data(v8::Isolate *isolate, function_data *data);
	private:
		value_watcher(){};

		std::map<PersistentValue *, bool> value_map;

		std::map<function_data *, bool> function_map;

		static std::map<v8::Isolate *, value_watcher> watcher_per_isolate;
	};

}

#endif