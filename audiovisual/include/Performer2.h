#pragma once

#include "noise.h"
#include "filterbank.h"

using namespace soundmath;

class Performer2
{
public:
	Performer2(int n, int overtones, int transp, double low_r, double high_r, double darkness = 0.8);
	~Performer2();

	void open(int note);
	double operator()(double sample, int note);
	void close(int note);
	void modulate(int note, double parameter);

	void tick();

private:
	Filterbank<double>* filters;
	Noise<double>* excitation;
	double* envelopes;
	double* attacks;
	double* decays;

	bool* active;
	double* radii;
	double* frequencies;
	
	int n, overtones, transp;
	double low_r, high_r, darkness;
	double attack = 0.9;

	// returns scaling coefficient for two-pole, two-zero resonant filter
	double resonant(double frequency, double Q);
};

// #pragma once

// // #include "slidebank.h"
// #include "filterbank.h"
// #include "noise.h"

// using namespace soundmath;

// // N overtones, n-EDO
// class Performer
// {
// public:
// 	Performer(int n, int overtones, int transp, double low_r, double high_r, double darkness = 0.8);
// 	~Performer();

// 	void open(int note);
// 	double operator()(double sample, int note);
// 	void close(int note);
// 	void modulate(int note, double parameter);

// 	void tick();

// private:
// 	Filterbank<double>** filters;
// 	Noise<double>* excitation;
// 	double* envelopes;
// 	double* attacks;
// 	double* decays;

// 	double* amplitudes;
// 	bool* active;
// 	double* radii;
// 	double* frequencies;
	
// 	int n, overtones, transp;
// 	double low_r, high_r, darkness;
// 	double attack = 0.9;
// };