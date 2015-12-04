#include "v8pp/context.hpp"
#include "v8pp/config.hpp"
#include "v8pp/convert.hpp"
#include "v8pp/function.hpp"
#include "v8pp/module.hpp"
#include "v8pp/throw_ex.hpp"
#include "v8pp/reference_tracker.h"
#include "v8pp/class.hpp"

#include "v8pp/any_object_hidden.h"
#include "v8pp/isolate_watcher.h"

#include <fstream>

#if defined(WIN32)
#include <windows.h>
static char const path_sep = '\\';
#else
#include <dlfcn.h>
static char const path_sep = '/';
#endif

std::map<v8::Isolate *, isolate_watcher> isolate_watcher::watcher_per_isolate;

#define STRINGIZE(s) STRINGIZE0(s)
#define STRINGIZE0(s) #s

namespace v8pp {

	struct context::dynamic_module
	{
		void* handle;
		v8::UniquePersistent<v8::Value> exports;

		dynamic_module() = default;
		dynamic_module(dynamic_module&& other)
			: handle(other.handle)
			, exports(std::move(other.exports))
		{
			other.handle = nullptr;
		}

		dynamic_module(dynamic_module const&) = delete;
	};

	void context::load_module(v8::FunctionCallbackInfo<v8::Value> const& args)
	{
		v8::Isolate* isolate = args.GetIsolate();

		v8::EscapableHandleScope scope(isolate);
		v8::Local<v8::Value> result;
		try
		{
			std::string const name = from_v8<std::string>(isolate, args[0], "");
			if (name.empty())
			{
				throw std::runtime_error("load_module: require module name string argument");
			}

			context* ctx = detail::get_external_data<context*>(args.Data());
			context::dynamic_modules::iterator it = ctx->modules_.find(name);

			// check if module is already loaded
			if (it != ctx->modules_.end())
			{
				result = v8::Local<v8::Value>::New(isolate, it->second.exports);
			}
			else
			{
				std::string filename = name;
				if (!ctx->lib_path_.empty())
				{
					filename = ctx->lib_path_ + path_sep + name;
				}
				std::string const suffix = V8PP_PLUGIN_SUFFIX;
				if (filename.size() >= suffix.size()
					&& filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) != 0)
				{
					filename += suffix;
				}

				dynamic_module module;
#if defined(WIN32)
				UINT const prev_error_mode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
				module.handle = LoadLibraryA(filename.c_str());
				::SetErrorMode(prev_error_mode);
#else
				module.handle = dlopen(filename.c_str(), RTLD_LAZY);
#endif

				if (!module.handle)
				{
					throw std::runtime_error("load_module(" + name + "): could not load shared library " + filename);
				}
#if defined(WIN32)
				void *sym = ::GetProcAddress((HMODULE)module.handle, STRINGIZE(V8PP_PLUGIN_INIT_PROC_NAME));
#else
				void *sym = dlsym(module.handle, STRINGIZE(V8PP_PLUGIN_INIT_PROC_NAME));
#endif
				if (!sym)
				{
					throw std::runtime_error("load_module(" + name + "): initialization function "
						STRINGIZE(V8PP_PLUGIN_INIT_PROC_NAME) " not found in " + filename);
				}

				using module_init_proc = v8::Handle<v8::Value>(*)(v8::Isolate*);
				module_init_proc init_proc = reinterpret_cast<module_init_proc>(sym);
				result = init_proc(isolate);
				module.exports.Reset(isolate, result);
				ctx->modules_.emplace(name, std::move(module));
			}
		}
		catch (std::exception const& ex)
		{
			result = throw_ex(isolate, ex.what());
		}
		args.GetReturnValue().Set(scope.Escape(result));
	}

	void context::run_file(v8::FunctionCallbackInfo<v8::Value> const& args)
	{
		v8::Isolate* isolate = args.GetIsolate();

		v8::EscapableHandleScope scope(isolate);
		v8::Local<v8::Value> result;
		try
		{
			std::string const filename = from_v8<std::string>(isolate, args[0], "");
			if (filename.empty())
			{
				throw std::runtime_error("run_file: require filename string argument");
			}

			context* ctx = detail::get_external_data<context*>(args.Data());
			result = to_v8(isolate, ctx->run_file(filename));
		}
		catch (std::exception const& ex)
		{
			result = throw_ex(isolate, ex.what());
		}
		args.GetReturnValue().Set(scope.Escape(result));
	}

	void context::run_source(v8::FunctionCallbackInfo<v8::Value> const& args)
	{
		v8::Isolate* isolate = args.GetIsolate();

		v8::EscapableHandleScope scope(isolate);
		v8::Local<v8::Value> result;
		try
		{
			std::string const source = from_v8<std::string>(isolate, args[0], "");

			context* ctx = detail::get_external_data<context*>(args.Data());
			result = to_v8(isolate, ctx->run_script(source));
		}
		catch (std::exception const& ex)
		{
			result = throw_ex(isolate, ex.what());
		}
		args.GetReturnValue().Set(scope.Escape(result));
	}

