// Audio.h
#pragma once
#include "AbstractAudio.h"
#include "RenderWindow.h"

inline int callback(const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void*);

class Audio : public AbstractAudio
{
public:
	using AbstractAudio::AbstractAudio;
	int process(const float* in, float* out, unsigned long frames);
	void args(int argc, char *argv[]);
	void prepare();
	void defaults();

	void graphics(RenderWindow* window, int screen_width, int screen_height, int draw_width, int draw_height, double radius);
	void scope(RenderWindow* window);

private:
	double gain, headroom, ringing, attack, decay;
	int out_offset;
};
