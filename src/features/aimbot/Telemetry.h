#pragma once

#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#endif

namespace aimbot
{
	struct telemetry_sample_t
	{
		float t = 0.f;
		float raw_yaw = 0.f;
		float raw_pitch = 0.f;
		float step_yaw = 0.f;
		float step_pitch = 0.f;
		float sx = 1.f;
		float sy = 1.f;
		float dist = 0.f;
		bool  primary = false;
	};

	inline constexpr int kTelemetryMax = 1024;

	inline telemetry_sample_t* telemetry_ring()
	{
		static telemetry_sample_t r[kTelemetryMax];
		return r;
	}

	inline unsigned& telemetry_head()
	{
		static unsigned h = 0;
		return h;
	}

	inline std::uint64_t& telemetry_epoch()
	{
		static std::uint64_t e = 0;
		return e;
	}

	inline float telemetry_now()
	{
#ifdef _WIN32
		const std::uint64_t now = GetTickCount64();
		if (!telemetry_epoch())
			telemetry_epoch() = now;
		return (float)(now - telemetry_epoch()) * 0.001f;
#else
		return 0.f;
#endif
	}

	inline void telemetry_push(const telemetry_sample_t& s)
	{
		const unsigned i = telemetry_head()++;
		telemetry_ring()[i % (unsigned)kTelemetryMax] = s;
	}

	inline int telemetry_read(telemetry_sample_t* out, int max)
	{
		if (!out || max <= 0)
			return 0;
		const unsigned h = telemetry_head();
		const int have  = (h < (unsigned)kTelemetryMax) ? (int)h : kTelemetryMax;
		const int take  = (have < max) ? have : max;
		if (take <= 0)
			return 0;
		const unsigned start = (h >= (unsigned)kTelemetryMax) ? (h - (unsigned)take) : 0u;
		telemetry_sample_t* r = telemetry_ring();
		for (int i = 0; i < take; ++i)
			out[i] = r[(start + (unsigned)i) % (unsigned)kTelemetryMax];
		return take;
	}

	inline void telemetry_reset()
	{
		telemetry_head() = 0;
		telemetry_epoch() = 0;
	}
}
