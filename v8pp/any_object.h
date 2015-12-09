#ifndef V8PP_ANY_OBJECT_H_INCLUDED
#define V8PP_ANY_OBJECT_H_INCLUDED

#include <memory>
#include "reference_tracker.h"
#include <functional>

namespace v8
{
	class Isolate;

	template <typename T>
	class Local;

	class PersistentValue;

	class Object;

	class Value;

	class Function;
}

/*
	Function notes
		- std::function<void()>
		- v8 function
			- v8::handle<v8::Function>
				-- no defined values for it
		- use template params to define the cpp functional function

		- call v8 function instead of cpp function
			- call back function that forwards the data

			template<class _Ret, typename ...Args>
			_Ret function(function_data *data, v8::local<v8::value> &this_object, Args... args)
			{
				returm (*data)(this_object, args);
			}
*/

namespace v8pp
{
	//contains a reference to a v8 value and its isolate
	//should be used when receiving a value to be stored in persistent memory
	class ValueIsolate : public ref_debug<ValueIsolate>
	{
	public:
		ValueIsolate(v8::Local<v8::Value> &value, v8::Isolate *isolate);
		// creates an empty object
		//	 true = null object
		ValueIsolate(bool null_value, v8::Isolate *isolate);
		//empty value
		ValueIsolate();
		~ValueIsolate();
		
		v8::Local<v8::Value> get_value() const;

		v8::Isolate *get_isolate() const;
	private:
		struct data_object;
		std::unique_ptr<data_object> _d;
	};

	//stores persistent v8 value and its isolate
	class PersistentValue : public ref_debug<PersistentValue>
	{
	public:
		PersistentValue();
		PersistentValue(PersistentValue const &other);
		PersistentValue(v8::Local<v8::Value> &value, v8::Isolate *isolate);
		PersistentValue(const ValueIsolate &v8_value_isolate);
		~PersistentValue();

		bool isEmpty();

		void set_value(v8::Local<v8::Value> &value, v8::Isolate *isolate);

		v8::Local<v8::Value> get_value() const;

		v8::Isolate *get_isolate() const;

		void destroy_value();

		PersistentValue& operator=(const PersistentValue &other);
		PersistentValue& operator=(const ValueIsolate &other);
	private:
		struct data_object;
		std::unique_ptr<data_object> object;
	};

	//v8 function or cpp function
	struct function_data;

	/*
		Use get_data to call the v8 function
	*/
	class function_base
	{
	public:
		function_base(v8::Local<v8::Function> &function, v8::Isolate *isolate);
		function_base();
		function_base(const function_base &other);
		~function_base();

		bool is_v8_function() const;

		virtual bool is_cpp_function() const = 0;

		void reset();

		virtual bool isEmpty() const = 0;

		void set_v8_function(v8::Local<v8::Function> &function, v8::Isolate *isolate);

		function_data *get_data() const;
	protected:
		std::unique_ptr<function_data> _d;
	};


	template<class T> class Function { };

	//
	//
	//		To call a stored v8 function you should first set the v8_forward_function
	//			- setting it manually allows for the v8 headers to be hidden
	//			- template functions would be compiled when needed
	//
	//
	template<class _Ret, class... ArgTypes>
	class Function<_Ret(ArgTypes...)> : public function_base
	{
	public:
		typedef Function<_Ret(ArgTypes...)> class_type;
		typedef std::function<_Ret(ArgTypes...)> std_function_type;
		typedef std::function<_Ret(class_type *, v8::Local<v8::Value>, ArgTypes...)> v8_forward_function;
		
		Function() : function_base()
		{

		}
		Function(v8::Local<v8::Function> &function, v8::Isolate *isolate) : function_base(function, isolate)
		{

		}

		_Ret operator()(ArgTypes... args)
		{ 
			if (is_v8_function() && v8_function)
				return v8_function(this, v8::Local<v8::Value>(), std::forward<ArgTypes>(args)...);
			return cpp_function(std::forward<ArgTypes>(args)...);
		};
		
		_Ret operator()(v8::Local<v8::Value> &this_object, ArgTypes... args)
		{
			return v8_function(this, this_object, std::forward<ArgTypes>(args)...);
		}

		void operator=(std_function_type function)
		{
			reset();
			cpp_function = function; 
		};

		/*
			should use the function_data_forward defined in any_object_hidden.h

			ex. 
				forward_struct<_Ret(ArgTypes...)>::function_data_forward<_Ret>
			or
				forward_functional<_Ret, Args..>
					-- contains no brackets
		*/
		void set_v8_forward_function(v8_forward_function function)
		{
			v8_function = function;
		};

		virtual bool isEmpty() const
		{
			if (is_v8_function())
				return false;

			if (is_cpp_function())
				return false;

			return true;
		}

		operator bool() const
		{
			return !isEmpty();
		}

		virtual bool is_cpp_function() const
		{
			if (cpp_function)
				return true;
			return false;
		}

		std_function_type get_cpp_function() const
		{
			return cpp_function;
		}
	private:
		std_function_type cpp_function;
		v8_forward_function v8_function;
	};


	/*
		Any value should try to store a v8 object
		or it should store a c++ object and attempt to convert it into a v8 object
		if fails to do so it should display an unknown object

		-- would have to use v8pp::class_ to complete -- which requires v8 functions...
			--- on hold for now
	*/
	/*class any_value : public ref_debug<any_value>
	{
	public:
		struct data_object;
		any_value();
		any_value(void *obj);
		~any_value();

		bool isEmpty();

		bool isJS();
		bool isCpp();

		void set_c_object(void *obj);

		std::string js_text_value();
		v8pp_value *js_value();

		void *cpp_value();

		data_object *get_data();
	private:
		std::unique_ptr<v8pp_value> v8_value;
		void *c_obj = nullptr;
	};*/
}

#endif