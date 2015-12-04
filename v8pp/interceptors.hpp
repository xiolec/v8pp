#ifndef V8PP_INTERCEPTOR_HPP_INCLUDED
#define V8PP_INTERCEPTOR_HPP_INCLUDED

#include "v8.h"
#include "function.hpp"
#include "property.hpp"

#include "member_checkers.h"
#include "reference_tracker.h"

namespace v8pp
{
	namespace detail {
		struct query_bool_return{};

		template <typename F>
		using is_bool_return = std::integral_constant<bool,
			call_from_v8_traits<F>::arg_count == 1 &&
			std::is_same < bool, typename function_traits<F>::return_type >::value> ;

		template<typename F>
		using select_query_tag = typename std::conditional<
			is_string_pointer_return<F>::value,
				interceptor_string_pointer_return,
				query_bool_return
		>::type;



		namespace property_helpers
		{
			template<typename ret_type, typename pure_arg = remove_all<ret_type>::type>
			static convertible_string<pure_arg> get_c_type(v8::Local<v8::String> &name, v8::Isolate *isolate)
			{
				return v8pp::from_v8<ret_type>(isolate, name);
			}

			template<typename ret_type, typename pure_arg = remove_all<ret_type>::type>
			static uint32_t get_c_type(uint32_t &num, v8::Isolate *isolate)
			{
				return num;
			}


		};

		template<typename method>
		struct handler_types
		{
			using Index_or_name = typename std::conditional < is_index_interceptor<method>::value,
				uint32_t, v8::Local < v8::String >> ::type;
			using class_type = typename std::tuple_element<0,
				typename function_traits<method>::arguments>::type;
		};

		template<typename Get>
		struct get_handlers : public handler_types<Get>
		{

			static_assert(is_getter<Get>::value
				|| is_direct_getter<Get>::value
				|| is_isolate_getter<Get>::value
				|| is_interceptor<Get>::value,
				"property get function must be either `T ()` or \
							`void (v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info)` or \
																								`T (v8::Isolate*)`");

			//index interceptor with pointer return
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

			//index interceptor non void return
			static void get_impl(class_type& obj,
				Get get,
				uint32_t index,
				v8::PropertyCallbackInfo<v8::Value> const& info, interceptor_tag,
				return_empty return_handle = return_empty::NONE)
			{
				v8::Isolate* isolate = info.GetIsolate();

				info.GetReturnValue().Set(to_v8(isolate, (obj.*get)(index)));
			}

			//index interceptor with string pointer return
			static void get_impl(class_type& obj,
				Get get,
				uint32_t index,
				v8::PropertyCallbackInfo<v8::Value> const& info, interceptor_string_pointer_return,
				return_empty return_handle = return_empty::NONE)
			{
				v8::Isolate* isolate = info.GetIsolate();
				typename function_traits<Get>::return_type ret = (obj.*get)(index);
				if (ret == nullptr)
				{
					if (return_handle == NONE)
						info.GetReturnValue().SetEmptyString();
					else if (return_handle == SET_NULL)
						info.GetReturnValue().SetNull();
					else
						info.GetReturnValue().SetUndefined();
				}
				else
					info.GetReturnValue().Set(to_v8(isolate, *ret));
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

			//named interceptor with string return
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
					else if (return_handle == SET_NULL)
						info.GetReturnValue().SetNull();
					else
						info.GetReturnValue().SetUndefined();
				}
				else
					info.GetReturnValue().Set(to_v8(isolate, *ret));
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
		};

		template<typename Set>
		struct set_handlers : public handler_types<Set>
		{
			static void set_impl(class_type& obj, Set set, Index_or_name,
				v8::Local<v8::Value> value, v8::PropertyCallbackInfo<v8::Value> const& info, setter_tag)
			{
				using value_type = typename call_from_v8_traits<Set>::template arg_type<0>;

				v8::Isolate* isolate = info.GetIsolate();

				(obj.*set)(v8pp::from_v8<value_type>(isolate, value));
			}

