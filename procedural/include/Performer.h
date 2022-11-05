// Performer.h
#pragma once

#include "Slidebank.h"
#include "noise.h"

using namespace soundmath;

class Performer
{
public:
	Performer(int n, int overtones, int transp, T ringing, T darkness = 0.7, int order = 1);
	~Performer();

	void full_mod(const std::vector<T>& midi);
	const ArrayCT* operator()(const ArrayCT* input);
	void tick();

private:
	Slidebank slidebank;
	Noise<T> excitation;

	int n, overtones, transp;
	T ringing;

	VectorCT attenuation;
	ArrayT envelopes;
	ArrayT attacks;
	ArrayT decays;

	ArrayCT* out;
};

/*
n-voice harmonic polyphonic subtractive synthesizer.

each voice consists of some (in)harmonic series of one-pole
complex filters. frequencies of these filters should be expected
to modulate at near audio rate. 

each voice has an associated envelope; the voice is fed that envelope
multiplied by noise. parameters of the envelope may be modulated as well. 
*/