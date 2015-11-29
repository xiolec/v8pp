#pragma once
#include "v8.h"
#include "convert.hpp"
#include <iostream>
#include <map>
#include <string>

namespace v8pp
{
	//void window_global_callback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& info);

	//void global_setter_callback(v8::Local<v8::Name> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info);

	//void global_query(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Integer>& info);

	//void global_deleter(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Boolean>& info);

	struct debug
	{
		static void set_debug_flag(v8::PropertyHandlerFlags debug_flag);

		static void set_debug_handler(v8::Handle<v8::ObjectTemplate> &obj_temp, const char *name, v8::Isolate *isolate);

		static v8::PropertyHandlerFlags _debug_flag;

		static bool enable_debug;
	};
}