// synth.h
#pragma once

#include "includes.h"
#include "oscillator.h"
#include "wave.h"

namespace soundmath
{
	template <typename T> class Synth : public Oscillator<T>
	{
	public:
		Synth(Wave<T>* form, double f, double phi = 0, double k = 2.0 / SR) : Oscillator<T>(f, phi, k)
		{ waveform = form; }

		T operator()()
		{ return (*waveform)(this->lookup()); }

	private:
		Wave<T>* waveform;
	};
}
