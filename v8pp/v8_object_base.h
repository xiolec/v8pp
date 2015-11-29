#ifndef V8PP_V8_OBJECT_H_INCLUDED
#define V8PP_V8_OBJECT_H_INCLUDED
#include <memory>
#include "reference_tracker.h"

namespace v8
{
	class Isolate;
}

class v8_object_base : public ref_debug<v8_object_base>
{
public:
	struct data_object;
	v8_object_base(bool secure_delete = false);
	v8_object_base(v8_object_base &other);
	~v8_object_base();

	data_object *get_data();

	v8::Isolate *get_object_isolate();

	void clear_v8_object();

	void release_v8_object();
private:
	std::unique_ptr<data_object> object;
	bool _secure_delete = false;
};

/*
	c++/v8 function mix

	c++ has access to functional function
	v8 has access to javascript function
*/

/*template <typename T>
class v8pp_function : public ref_debug<v8_object_base>
{
public:
	using ret_type = typename function_traits<T>::return_type;

	//call function would have to be declared in header.... which would require v8 headers to be opened
	//unless all call code is put into protected class
	template<typename ...Args>
	ret_type call_function(Args... args);
};*/

#endif