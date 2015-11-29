#ifndef V8PP_FUNCTION_HPP_INCLUDED
#define V8PP_FUNCTION_HPP_INCLUDED

#include <string>
#include <tuple>
#include <type_traits>

#include "v8pp/call_from_v8.hpp"
#include "v8pp/throw_ex.hpp"
#include "v8pp/utility.hpp"
#include "v8pp/external_type_data.h"

namespace v8pp {

	enum return_empty
	{
		NONE,
		SET_NULL,
		SET_UNDEFINED,
		SKIP
	};
	namespace detail {

		template<class T> struct is_std_function : std::false_type { using type = T; };

		template<class _Ret, class... ArgTypes>
		struct is_std_function<std::function<_Ret(ArgTypes...)>> : std::true_type
		{
			using type = std::function<_Ret(ArgTypes...)>;
		};

		//template<typename T>
		//using is_function_pointer =
			//typename std::conditional <std::is_function<typename std::remove_pointer<T>::type>::value,//is_std_function<T>::value,
				//std::is_function<typename std::remove_pointer<T>::type>,//is_std_function<T>,
					//	std::is_function<typename std::remove_pointer<T>::type>>::type;

		template<typename T>
		using is_function_pointer = std::is_function < typename std::remove_pointer<T>::type > ;

		template<typename F>
		using is_void_return = std::is_same < void, typename function_traits<F>::return_type >;

		template<typename F>
		using is_pointer_return = std::is_pointer < typename function_traits<F>::return_type >;

		template<typename F>
		using is_string_return = std::is_same < std::string, typename function_traits<F>::return_type >;

		template<typename F>
		using is_string_pointer_return = std::is_same < std::string*, typename function_traits<F>::return_type >;

		template<typename T>
		union pointer_cast
		{
		private:
			void* ptr;
			T value;

		public:
			static_assert(is_pointer_cast_allowed<T>::value,
				"data size should be not greater than pointer size");

			explicit pointer_cast(void* ptr) : ptr(ptr) {};
			explicit pointer_cast(T value) : value(value) {};

			operator void*() const { return ptr; };
			operator T() const { return value; };
		};

		template<typename T>
		typename std::enable_if<is_pointer_cast_allowed<T>::value, v8::Local<v8::Value>>::type
			set_external_data(v8::Isolate* isolate, T value)
		{
			return v8::External::New(isolate, pointer_cast<T>(value));
		}

		/*template<typename T>
		typename std::enable_if<!is_pointer_cast_allowed<T>::value, v8::Local<v8::Value>>::type
			set_external_data(v8::Isolate* isolate, T const& value)
		{
			T* data = new T(value);

			v8::Local<v8::External> ext = v8::External::New(isolate, data);

			v8::Persistent<v8::External> pext(isolate, ext);
			pext.SetWeak(data, [](v8::WeakCallbackData<v8::External, T> const& data)
			{
				T* data_ret = (T*)data.GetParameter();
				delete data_ret;
			});

			return ext;
		}*/

		template<typename T>
		typename std::enable_if<is_pointer_cast_allowed<T>::value, T>::type
			get_external_data(v8::Handle<v8::Value> value)
		{
			return pointer_cast<T>(value.As<v8::External>()->Value());
		}

		template<typename T>
		typename std::enable_if<!is_pointer_cast_allowed<T>::value, T&>::type
			get_external_data(v8::Handle<v8::Value> value)
		{
			T* data = static_cast<T*>(value.As<v8::External>()->Value());
			return *data;
		}

		template<typename F>
		typename std::enable_if<is_function_pointer<F>::value || is_std_function<F>::value,
			typename function_traits<F>::return_type>::type
			invoke(v8::FunctionCallbackInfo<v8::Value> const& args)
		{
			F f = get_external_data<F>(args.Data());
			return call_from_v8(std::forward<F>(f), args);
		}

		template<typename F>
		typename std::enable_if<std::is_member_function_pointer<F>::value,
			typename function_traits<F>::return_type>::type
			invoke(v8::FunctionCallbackInfo<v8::Value> const& args)
		{
			using arguments = typename function_traits<F>::arguments;
			static_assert(std::tuple_size<arguments>::value > 0, "");
			using class_type = typename std::tuple_element<0, arguments>::type;

			F f = get_external_data<F>(args.Data());
			class_type& obj = from_v8<class_type&>(args.GetIsolate(), args.This());

			return call_from_v8(obj, std::forward<F>(f), args);
		}

		template<typename F>
		typename std::enable_if<is_void_return<F>::value>::type
			forward_ret(v8::FunctionCallbackInfo<v8::Value> const& args, return_empty e_type = NONE)
		{
			invoke<F>(args);
		}

		template<typename F>
		typename std::enable_if<is_pointer_return<F>::value &&
			!is_string_pointer_return<F>::value>::type
			forward_ret(v8::FunctionCallbackInfo<v8::Value> const& args, return_empty e_type = NONE)
		{
			typename function_traits<F>::return_type ret = invoke<F>(args);
			if (ret == nullptr)
				if ((e_type == SET_NULL) || (e_type == NONE))
					args.GetReturnValue().SetNull();
				else
					args.GetReturnValue().SetUndefined();
			else
				args.GetReturnValue().Set(to_v8(args.GetIsolate(), ret));
		}

