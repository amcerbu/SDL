// Audio.cpp
#include "Audio.h"
#include "Performer.h"
#include "Score.h"
#include "metro.h"
#include "noise.h"
#include "synth.h"
#include "argparse.h"

using namespace soundmath;

const double def_gain = 0.5;
const double def_headroom = 0.5;
const double def_ringing = 10;
const double def_attack = 0.001;
const double def_decay = 1;

// gain = def_gain;
// headroom = def_headroom;
// ringing = def_ringing;
// attack = def_attack;
// decay = def_decay;

const int n = 12;
const int overtones = 7;
const int transp = 36;
// ringing. 1: windy. 10 <== much louder!: 
double darkness = 0.25; // rolloff of high overtones
const int filter_order = 1;
Performer p(n, overtones, transp, def_ringing, def_attack, def_decay, darkness, filter_order);

bool toggler = true;
double metro_tempo = 1;
Metro<double> metro(toggler * metro_tempo); 
Metro<double> lfoer(10);
Noise<double> noise;
Score score("score1.txt");
std::vector<int> notes;
ArrayT chord(n);
ArrayT pitches(n);

Synth<double> lfo(&cycle, 1);

const bool print_matrices = true;
const double width = 0.00625; // setting this to 12 gives mono
const int convolutions = 5; // 
MatrixT kernel(n, n);
MatrixT projection;
MatrixT mix;

int sgn(T x)
{
	return (x >= 0) - (x <= 0);
}

T softclip(T sample, T width = 0.5)
{
	if (abs(sample) < width)
		return sample;

	int sign = sgn(sample);
	T gap = sample - sign * width;
	return sign * width + (1 - width) * std::tanh(gap / (1 - width));
}

T (*distortion)(T in) = std::tanh;
// T (*distortion)(T in) = softclip;

void Audio::defaults()
{
	gain = def_gain;
	headroom = def_headroom;
	ringing = def_ringing;
	attack = def_attack;
	decay = def_decay;
	out_offset = 0;
}

int Audio::process(const float* in, float* out, unsigned long frames)
{
	for (int i = 0; i < frames; i++)
	{
		chord.setZero();
		pitches.setZero();
		for (int a : notes)
		{
			chord(a % n) = 1;
			pitches(a % n) = a + 0.5 * lfo();
		}
		
		if (lfoer())
		{	
			p.set_notes(&chord, &pitches);
			p.modulate();
		}

		if (metro() || toggler)
			metro.freqmod(toggler * metro_tempo);

		ArrayCT input = metro() * chord;
		// ArrayCT input = chord;
		ArrayT clipping = p.impulse(&input)->real();
		// ArrayT output = gain * clipping.unaryExpr([](T x) { return softclip(x, 0.99); });
		ArrayT output = gain * clipping;
		
		// distribute voices across channels
		ArrayT distributed = mix * output.matrix();
		for (int j = 0; j < out_chans; j++)
		{
			out[out_offset + j + i * out_chans] = distributed(j) * headroom;
		}

		p.tick();
		metro.tick();
		lfo.tick();
		lfoer.tick();
		noise.tick();
	}

	return 0;
}

