#ifndef V8PP_V8_OBJECT_HIDDEN_H_INCLUDED
#define V8PP_V8_OBJECT_HIDDEN_H_INCLUDED

#include "v8_object_base.h"
#include "v8.h"
#include <v8pp/convert.hpp>

struct v8_object_base::data_object
{
	void set_object(v8::Handle<v8::Object> &obj);

	v8::Handle<v8::Object> get_object();

	void clear_object();

	data_object(bool secure_data_delete = false)
	{
		_isolate = nullptr;
		_secure_delete = secure_data_delete;
	}
	~data_object();
private:
	v8::Persistent<v8::Object> base_object;
	v8::Isolate *_isolate = nullptr;
	bool _secure_delete = false;
};

#endif