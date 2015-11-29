#include "v8pp/any_object.h"

#include "test.hpp"

#include "v8pp/any_object_hidden.h"
#include "v8pp/function.hpp"

bool ran = false;

void function_to_call()
{
	ran = true;
}

void test_functional()
{
	v8pp::context context;
	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);

	v8pp::Function<void()> function;
	function = function_to_call;
	function();
	check_eq("Tried running test function", ran, true);


	v8pp::Function<void()> v8_func = run_script<v8pp::Function<void()>>(context,
		"var function_test_ran = false; function test_v8_function(){function_test_ran = true;};test_v8_function;");
	check_eq("Getting function from v8", v8_func.is_v8_function(), true);
	v8_func.set_v8_forward_function(v8pp::forward_functional<void>);
	v8_func();//v8::Handle<v8::Value>::Cast(context.global()));
	check_eq("V8 function ran from cpp", run_script<bool>(context, "function_test_ran;"), true);

	v8::Local<v8::Function> cpp_function = v8pp::to_v8<v8pp::Function<void()>>(isolate, function);

	ran = false;
	cpp_function->Call(v8::Handle<v8::Value>::Cast(context.global()), 0, NULL);
	check_eq("Tried converting cpp function to v8", ran, true);


	v8pp::Function<void(int)> v8_func2 = run_script<v8pp::Function<void(int)>>(context,
		"var int_result = 0; function test_v8_function2(int_pass){int_result = int_pass;};test_v8_function2;");

	v8_func2.set_v8_forward_function(v8pp::forward_functional<void, int>);
	v8_func2(v8::Handle<v8::Value>::Cast(context.global()), 2);

	check_eq("v8 function call with args", run_script<int>(context, "int_result;"), 2);
}