#ifndef V8PP_PROPERTY_HPP_INCLUDED
#define V8PP_PROPERTY_HPP_INCLUDED

#include <cassert>

#include "v8pp/convert.hpp"
#include "v8pp/function.hpp"
#include <vector>
#include <list>

namespace v8pp {

template<typename Get, typename Set>
struct property_;

namespace detail {

struct getter_tag {};
struct pointer_getter_tag{};
struct pointer_string_getter_tag{};
struct direct_getter_tag {};
struct isolate_getter_tag {};

struct interceptor_tag {};
struct interceptor_pointer_return {};
struct interceptor_string_pointer_return {};

struct setter_tag {};
struct direct_setter_tag {};
struct isolate_setter_tag {};

struct setter_interceptor_tag {};

struct enum_tag{};

template <typename Container>
struct is_container : std::false_type { };

template <typename... Ts> struct is_container<std::list<Ts...> > : std::true_type{};
template <typename... Ts> struct is_container<std::vector<Ts...> > : std::true_type{};

template<typename F>
using is_enumer = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 0 && 
	is_container<typename function_traits<F>::return_type>::value &&
	!is_void_return<F>::value>;

template<typename F>
using is_getter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 0 && !is_void_return<F>::value>;

template<typename F>
using is_pointer_getter = std::integral_constant < bool,
	call_from_v8_traits<F>::arg_count == 0 && !is_void_return<F>::value 
	&& is_pointer_return<F>::value>;

template<typename F>
using is_direct_getter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 2 &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<0>,
		v8::Local<v8::String>>::value &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<1>,
		v8::PropertyCallbackInfo<v8::Value> const&>::value &&
	is_void_return<F>::value
>;

/*
	-- should only use for masking where the value doesn't exist
		-- if it does exist it will try to return a value before checking the actual name
*/
template<typename F>
using is_index_interceptor = std::integral_constant < bool,
	(call_from_v8_traits<F>::arg_count == 2 || call_from_v8_traits<F>::arg_count == 1) &&
	(std::is_same<typename call_from_v8_traits<F>::template arg_type<0>, unsigned int> ::value
	|| std::is_same<typename call_from_v8_traits<F>::template arg_type<0>, int> ::value)
> ;

template<typename F>
using is_interceptor = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 1 &&
	(std::is_same<typename call_from_v8_traits<F>::template arg_type<0>, std::string>::value 
	|| std::is_same<typename call_from_v8_traits<F>::template arg_type<0>, const char *> ::value
	|| std::is_same<typename call_from_v8_traits<F>::template arg_type<0>, unsigned int>::value
	|| std::is_same<typename call_from_v8_traits<F>::template arg_type<0>, int>::value)
	&&
	!is_void_return<F>::value
> ;

template<typename F>
using is_interceptor_pointer = std::integral_constant < bool,
	call_from_v8_traits<F>::arg_count == 1 &&
	(std::is_same<typename call_from_v8_traits<F>::template arg_type<0>, std::string> ::value ||
	std::is_same<typename call_from_v8_traits<F>::template arg_type<0>, const char *> ::value ||
	std::is_same<typename call_from_v8_traits<F>::template arg_type<0>, unsigned int>::value ||
	std::is_same<typename call_from_v8_traits<F>::template arg_type<0>, int>::value) &&
	is_pointer_return<F>::value
> ;

template<typename F>
using is_isolate_getter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 1 &&
	is_first_arg_isolate<F>::value &&
	!is_void_return<F>::value>;

template<typename F>
using is_setter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 1 && is_void_return<F>::value>;

template<typename F>
using is_direct_setter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 3 &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<0>,
		v8::Local<v8::String>>::value &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<1>,
		v8::Local<v8::Value>>::value &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<2>,
		v8::PropertyCallbackInfo<void> const&>::value &&
	is_void_return<F>::value
>;

template<typename F>
using is_direct_setter_intercept = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 3 &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<0>,
		v8::Local < v8::String >> ::value &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<1>,
		v8::Local<v8::Value >> ::value &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<2>,
		v8::PropertyCallbackInfo<v8::Value> const&>::value &&
	is_void_return<F>::value
	>;

template<typename F>
using is_isolate_setter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 2 &&
	is_first_arg_isolate<F>::value &&
	is_void_return<F>::value>;

template<typename F>
using is_setter_interceptor = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 2 &&
	(std::is_same<typename call_from_v8_traits<F>::template arg_type<0>, std::string>::value
	|| std::is_same<typename call_from_v8_traits<F>::template arg_type<0>, std::wstring>::value
	|| std::is_same<typename call_from_v8_traits<F>::template arg_type<0>, const char*>::value
	|| std::is_same<typename call_from_v8_traits<F>::template arg_type<0>, const wchar_t*>::value
	|| std::is_same<typename call_from_v8_traits<F>::template arg_type<0>, int>::value
	|| std::is_same<typename call_from_v8_traits<F>::template arg_type<0>, unsigned int>::value
	|| std::is_same<typename call_from_v8_traits<F>::template arg_type<0>, uint32_t>::value)
	&& is_void_return<F>::value>;

template<typename F>
using select_getter_tag = typename std::conditional<is_direct_getter<F>::value,
	direct_getter_tag,
	typename std::conditional<is_isolate_getter<F>::value,
		isolate_getter_tag, 
		typename std::conditional<is_pointer_getter<F>::value,
			
