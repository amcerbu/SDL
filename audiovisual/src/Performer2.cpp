#include "Performer2.h"

template <typename T> void fill(T* array, T val, int num)
{
	for (int i = 0; i < num; i++)
		array[i] = val;
}

Performer2::Performer2(int n, int overtones, int transp, double low_r, double high_r, double darkness) :
	n(n), overtones(overtones), transp(transp),
	low_r(low_r), high_r(high_r), darkness(darkness)
{
	excitation = new Noise<double>;

	envelopes = new double[n];
	memset(envelopes, 0, n * sizeof(double));

	attacks = new double[n];
	fill(attacks, 1.0, n);

	decays = new double[n];
	fill(decays, 0.005, n);

	active = new bool[n];
	fill(active, false, n);

	radii = new double[n];
	fill(radii, 0.9999, n);

	frequencies = new double[n];
	fill(frequencies, 0.0, n);

	filters = new Filterbank<double>(2, n * overtones);
	for (int i = 0; i < n * overtones; i++)
		filters->coefficients(i, {0,0,0}, {0});

	for (int i = 0; i < n; i++)
		for (int j = 0; j < overtones; j++)
			filters->boost(i * overtones + j, pow(darkness, j));

	filters->open();
}

Performer2::~Performer2()
{
	delete [] envelopes;
	delete [] decays;
	delete [] active;
	delete [] radii;
	delete [] frequencies;

	delete excitation;

	delete filters;
}

void Performer2::open(int note)
{
	active[note % n] = true;
}

void Performer2::tick()
{
	// update and tick filters
	for (int i = 0; i < n; i++)
		for (int j = 0; j < overtones; j++)
		{
			double frequency = frequencies[i] * (1 + j);
			double cosine = cos(2 * PI * frequency / SR);
			double gain = resonant(frequency, radii[i]);

			filters->mix(i * overtones + j, active[i]);
			filters->coefficients(i * overtones + j, {gain, 0, -gain}, {-2 * radii[i] * cosine, radii[i] * radii[i]});
		}

	filters->tick();
}

double Performer2::operator()(double sample, int note)
{
	int i = note % n;
	double jump = sample - envelopes[i];
	envelopes[i] += sample > 0 ? attacks[i] * jump : decays[i] * jump;

	return (*filters)(envelopes[i] * (*excitation)());
}

void Performer2::close(int note)
{
	active[note % n] = false;
}

void Performer2::modulate(int note, double parameter)
{
	frequencies[note % n] = mtof(note + transp);
	// eventually modify radii by parameter somehow
}


// returns scaling coefficient for two-pole, two-zero resonant filter
double Performer2::resonant(double frequency, double Q)
{
	using namespace std::complex_literals;

	std::complex<double> cosine2(cos(4 * PI * frequency / SR), 0);
	std::complex<double> sine2(sin(4 * PI * frequency / SR), 0);
	
	std::complex<double> maximum = 1.0 / (Q - 1) - 1.0 / (Q - cosine2 - 1.0i * sine2);
	return 1 / sqrt(abs(maximum));
}

// #include "Performer.h"

// template <typename T> void fill(T* array, T val, int num)
// {
// 	for (int i = 0; i < num; i++)
// 		array[i] = val;
// }

// Performer::Performer(int n, int overtones, int transp, double low_r, double high_r, double darkness) :
// 	n(n), overtones(overtones), transp(transp),
// 	low_r(low_r), high_r(high_r), darkness(darkness)
// {
// 	filters = new Filterbank<double>*[n];
// 	for (int i = 0; i < n; i++)
// 	{
// 		filters[i] = new Filterbank<double>(2, overtones);
// 		for (int j = 0; j < overtones; j++)
// 			filters[i]->mix(j, pow(darkness, j));
// 	}

// 	excitation = new Noise<double>;

// 	envelopes = new double[n];
// 	memset(envelopes, 0, n * sizeof(double));

// 	attacks = new double[n];
// 	fill(attacks, 1.0, n);
// 	// memset(attacks, 0.001, n * sizeof(double));

// 	decays = new double[n];
// 	fill(decays, 0.005, n);
// 	// memset(decays, 0.9999, n * sizeof(double));

// 	amplitudes = new double[n];
// 	fill(amplitudes, 0.0, n);
// 	// memset(amplitudes, 0, n * sizeof(double));

// 	active = new bool[n];
// 	fill(active, false, n);
// 	// memset(active, false, n * sizeof(bool));

// 	radii = new double[n];
// 	fill(radii, 0.9999, n);
// 	// memset(radii, 0.9999, n * sizeof(double));

// 	frequencies = new double[n];
// 	fill(frequencies, 0.0, n);
// 	// memset(frequencies, 0, n * sizeof(double));
// }

// Performer::~Performer()
// {
// 	delete [] envelopes;
// 	delete [] decays;
// 	delete [] amplitudes;
// 	delete [] active;
// 	delete [] radii;
// 	delete [] frequencies;

// 	for (int i = 0; i < n; i++)
// 		delete filters[i];
// 	delete [] filters;

// 	delete excitation;
// }

// void Performer::open(int note)
// {
// 	active[note % n] = true;
// }

// double Performer::operator()(double sample, int note)
// {
// 	int i = note % n;
// 	double jump = sample - envelopes[i];
// 	envelopes[i] += sample > 0 ? attacks[i] * jump : decays[i] * jump;

// 	double out = 0;
// 	// if (amplitudes[i])
// 	// 	out += (*filters[i])(envelopes[i] * (*excitation)());

// 	return out;
// }

// void Performer::close(int note)
// {
// 	active[note % n] = false;
// }

// void Performer::modulate(int note, double parameter)
// {
// 	frequencies[note % n] = mtof(note + transp);
// 	// eventually modify radii by parameter somehow
// }

// // returns scaling coefficient for two-pole, two-zero resonant filter
// double resonant(double frequency, double Q)
// {
// 	using namespace std::complex_literals;

// 	std::complex<double> cosine2(cos(4 * PI * frequency / SR), 0);
// 	std::complex<double> sine2(sin(4 * PI * frequency / SR), 0);
	
// 	std::complex<double> maximum = 1.0 / (Q - 1) - 1.0 / (Q - cosine2 - 1.0i * sine2);
// 	return 1 / sqrt(abs(maximum));
// }

// void Performer::tick()
// {
// 	for (int i = 0; i < n; i++)
// 		for (int j = 0; j < overtones; j++)
// 			filters[i]->boost(j, active[i]);

// 	// update and tick filters
// 	for (int i = 0; i < n; i++)
// 	{
// 		for (int j = 0; j < overtones; j++)
// 		{
// 			double frequency = frequencies[i] * (1 + j);
// 			double cosine = cos(2 * PI * frequency / SR);
// 			double gain = resonant(frequency, radii[i]);

// 			filters[i]->coefficients(j, {gain, 0, -gain}, {-2 * radii[i] * cosine, radii[i] * radii[i]});
// 		}

// 		filters[i]->tick();
// 	}

// 	excitation->tick();
// }
