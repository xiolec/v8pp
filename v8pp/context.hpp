#ifndef V8PP_CONTEXT_HPP_INCLUDED
#define V8PP_CONTEXT_HPP_INCLUDED

#include <string>
#include <map>

#include <v8.h>

#include "v8pp/convert.hpp"
#include "v8pp/function.hpp"
#include "v8pp/property.hpp"
#include "v8pp/reference_tracker.h"

namespace v8pp {

class module;

template<typename T>
class class_;

/// V8 isolate and context wrapper
class context : public ref_debug<context>
{
public:
	/// Create context with optional existing v8::Isolate
	explicit context(v8::Isolate* isolate = nullptr, int global_fields = 0, 
		v8::NamedPropertyHandlerConfiguration *config = NULL, 
		bool allow_java_run = false);
	~context();

	void set_security_token(int value);
	void set_security_token(const char *value);
	void set_security_token(context &give_secure);

	/// V8 isolate associated with this context
	v8::Isolate* isolate() { return isolate_; }

	/// Library search path
	std::string const& lib_path() const { return lib_path_; }

	/// Set new library search path
	void set_lib_path(std::string const& lib_path) { lib_path_ = lib_path; }

	/// Run script file, returns script result
	/// or empty handle on failure, use v8::TryCatch around it to find out why.
	/// Must be invoked in a v8::HandleScope
	v8::Handle<v8::Value> run_file(std::string const& filename);

	/// The same as run_file but uses string as the script source
	v8::Handle<v8::Value> run_script(std::string const& source, std::string const& filename = "", bool report_exception = true);

	//executes script and prints to console
	void execPrintScript(std::string const& source, std::string const& filename, bool report_exception = true);

	/// Set a V8 value in the context global object with specified name
	context& set(char const* name, v8::Handle<v8::Value> value);

	/// Give access to other context global object -- security token must match
	context& set(char const* name, context &other_context);

	/// Set module to the context global object
	context& set(char const *name, module& m);

	/// Set class to the context global object
	template<typename T>
	context& set(char const* name, class_<T>& cl)
	{
		v8::HandleScope scope(isolate_);
		//cl.class_function_template()->SetClassName(v8pp::to_v8(isolate_, name));
		cl.set_class_name(name, isolate_);
		return set(name, cl.js_function_template()->GetFunction());
	}

	v8::Handle<v8::Object> v8pp::context::global()
	{
		return v8pp::to_local(isolate_, impl_)->Global();
	}

	v8::Handle<v8::Object> v8pp::context::global_prototype()
	{
		return v8::Handle<v8::Object>::Cast(v8pp::to_local(isolate_, impl_)->Global()->GetPrototype());
	}

	template<typename GetFunction, typename SetFunction>
	typename std::enable_if<
		detail::is_function_pointer<GetFunction>::value &&
		detail::is_function_pointer<SetFunction>::value,
		context&>::type
		set(char const *name, property_<GetFunction, SetFunction> prop)
	{
		v8::HandleScope scope(isolate_);

		v8::AccessorGetterCallback getter = property_<GetFunction, SetFunction>::get;
		v8::AccessorSetterCallback setter = property_<GetFunction, SetFunction>::set;
		if (property_<GetFunction, SetFunction>::is_readonly)
		{
			setter = nullptr;
		}

		v8::Handle<v8::Value> data = detail::set_external_data(isolate_, prop);
		v8::PropertyAttribute const prop_attrs = v8::PropertyAttribute(v8::DontDelete | (setter ? 0 : v8::ReadOnly));

		global_prototype()->SetAccessor(v8pp::to_v8(isolate(), name), getter, setter, data, v8::DEFAULT, prop_attrs);
		return *this;
	}
	
	bool Has(const char *name);
	bool Has(uint32_t index);

	bool Delete(const char *name);
	bool Delete(uint32_t index);

	void ReportException(v8::TryCatch* try_catch);
private:
	bool own_isolate_;
	v8::Isolate* isolate_;
	v8::Persistent<v8::Context> impl_;

	struct dynamic_module;
	using dynamic_modules = std::map<std::string, dynamic_module>;

	static void load_module(v8::FunctionCallbackInfo<v8::Value> const& args);
	static void run_file(v8::FunctionCallbackInfo<v8::Value> const& args);
	static void run_source(v8::FunctionCallbackInfo<v8::Value> const& args);

	dynamic_modules modules_;
	std::string lib_path_;
};

} // namespace v8pp

#endif // V8PP_CONTEXT_HPP_INCLUDED