struct array_buffer_allocator : v8::ArrayBuffer::Allocator
{
	void* Allocate(size_t length)
	{
		return calloc(length, 1);
	}
	void* AllocateUninitialized(size_t length)
	{
		return malloc(length);
	}
	void Free(void* data, size_t length)
	{
		free(data);
	}
};

static array_buffer_allocator array_buffer_allocator_;

context::context(v8::Isolate* isolate, 
	context_callback create_global,
	global_object_callback wrap_global,
	bool allow_java_run)
{
	own_isolate_ = (isolate == nullptr);
	if (own_isolate_)
	{
		v8::Isolate::CreateParams create_params;
		create_params.array_buffer_allocator = &array_buffer_allocator_;

		isolate = v8::Isolate::New(create_params);
		isolate->Enter();
	}
	isolate_ = isolate;

	v8::HandleScope scope(isolate_);

	v8::Handle<v8::Value> data = detail::set_external_data(isolate_, this);

	v8::Handle<v8::ObjectTemplate> global;
	v8::Handle<v8::Value> global_obj = v8::Handle<v8::Value>();
	if (!create_global)
	{
		const char *name = "global";
		v8::Handle<v8::FunctionTemplate> func_temp = v8::FunctionTemplate::New(isolate_);
		func_temp->SetClassName(v8pp::to_v8(isolate, name));
		global = v8::ObjectTemplate::New(isolate_, func_temp);
	}
	else
	{
		create_global(isolate_, global, global_obj);
		//if (global->InternalFieldCount() == 0)
			//global->SetInternalFieldCount(2);
	}

	if (allow_java_run)
	{
		global->Set(isolate_, "require", v8::FunctionTemplate::New(isolate_, context::load_module, data));
		global->Set(isolate_, "run", v8::FunctionTemplate::New(isolate_, context::run_file, data));
	}

	v8::Handle<v8::Context> impl = v8::Context::New(isolate_, nullptr, global, global_obj);

	int n_feilds = v8::Handle<v8::Object>::Cast(impl->Global()->GetPrototype())->InternalFieldCount();
	if (wrap_global)
		wrap_global(isolate_, v8::Handle<v8::Object>::Cast(impl->Global()->GetPrototype()));
	
	if (own_isolate_)
		impl->Enter();
	impl_.Reset(isolate_, impl);
}

context::~context()
{
	for (auto& kv : modules_)
	{
		dynamic_module& module = kv.second;
		module.exports.Reset();
		if (module.handle)
		{
#if defined(WIN32)
			::FreeLibrary((HMODULE)module.handle);
#else
			dlclose(module.handle);
#endif
		}
	}
	modules_.clear();

	v8::Local<v8::Context> impl = to_local(isolate_, impl_);
	if (own_isolate_)
	{
		using singleton_instances = std::vector<void*>;

		singleton_instances* singletons =
			static_cast<singleton_instances*>(isolate()->GetData(V8PP_ISOLATE_DATA_SLOT));

		if (singletons)
		{
			//std::string const v8_flags = "--expose_gc";
			//v8::V8::SetFlagsFromString(v8_flags.data(), (int)v8_flags.length());
			//isolate_->RequestGarbageCollectionForTesting(v8::Isolate::GarbageCollectionType::kFullGarbageCollection);

			
			for (size_t x = 0; x < singletons->size(); x++)
			{
				detail::class_info* class_in = ((detail::class_info*)singletons->at(x));
				class_in->remove_class_info();
				//ref_static::ref_count;
			}
			delete singletons;
		}

		impl->Exit();
	}

	impl_.Reset();
	if (own_isolate_)
	{
		detail::external_info::delete_isolate_instance(isolate_);
		value_watcher::delete_isolate_instance(isolate_);
		isolate_watcher::delete_isolate_instance(isolate_);
		isolate_->ContextDisposedNotification();
		isolate_->LowMemoryNotification();
		while (isolate_->IdleNotification(100)){};
		isolate_->Exit();
		isolate_->Dispose();
	}
}

void context::set_security_token(const char *value)
{
	v8::HandleScope scope(isolate_);
	to_local(isolate_, impl_)->SetSecurityToken(v8pp::to_v8(isolate_, value));
}

