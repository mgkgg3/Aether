#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <mmsystem.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

// The panel's own small sounds.
//
// Two clicks and nothing else: one for a press on a control, one a little
// lower and softer for a toggle changing state.  Both are synthesised here at
// startup rather than shipped as files - a 20 ms blip is cheaper to describe
// than to store, and a resource would have to be found, loaded and kept alive
// for the same effect.
//
// The shape is the whole trick, and it took two passes to get right.  A steady
// sine with harmonics over a hard attack is a *notification*: pitched, flat and
// slightly piercing, which is what a click was.  A switch is a transient -
// something struck, whose pitch falls away as it settles.  So the tone glides
// down from a fifth above, the partials are a fifth *under* (body) and an octave
// up (definition) instead of stacked harmonics, a 3 ms noise tick sits on the
// onset, and a one-pole low-pass takes the fizz off the ends.  The tail is faded
// to zero over the last few milliseconds so PlaySound cutting the buffer cannot
// add a click of its own.
namespace ui_sound
{
	enum class kind : int
	{
		click = 0,
		toggle = 1,
		count
	};

	namespace detail
	{
		// Two buffers per sound, used alternately: PlaySound is handed a pointer
		// and reads it asynchronously, so the buffer being rebuilt for a later
		// click is never the one the previous click is still reading from.
		inline std::vector<BYTE> g_wav[(int)kind::count][2];
		inline int   g_slot       = 0;
		inline int   g_built_step = -1;   // volume bucket of the files on hand
		inline int   g_last_ms    = 0;

		inline void put16(std::vector<BYTE>& v, std::uint16_t x)
		{
			v.push_back((BYTE)(x & 0xFF));
			v.push_back((BYTE)((x >> 8) & 0xFF));
		}

		inline void put32(std::vector<BYTE>& v, std::uint32_t x)
		{
			v.push_back((BYTE)(x & 0xFF));
			v.push_back((BYTE)((x >> 8) & 0xFF));
			v.push_back((BYTE)((x >> 16) & 0xFF));
			v.push_back((BYTE)((x >> 24) & 0xFF));
		}

		// A RIFF/WAVE wrapper around 16-bit mono samples: PlaySound wants the
		// file, not the samples, even when the file only ever exists in RAM.
		inline void build(int slot, kind k, float freq, float seconds, float gain, float decay)
		{
			constexpr int   rate = 44100;
			const int       n    = (int)(rate * seconds);
			std::vector<short> pcm((std::size_t)n);

			// The tone falls in pitch instead of holding one: a struck object does
			// not keep its note, and the glide is what stops it reading as a beep.
			// The phase is integrated because the frequency is not constant.
			float phase = 0.f;
			float lp    = 0.f;                    // one-pole state, ~5 kHz corner
			const float lp_a = 1.f - expf(-6.2831853f * 5200.f / (float)rate);
			const float tail_n = (float)(rate / 330);   // ~3 ms to fade out
			const bool  strike = (k == kind::click);
			unsigned    rng = 0x2545F491u;

			for (int i = 0; i < n; ++i)
			{
				const float t = (float)i / (float)rate;

				// A 2 ms raised attack, then an exponential body.  The attack is
				// squared so the onset has no edge, and the tail is ramped to zero
				// so the buffer ends on silence rather than on a step.
				const float rise = 1.f - expf(-t * 1400.f);
				const float tail = (float)(n - 1 - i) / tail_n;
				const float env  = rise * rise * expf(-t * decay) * (tail < 1.f ? tail : 1.f);

				const float f = freq * (1.f + 0.30f * expf(-t * 60.f));
				phase += 6.2831853f * f / (float)rate;

				// 1 : 0.30 : 0.08 - a body a fifth down, the note, a hair of octave.
				const float s = sinf(phase) + 0.30f * sinf(phase * 0.5f) + 0.08f * sinf(phase * 2.f);

				float v = env * s;

				// The tick.  A switch actuates before it rings, and that very short
				// broadband edge is the whole difference between "click" and "tone".
				if (strike)
				{
					rng = rng * 1664525u + 1013904223u;
					const float nz = (float)((rng >> 9) & 0x7FFF) / 16384.f - 1.f;
					v += 0.55f * nz * expf(-t * 900.f) * (1.f - expf(-t * 9000.f));
				}

				v = gain * v;
				lp += lp_a * (v - lp);
				v = lp;
				if (v >  1.f) v =  1.f;
				if (v < -1.f) v = -1.f;
				pcm[(std::size_t)i] = (short)(v * 32767.f);
			}

			std::vector<BYTE>& v = g_wav[(int)k][slot];
			v.clear();
			v.reserve(44 + pcm.size() * 2);

			const std::uint32_t data_bytes = (std::uint32_t)(pcm.size() * 2);
			v.insert(v.end(), { 'R', 'I', 'F', 'F' });
			put32(v, 36 + data_bytes);
			v.insert(v.end(), { 'W', 'A', 'V', 'E' });

			v.insert(v.end(), { 'f', 'm', 't', ' ' });
			put32(v, 16);
			put16(v, 1);                 // PCM
			put16(v, 1);                 // mono
			put32(v, rate);
			put32(v, rate * 2);          // bytes per second
			put16(v, 2);                 // block align
			put16(v, 16);                // bits

			v.insert(v.end(), { 'd', 'a', 't', 'a' });
			put32(v, data_bytes);
			const BYTE* bytes = reinterpret_cast<const BYTE*>(pcm.data());
			v.insert(v.end(), bytes, bytes + data_bytes);
		}

		// 1.7 kHz is above the body of speech and below the hiss of a
		// notification; the toggle sits a fifth under it so the two are
		// distinguishable without either being loud.
		inline void bake(int step)
		{
			if (step == g_built_step)
				return;
			g_built_step = step;

			g_slot = (g_slot + 1) & 1;
			const float v = (float)step / 10.f;   // 1.0 at the top of the range
			// 1.1 kHz is where a switch sits; the toggle a fifth under it so the
			// two are distinguishable without either one being loud.
			build(g_slot, kind::click,  1100.f, 0.045f, 0.34f * v, 105.f);
			build(g_slot, kind::toggle,  740.f, 0.055f, 0.30f * v,  85.f);
		}
	}

	// `volume` is 0..100 from the settings.  A click is deliberately quiet -
	// it is a confirmation, not an alert - so the top of the range is still
	// well under what PlaySound will play at.
	inline void play(kind k, float volume)
	{
		if (volume <= 0.f)
			return;

		const int idx = (int)k;
		if (idx < 0 || idx >= (int)kind::count)
			return;

		// Baked in ten steps rather than per click: a slider dragged across its
		// range must not rebuild a WAV on every frame it moves.
		int step = (int)(volume / 10.f + 0.5f);
		if (step < 1)  step = 1;
		if (step > 10) step = 10;
		detail::bake(step);

		if (detail::g_wav[idx][detail::g_slot].empty())
			return;

		// Two clicks inside 25 ms are one gesture, not two sounds: a double
		// click would otherwise play the same blip twice and read as a stutter.
		const int now = (int)GetTickCount();
		if (now - detail::g_last_ms < 25)
			return;
		detail::g_last_ms = now;

		// PlaySound cannot mix with itself, which is why the guard above exists
		// rather than a queue: the samples are 40 ms, so a second call inside
		// the first would cut it off mid-tail.
		PlaySoundA(reinterpret_cast<LPCSTR>(detail::g_wav[idx][detail::g_slot].data()), nullptr,
			SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
	}
}
