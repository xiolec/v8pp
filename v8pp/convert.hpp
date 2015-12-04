#ifndef V8PP_CONVERT_HPP_INCLUDED
#define V8PP_CONVERT_HPP_INCLUDED

#include <v8.h>

#include <climits>
#include <string>
#include <vector>
#include <map>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>
#include "v8pp/reference_tracker.h"
#include "v8pp/any_object.h"

#if defined(WIN32)
#include <iostream>
#include <locale>
#include <codecvt>
#include <string>
#endif

namespace v8pp  {

namespace detail {

	template<class T, class U =
		typename std::remove_cv<
		typename std::remove_pointer<
		typename std::remove_reference<
		typename std::remove_extent<
		T
		>::type
		>::type
		>::type
		>::type
	> struct remove_all : remove_all<U> {};
	template<class T> struct remove_all<T, T> { typedef T type; };

// A string that converts to Char const * (useful for fusion::invoke)
template<typename Char>
struct convertible_string : std::basic_string<Char>, ref_debug<convertible_string<Char>>
{
	convertible_string(Char const *str, size_t len) : std::basic_string<Char>(str, len) {}

	convertible_string(std::basic_string<Char> &sz) : std::basic_string<Char>(sz.c_str(), sz.size()) {}

	operator Char const*() const { return this->c_str(); }
};

} // namespace detail

template<typename T>
class class_;

// Generic convertor
template<typename T, typename Enable = void>
struct convert;
/*
{
	using from_type = T;
	using to_type = v8::Handle<v8::Value>;

	static bool is_valid(v8::Isolate* isolate, v8::Handle<v8::Value> value);

	static from_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value);
	static to_type to_v8(v8::Isolate* isolate, T const& value);
};
*/

template<typename U>
static typename std::enable_if<std::is_same<U, char>::value, 
	std::string>::type num_to_string(v8::Handle<v8::Number> &num_val, v8::Isolate *isolate)
{
	return std::to_string(static_cast<long>(num_val->NumberValue()));
}

template<typename U>
static typename std::enable_if<std::is_same<U, wchar_t>::value,
	std::wstring> ::type num_to_string(v8::Handle<v8::Number> &num_val, v8::Isolate *isolate)
{
	return std::to_wstring(static_cast<long>(num_val->NumberValue()));
}

// converter specializations for string types
template<typename Char, typename Traits, typename Alloc>
struct convert< std::basic_string<Char, Traits, Alloc>>
{
	static_assert(sizeof(Char) <= sizeof(uint16_t),
		"only UTF-8 and UTF-16 strings are supported");

	using from_type = std::basic_string<Char, Traits, Alloc>;
	using to_type = v8::Handle<v8::String>;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return !value.IsEmpty() && (value->IsString() || value->IsNull() || value->IsUndefined() || value->IsNumber()
			|| value->IsBoolean() || value->IsObject() || value->IsArray());
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		if (value->IsNull())
			return detail::convertible_string<Char>("null", 4);
		if (value->IsUndefined())
			return detail::convertible_string<Char>("undefined", 9);
		if (value->IsNumber())
			return num_to_string<Char>(v8::Handle<v8::Number>::Cast(value), isolate);
		else if (value->IsBoolean())
		{
			if (value->ToBoolean()->BooleanValue())
				return detail::convertible_string<Char>("true", 4);
			else
				return detail::convertible_string<Char>("false", 5);
		}
		else if (value->IsArray())
		{
			std::basic_string<Char> ret_str;
			v8::Handle<v8::Array> array_obj = v8::Handle<v8::Array>::Cast(value);
			for (size_t x = 0; x < array_obj->Length(); x++)
			{
				if (x != 0)
					ret_str += ",";
				ret_str += from_v8(isolate, array_obj->Get(x));
			}
			return ret_str;
		}
		else if (value->IsObject())
			return from_v8(isolate, value->ToString());
		if (!is_valid(isolate, value))
		{
			throw std::invalid_argument("expected String");
		}

