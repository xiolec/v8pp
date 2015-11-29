#include "reference_tracker.h"


long ref_static::ref_count = 0;
std::list<std::pair<void*, std::string>> ref_static::instances;
