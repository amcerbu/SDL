// Audio.h
#pragma once
#include "AbstractAudio.h"
#include "RenderWindow.h"
#include "Multitouch.h"

inline int callback(const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void*);

class Audio : public AbstractAudio
{
public:
	using AbstractAudio::AbstractAudio;
	int process(const float* in, float* out, unsigned long frames);
	void args(int argc, char *argv[]);
	void prepare();
	void defaults();

	void tempo(T scale);
	void toggle();

private:
	double gain, headroom, ringing, attack, decay;
	int out_offset;

};