			typename std::conditional<is_string_pointer_return<F>::value,
				pointer_string_getter_tag, pointer_getter_tag>::type,

			typename std::conditional<is_getter<F>::value,
				getter_tag, 
				typename std::conditional<is_interceptor_pointer<F>::value,
					typename std::conditional<is_string_pointer_return<F>::value,
					interceptor_string_pointer_return, interceptor_pointer_return>::type,
				
				interceptor_tag
				>::type
			>::type
		>::type
	>::type
>::type;

template<typename F>
using select_setter_tag = typename std::conditional<is_direct_setter<F>::value,
	direct_setter_tag,
	typename std::conditional<is_isolate_setter<F>::value,
		isolate_setter_tag, setter_tag>::type
>::type;

template<typename F>
using select_setter_intercept_tag = typename std::conditional<is_direct_setter_intercept<F>::value,
	direct_setter_tag,
	typename std::conditional<is_isolate_setter<F>::value,
		isolate_setter_tag, 
		typename std::conditional<is_setter_interceptor<F>::value,
			setter_interceptor_tag, setter_tag
		>::type
	>::type
>::type;

template<typename Get, typename Set, bool get_is_mem_fun>
struct r_property_impl;

template<typename Get, typename Set, bool set_is_mem_fun>
struct rw_property_impl;

template<typename Get, typename Set>
struct r_property_impl<Get, Set, true>
{
	using Property = property_<Get, Set>;
	using F = typename std::conditional < is_index_interceptor<Get>::value,
										uint32_t, v8::Local < v8::String >> ::type;
	using class_type = typename std::tuple_element<0,
		typename function_traits<Get>::arguments> ::type;

	static_assert(is_getter<Get>::value
		|| is_direct_getter<Get>::value
		|| is_isolate_getter<Get>::value
		|| is_interceptor<Get>::value,
		"property get function must be either `T ()` or \
					`void (v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info)` or \
					`T (v8::Isolate*)`");

	//named interceptor with pointer return
	static void get_impl(class_type& obj,
		Get get,
		uint32_t index,
		v8::PropertyCallbackInfo<v8::Value> const& info, interceptor_pointer_return, 
		return_empty return_handle = return_empty::NONE)
	{
		v8::Isolate* isolate = info.GetIsolate();

		typename function_traits<Get>::return_type ret = (obj.*get)(index);
		if (ret == nullptr)
		{
			if ((return_handle == NONE) || (return_handle == SET_NULL))
				info.GetReturnValue().SetNull();
			else
				info.GetReturnValue().SetUndefined();
		}
		else
			info.GetReturnValue().Set(to_v8(isolate, ret));
	}