		template<typename F>
		typename std::enable_if<is_string_pointer_return<F>::value>::type
		forward_ret(v8::FunctionCallbackInfo<v8::Value> const& args, return_empty e_type = NONE)
		{
			typename function_traits<F>::return_type ret = invoke<F>(args);
			if (ret == nullptr)
				if (e_type == SET_NULL)
					args.GetReturnValue().SetNull();
				else if(e_type == SET_UNDEFINED)
					args.GetReturnValue().SetUndefined();
				else
					args.GetReturnValue().SetEmptyString();
			else
				args.GetReturnValue().Set(to_v8(args.GetIsolate(), *ret));
		}

		template<typename F>
		typename std::enable_if < !is_pointer_return<F>::value &&
			!is_void_return<F>::value &&
			!is_string_return<F>::value> ::type
			forward_ret(v8::FunctionCallbackInfo<v8::Value> const& args, return_empty e_type = NONE)
		{
			args.GetReturnValue().Set(to_v8(args.GetIsolate(), invoke<F>(args)));
		}

		template<typename F>
		typename std::enable_if < !is_pointer_return<F>::value &&
			!is_void_return<F>::value &&
			is_string_return<F>::value
		> ::type
		forward_ret(v8::FunctionCallbackInfo<v8::Value> const& args, return_empty e_type = NONE)
		{
			function_traits<F>::return_type ret = invoke<F>(args);
			if (ret.empty() && e_type != NONE)
			{
				if (e_type == SET_NULL)
					args.GetReturnValue().SetNull();
				else
					args.GetReturnValue().SetUndefined();
			}
			else
				args.GetReturnValue().Set(to_v8(args.GetIsolate(), ret));
		}

		template<typename F>
		void forward_function(v8::FunctionCallbackInfo<v8::Value> const& args)
		{
			static_assert(detail::is_function_pointer<F>::value
				|| std::is_member_function_pointer<F>::value
				|| detail::is_std_function<F>::value,
				"required pointer to a free or member function");

			v8::Isolate* isolate = args.GetIsolate();
			v8::HandleScope scope(isolate);

			try
			{
				forward_ret<F>(args);
			}
			catch (std::exception const& ex)
			{
				args.GetReturnValue().Set(throw_ex(isolate, ex.what()));
			}
		}

		template<typename F>
		void forward_function_using_null(v8::FunctionCallbackInfo<v8::Value> const& args)
		{
			static_assert(detail::is_function_pointer<F>::value
				|| std::is_member_function_pointer<F>::value,
				"required pointer to a free or member function");

			v8::Isolate* isolate = args.GetIsolate();
			v8::HandleScope scope(isolate);

			try
			{
				forward_ret<F>(args, SET_NULL);
			}
			catch (std::exception const& ex)
			{
				args.GetReturnValue().Set(throw_ex(isolate, ex.what()));
			}
		}

		template<typename F>
		void forward_function_using_undefined(v8::FunctionCallbackInfo<v8::Value> const& args)
		{
			static_assert(detail::is_function_pointer<F>::value
				|| std::is_member_function_pointer<F>::value,
				"required pointer to a free or member function");

			v8::Isolate* isolate = args.GetIsolate();
			v8::HandleScope scope(isolate);

			try
			{
				forward_ret<F>(args, SET_UNDEFINED);
			}
			catch (std::exception const& ex)
			{
				args.GetReturnValue().Set(throw_ex(isolate, ex.what()));
			}
		}

	} // namespace detail

	/// Wrap C++ function into new V8 function template
	template<typename F>
	v8::Handle<v8::FunctionTemplate> wrap_function_template(v8::Isolate* isolate, F func, return_empty empty_return = NONE)
	{
		if (empty_return == NONE)
		return v8::FunctionTemplate::New(isolate, &detail::forward_function<F>,
			detail::set_external_data(isolate, func));
		else if (empty_return == SET_NULL)
			return v8::FunctionTemplate::New(isolate, &detail::forward_function_using_null<F>,
			detail::set_external_data(isolate, func));
		else
			return v8::FunctionTemplate::New(isolate, &detail::forward_function_using_undefined<F>,
			detail::set_external_data(isolate, func));
	}

	/// Wrap C++ function into new V8 function
	/// Set nullptr or empty string for name
	/// to make the function anonymous
	template<typename F>
	v8::Handle<v8::Function> wrap_function(v8::Isolate* isolate, char const* name, F func)
	{
		v8::Handle<v8::Function> fn = v8::Function::New(isolate, &detail::forward_function<F>,
			detail::set_external_data(isolate, func));
		if (name && *name)
		{
			fn->SetName(to_v8(isolate, name));
		}
		return fn;
	}

} // namespace v8pp

#endif // V8PP_FUNCTION_HPP_INCLUDED
