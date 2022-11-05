// Audio.h
#pragma once
#include "AbstractAudio.h"

inline int callback(const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void*);

class Audio : public AbstractAudio
{
public:
	using AbstractAudio::AbstractAudio;
	int process(const float* in, float* out, unsigned long frames);
	void prepare();
};
