#include "DebugConsole.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <ShlObj.h>
#include <ctime>
#include <mutex>
#include <filesystem>

namespace debug
{
	static FILE*   g_log_file = nullptr;
	static HANDLE  g_con_out  = nullptr;
	static bool    g_ready    = false;
	static std::mutex g_mtx;

	static int g_console_shown = -1;
	static bool g_quiet = false;

	void set_quiet(bool quiet)
	{
		std::lock_guard<std::mutex> lk(g_mtx);
		g_quiet = quiet;
	}

	void set_console_visible(bool visible)
	{
		const int wanted = visible ? 1 : 0;
		if (g_console_shown == wanted)
			return;
		g_console_shown = wanted;

		if (HWND console = GetConsoleWindow())
			ShowWindow(console, visible ? SW_SHOW : SW_HIDE);
	}

	void init_console()
	{
		{

			std::lock_guard<std::mutex> lk(g_mtx);
			if (g_ready)
				return;

			if (!AllocConsole())
			{

				g_con_out = GetStdHandle(STD_OUTPUT_HANDLE);
			}
			else
			{
				g_con_out = GetStdHandle(STD_OUTPUT_HANDLE);
				SetConsoleTitleA("aether");
				SetConsoleOutputCP(CP_UTF8);

				FILE* con_stream = nullptr;
				freopen_s(&con_stream, "CONOUT$", "w", stdout);
			}

			setvbuf(stdout, nullptr, _IONBF, 0);

			char path[MAX_PATH];
			if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, path)))
			{
				std::string dir = std::string(path) + "\\Aether\\logs\\";
				std::error_code ec;
				std::filesystem::create_directories(dir, ec);
				if (!ec)
				{
					time_t t = time(nullptr);
					tm tm_{};
					localtime_s(&tm_, &t);
					char fname[64];
					strftime(fname, sizeof(fname), "%Y-%m-%d_%H-%M-%S.log", &tm_);

					g_log_file = _fsopen((dir + fname).c_str(), "w", _SH_DENYWR);
				}
			}

			g_ready = true;
		}

		section("Aether debug console");
		log("core", "console ready, log file %s", g_log_file ? "attached" : "unavailable");
	}

	static void emit(WORD color, const char* tag, const char* fmt, va_list args, bool to_console = false)
	{
		char msg[1024];
		int n = vsnprintf(msg, sizeof(msg), fmt, args);
		if (n < 0)
			return;
		if (n >= (int)sizeof(msg))
			n = (int)sizeof(msg) - 1;

		time_t t = time(nullptr);
		tm tm_{};
		localtime_s(&tm_, &t);
		char ts[16];
		strftime(ts, sizeof(ts), "%H:%M:%S", &tm_);

		std::lock_guard<std::mutex> lk(g_mtx);

		if (g_con_out && (to_console || !g_quiet))
		{
			CONSOLE_SCREEN_BUFFER_INFO info{};
			GetConsoleScreenBufferInfo(g_con_out, &info);
			DWORD written = 0;

			SetConsoleTextAttribute(g_con_out, FOREGROUND_INTENSITY | 0x0008);
			WriteConsoleA(g_con_out, "[", 1, &written, nullptr);
			WriteConsoleA(g_con_out, ts, (DWORD)strlen(ts), &written, nullptr);
			WriteConsoleA(g_con_out, "] ", 2, &written, nullptr);

			SetConsoleTextAttribute(g_con_out, color | FOREGROUND_INTENSITY);
			WriteConsoleA(g_con_out, "[", 1, &written, nullptr);
			WriteConsoleA(g_con_out, tag, (DWORD)strlen(tag), &written, nullptr);
			WriteConsoleA(g_con_out, "] ", 2, &written, nullptr);

			SetConsoleTextAttribute(g_con_out, info.wAttributes);
			WriteConsoleA(g_con_out, msg, (DWORD)n, &written, nullptr);
			WriteConsoleA(g_con_out, "\n", 1, &written, nullptr);
		}

		if (g_log_file)
		{
			fprintf(g_log_file, "[%s] [%s] %s\n", ts, tag, msg);
			fflush(g_log_file);
		}
	}

	void log(const char* tag, const char* fmt, ...)
	{
		va_list a; va_start(a, fmt);
		emit(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, tag, fmt, a);
		va_end(a);
	}

	void info(const char* fmt, ...)
	{
		va_list a; va_start(a, fmt);
		emit(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, "info", fmt, a);
		va_end(a);
	}

	void ok(const char* fmt, ...)
	{
		va_list a; va_start(a, fmt);
		emit(FOREGROUND_GREEN, "ok", fmt, a);
		va_end(a);
	}

	void warn(const char* fmt, ...)
	{
		va_list a; va_start(a, fmt);
		emit(FOREGROUND_RED | FOREGROUND_GREEN, "warn", fmt, a, true);
		va_end(a);
	}

	void error(const char* fmt, ...)
	{
		va_list a; va_start(a, fmt);
		emit(FOREGROUND_RED, "error", fmt, a, true);
		va_end(a);
	}

	void status(const char* label, const char* fmt, ...)
	{
		// emit() takes a va_list - a pointer to the argument slots, not an
		// argument. Pre-formatting into a buffer and passing that buffer as the
		// va_list compiles cleanly on x64 (va_list IS char*) but makes emit
		// treat the first bytes of the text as the '%s' pointer. Forward the
		// real va_list instead.
		va_list a; va_start(a, fmt);
		emit(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, label, fmt, a, true);
		va_end(a);
	}

	void section(const char* title)
	{
		std::lock_guard<std::mutex> lk(g_mtx);
		char line[] = "================================================\n";
		if (g_con_out && !g_quiet)
		{
			DWORD w = 0;
			SetConsoleTextAttribute(g_con_out, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			WriteConsoleA(g_con_out, line, (DWORD)(sizeof(line) - 1), &w, nullptr);
			WriteConsoleA(g_con_out, title, (DWORD)strlen(title), &w, nullptr);
			WriteConsoleA(g_con_out, "\n", 1, &w, nullptr);
			WriteConsoleA(g_con_out, line, (DWORD)(sizeof(line) - 1), &w, nullptr);
		}
		if (g_log_file)
			fprintf(g_log_file, "%s%s\n%s", line, title, line);
	}
}
