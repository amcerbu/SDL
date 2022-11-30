// Performer.h
#pragma once

#include "Slidebank.h"
#include "noise.h"
#include "RenderWindow.h"

using namespace soundmath;

class Performer
{
public:
	Performer(int n, int overtones, int transp, T ringing, T attack, T decay, T darkness = 0.7, int order = 1, T thresh = 0.75);
	// Performer(int n, int overtones, T darkness = 0.7, int order = 1, T thresh = 0.75);
	~Performer();

	void set_notes(const ArrayT* notes, const ArrayT* octaves);
	void set_envelope(T attack, T decay);
	void set_ringing(T ringing);

	void mod_decay(ArrayT* decay);
	void mod_ringing(ArrayT* ringing);
	void modulate();

	const ArrayCT* impulse(const ArrayCT* input);
	const ArrayCT* operator()(const ArrayCT* input);
	void tick();
	void graphics(RenderWindow* window, int screen_width, int screen_height, int draw_width, int draw_height, double radius, double growth);
	void scope(RenderWindow* window);

private:
	Slidebank slidebank;
	Noise<T> excitation;

	int n, overtones, transp;
	ArrayT ringing;
	T darkness;
	CT correction;

	VectorCT attenuation;
	ArrayB loading;
	ArrayT envelopes;
	ArrayT attack_times;
	ArrayT attacks;
	ArrayT decay_times;
	ArrayT decays;
	ArrayT env_thresh;
	ArrayT env_correction;

	ArrayT notes;
	ArrayT octaves;
	ArrayT ball_radii;
	ArrayT ball_values;

	ArrayCT* out;

	void setup_envelopes();
};

/*
n-voice harmonic polyphonic subtractive synthesizer.

each voice consists of some (in)harmonic series of one-pole
complex filters. frequencies of these filters should be expected
to modulate at near audio rate. 

each voice has an associated envelope; the voice is fed that envelope
multiplied by noise. parameters of the envelope may be modulated as well. 
*/