		if (sizeof(Char) == 1)
		{
			v8::String::Utf8Value const str(value);
			return from_type(reinterpret_cast<Char const*>(*str), str.length());
		}
		else
		{
			v8::String::Value const str(value);
			return from_type(reinterpret_cast<Char const*>(*str), str.length());
		}
	}

	static to_type to_v8(v8::Isolate* isolate, from_type const& value)
	{
		if (sizeof(Char) == 1)
		{
#if defined(WIN32)
			std::string test_sz = value;
			std::wstring winstr;
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
			winstr = converter.from_bytes(test_sz);
			return v8::String::NewFromTwoByte(isolate, reinterpret_cast<uint16_t const*>(winstr.c_str()), v8::String::kNormalString, winstr.length());
#else
			return v8::String::NewFromUtf8(isolate, reinterpret_cast<char const*>(value.data()),
				v8::String::kNormalString, static_cast<int>(value.length()));
#endif
		}
		else
		{
			return v8::String::NewFromTwoByte(isolate, reinterpret_cast<uint16_t const*>(value.data()),
				v8::String::kNormalString, static_cast<int>(value.length()));
		}
	}
};

template<typename Char>
struct convert<Char const*>
{
	static_assert(sizeof(Char) <= sizeof(uint16_t),
		"only UTF-8 and UTF-16 strings are supported");

	using from_type = detail::convertible_string<Char>;
	using to_type = v8::Handle<v8::String>;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return !value.IsEmpty() && (value->IsString() || value->IsNull() || value->IsUndefined() || value->IsNumber()
			|| value->IsBoolean() || value->IsObject() || value->IsArray());
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		if (value->IsNull())
			return detail::convertible_string<Char>("null", 4);
		else if (value->IsUndefined())
			return detail::convertible_string<Char>("undefined", 9);
		else if (value->IsNumber())
			return num_to_string<Char>(v8::Handle<v8::Number>::Cast(value), isolate);
		else if (value->IsBoolean())
		{
			if (value->ToBoolean()->BooleanValue())
				return detail::convertible_string<Char>("true", 4);
			else
				return detail::convertible_string<Char>("false", 5);
		}
		else if (value->IsArray())
		{
			std::basic_string<Char> ret_str;
			v8::Handle<v8::Array> array_obj = v8::Handle<v8::Array>::Cast(value);
			for (size_t x = 0; x < array_obj->Length(); x++)
			{
				if (x != 0)
					ret_str += ",";
				ret_str += from_v8(isolate, array_obj->Get(x));
			}
			return ret_str;
		}
		else if (value->IsObject())
			return from_v8(isolate, value->ToString());

		if (!is_valid(isolate, value))
		{
			throw std::invalid_argument("expected String");
		}

		if (sizeof(Char) == 1)
		{
			v8::String::Utf8Value const str(value);
			return from_type(reinterpret_cast<Char const*>(*str), str.length());
		}
		else
		{
			v8::String::Value const str(value);
			return from_type(reinterpret_cast<Char const*>(*str), str.length());
		}
	}

	static to_type to_v8(v8::Isolate* isolate, Char const* value)
	{
		if (sizeof(Char) == 1)
		{
			return v8::String::NewFromUtf8(isolate, reinterpret_cast<char const*>(value));
		}
		else
		{
			return v8::String::NewFromTwoByte(isolate, reinterpret_cast<uint16_t const*>(value));
		}
	}
};

// converter specializations for primitive types
template<>
struct convert<bool>
{
	using from_type = bool;
	using to_type = v8::Handle<v8::Boolean>;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return !value.IsEmpty() && value->IsBoolean();
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			throw std::invalid_argument("expected Boolean");
		}
		return value->ToBoolean()->Value();
	}

	static to_type to_v8(v8::Isolate* isolate, bool value)
	{
		return v8::Boolean::New(isolate, value);
	}
};

template<typename T>
struct convert<T, typename std::enable_if<std::is_integral<T>::value>::type>
{
	using from_type = T;
	using to_type = v8::Handle<v8::Number>;