			static void set_impl(class_type& obj, Set set, Index_or_name name,
				v8::Local<v8::Value> value, v8::PropertyCallbackInfo<v8::Value> const& info, direct_setter_tag)
			{
				(obj.*set)(name, value, info);
			}

			static void set_impl(class_type& obj, Set set, Index_or_name,
				v8::Local<v8::Value> value, v8::PropertyCallbackInfo<v8::Value> const& info, isolate_setter_tag)
			{
				using value_type = typename call_from_v8_traits<Set>::template arg_type<1>;

				v8::Isolate* isolate = info.GetIsolate();
				(obj.*set)(isolate, v8pp::from_v8<value_type>(isolate, value));
			}

			static void set_impl(class_type& obj, Set set, Index_or_name index_name,
				v8::Local<v8::Value> value, v8::PropertyCallbackInfo<v8::Value> const& info, setter_interceptor_tag)
			{
				using arg0 = typename call_from_v8_traits<Set>::template arg_type<0>;
				using arg1 = typename call_from_v8_traits<Set>::template arg_type<1>;

				v8::Isolate* isolate = info.GetIsolate();
				auto param0 = property_helpers::get_c_type<arg0>(index_name, isolate);

				(obj.*set)(param0, v8pp::from_v8<arg1>(isolate, value));
			}
		};

		/*
			Enum possibilities

			vector<type> get_enum();
		*/
		template<typename Enum>
		struct enum_handlers : public handler_types<Enum>
		{
			static_assert(is_enumer<Enum>::value,
				"Enumeration hanndler must be `container<type> Enum_function()`");

			static void enum_impl(class_type& obj, Enum enumer, const v8::PropertyCallbackInfo<v8::Array>& info)
			{
				using value_type = typename function_traits<Enum>::return_type;

				v8::Isolate* isolate = info.GetIsolate();
				value_type c_ret = (obj.*enumer)();

				info.GetReturnValue().Set(v8pp::to_v8(isolate, c_ret.begin(), c_ret.end()));
			}
		};

		template<typename Query>
		struct query_handlers : public handler_types<Query>
		{

			static void query_impl(class_type& obj,
				Query get,
				Index_or_name name,
				v8::PropertyCallbackInfo<v8::Integer> const& info, interceptor_string_pointer_return)
			{
				v8::Isolate* isolate = info.GetIsolate();
				using value_type = typename call_from_v8_traits<Query>::template arg_type<0>;
				typename function_traits<Query>::return_type ret = (obj.*get)(property_helpers::get_c_type<value_type>(name, info.GetIsolate()));
				if (ret != nullptr)
					info.GetReturnValue().Set(int32_t(v8::None));
			}

			static void query_impl(class_type& obj,
				Query get,
				Index_or_name name,
				v8::PropertyCallbackInfo<v8::Integer> const& info, query_bool_return)
			{
				v8::Isolate* isolate = info.GetIsolate();
				using value_type = typename call_from_v8_traits<Query>::template arg_type<0>;
				bool ret = (obj.*get)(property_helpers::get_c_type<value_type>(name, info.GetIsolate()));
				if (ret)
					info.GetReturnValue().Set(int32_t(v8::None));
			}
		};

		template<typename Del>
		struct del_handlers : public handler_types<Del>
		{
			//template<typename Name>
			static void del_impl(class_type& obj,
				Del get,
				Index_or_name name,
				v8::PropertyCallbackInfo<v8::Boolean> const& info)
			{
				v8::Isolate* isolate = info.GetIsolate();
				using value_type = typename call_from_v8_traits<Del>::template arg_type<0>;
				bool ret = (obj.*get)(property_helpers::get_c_type<value_type>(name, isolate));
				if (ret)
					info.GetReturnValue().Set(ret);
			}
		};
	}

	

