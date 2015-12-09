#pragma once;

#include <v8pp/convert.hpp>

namespace v8pp
{
	//for easily adding values to a v8::object
	class v8_object_helper
	{
	public:
		v8_object_helper(v8::Local<v8::Object> &object, v8::Isolate *isolate)
			: object_(object)
		{
			isolate_ = isolate;
		};

		v8_object_helper& set(char const* name, v8::Handle<v8::Value> value)
		{
			v8::HandleScope scope(isolate_);
			object_->Set(to_v8(isolate_, name), value);
			return *this;
		}

		/// Set class to the object
		template<typename T>
		v8_object_helper& set(char const* name, class_<T>& cl)
		{
			v8::HandleScope scope(isolate_);
			cl.set_class_name(name, isolate_);
			return set(name, cl.js_function_template()->GetFunction());
		}

	private:
		v8::Local<v8::Object> &object_;
		v8::Isolate *isolate_;
	};

};