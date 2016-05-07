#pragma once
#include <memory>

namespace file_copy {
	class task {
	public:
		// must return true on success
		virtual bool operator()() = 0;
	};

	using task_ptr = std::shared_ptr<task>;
}