	CREATE_MEMBER_CHECK(GLOBAL_INSTANCE);
	template<typename Get, typename Set, typename Enum, typename Query, typename Del>
	struct base_types
	{
		using active_type =
			typename std::conditional<std::is_same<Get, bool>::value,
				typename std::conditional<std::is_same<Set, bool>::value,
					typename std::conditional<std::is_same<Query, bool>::value,
						bool,
						Query
					>::type,
					Set
				>::type,
				Get
			> ::type;
		using active_type_enum =
			typename std::conditional<std::is_same<active_type, bool>::value,
				typename std::conditional<std::is_same<Enum, bool>::value,
					bool,
					Enum
				>::type,
				active_type
			> ::type;

		using class_type = typename std::tuple_element<0,
			typename detail::function_traits<active_type_enum>::arguments>::type;

		using pure_class = typename detail::remove_all<class_type>::type;
	};

	template<typename Get, typename Set, typename Enum, typename Query, typename Del>
	struct interceptor_data;
	template<typename Get, typename Set, typename Enum, typename Query, typename Del>
	struct indexed_interceptor : base_types<Get, Set, Enum, Query, Del>
	{
		using Indexed_Data = interceptor_data<Get, Set, Enum, Query, Del>;

		using Index_or_name =
			typename std::conditional<std::is_same<active_type, bool>::value, 
					v8::Local < v8::String>,
					typename std::conditional<detail::is_index_interceptor<active_type>::value, //gets the value from setter
						uint32_t, v8::Local < v8::String >
					> ::type
			> ::type;

		template<typename U = pure_class>
		static void get_global_obj(v8::Isolate *isolate, 
			typename std::enable_if<has_member_GLOBAL_INSTANCE<U>::value>::type *obj)
		{
			obj = pure_class::GLOBAL_INSTANCE(isolate);
		}

		template<typename U = pure_class>
		static void get_global_obj(v8::Isolate *isolate, 
			typename std::enable_if<!has_member_GLOBAL_INSTANCE<U>::value>::type *obj)
		{
		}

		template<typename U = Enum>
		static typename std::enable_if<std::is_same<U, bool>::value>::type propertyEnumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
		{
		}

		template<typename U = Enum>
		static typename std::enable_if<!std::is_same<U, bool>::value>::type propertyEnumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
		{
			v8::Isolate* isolate = info.GetIsolate();

			pure_class* obj = v8pp::class_<pure_class>::unwrap_object(isolate, info.This());

			Indexed_Data prop = detail::get_external_data<Indexed_Data>(info.Data());
			assert(prop.enum_);

			if (prop.enum_)
			try
			{
				detail::enum_handlers<Enum>::enum_impl(*obj, prop.enum_, info);
			}
			catch (std::exception const& ex)
			{
				//info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
			}
		}