void context::set_security_token(int value)
{
	v8::HandleScope scope(isolate_);
	to_local(isolate_, impl_)->SetSecurityToken(v8pp::to_v8(isolate_, value));
}

void context::set_security_token(context &give_secure)
{
	v8::HandleScope scope(isolate_);
	to_local(isolate_, impl_)->SetSecurityToken(v8pp::to_v8(isolate_, 
											to_local(give_secure.isolate(), give_secure.impl_)->GetSecurityToken()));
}

context& context::set(char const* name, v8::Handle<v8::Value> value)
{
	v8::HandleScope scope(isolate_);
	global_prototype()->Set(to_v8(isolate_, name), value);
	return *this;
}

context& context::set(char const* name, context &other_context)
{
	v8::HandleScope scope(isolate_);

	global()->Set(to_v8(isolate_, name), other_context.global());

	return *this;
}

context& context::set(char const* name, module& m)
{
	return set(name, m.new_instance());
}

v8::Handle<v8::Value> context::run_file(std::string const& filename)
{
	std::ifstream stream(filename.c_str());
	if (!stream)
	{
		throw std::runtime_error("could not locate file " + filename);
	}

	std::istreambuf_iterator<char> begin(stream), end;
	return run_script(std::string(begin, end), filename);
}

void context::execPrintScript(std::string const& source, std::string const& filename, bool report_exception)
{
	v8::Local<v8::Value> result = 
		run_script(source, filename, report_exception);

	v8::String::Utf8Value utf8(result);
	printf("%s\n", *utf8);
}

v8::Handle<v8::Value> context::run_script(std::string const& source, std::string const& filename, bool report_exception)
{
	//v8::EscapableHandleScope scope(isolate_);
	if (!own_isolate_)
		to_local(isolate_, impl_)->Enter();
	
	v8::Local<v8::Value> result;
	v8::TryCatch try_catch(isolate_);
	//try_catch.SetVerbose(true);
	v8::Local<v8::Script> script = v8::Script::Compile(
		to_v8(isolate_, source), to_v8(isolate_, filename));

	if (!script.IsEmpty())
	{
		//result = script->Run();
		if (!script->Run(v8pp::to_local(isolate_, impl_)).ToLocal(&result))
		{
			assert(try_catch.HasCaught());
			// Print errors that happened during execution.
			if (report_exception)
				ReportException(&try_catch);
			//return false;
		}
		else
			assert(!try_catch.HasCaught());
	}
	else
	{
		if (report_exception)
			ReportException(&try_catch);
	}

	if (!own_isolate_)
		to_local(isolate_, impl_)->Exit();
	return result;
	//return scope.Escape(result);
}

void context::ReportException(v8::TryCatch* try_catch)
{
	v8::HandleScope handle_scope(isolate_);
	v8::String::Utf8Value exception(try_catch->Exception());
	const char* exception_string = *exception;
	v8::Local<v8::Message> message = try_catch->Message();
	if (message.IsEmpty()) {
		// V8 didn't provide any extra information about this error; just
		// print the exception.
		fprintf(stderr, "%s\n", exception_string);
	}
	else {
		// Print (filename):(line number): (message).
		v8::String::Utf8Value filename(message->GetScriptOrigin().ResourceName());
		v8::Local<v8::Context> context(v8pp::to_local(isolate_, impl_));
		const char* filename_string = *filename;
		int linenum = message->GetLineNumber(context).FromJust();
		fprintf(stderr, "%s:%i: %s\n", filename_string, linenum, exception_string);
		// Print line of source code.
		v8::String::Utf8Value sourceline(
			message->GetSourceLine(context).ToLocalChecked());
		const char* sourceline_string = *sourceline;
		fprintf(stderr, "%s\n", sourceline_string);
		// Print wavy underline (GetUnderline is deprecated).
		int start = message->GetStartColumn(context).FromJust();
		for (int i = 0; i < start; i++) {
			fprintf(stderr, " ");
		}
		int end = message->GetEndColumn(context).FromJust();
		for (int i = start; i < end; i++) {
			fprintf(stderr, "^");
		}
		fprintf(stderr, "\n");
		v8::String::Utf8Value stack_trace(
			try_catch->StackTrace());
		if (stack_trace.length() > 0) {
			const char* stack_trace_string = *stack_trace;
			fprintf(stderr, "%s\n", stack_trace_string);
		}
	}
}

bool context::Has(const char *name)
{
	return global()->Has(to_v8(isolate_, name));
}

bool context::Has(uint32_t index)
{
	return global()->Has(index);
}

bool context::Delete(const char *name)
{
	return global()->Delete(to_v8(isolate_, name));
}

bool context::Delete(uint32_t index)
{
	return global()->Delete(index);
}

} // namespace v8pp
