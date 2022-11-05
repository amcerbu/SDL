// Performer.cpp
#include "Performer.h"
#include <Eigen/Core>

std::vector<CT> radii(int n, int overtones, int transp, T ringing)
{
	std::vector<CT> r;
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < overtones; j++)
		{
			T theta = 2 * PI * mtof(i + transp) * (1 + j) / SR;
			r.push_back(CT(exp(-1.0 / ringing) * cos(theta), exp(-1.0 / ringing) * sin(theta)));
		}
	}

	return r;
}

Performer::Performer(int n, int overtones, int transp, T ringing, T darkness, int order) : 
	slidebank(n * overtones, order, radii(n, overtones, transp, ringing / order)),
	n(n), overtones(overtones), transp(transp), ringing(ringing)
{
	out = new ArrayCT(n);

	envelopes.resize(n);
	attacks.resize(n);
	decays.resize(n);

	envelopes.setZero();
	attacks = exp(-1.0 / 20);
	decays = 1 - exp(-1.0 / 100);

	attenuation.resize(overtones);
	for (int i = 0; i < overtones; i++)
		attenuation(i) = pow(darkness, i);
}

Performer::~Performer()
{
	delete out;
}

// // modulate note in [0,...,n) to midi
// void Performer::pitch_mod(int note, double midi)
// {

// }

// modulate all notes
void Performer::full_mod(const std::vector<T>& midi)
{
	std::vector<CT> radii(n * overtones);
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < overtones; j++)
		{
			T theta = 2 * PI * mtof(midi[i] + transp) * (1 + j) / SR;
			radii[j + i * overtones] = CT(exp(-1.0 / ringing) * cos(theta), exp(-1.0 / ringing) * sin(theta));
		}
	}

	slidebank.modify(radii);
}

const ArrayCT* Performer::operator()(const ArrayCT* input)
{
	ArrayT jump = (*input).real() - envelopes;
	ArrayB positive = jump > 0;
	ArrayT scale = positive.template cast<T>();
	envelopes += scale * attacks * jump + (1 - scale) * decays * jump;

	ArrayCT impulse = excitation() * envelopes.template cast<CT>();
	ArrayCT feed = impulse.rowwise().replicate(overtones).transpose().reshaped().array();
	const ArrayCT* output = slidebank(&feed);
	MatrixCT shape = output->matrix().reshaped(overtones, n);
	*out = (shape.transpose() * attenuation).array();
	// *out = shape.rowwise().sum().array();
	
	return out;
}

void Performer::tick()
{
	excitation.tick();
	slidebank.tick();
}