	//named interceptor non void return
	static void get_impl(class_type& obj,
		Get get,
		uint32_t index,
		v8::PropertyCallbackInfo<v8::Value> const& info, interceptor_tag, 
		return_empty return_handle = return_empty::NONE)
	{
		v8::Isolate* isolate = info.GetIsolate();

		info.GetReturnValue().Set(to_v8(isolate, (obj.*get)(index)));
	}

	//named interceptor with pointer return
	static void get_impl(class_type& obj,
		Get get,
		v8::Local<v8::String> name,
		v8::PropertyCallbackInfo<v8::Value> const& info, interceptor_pointer_return, 
		return_empty return_handle = return_empty::NONE)
	{
		v8::Isolate* isolate = info.GetIsolate();
		using value_type = typename call_from_v8_traits<Get>::template arg_type<0>;
		typename function_traits<Get>::return_type ret = (obj.*get)(v8pp::from_v8<value_type>(isolate, name));
		if (ret == nullptr)
		{
			if ((return_handle == NONE) || (return_handle == SET_NULL))
				info.GetReturnValue().SetNull();
			else
				info.GetReturnValue().SetUndefined();
		}
		else
			info.GetReturnValue().Set(to_v8(isolate, ret));
	}

	static void get_impl(class_type& obj,
		Get get,
		v8::Local<v8::String> name,
		v8::PropertyCallbackInfo<v8::Value> const& info, interceptor_string_pointer_return, 
		return_empty return_handle = return_empty::NONE)
	{
		v8::Isolate* isolate = info.GetIsolate();
		using value_type = typename call_from_v8_traits<Get>::template arg_type<0>;
		typename function_traits<Get>::return_type ret = (obj.*get)(v8pp::from_v8<value_type>(isolate, name));
		if (ret == nullptr)
		{
			if (return_handle == NONE)
				info.GetReturnValue().SetEmptyString();
			else if(return_handle == SET_NULL)
				info.GetReturnValue().SetNull();
			else if (return_handle == SET_UNDEFINED)
				info.GetReturnValue().SetUndefined();
		}
		else
			info.GetReturnValue().Set(to_v8(isolate, *ret));
	}

	//named interceptor non void return
	static void get_impl(class_type& obj,
		Get get,
		v8::Local<v8::String> name,
		v8::PropertyCallbackInfo<v8::Value> const& info, interceptor_tag, 
		return_empty return_handle = return_empty::NONE)
	{
		v8::Isolate* isolate = info.GetIsolate();
		using value_type = typename call_from_v8_traits<Get>::template arg_type<0>;
		info.GetReturnValue().Set(to_v8(isolate, (obj.*get)(v8pp::from_v8<value_type>(isolate, name))));
	}

	//return is pointer
	static void get_impl(class_type& obj,
		Get get,
		v8::Local<v8::String>,
		v8::PropertyCallbackInfo<v8::Value> const& info, pointer_getter_tag, 
		return_empty return_handle = return_empty::NONE)
	{
		v8::Isolate* isolate = info.GetIsolate();

		typename function_traits<Get>::return_type ret = (obj.*get)();
		if (ret == nullptr)
			info.GetReturnValue().SetNull();
		else
			info.GetReturnValue().Set(to_v8(isolate, ret));
	}

		static void get_impl(class_type& obj, 
		Get get, 
		v8::Local<v8::String>,
		v8::PropertyCallbackInfo<v8::Value> const& info, getter_tag, 
		return_empty return_handle = return_empty::NONE)
	{
		v8::Isolate* isolate = info.GetIsolate();

		info.GetReturnValue().Set(to_v8(isolate, (obj.*get)()));
	}

	static void get_impl(class_type& obj, Get get,
		v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info,
		direct_getter_tag, return_empty return_handle = return_empty::NONE)
	{
		(obj.*get)(name, info);
	}

