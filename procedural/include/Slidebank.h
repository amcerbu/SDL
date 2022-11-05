// Slidebank.h
#pragma once

#include "includes.h"
#include "EigenTypes.h"

// bank of (conceivably high-order) lowpass filters
// with poles optimized for particular decay times
// N ins, N outs
class Slidebank
{
public:
	Slidebank();
	~Slidebank();

	// initialize N DC-pass filters of a given order
	// then filter using response. the idea of response is to 
	// provide different radii for positive and negative changes
	Slidebank(int N, int order, const std::vector<std::complex<T>>& radii);

	void setup(const std::vector<std::complex<T>>& radii);
	void modify(const std::vector<std::complex<T>>& radii);

	// get the result of filters applied to a sample
	const ArrayCT* operator()(const ArrayCT* input);
	const ArrayCT* operator()(const ArrayT* input);
	const ArrayCT* operator()();

	void tick();

	// writes to a boolean array passed by reference
	void poll(bool* voices, T thresh, T proportion = 0.8);

	void print();

private:
	int N;
	int order;
	std::complex<T>* radii;

	SpMatrixCT back;
	VectorCT* inputs;
	ArrayCT* out;

	bool computed = false;

	void compute(const ArrayT* input);
	void compute(const ArrayCT* input);
};