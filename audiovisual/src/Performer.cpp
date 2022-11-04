#include "Performer.h"

template <typename T> void fill(T* array, T val, int num)
{
	for (int i = 0; i < num; i++)
		array[i] = val;
}

Performer::Performer(int n, int overtones, int transp, double low_r, double high_r, double darkness) :
	n(n), overtones(overtones), transp(transp),
	low_r(low_r), high_r(high_r), darkness(darkness)
{
	filters = new Filter<double>*[n * overtones];
	for (int i = 0; i < n * overtones; i++)
		filters[i] = new Filter<double>({1,0,0},{0});

	excitation = new Noise<double>;

	envelopes = new double[n];
	memset(envelopes, 0, n * sizeof(double));

	attacks = new double[n];
	fill(attacks, 0.9, n);

	decays = new double[n];
	fill(decays, 0.005, n);

	amplitudes = new double[n];
	fill(amplitudes, 0.0, n);

	active = new bool[n];
	fill(active, false, n);

	radii = new double[n];
	fill(radii, 0.99999, n);

	frequencies = new double[n];
	fill(frequencies, 0.0, n);
}

Performer::~Performer()
{
	delete [] envelopes;
	delete [] decays;
	delete [] amplitudes;
	delete [] active;
	delete [] radii;
	delete [] frequencies;

	for (int i = 0; i < n * overtones; i++)
		delete filters[i];
	delete [] filters;

	delete excitation;
}

void Performer::open(int note)
{
	active[note % n] = true;
}

double Performer::operator()(double sample, int note)
{
	int i = note % n;
	double jump = sample - envelopes[i];
	envelopes[i] += jump > 0 ? attacks[i] * jump : decays[i] * jump;

	double out = 0;
	// if (amplitudes[i])
		for (int j = 0; j < overtones; j++)
			out += pow(darkness, j) * ((*filters[i * overtones + j])(envelopes[i] * (*excitation)()));

	return out;
}

void Performer::close(int note)
{
	active[note % n] = false;
}

void Performer::modulate(int note, double parameter)
{
	int i = note % n;
	frequencies[i] = mtof(note + transp);
	radii[i] = (1 - parameter) * low_r + parameter * high_r;
	decays[i] = 0.9 * (1 - parameter) + 0.001 * parameter;
	// decays[i] = log(2.46 * (1 - parameter) + 1.000 * parameter);
	// eventually modify radii by parameter somehow
}

void Performer::freq_mod(int note, double parameter)
{
	int i = note % n;
	frequencies[i] = mtof(note + transp + parameter);
}

void Performer::tick()
{
	// for (int i = 0; i < n; i++)
	// 	amplitudes[i] = (1 - attack) * active[i] + attack * amplitudes[i];

	// for (int i = 0; i < n; i++)
	// 	envelopes[i] += decays[i];

	// update and tick filters
	for (int i = 0; i < n; i++)
		// if (amplitudes[i])
		for (int j = 0; j < overtones; j++)
		{
			double frequency = frequencies[i] * (1 + j);

			filters[i * overtones + j]->resonant(frequency, radii[i]);
			filters[i * overtones + j]->tick();
		}

	excitation->tick();
}
