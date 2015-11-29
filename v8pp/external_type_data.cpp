#include "external_type_data.h"
namespace v8pp
{
	namespace detail {
		std::map<v8::Isolate *, external_info> external_info::isolate_storage;
	}
}