	enum { bits = sizeof(T) * CHAR_BIT, is_signed = std::is_signed<T>::value };

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return !value.IsEmpty() && value->IsNumber();
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			throw std::invalid_argument("expected Number");
		}

		if (bits <= 32)
		{
			if (is_signed)
			{
				return static_cast<T>(value->Int32Value());
			}
			else
			{
				return static_cast<T>(value->Uint32Value());
			}
		}
		else
		{
			return static_cast<T>(value->IntegerValue());
		}
	}

	static to_type to_v8(v8::Isolate* isolate, T value)
	{
		if (bits <= 32)
		{
			if (is_signed)
			{
				return v8::Integer::New(isolate, static_cast<int32_t>(value));
			}
			else
			{
				return v8::Integer::NewFromUnsigned(isolate, static_cast<uint32_t>(value));
			}
		}
		else
		{
			//TODO: check value < (1<<57) to fit in double?
			return v8::Number::New(isolate, static_cast<double>(value));
		}
	}
};

template<typename T>
struct convert<T, typename std::enable_if<std::is_enum<T>::value>::type>
{
	using underlying_type = typename std::underlying_type<T>::type;

	using from_type = T;
	using to_type = typename convert<underlying_type>::to_type;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return !value.IsEmpty() && value->IsNumber();
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			throw std::invalid_argument("expected Number");
		}
		return static_cast<T>(convert<underlying_type>::from_v8(isolate, value));
	}

	static to_type to_v8(v8::Isolate* isolate, T value)
	{
		return convert<underlying_type>::to_v8(isolate, value);
	}
};

template<typename T>
struct convert<T, typename std::enable_if<std::is_floating_point<T>::value>::type>
{
	using from_type = T;
	using to_type = v8::Handle<v8::Number>;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return !value.IsEmpty() && value->IsNumber();
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			throw std::invalid_argument("expected Number");
		}

		return static_cast<T>(value->NumberValue());
	}

	static to_type to_v8(v8::Isolate* isolate, T value)
	{
		return v8::Number::New(isolate, value);
	}
};

// convert Array <-> std::vector
template<typename T, typename Alloc>
struct convert<std::vector<T, Alloc>>
{
	using from_type = std::vector<T, Alloc>;
	using to_type = v8::Handle<v8::Array>;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return !value.IsEmpty() && value->IsArray();
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			throw std::invalid_argument("expected Array");
		}

		v8::HandleScope scope(isolate);
		v8::Local<v8::Array> array = value.As<v8::Array>();

		from_type result;
		result.reserve(array->Length());
		for (uint32_t i = 0, count = array->Length(); i < count; ++i)
		{
			result.emplace_back(convert<T>::from_v8(isolate, array->Get(i)));
		}
		return result;
	}

	static to_type to_v8(v8::Isolate* isolate, from_type const& value)
	{
		v8::EscapableHandleScope scope(isolate);

		uint32_t const size = static_cast<uint32_t>(value.size());
		v8::Local<v8::Array> result = v8::Array::New(isolate, size);
		for (uint32_t i = 0; i < size; ++i)
		{
			result->Set(i, convert<T>::to_v8(isolate, value[i]));
		}
		return scope.Escape(result);
	}
};

// convert Object <-> std::map
template<typename Key, typename Value, typename Less, typename Alloc>
struct convert<std::map<Key, Value, Less, Alloc>>
{
	using from_type = std::map<Key, Value, Less, Alloc>;
	using to_type = v8::Handle<v8::Object>;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return !value.IsEmpty() && value->IsObject();
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			throw std::invalid_argument("expected Object");
		}

		v8::HandleScope scope(isolate);
		v8::Local<v8::Object> object = value.As<v8::Object>();
		v8::Local<v8::Array> prop_names = object->GetPropertyNames();

		from_type result;
		for (uint32_t i = 0, count = prop_names->Length(); i < count; ++i)
		{
			v8::Local<v8::Value> key = prop_names->Get(i);
			v8::Local<v8::Value> val = object->Get(key);
			result.emplace(convert<Key>::from_v8(isolate, key), convert<Value>::from_v8(isolate, val));
		}
		return result;
	}

	static to_type to_v8(v8::Isolate* isolate, from_type const& value)
	{
		v8::EscapableHandleScope scope(isolate);

		v8::Local<v8::Object> result = v8::Object::New(isolate);
		for (auto const& item: value)
		{
			result->Set(convert<Key>::to_v8(isolate, item.first), convert<Value>::to_v8(isolate, item.second));
		}
		return scope.Escape(result);
	}
};