	static void get_impl(class_type& obj, Get get, v8::Local<v8::String>,
		v8::PropertyCallbackInfo<v8::Value> const& info, isolate_getter_tag,
		return_empty return_handle = return_empty::NONE)
	{
		v8::Isolate* isolate = info.GetIsolate();

		info.GetReturnValue().Set(to_v8(isolate, (obj.*get)(isolate)));
	}

	static void get(F name, v8::PropertyCallbackInfo<v8::Value> const& info)
	{
		v8::Isolate* isolate = info.GetIsolate();

		class_type& obj = v8pp::from_v8<class_type&>(isolate, info.This());

		Property prop = detail::get_external_data<Property>(info.Data());
		assert(prop.get_);

		if (prop.get_)
		try
		{
			get_impl(obj, prop.get_, name, info, select_getter_tag<Get>(), prop.r_type);
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}

	static void set(F, v8::Local<v8::Value>, v8::PropertyCallbackInfo<void> const&)
	{
		assert(false && "never should be called");
	}
};

template<typename Get, typename Set>
struct r_property_impl<Get, Set, false>
{
	using F = typename std::conditional<is_index_interceptor<Get>::value,
										uint32_t, v8::Local<v8::String>> ::type;
	using Property = property_<Get, Set>;

	static void get_impl(Get get, F,
		v8::PropertyCallbackInfo<v8::Value> const& info, getter_tag)
	{
		v8::Isolate* isolate = info.GetIsolate();

		info.GetReturnValue().Set(to_v8(isolate, get()));
	}

	static void get_impl(Get get, F name,
		v8::PropertyCallbackInfo<v8::Value> const& info, direct_getter_tag)
	{
		get(name, info);
	}

	static void get_impl(Get get, F,
		v8::PropertyCallbackInfo<v8::Value> const& info, isolate_getter_tag)
	{
		v8::Isolate* isolate = info.GetIsolate();

		info.GetReturnValue().Set(to_v8(isolate, (get)(isolate)));
	}

