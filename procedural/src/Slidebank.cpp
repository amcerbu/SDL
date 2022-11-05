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

// initialize N DC-pass filters of a given order
// then filter using response. the idea of response is to 
// provide different radii for positive and negative changes
Slidebank::Slidebank(int N, int order, const std::vector<CT>& radii) : N(N), back(order * N, (order + 1) * N)
{
	this->order = order = std::max(1, order);
	this->radii = new CT[N]; // one radius per filter

	// back = SpMatrixCT(order * N, (order + 1) * N); // feedback is a sparse matrix
	inputs = new VectorCT((order + 1) * N); // inputs is unrolled vector; stores history
	out = new ArrayCT(N);

	setup(radii);
	// print();

	inputs->setZero();
	out->setZero();

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
	computed = true;
}