// converter specializations for V8 Handles
template<typename T>
struct convert<v8::Handle<T>, typename std::enable_if<
	!std::is_same<v8::Handle<T>, v8::Local<T>>::value>::type>
{
	using from_type = v8::Handle<T>;
	using to_type = v8::Handle<T>;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return !value.As<T>().IsEmpty();
	}

	static v8::Handle<T> from_v8(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return value.As<T>();
	}

	static v8::Handle<T> to_v8(v8::Isolate*, v8::Handle<T> value)
	{
		return value;
	}
};

template<typename T>
struct convert<v8::Local<T>>
{
	using from_type = v8::Handle<T>;
	using to_type = v8::Handle<T>;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return !value.As<T>().IsEmpty();
	}

	static v8::Handle<T> from_v8(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return value.As<T>();
	}

	static v8::Handle<T> to_v8(v8::Isolate*, v8::Local<T> value)
	{
		return value;
	}
};

template<>
struct convert<PersistentValue>
{
	using from_type = PersistentValue;
	using to_type = v8::Local<v8::Value>;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return !value.IsEmpty();
	}

	static from_type from_v8(v8::Isolate *isolate, v8::Handle<v8::Value> value)
	{
		return PersistentValue(value, isolate);
	}

	static to_type to_v8(v8::Isolate*, from_type const& value)
	{
		return value.get_value();
	}
};

template<typename T>
struct convert<T, typename std::enable_if<std::is_base_of<function_base, T>::value>::type>
{
	using from_type = T;
	using to_type = v8::Local<v8::Function>;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		if (value.IsEmpty())
			return false;
		return value->IsFunction();
	}

	static from_type from_v8(v8::Isolate *isolate, v8::Handle<v8::Value> value)
	{
		return T(v8::Handle<v8::Function>::Cast(value), isolate);
	}

	static to_type to_v8(v8::Isolate *isolate, from_type const& value)
	{
		if (value.is_v8_function())
			return value.get_data()->get_function();
		if (value.isEmpty())
			return v8::Handle<v8::Function>();

		return v8::Function::New(isolate, &detail::forward_function<T::std_function_type>,
			detail::set_external_data(isolate, value.get_cpp_function()));
	}
};

template<>
struct convert<ValueIsolate>
{
	using from_type = ValueIsolate;
	using to_type = v8::Local<v8::Value>;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return !value.IsEmpty();
	}

	static from_type from_v8(v8::Isolate *isolate, v8::Handle<v8::Value> value)
	{
		return ValueIsolate(value, isolate);
	}

	static to_type to_v8(v8::Isolate*, from_type const& value)
	{
		return value.get_value();
	}
};

template<typename T>
struct convert<v8::Persistent<T>>
{
	using from_type = v8::Handle<T>;
	using to_type = v8::Handle<T>;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return !value.As<T>().IsEmpty();
	}

	static v8::Handle<T> from_v8(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return value.As<T>();
	}

	static v8::Handle<T> to_v8(v8::Isolate*, v8::Persistent<T> value)
	{
		return value;
	}
};

template<typename T, typename Enable = void>
struct is_wrapped_class;

// convert specialization for wrapped user classes
template<typename T>
struct is_wrapped_class<T>
	: std::conditional<!std::is_base_of<function_base, T>::value,  
						std::is_class<T>,
						std::false_type>::type{};

template<typename T>
struct is_wrapped_class<v8::Handle<T>, typename std::enable_if<
	!std::is_same<v8::Handle<T>, v8::Local<T>>::value, int>::type> : std::false_type{};