void Audio::prepare()
{
	projection.resize(out_chans, n);
	projection.setZero();
	for (int i = 0; i < out_chans; i++)
	{
		for (int j = 0; j < n; j++)
		{
			double discrep = 1;
			for (int l = -1; l <= 1; l++)
				discrep = fmin(discrep, abs(l + (double)i / out_chans - (double)j / n));

			double entry = 1 - discrep;
			double thresh = (1 - 1.0 / (out_chans + 1)); // this controls how localized each source is

			projection(i, (7 * j) % n) = entry > thresh ? entry * entry : 0;
		}
	}
	projection.rowwise().normalize();

	kernel.setZero();
	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < n; j++)
		{
			kernel(j, (i + j) % n) = 1;
			kernel(j, (i + j + 1) % n) = width;
			kernel(j, (n + i + j - 1) % n) = width;
		}
	}
	kernel.rowwise().normalize();

	for (int i = 0; i < convolutions; i++)
	{
		kernel = kernel * kernel;
		kernel.rowwise().normalize();
	}

	mix = projection * kernel;
	mix.colwise().normalize();

	if (print_matrices)
	{
		std::cout << "convolution:\n";
		for (int i = 0; i < n; i++)
		{
			for (int j = 0; j < n; j++)
				std::cout << std::setprecision(5) << kernel(i,j) << "\t";
			std::cout << std::endl;
		}
		std::cout << std::endl;

		std::cout << "projection:\n";
		for (int i = 0; i < out_chans; i++)
		{
			for (int j = 0; j < n; j++)
				std::cout << std::setprecision(5) << projection(i,j) << "\t";
			std::cout << std::endl;
		}
		std::cout << std::endl;

		std::cout << "mix:\n";
		for (int i = 0; i < out_chans; i++)
		{
			for (int j = 0; j < n; j++)
				std::cout << std::setprecision(5) << mix(i,j) << "\t";
			std::cout << std::endl;
		}
	}

	p.set_envelope(attack, decay);
	p.set_ringing(ringing);

	notes = {10, 15, 26, 19, 29, 12};
	chord.setZero();
	pitches.setZero();
	for (int a : notes)
	{
		chord(a % n) = 1;
		pitches(a % n) = a;
	}
	
	p.set_notes(&chord, &pitches);
	p.modulate();
}

void Audio::tempo(T scale)
{
	metro_tempo *= scale;
}

void Audio::toggle()
{
	toggler = !toggler;
}

void Audio::args(int argc, char *argv[])
{
	argparse::ArgumentParser program("");

	int def_in = 0;
	int def_out = 0;
	initialize(false, &def_in, &def_out);

	program.add_argument("-i", "--input")
		.default_value<int>((int)def_in)
		.required()
		.scan<'i', int>()
		.help("device id for audio in");

	program.add_argument("-if", "--in_framesize")
		.default_value<int>(1)
		.required()
		.scan<'i', int>()
		.help("channels per frame of input");

	program.add_argument("-ic", "--in_channel")
		.default_value<int>(0)
		.required()
		.scan<'i', int>()
		.help("input channel selector (in [0, if - 1])");

	program.add_argument("-o", "--output")
		.default_value<int>((int)def_out)
		.required()
		.scan<'i', int>()
		.help("device id for audio out");

	program.add_argument("-of", "--out_framesize")
		.default_value<int>(2)
		.required()
		.scan<'i', int>()
		.help("channels per frame of output");

	program.add_argument("-oc", "--out_channel")
		.default_value<int>(0)
		.required()
		.scan<'i', int>()
		.help("channel offset for output");


	program.add_argument("-d", "--devices")
		.help("list audio device names and exits")
		.default_value(false)
		.implicit_value(true);

	program.add_argument("-g", "--gain")
		.default_value<double>((double)def_gain)
		.required()
		.scan<'f', double>()
		.help("pre-distortion gain");

	program.add_argument("-h", "--headroom")
		.default_value<double>((double)def_headroom)
		.required()
		.scan<'f', double>()
		.help("post-distortion gain");

	program.add_argument("-r", "--ringing")
		.default_value<double>((double)def_ringing)
		.required()
		.scan<'f', double>()
		.help("wavelengths of filter ring");

	program.add_argument("-a", "--attack")
		.default_value<double>((double)def_attack)
		.required()
		.scan<'f', double>()
		.help("attack of envelope (seconds)");

	program.add_argument("-k", "--decay")
		.default_value<double>((double)def_decay)
		.required()
		.scan<'f', double>()
		.help("decay of envelope (seconds)");

	try
	{
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err)
	{
		std::cerr << err.what() << std::endl;
		std::cerr << program;
		std::exit(1);
	}

	in_device = program.get<int>("-i");
	in_chans = program.get<int>("-if");
	in_channel = program.get<int>("-ic");
	out_device = program.get<int>("-o");
	out_chans = program.get<int>("-of");
	this->out_offset = program.get<int>("-oc");
	this->gain = program.get<double>("-g");
	this->headroom = program.get<double>("-h");
	this->ringing = program.get<double>("-r");
	this->attack = program.get<double>("-a");
	this->decay = program.get<double>("-k");

	initialize(program.is_used("-d"));

	if (program.is_used("-d"))
	{
		std::exit(1);
	}
}
