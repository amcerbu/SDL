// EigenTypes.h
#pragma once

#include <Eigen/SparseCore>

using Eigen::Matrix;
using Eigen::Dynamic;
using Eigen::Map;
using Eigen::Array;
using Eigen::SparseMatrix;
using Eigen::seqN;
using Eigen::seq;

typedef double T;
typedef std::complex<T> CT;

typedef Matrix<CT, Dynamic, Dynamic> MatrixCT;
typedef Matrix<CT, Dynamic, 1> VectorCT;
typedef Array<CT, Dynamic, 1> ArrayCT;
typedef Matrix<T, Dynamic, Dynamic> MatrixT;
typedef Matrix<T, Dynamic, 1> VectorT;
typedef Array<T, Dynamic, 1> ArrayT;
typedef Array<bool, Dynamic, 1> ArrayB;
	
typedef SparseMatrix<std::complex<T>> SpMatrixCT;
typedef Eigen::Triplet<std::complex<T>> Triplet;