template<typename T>
struct is_wrapped_class<v8::Local<T>> : std::false_type {};

template<typename T>
struct is_wrapped_class<v8::Persistent<T>> : std::false_type {};

template<typename Char, typename Traits, typename Alloc>
struct is_wrapped_class<std::basic_string<Char, Traits, Alloc>> : std::false_type {};

template<typename T, typename Alloc>
struct is_wrapped_class<std::vector<T, Alloc>> : std::false_type {};

template<typename Key, typename Value, typename Less, typename Alloc>
struct is_wrapped_class<std::map<Key, Value, Less, Alloc>> : std::false_type {};

template<typename T>
struct convert<T*, typename std::enable_if<is_wrapped_class<T>::value>::type>
{
	using from_type = T*;
	using to_type = v8::Handle<v8::Object>;
	using class_type = typename std::remove_cv<T>::type;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return !value.IsEmpty() && value->IsObject();
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			return nullptr;
		}
		return class_<class_type>::unwrap_object(isolate, value);
	}

	static to_type to_v8(v8::Isolate* isolate, T const* value)
	{
		return class_<class_type>::find_object(isolate, value);
	}

	static to_type to_v8(v8::Isolate* isolate, T * value)
	{
		return class_<class_type>::convert(isolate, value);
	}
};

template<typename T>
struct convert<T, typename std::enable_if<is_wrapped_class<T>::value>::type>
{
	using from_type = T&;
	using to_type = v8::Handle<v8::Object>;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return !value.IsEmpty() && value->IsObject();
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			throw std::invalid_argument("expected Object");
		}
		if (T* object = convert<T*>::from_v8(isolate, value))
		{
			return *object;
		}
		throw std::runtime_error(std::string("expected C++ wrapped object"));
	}

	static to_type to_v8(v8::Isolate* isolate, T const& value)
	{
		return convert<T*>::to_v8(isolate, &value);
	}
};

template<typename T>
struct convert<T&> : convert<T> {};

template<typename T>
struct convert<T const&> : convert<T> {};

template<typename T>
typename convert<T>::from_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
{
	return convert<T>::from_v8(isolate, value);
}

template<typename T, typename U>
typename convert<T>::from_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value, U const& default_value)
{
	return convert<T>::is_valid(isolate, value)? convert<T>::from_v8(isolate, value) : default_value;
}

inline v8::Handle<v8::String> to_v8(v8::Isolate* isolate, char const* str, int len = -1)
{
	return v8::String::NewFromUtf8(isolate, str, v8::String::kNormalString, len);
}

#ifdef WIN32
inline v8::Handle<v8::String> to_v8(v8::Isolate* isolate, wchar_t const* str, int len = -1)
{
	return v8::String::NewFromTwoByte(isolate, reinterpret_cast<uint16_t const*>(str), v8::String::kNormalString, len);
}
#endif

template<typename T>
typename convert<T>::to_type to_v8(v8::Isolate* isolate, T const& value)
{
	return convert<T>::to_v8(isolate, value);
}

template<typename T>
typename convert<T>::to_type to_v8(v8::Isolate* isolate, T & value)
{
	return convert<T>::to_v8(isolate, value);
}

template<typename Iterator>
v8::Handle<v8::Array> to_v8(v8::Isolate* isolate, Iterator begin, Iterator end)
{
	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::Array> result = v8::Array::New(isolate);
	for (uint32_t idx = 0; begin != end; ++begin, ++idx)
	{
		result->Set(idx, to_v8(isolate, *begin));
	}
	return scope.Escape(result);
}

template<typename T>
v8::Local<T> to_local(v8::Isolate* isolate, v8::PersistentBase<T> const& handle)
{
	if (handle.IsWeak())
	{
		return v8::Local<T>::New(isolate, handle);
	}
	else
	{
		return *reinterpret_cast<v8::Local<T>*>(const_cast<v8::PersistentBase<T>*>(&handle));
	}
}

} // namespace v8pp

#endif // V8PP_CONVERT_HPP_INCLUDED
