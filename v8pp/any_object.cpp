#include "any_object_hidden.h"

namespace v8pp
{
	std::map<v8::Isolate *, value_watcher> value_watcher::watcher_per_isolate;
	void PersistentValue::data_object::set_value(v8::Local<v8::Value> &obj, v8::Isolate *isolate)
	{
		_isolate = isolate;
		base_object.Reset(_isolate, obj);
	}

	bool PersistentValue::data_object::isEmpty()
	{
		if (base_object.IsEmpty())
			return true;
		return false;
	} 

	void PersistentValue::set_value(v8::Local<v8::Value> &value, v8::Isolate *isolate)
	{
		object->set_value(value, isolate);

		value_watcher::emplace_data(isolate, this);
	}

	bool PersistentValue::isEmpty()
	{
		return object->isEmpty();
	}

	v8::Isolate *PersistentValue::data_object::get_isolate()
	{
		if (isEmpty())
			return nullptr;
		return _isolate;
	}

	v8::Isolate *PersistentValue::get_isolate() const
	{
		return object->get_isolate();
	}

	v8::Local<v8::Value> PersistentValue::data_object::get_value()
	{
		if ((_isolate == nullptr) || (base_object.IsEmpty()))
			return v8::Local<v8::Value>();

		return v8pp::to_local(_isolate, base_object);
	}

	v8::Local<v8::Value> PersistentValue::get_value() const
	{
		return object->get_value();
	}

	void PersistentValue::destroy_value()
	{
		object.reset(new data_object());
	}

	PersistentValue& PersistentValue::operator=(const PersistentValue &other)
	{
		set_value(other.get_value(), other.get_isolate());
		return *this;
	}
	PersistentValue& PersistentValue::operator=(const ValueIsolate &other)
	{
		set_value(other.get_value(), other.get_isolate());

		return *this;
	}


	PersistentValue::data_object::~data_object()
	{
		if (_isolate != nullptr)
		{
			base_object.Reset();
			_isolate = nullptr;
		}
	}
	PersistentValue::PersistentValue(PersistentValue const &other) : object(new data_object())
	{
		if (!other.object->isEmpty())
			set_value(other.object->get_value(), other.object->get_isolate());
	}

	PersistentValue::PersistentValue() : object(new data_object())
	{
	}

	PersistentValue::PersistentValue(v8::Local<v8::Value> &value, v8::Isolate *isolate) : object(new data_object())
	{
		set_value(value, isolate);
	}

	PersistentValue::PersistentValue(const ValueIsolate &v8_value_isolate) : object(new data_object())
	{
		set_value(v8_value_isolate.get_value(), v8_value_isolate.get_isolate());
	}

	PersistentValue::~PersistentValue()
	{
		if (!object->isEmpty())
			value_watcher::remove_data(object->get_isolate(), this);
		object.reset();
	}

	ValueIsolate::data_object::data_object(v8::Local<v8::Value> &value, v8::Isolate *isolate)
	{
		_value = value;
		_isolate = isolate;
	}

	ValueIsolate::data_object::~data_object()
	{}

	ValueIsolate::~ValueIsolate()
	{
		_d.reset();
	}

	ValueIsolate::ValueIsolate(v8::Local<v8::Value> &value, v8::Isolate *isolate)
	{
		_d.reset(new ValueIsolate::data_object(value, isolate));
	}

	v8::Local<v8::Value> ValueIsolate::get_value() const
	{
		return _d->_value;
	}

	v8::Isolate *ValueIsolate::get_isolate() const
	{
		return _d->_isolate;
	}


	//v8pp function
	function_base::function_base()
	{
		_d.reset(new function_data);
	}

	function_base::function_base(v8::Local<v8::Function> &function, v8::Isolate *isolate)
	{
		_d.reset(new function_data);
		set_v8_function(function, isolate);
	}

	function_base::function_base(const function_base &other)
	{
		_d.reset(new function_data);
		if (!other._d->isEmpty())
			_d->set_value(other._d->get_function(), other._d->get_isolate());
	}

	function_base::~function_base()
	{
		_d.reset();
	}

	void function_base::set_v8_function(v8::Local<v8::Function> &function, v8::Isolate *isolate)
	{
		_d->set_value(function, isolate);
	}

	function_data *function_base::get_data() const
	{
		return _d.get();
	}

	bool function_base::is_v8_function() const
	{
		return !_d->isEmpty();
	}

	void function_base::reset()
	{
		_d->reset();
	}

	function_data::~function_data()
	{
		if (isolate_ != nullptr)
			value_watcher::remove_data(isolate_, this);
	}

	bool function_data::isEmpty()
	{
		return function_.IsEmpty();
	}

	void function_data::set_value(v8::Local<v8::Function> &function, v8::Isolate *isolate)
	{
		function_.Reset(isolate, function);
		isolate_ = isolate;

		value_watcher::emplace_data(isolate_, this);
	}

	void function_data::reset()
	{
		function_.Reset();
		isolate_ = nullptr;
	}

	v8::Local<v8::Function> function_data::get_function()
	{
		return v8pp::to_local(isolate_, function_);
	}
	v8::Isolate *function_data::get_isolate()
	{
		return isolate_;
	}




	//value watcher
	value_watcher *value_watcher::get_value_watcher(v8::Isolate *isolate)
	{
		auto finder = watcher_per_isolate.find(isolate);
		if (finder == watcher_per_isolate.end())
			finder = watcher_per_isolate.emplace(isolate, value_watcher()).first;

		return &finder->second;
	}


	void value_watcher::emplace_data(v8::Isolate *isolate, PersistentValue *data)
	{
		value_watcher *e_info = get_value_watcher(isolate);
		e_info->value_map.emplace(data, true);
	}

	void value_watcher::remove_data(v8::Isolate *isolate, PersistentValue *data)
	{
		value_watcher *e_info = get_value_watcher(isolate);

		auto it = e_info->value_map.find(data);
		if (it != e_info->value_map.end())
			e_info->value_map.erase(it);
	}

	void value_watcher::emplace_data(v8::Isolate *isolate, function_data *data)
	{
		value_watcher *e_info = get_value_watcher(isolate);
		e_info->function_map.emplace(data, true);
	}

	void value_watcher::remove_data(v8::Isolate *isolate, function_data *data)
	{
		value_watcher *e_info = get_value_watcher(isolate);

		auto it = e_info->function_map.find(data);
		if (it != e_info->function_map.end())
			e_info->function_map.erase(it);
	}
	void value_watcher::delete_isolate_instance(v8::Isolate *isolate)
	{
		value_watcher *e_info = get_value_watcher(isolate);

		auto it = e_info->value_map.begin();
		for (; it != e_info->value_map.end();)
		{
			it->first->destroy_value();
			it = e_info->value_map.erase(it);
		}

		auto f_it = e_info->function_map.begin();
		for (; f_it != e_info->function_map.end(); f_it++)
		{
			f_it->first->reset();
			f_it = e_info->function_map.erase(f_it);
		}

		auto finder = watcher_per_isolate.find(isolate);
		if (finder != watcher_per_isolate.end())
			watcher_per_isolate.erase(finder);
	}
};