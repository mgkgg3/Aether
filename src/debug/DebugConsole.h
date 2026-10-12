#pragma once

#include <cstdio>
#include <cstdarg>
#include <string>

namespace debug
{

	void init_console();

	void set_console_visible(bool visible);

	void log(const char* tag, const char* fmt, ...);

	void info(const char* fmt, ...);
	void warn(const char* fmt, ...);
	void error(const char* fmt, ...);
	void ok(const char* fmt, ...);

	void section(const char* title);


	void set_quiet(bool quiet);


	void status(const char* label, const char* fmt, ...);
}
