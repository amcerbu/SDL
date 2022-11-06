// Performer.cpp
#include "Performer.h"
#include <Eigen/Core>

// given a relaxation time and sample rate, returns r
// approximate 1 - e^(-1/x) for large x:
// 1 - \sum_{n=0}^\infty (-1/x)^n/n! = \sum_{n=1}^\infty -(-1/x)^n/n!
// as x -> 0, 1 - e^(-x) -> x; thus e^(-1/x) appox 1 - 1/x
T relax(T time, T SR)
{
	if (time * SR <= 1)
		return 0;

	return 1 - 1.0 / (time * SR);
}

ArrayT relax(ArrayT& time, T SR)
{
	ArrayB thresh = SR * time > 1;
	ArrayT scale = thresh.template cast<T>();

	return scale * (1 - 1.0 / (SR * time));
}

void Performer::setup_envelopes()
{
	env_correction = (decay_times * SR - 1) / (decay_times * SR); // dominated by the tail:
		// ((1.0 / (1.0 - env_thresh)).log() - env_thresh) / (attack_times * SR); // integral of rising envelope
}

void Performer::set_envelope(T attack, T decay)
{
	attack_times = attack;
	decay_times = decay;

	attacks = relax(attack_times, SR);
	decays = relax(decay_times, SR); // 
	setup_envelopes(); // sum of envelope approx 1
}

void Performer::set_ringing(T ringing)
{
	this->ringing = ringing;
	modulate();
}


void Performer::mod_decay(ArrayT* decay)
{
	decay_times = *decay;
	this->decays = relax(decay_times, SR);
	setup_envelopes();
}

void Performer::mod_ringing(ArrayT* ringing)
{
	this->ringing = *ringing;
}

Performer::Performer(int n, int overtones, int transp, T ringing, T attack, T decay, T darkness, int order, T thresh) : 
	slidebank(n * overtones, order), darkness(darkness),
	n(n), overtones(overtones), transp(transp), ringing(n), loading(n),
	envelopes(n), attacks(n), decays(n), env_thresh(n), env_correction(n),
	attack_times(n), decay_times(n)
{
	out = new ArrayCT(n);

	this->ringing = ringing;
	loading = false;
	envelopes.setZero();
	attack_times = attack;
	decay_times = decay;
	env_thresh = thresh;

	attacks = relax(attack_times, SR);
	decays = relax(decay_times, SR); // 
	setup_envelopes(); // sum of envelope approx 1

	attenuation.resize(overtones);
	for (int i = 0; i < overtones; i++)
		attenuation(i) = pow(darkness, i);

	correction = CT(1.0, 0) / attenuation.sum();

	ball_radii.resize(n);
	ball_values.resize(n);
	notes.resize(n);
	octaves.resize(n);

	ball_radii.setZero();
	ball_values.setZero();
	notes.setZero();
	octaves.setZero();

	modulate();
}

Performer::~Performer()
{
	delete out;
}

// expects indicator vector of notes and list of octave displacements
void Performer::set_notes(const ArrayT* notes, const ArrayT* octaves)
{
	this->notes = *notes;
	this->octaves = *octaves;
}

// modulate all notes
void Performer::modulate()
{
	std::vector<CT> radii(n * overtones);
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < overtones; j++)
		{
			T freq = mtof(i + n * octaves(i) + transp) * (1 + j);
			T theta = 2 * PI * freq / SR;
			T rad = relax(ringing(i) / freq, SR);
			// T rad = relax(ringing(i), SR);
			radii[j + i * overtones] = CT(rad * cos(theta), rad * sin(theta));
		}
	}

	slidebank.modify(radii);
}

const ArrayCT* Performer::impulse(const ArrayCT* input)
{
	ArrayT jump = (*input).real() - envelopes;
	loading = (jump > 0) || loading; // loading can only increase if jump > 0
	loading = (envelopes < env_thresh) && loading; // if it happens that envelopes has crossed, cut off

	// ArrayB positive = jump > 0 && envelopes < env_thresh;
	ArrayT scale = loading.template cast<T>();
	// envelopes += scale * attacks * jump + (1 - scale) * (1 - decays) * jump;
	envelopes += scale * attacks * jump + (1 - scale) * (1 - decays) * jump;

	ArrayCT burst = excitation() * envelopes.template cast<CT>() * env_correction;
	ArrayCT feed = burst.rowwise().replicate(overtones).transpose().reshaped().array();
	const ArrayCT* output = slidebank(&feed);
	MatrixCT shape = output->matrix().reshaped(overtones, n);
	*out = correction * (shape.transpose() * attenuation).array();
	// *out = shape.rowwise().sum().array();
	
	return out;
}

const ArrayCT* Performer::operator()(const ArrayCT* input)
{
	ArrayCT burst = *input;
	ArrayCT feed = burst.rowwise().replicate(overtones).transpose().reshaped().array();
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

void Performer::graphics(RenderWindow* window, int screen_width, int screen_height, int draw_width, int draw_height, double radius, double growth)
{
	double smooth = relax(3.0 / 60, 60);
	ball_radii = smooth * ball_radii + (1 - smooth) * notes;
	ball_values = smooth * ball_values + (1 - smooth) * octaves;

	for (int i = 0; i < n; i++)
	{
		double x = (screen_width / 2 + (1 + 0.25 * ball_values(i)) * draw_width / 4 * sin(2 * M_PI * (double) i / n));
		double y = (screen_height / 2 - (1 + 0.25 * ball_values(i)) * draw_height / 4 * cos(2 * M_PI * (double) i / n));
		Color color;
		color.hsva((double)i / n, 2.0 / 3, 1.0 / 3 + 1.0 / 6 * ball_values(i), 0.75);

		window->color(color);
		window->circle(x, y, radius * ball_radii(i));
	}
}

void Performer::scope(RenderWindow* window)
{

}