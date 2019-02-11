/*
 * Diffusion.h
 *
 *  Created on: 26.06.2017
 *      Author: Moritz v. Looz
 *
 * Contains methods for graph diffusion and generators for related methods.
 */

#pragma once

#include <scai/lama.hpp>
#include <scai/lama/Vector.hpp>
#include <scai/lama/matrix/CSRSparseMatrix.hpp>
#include <scai/lama/matrix/DenseMatrix.hpp>
#include "Settings.h"

namespace ITI {

template<typename IndexType, typename ValueType>
class Diffusion {

public:
	Diffusion() = default;
	virtual ~Diffusion() = default;

    /**
     * Computes the potential vector of a diffusion flow in a graph. Calls a linear solver to solve Lx=d for x, where L is the graph laplacian and d the demand vector.
     *
     * @param laplacian The laplacian of the graph
     * @param nodeWeights The demand at each (non-source) node. When in doubt, set uniformly to 1.
     * @param source Index of the node where the flow enters.
     * @param eps accuracy
     *
     */
	static scai::lama::DenseVector<ValueType> potentialsFromSource(scai::lama::CSRSparseMatrix<ValueType> laplacian, scai::lama::DenseVector<ValueType> nodeWeights, IndexType source, ValueType eps=1e-6);

	/**
	 * @brief Calls potentialsFromSource several times, once for each source in sources
	 *
	 * @param laplacian The laplacian of the graph
     * @param nodeWeights The demand at each (non-source) node. When in doubt, set uniformly to 1.
     * @param sources list of source indices
     * @param eps accuracy
	 *
	 * @return dense matrix, each row contains one set of potentials, usabel as coordinates
	 */
	static scai::lama::DenseMatrix<ValueType> multiplePotentials(scai::lama::CSRSparseMatrix<ValueType> laplacian, scai::lama::DenseVector<ValueType> nodeWeights, std::vector<IndexType> sources, ValueType eps=1e-6);

};

} /* namespace ITI */