	static void get(F name, v8::PropertyCallbackInfo<v8::Value> const& info)
	{
		v8::Isolate* isolate = info.GetIsolate();

		Property prop = detail::get_external_data<Property>(info.Data());
		assert(prop.get_);

		if (prop.get_)
		try
		{
			get_impl(prop.get_, name, info, select_getter_tag<Get>());
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}

	static void set(F, v8::Local<v8::Value>, v8::PropertyCallbackInfo<void> const&)
	{
		assert(false && "never should be called");
	}
};

template<typename Get, typename Set>
struct rw_property_impl<Get, Set, true>
	: r_property_impl<Get, Set, std::is_member_function_pointer<Get>::value>
{
	using Property = property_<Get, Set>;
	using Index_or_name = typename std::conditional < is_index_interceptor<Get>::value,
										uint32_t, v8::Local < v8::String >> ::type;
	using class_type = typename std::tuple_element<0,
		typename function_traits<Set>::arguments>::type;

	static void set_impl(class_type& obj, Set set, Index_or_name,
		v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info, setter_tag)
	{
		using value_type = typename call_from_v8_traits<Set>::template arg_type<0>;

		v8::Isolate* isolate = info.GetIsolate();

		(obj.*set)(v8pp::from_v8<value_type>(isolate, value));
	}

	static void set_impl(class_type& obj, Set set, Index_or_name name,
		v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info, direct_setter_tag)
	{
		(obj.*set)(name, value, info);
	}

	static void set_impl(class_type& obj, Set set, Index_or_name,
		v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info, isolate_setter_tag)
	{
		using value_type = typename call_from_v8_traits<Set>::template arg_type<1>;

		v8::Isolate* isolate = info.GetIsolate();
		(obj.*set)(isolate, v8pp::from_v8<value_type>(isolate, value));
	}

	static void set(Index_or_name name, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info)
	{
		v8::Isolate* isolate = info.GetIsolate();

		class_type& obj = v8pp::from_v8<class_type&>(isolate, info.This());

		Property prop = detail::get_external_data<Property>(info.Data());
		assert(prop.set_);

		if (prop.set_)
		try
		{
			set_impl(obj, prop.set_, name, value, info, select_setter_tag<Set>());
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}
};

template<typename Get, typename Set>
struct rw_property_impl<Get, Set, false>
	: r_property_impl<Get, Set, std::is_member_function_pointer<Get>::value>
{
	using Property = property_<Get, Set>;
	using Index_or_name = typename std::conditional < is_index_interceptor<Get>::value,
		uint32_t, v8::Local < v8::String >> ::type;
	static void set_impl(Set set, Index_or_name, v8::Local<v8::Value> value,
		v8::PropertyCallbackInfo<void> const& info, setter_tag)
	{
		using value_type = typename call_from_v8_traits<Set>::template arg_type<0>;

		v8::Isolate* isolate = info.GetIsolate();

		set(v8pp::from_v8<value_type>(isolate, value));
	}

	static void set_impl(Set set, Index_or_name name, v8::Local<v8::Value> value,
		v8::PropertyCallbackInfo<void> const& info, direct_setter_tag)
	{
		set(name, value, info);
	}

	static void set_impl(Set set, Index_or_name, v8::Local<v8::Value> value,
		v8::PropertyCallbackInfo<void> const& info, isolate_setter_tag)
	{
		using value_type = typename call_from_v8_traits<Set>::template arg_type<1>;

		v8::Isolate* isolate = info.GetIsolate();

		set(isolate, v8pp::from_v8<value_type>(isolate, value));
	}

	static void set(Index_or_name name, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info)
	{
		v8::Isolate* isolate = info.GetIsolate();

		Property prop = detail::get_external_data<Property>(info.Data());
		assert(prop.set_);

		if (prop.set_)
		try
		{
			set_impl(prop.set_, name, value, info, select_setter_tag<Set>());
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}
};

} // namespace detail

/// Property with get and set functions
template<typename Get, typename Set>
struct property_ : detail::rw_property_impl<Get, Set, std::is_member_function_pointer<Set>::value>, public ref_debug<property_<Get, Set>>
{
	static_assert(detail::is_getter<Get>::value
		|| detail::is_direct_getter<Get>::value
		|| detail::is_isolate_getter<Get>::value
		|| detail::is_interceptor<Get>::value,
		"property get function must be either `T ()` or "
		"`void (v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info)` or "
		"`T (v8::Isolate*)`");

	static_assert(detail::is_setter<Set>::value
		|| detail::is_direct_setter<Set>::value
		|| detail::is_isolate_setter<Set>::value
		|| detail::is_interceptor<Get>::value,
		"property set function must be either `void (T)` or \
		`void (v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info)` or \
		`void (v8::Isolate*, T)`");

	Get get_;
	Set set_;
	return_empty r_type;
	enum { is_readonly = false };
};

/// Read-only property class specialization for get only method
template<typename Get>
struct property_<Get, Get> : detail::r_property_impl<Get, Get, std::is_member_function_pointer<Get>::value>, public ref_debug<property_<Get, Get>>
{
	static_assert(detail::is_getter<Get>::value
		|| detail::is_direct_getter<Get>::value
		|| detail::is_isolate_getter<Get>::value
		|| detail::is_interceptor<Get>::value,
		"property get function must be either `T ()` or "
		"void (v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info)` or "
		"`T (v8::Isolate*)`");

	Get get_;
	return_empty r_type;
	enum { is_readonly = true };
};

/// Create read/write property from get and set member functions
template<typename Get, typename Set>
property_<Get, Set> property(Get get, Set set, return_empty return_handle = return_empty::NONE)
{
	property_<Get, Set> prop;
	prop.get_ = get;
	prop.set_ = set;
	prop.r_type = return_handle;
	return prop;
}

/// Create read-only property from a get function
template<typename Get>
property_<Get, Get> property(Get get, return_empty return_handle = return_empty::NONE)
{
	property_<Get, Get> prop;
	prop.get_ = get;
	prop.r_type = return_handle;
	return prop;
}

} // namespace v8pp

#endif // V8PP_PROPERTY_HPP_INCLUDED