		template<typename U = Set>
		static typename std::enable_if<std::is_same<U, bool>::value>::type propertySetter(Index_or_name index, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
		{
		}

		template<typename U = Set>
		static typename std::enable_if<!std::is_same<U, bool>::value>::type propertySetter(Index_or_name index, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
		{
			v8::Isolate* isolate = info.GetIsolate();
			//using pure_class_type = typename detail::remove_all<class_type>::type;
			pure_class* obj = v8pp::class_<pure_class>::unwrap_object(isolate, info.This());

			if (obj == nullptr)
			{
				get_global_obj(isolate, obj);
				if (obj == nullptr)
					return;
			}

			Indexed_Data prop = detail::get_external_data<Indexed_Data>(info.Data());
			assert(prop.set_);

			if (prop.set_)
				try
			{
				detail::set_handlers<Set>::set_impl(*obj, prop.set_, index, value, info, detail::select_setter_intercept_tag<Set>());
				info.GetReturnValue().SetNull();
			}
			catch (std::exception const& ex)
			{
				info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
			}
		}

		template<typename U = Get>
		static typename std::enable_if<std::is_same<U, bool>::value>::type propertyGetter(Index_or_name index, const v8::PropertyCallbackInfo<v8::Value>& info)
		{
		}

		template<typename U = Get>
		static typename std::enable_if<!std::is_same<U, bool>::value>::type propertyGetter(Index_or_name index, const v8::PropertyCallbackInfo<v8::Value>& info)
		{
			v8::Isolate* isolate = info.GetIsolate();

			pure_class* obj = v8pp::class_<pure_class>::unwrap_object(isolate, info.This());

			Indexed_Data data = detail::get_external_data<Indexed_Data>(info.Data());
			assert(data.get_);

			if (data.get_)
			try
			{
				detail::get_handlers<Get>::get_impl(*obj, data.get_, index, info, detail::select_getter_tag<Get>(), data.r_type);
			}
			catch (std::exception const& ex)
			{
				info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
			}
		}

		template<typename U = Query>
		static typename std::enable_if<std::is_same<U, bool>::value>::type propertyQuery(Index_or_name index, const v8::PropertyCallbackInfo<v8::Integer>& info)
		{
		}

		template<typename U = Query>
		static typename std::enable_if<!std::is_same<U, bool>::value>::type propertyQuery(Index_or_name index, const v8::PropertyCallbackInfo<v8::Integer>& info)
		{
			v8::Isolate* isolate = info.GetIsolate();

			pure_class* obj = v8pp::class_<pure_class>::unwrap_object(isolate, info.This());

			Indexed_Data data = detail::get_external_data<Indexed_Data>(info.Data());
			assert(data.query_);

			if (data.query_)
			try
			{
				detail::query_handlers<Query>::query_impl(*obj, data.query_, index, info, detail::select_query_tag<Query>());
			}
			catch (std::exception const& ex)
			{
				//info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
			}
		}

		template<typename U = Del>
		static typename std::enable_if<std::is_same<U, bool>::value>::type propertyDel(Index_or_name index, const v8::PropertyCallbackInfo<v8::Boolean>& info)
		{
		}

		template<typename U = Del>
		static typename std::enable_if<!std::is_same<U, bool>::value>::type propertyDel(Index_or_name index, const v8::PropertyCallbackInfo<v8::Boolean>& info)
		{
			v8::Isolate* isolate = info.GetIsolate();

			pure_class* obj = v8pp::class_<pure_class>::unwrap_object(isolate, info.This());

			Indexed_Data data = detail::get_external_data<Indexed_Data>(info.Data());
			assert(data.del_);

			if (data.del_)
			try
			{
				detail::del_handlers<Del>::del_impl(*obj, data.del_, index, info);
			}
			catch (std::exception const& ex)
			{
				//info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
			}
		}


	};


	template<typename Get, typename Set, typename Enum, typename Query, typename Del>
	struct interceptor_data : public ref_debug<interceptor_data<Get, Set, Enum, Query, Del>>
	{
		Set set_;
		Get get_;
		Enum enum_;
		Query query_;
		Del del_;
		return_empty r_type = return_empty::NONE;

		static const bool using_set = !std::is_same<Set, bool>::value;
		static const bool using_get = !std::is_same<Get, bool>::value;
		static const bool using_enum = !std::is_same<Enum, bool>::value;
		static const bool using_query = !std::is_same<Query, bool>::value;
		static const bool using_del = !std::is_same<Del, bool>::value;
	};

	template<typename Get>
	interceptor_data<Get, bool, bool, bool, bool> intercept_get(Get get, return_empty return_handle = return_empty::NONE)
	{
		interceptor_data<Get, bool, bool, bool, bool> data;
		data.get_ = get;
		data.r_type = return_handle;
		return data;
	}

	template<typename Get, typename Del>
	interceptor_data<Get, bool, bool, bool, Del> intercept_get_del(Get get, Del del, return_empty return_handle = return_empty::NONE)
	{
		interceptor_data<Get, bool, bool, bool, Del> data;
		data.get_ = get;
		data.del_ = del;
		data.r_type = return_handle;
		return data;
	}

	template<typename Get, typename Query>
	interceptor_data<Get, bool, bool, Query, bool> intercept_get_query(Get get, Query query, return_empty return_handle = return_empty::NONE)
	{
		interceptor_data<Get, bool, bool, Query, bool> data;
		data.get_ = get;
		data.query_ = query;
		data.r_type = return_handle;
		return data;
	}

	template<typename Get, typename Query, typename Del>
	interceptor_data<Get, bool, bool, Query, Del> intercept_get_query_del(Get get, Query query, Del del, return_empty return_handle = return_empty::NONE)
	{
		interceptor_data<Get, bool, bool, Query, Del> data;
		data.get_ = get;
		data.query_ = query;
		data.del_ = del;
		data.r_type = return_handle;
		return data;
	}

	template<typename Get, typename Enum>
	interceptor_data<Get, bool, Enum, bool, bool> intercept_get_enum(Get get, Enum enumer, return_empty return_handle = return_empty::NONE)
	{
		interceptor_data<Get, bool, Enum, bool, bool> data;
		data.get_ = get;
		data.enum_ = enumer;
		data.r_type = return_handle;
		return data;
	}

	template<typename Get, typename Enum, typename Del>
	interceptor_data<Get, bool, Enum, bool, Del> intercept_get_enum_del(Get get, Enum enumer, Del del, return_empty return_handle = return_empty::NONE)
	{
		interceptor_data<Get, bool, Enum, bool, Del> data;
		data.get_ = get;
		data.enum_ = enumer;
		data.del_ = del;
		data.r_type = return_handle;
		return data;
	}

	template<typename Get, typename Enum, typename Query>
	interceptor_data<Get, bool, Enum, Query, bool> intercept_get_enum_query(Get get, Enum enumer, Query query, return_empty return_handle = return_empty::NONE)
	{
		interceptor_data<Get, bool, Enum, Query, bool> data;
		data.get_ = get;
		data.enum_ = enumer;
		data.query_ = query;
		data.r_type = return_handle;
		return data;
	}

	template<typename Get, typename Enum, typename Query, typename Del>
	interceptor_data<Get, bool, Enum, Query, Del> intercept_get_enum_query_del(Get get, Enum enumer, Query query, Del del, return_empty return_handle = return_empty::NONE)
	{
		interceptor_data<Get, bool, Enum, Query, Del> data;
		data.get_ = get;
		data.enum_ = enumer;
		data.query_ = query;
		data.del_ = del;
		data.r_type = return_handle;
		return data;
	}

	template<typename Get, typename Set>
	interceptor_data<Get, Set, bool, bool, bool> intercept_get_set(Get get, Set set, return_empty return_handle = return_empty::NONE)
	{
		interceptor_data<Get, Set, bool, bool, bool> data;
		data.get_ = get;
		data.set_ = set;
		data.r_type = return_handle;
		return data;
	}

	template<typename Get, typename Set, typename Del>
	interceptor_data<Get, Set, bool, bool, Del> intercept_get_set_del(Get get, Set set, Del del, return_empty return_handle = return_empty::NONE)
	{
		interceptor_data<Get, Set, bool, bool, Del> data;
		data.get_ = get;
		data.set_ = set;
		data.del_ = del;
		data.r_type = return_handle;
		return data;
	}

	template<typename Get, typename Set, typename Query>
	interceptor_data<Get, Set, bool, Query, bool> intercept_get_set_query(Get get, Set set, Query query, return_empty return_handle = return_empty::NONE)
	{
		interceptor_data<Get, Set, bool, Query, bool> data;
		data.get_ = get;
		data.set_ = set;
		data.query_ = query;
		data.r_type = return_handle;
		return data;
	}

	template<typename Get, typename Set, typename Query, typename Del>
	interceptor_data<Get, Set, bool, Query, Del> intercept_get_set_query_del(Get get, Set set, Query query, Del del, return_empty return_handle = return_empty::NONE)
	{
		interceptor_data<Get, Set, bool, Query, Del> data;
		data.get_ = get;
		data.set_ = set;
		data.del_ = del;
		data.query_ = query;
		data.r_type = return_handle;
		return data;
	}

	template<typename Get, typename Set, typename Enum, typename Query, typename Del>
	interceptor_data<Get, Set, Enum, Query, Del> intercept_all(Get get, Set set, Enum enumer, Query query, Del del, return_empty return_handle = return_empty::NONE)
	{
		interceptor_data<Get, Set, Enum, Query, Del> data;
		data.get_ = get;
		data.set_ = set; 
		data.enum_ = enumer;
		data.query_ = query;
		data.del_ = del;
		data.r_type = return_handle;
		return data;
	}

	template<typename Set>
	interceptor_data<bool, Set, bool, bool, bool> intercept_set(Set set, return_empty return_handle = return_empty::NONE)
	{
		interceptor_data<bool, Set, bool, bool, bool> data;
		data.set_ = set;
		data.r_type = return_handle;
		return data;
	}

	template<typename Set, typename Del>
	interceptor_data<bool, Set, bool, bool, Del> intercept_set_del(Set set, Del del, return_empty return_handle = return_empty::NONE)
	{
		interceptor_data<bool, Set, bool, bool, Del> data;
		data.set_ = set;
		data.del_ = del;
		data.r_type = return_handle;
		return data;
	}

	template<typename Set, typename Query>
	interceptor_data<bool, Set, bool, Query, bool> intercept_set_query(Set set, Query query, return_empty return_handle = return_empty::NONE)
	{
		interceptor_data<bool, Set, bool, Query, bool> data;
		data.set_ = set;
		data.query_ = query;
		data.r_type = return_handle;
		return data;
	}

	template<typename Set, typename Query, typename Del>
	interceptor_data<bool, Set, bool, Query, Del> intercept_set_query_del(Set set, Query query, Del del, return_empty return_handle = return_empty::NONE)
	{
		interceptor_data<bool, Set, bool, Query, Del> data;
		data.set_ = set;
		data.query_ = query;
		data.del_ = del;
		data.r_type = return_handle;
		return data;
	}

	template<typename Set, typename Enum>
	interceptor_data<bool, Set, Enum, bool, bool> intercept_set_enum(Set set, Enum enumer, return_empty return_handle = return_empty::NONE)
	{
		interceptor_data<bool, Set, Enum, bool, bool> data;
		data.set_ = set;
		data.enum_ = enumer;
		data.r_type = return_handle;
		return data;
	}

	template<typename Set, typename Enum, typename Del>
	interceptor_data<bool, Set, Enum, bool, Del> intercept_set_enum_del(Set set, Enum enumer, Del del, return_empty return_handle = return_empty::NONE)
	{
		interceptor_data<bool, Set, Enum, bool, Del> data;
		data.set_ = set;
		data.enum_ = enumer;
		data.del_ = del;
		data.r_type = return_handle;
		return data;
	}

	template<typename Set, typename Enum, typename Query>
	interceptor_data<bool, Set, Enum, Query, bool> intercept_set_enum_query(Set set, Enum enumer, Query query, return_empty return_handle = return_empty::NONE)
	{
		interceptor_data<bool, Set, Enum, Query, bool> data;
		data.set_ = set;
		data.enum_ = enumer;
		data.query_ = query;
		data.r_type = return_handle;
		return data;
	}

	template<typename Set, typename Enum, typename Query, typename Del>
	interceptor_data<bool, Set, Enum, Query, Del> intercept_set_enum_query_del(Set set, Enum enumer, Query query, Del del, return_empty return_handle = return_empty::NONE)
	{
		interceptor_data<bool, Set, Enum, Query, Del> data;
		data.set_ = set;
		data.enum_ = enumer;
		data.query_ = query;
		data.del_ = del;
		data.r_type = return_handle;
		return data;
	}
}

#endif