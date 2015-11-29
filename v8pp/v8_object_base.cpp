#include "v8_object_base_hidden.h"

#include "isolate_watcher.h"

void v8_object_base::data_object::set_object(v8::Handle<v8::Object> &obj)
{
	_isolate = obj->GetIsolate();
	base_object.Reset(_isolate, obj);

	if (_secure_delete)
		isolate_watcher::emplace_data(_isolate, this, std::bind(&v8_object_base::data_object::clear_object, this));
}

void v8_object_base::data_object::clear_object()
{
	_isolate = nullptr;
	base_object.Reset();
}

v8::Isolate *v8_object_base::get_object_isolate()
{
	if (object->get_object().IsEmpty())
		return nullptr;
	return object->get_object()->GetIsolate();
}

v8::Handle<v8::Object> v8_object_base::data_object::get_object()
{
	if (_isolate == nullptr)
		return v8::Handle<v8::Object>();

	return v8pp::to_local(_isolate, base_object);
}

void v8_object_base::clear_v8_object()
{
	v8::Handle<v8::Object> obj = object->get_object();
	if (obj.IsEmpty())
		return;
	v8::Handle<v8::Array> arr = obj->GetOwnPropertyNames();
	for (size_t x = 0; x < arr->Length(); x++)
	{
		if (arr->Get(x)->IsString())
		{
			obj->Delete(arr->Get(x));
		}
	}
}

void v8_object_base::release_v8_object()
{
	object.reset(new data_object(_secure_delete));
}

v8_object_base::data_object::~data_object()
{
	if (_isolate != nullptr)
	{
		if (_secure_delete)
			isolate_watcher::remove_data(_isolate, this);
		base_object.Reset();
		_isolate = nullptr;
	}
}

v8_object_base::v8_object_base(v8_object_base &other) : object(new data_object(other._secure_delete))
{
}

v8_object_base::v8_object_base(bool secure_delete) : object(new data_object(secure_delete))
{
	//object = new data_object();
	_secure_delete = secure_delete;
}

v8_object_base::~v8_object_base()
{
	//delete object;
}

v8_object_base::data_object *v8_object_base::get_data()
{
	//return object;
	return object.get();
}


