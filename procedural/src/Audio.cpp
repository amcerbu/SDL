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
const double def_ringing = 100;
const double def_attack = 0.001;
const double def_decay = 1;

// gain = def_gain;
// headroom = def_headroom;
// ringing = def_ringing;
// attack = def_attack;
// decay = def_decay;

const int n = 12;
const int overtones = 5;
const int transp = 36;
// ringing. 1: windy. 10 <== much louder!: 
double darkness = 0.5; // rolloff of high overtones
const int filter_order = 1;
Performer p(n, overtones, transp, def_ringing, def_attack, def_decay, darkness, filter_order);

bool toggler = false;
double metro_tempo = 10;
Metro<double> metro(toggler * metro_tempo); 
Noise<double> noise;
Score score("score1.txt");
ArrayT chord(n);
ArrayT octaves(n);

Synth<double> lfo(&cycle, 0.05);

const double width = 0.0125; // setting this to 12 gives mono
const int convolutions = 5; // setting this to 12 gives mono
MatrixT kernel(n, n);
MatrixT projection;
MatrixT mix;

T (*distortion)(T in) = std::tanh;

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
		if (metro())
		{
			std::vector<int> notes = score.read();
			std::vector<T> loader(n, 0);
			chord.setZero();
			octaves.setZero();
			for (int a : notes)
			{
				loader[a % n] = a;
				chord(a % n) = 1;
				octaves(a % n) = (a - a % n) / n;
			}

			p.set_notes(&chord, &octaves);
			p.modulate();
		}
		
		// for (int j = 0; j < n; j++)
		// {
		// 	T t = (1 + sin(2 * M_PI * (lfo()) * (double)j / n)) / 2;
		// 	decays(j) = t * 0.1 + (1 - t) * 0.0001;
		// 	ringings(j) = (1 - t) * 0.1 + (t) * 100;
		// }

		

		ArrayCT input = metro() * chord;
		ArrayT clipping = p.impulse(&input)->imag();
		ArrayT output = gain * clipping.unaryExpr(distortion);
		// ArrayCT input = noise() * chord;
		// ArrayCT output = *p(&input);

		
		// distribute voices across channels
		ArrayT distributed = mix * output.matrix();
		for (int j = 0; j < out_chans; j++)
		{
			out[out_offset + j + i * out_chans] = distributed(j) * headroom;
		}

		metro.freqmod(toggler * metro_tempo);

		p.tick();
		metro.tick();
		lfo.tick();
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
			double thresh = 0.875; // this controls how localized each source is

			projection(i, j) = entry > thresh ? entry * entry : 0;
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

	bool print_matrices = false;

	if (print_matrices)
	{
		for (int i = 0; i < n; i++)
		{
			for (int j = 0; j < n; j++)
				std::cout << kernel(i,j) << " ";
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}

	mix = projection * kernel;
	mix.colwise().normalize();

	if (print_matrices)
	{
		for (int i = 0; i < out_chans; i++)
		{
			for (int j = 0; j < n; j++)
				std::cout << mix(i,j) << " ";
			std::cout << std::endl;
		}
	}

	p.set_envelope(attack, decay);
	p.set_ringing(ringing);
}

void Audio::graphics(RenderWindow* window, int screen_width, int screen_height, int draw_width, int draw_height, double radius)
{
	ArrayT decays(n);
	ArrayT ringings(n);

	controller->set_decays(&decays, n);
	controller->set_ringing(&ringings, n);

	p.mod_decay(&decays);
	p.mod_ringing(&ringings);


	p.graphics(window, screen_width, screen_height, draw_width, draw_height, radius, radius / 3);
}

void Audio::tempo(T scale)
{
	metro_tempo *= scale;
}

void Audio::toggle()
{
	toggler = !toggler;
}

void Audio::scope(RenderWindow* window)
{
	p.scope(window);
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
		.default_value<int>(1)
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


void Audio::bind(Multitouch* controller)
{
	this->controller = controller;
}