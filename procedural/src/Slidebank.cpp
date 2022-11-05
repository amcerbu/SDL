// Slidebank.cpp
#include "Slidebank.h"
#include <iomanip>

Slidebank::Slidebank() : N(1)
{
}

Slidebank::~Slidebank()
{
	delete [] radii;
	// delete back;
	delete inputs;
	delete out;
}

void Slidebank::create_objects()
{
	rolloff = 10; // attenuate per 1000 hz
	this->radii = new CT[N]; // one radius per filter

	inputs = new VectorCT((order + 1) * N); // inputs is unrolled vector; stores history
	out = new ArrayCT(N);

	inputs->setZero();
	out->setZero();
	correction = 1;
}

// initialize N DC-pass filters of a given order
// then filter using response. the idea of response is to 
// provide different radii for positive and negative changes
Slidebank::Slidebank(int N, int order, const std::vector<CT>& radii) : 
	N(N), order(std::max(1, order)), back(order * N, (order + 1) * N), correction(N)
{
	create_objects();
	setup(radii);
}

Slidebank::Slidebank(int N, int order) : 
	N(N), order(std::max(1, order)), back(order * N, (order + 1) * N), correction(N)
{
	create_objects();

	std::vector<CT> radii(N, 0);
	setup(radii);
}

void Slidebank::setup_correction(const std::vector<CT>& radii)
{
	for (int j = 0; j < N; j++)
	{
		T ang_freq = abs(std::arg(radii[j]) / (2 * M_PI));
		// correction(j) = (1 - abs(radii[j])) / (1 + rolloff * ang_freq);
		correction(j) = pow((1 - abs(radii[j])), order) / (1 + rolloff * ang_freq);
		// correction(j) = pow((1 - abs(radii[j])), order);
		correction(j) = 1;
	}
}

void Slidebank::setup(const std::vector<CT>& radii)
{
	for (int i = 0; i < N; i++)
		this->radii[i] = radii[i];

	std::vector<Triplet> coefficients;

	for (int j = 0; j < N; j++)
	{
		coefficients.push_back(Triplet(j * order, j, CT(1,0) - radii[j]));
		coefficients.push_back(Triplet(j * order, N + j * order, radii[j]));
	}
	for (int i = 0; i < N; i++)
	{
		for (int j = 1; j < order; j++)
		{
			coefficients.push_back(Triplet(j + i * order, N + j - 1 + i * order, CT(1,0) - radii[i]));
			coefficients.push_back(Triplet(j + i * order, N + j + i * order, radii[i]));
		}
	}
	back.setFromTriplets(coefficients.begin(), coefficients.end());
	back.makeCompressed();

	setup_correction(radii);
}

void Slidebank::modify(const std::vector<CT>& radii)
{
	for (int k = 0; k < back.outerSize(); ++k)
	{
		for (SpMatrixCT::InnerIterator it(back, k); it; ++it)
		{
			int row = it.row();
			int col = it.col();
			if (row % order == 0)
			{
				int j = row / order;
				CT r = radii[j];

				if (col == j)
					it.valueRef() = CT(1,0) - r;
				else if (col == N + row)
					it.valueRef() = r;
				else
					assert(false);
			}
			else
			{
				int j = row % order;
				int i = (row - j) / order;
				CT r = radii[i];

				if (col == N - 1 + row)
					it.valueRef() = CT(1,0) - r;
				else if (col == N + row)
					it.valueRef() = r;
				else
					assert(false);
			}
		}
	}

	setup_correction(radii);
}

// get the result of filters applied to a sample
const ArrayCT* Slidebank::operator()(const ArrayCT* input)
{
	if (!computed)
		compute(input);

	return out;
}

// get the result of filters applied to a sample
const ArrayCT* Slidebank::operator()(const ArrayT* input)
{
	if (!computed)
		compute(input);

	return out;
}

const ArrayCT* Slidebank::operator()()
{
	if (computed)
		return out;

	return NULL;
}

void Slidebank::tick()
{
	computed = false;
}

// writes to a boolean array passed by reference
void Slidebank::poll(bool* voices, T thresh, T proportion)
{
	T total = 0;
	for (int i = 0 ; i < N; i++)
		total += std::norm((*out)(i));

	T square = thresh * thresh;
	if (total < square)
	{
		for (int i = 0; i < N; i++)
			voices[i] = false;
	}
	else
	{
		for (int i = 0; i < N; i++)
		{
			if (std::norm((*out)(i)) > total * proportion)
				voices[i] = true;
			else
				voices[i] = false;
		}
	}
}

void Slidebank::print()
{
	for (int i = 0; i < order * N; i++)
	{
		for (int j = 0; j < (order + 1) * N; j++)
		{
			std::cout << "\t" << std::real(back.coeffRef(i,j)) << " ";
		}
		std::cout << std::endl;
	}
}

void Slidebank::compute(const ArrayT* input)
{
	ArrayCT casted = input->template cast<CT>();
	compute(&casted);
}

void Slidebank::compute(const ArrayCT* input)
{
	(*inputs)(seqN(0, N)) = input->matrix();

	VectorCT temp = back * (*inputs);
	(*inputs)(seq(N, Eigen::last)) = temp;

	*out = (*inputs)(seqN(N + order - 1, N, order));
	*out = correction * (*out);
	computed = true;
}