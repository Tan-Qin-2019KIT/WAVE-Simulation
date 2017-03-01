#include "FDTD3D.hpp"
using namespace scai;

//! \brief Constructor to support Configuration
/*!
 *
 \param dist Distribution of the wavefield
 \param ctx Context
 \param config Configuration
 \param comm Communicator
 */
template <typename ValueType>
KITGPI::ForwardSolver::Derivatives::FDTD3D<ValueType>::FDTD3D(scai::dmemo::DistributionPtr dist, scai::hmemo::ContextPtr ctx, Configuration::Configuration const &config, scai::dmemo::CommunicatorPtr comm)
{
    init(dist, ctx, config, comm);
}

//! \brief Initialisation to support Configuration
/*!
 *
 \param dist Distribution of the wavefield
 \param ctx Context
 \param config Configuration
 \param comm Communicator
 */
template <typename ValueType>
void KITGPI::ForwardSolver::Derivatives::FDTD3D<ValueType>::init(scai::dmemo::DistributionPtr dist, scai::hmemo::ContextPtr ctx, Configuration::Configuration const &config, scai::dmemo::CommunicatorPtr comm)
{
    useFreeSurface = config.get<IndexType>("FreeSurface");
    Derivatives<ValueType>::initializeMatrices(dist, ctx, config, comm);
}

//! \brief Constructor of the derivative matrices
/*!
 *
 \param dist Distribution of the wavefield
 \param ctx Context
 \param NX Total number of grid points in X
 \param NY Total number of grid points in Y
 \param NZ Total number of grid points in Z
 \param DH Grid spacing (equidistant)
 \param DT Temporal sampling interval#
 \param spatialFDorderInput FD-order of spatial derivative stencils
 \param comm Communicator
 */
template <typename ValueType>
KITGPI::ForwardSolver::Derivatives::FDTD3D<ValueType>::FDTD3D(scai::dmemo::DistributionPtr dist, scai::hmemo::ContextPtr ctx, IndexType NX, IndexType NY, IndexType NZ, ValueType DH, ValueType DT, IndexType spatialFDorderInput, scai::dmemo::CommunicatorPtr comm)
{
    initializeMatrices(dist, ctx, NX, NY, NZ, DH, DT, spatialFDorderInput, comm);
}

//! \brief Initializsation of the derivative matrices
/*!
 *
 \param dist Distribution of the wavefield
 \param ctx Context
 \param NX Total number of grid points in X
 \param NY Total number of grid points in Y
 \param NZ Total number of grid points in Z
 \param DH Grid spacing (equidistant)
 \param DT Temporal sampling interval
 \param spatialFDorderInput FD-order of spatial stencils
 \param comm Communicator
 */
template <typename ValueType>
void KITGPI::ForwardSolver::Derivatives::FDTD3D<ValueType>::initializeMatrices(scai::dmemo::DistributionPtr dist, scai::hmemo::ContextPtr ctx, IndexType NX, IndexType NY, IndexType NZ, ValueType DH, ValueType DT, IndexType spatialFDorderInput, scai::dmemo::CommunicatorPtr comm)
{

    SCAI_REGION("initializeMatrices")

    HOST_PRINT(comm, "Initialization of the matrices Dxf, Dyf, Dzf, Dxb, Dyb, Dzb…\n");

    // Set FD-order to class member
    spatialFDorder = spatialFDorderInput;

    /* Set FD-Coefficients */
    this->setFDCoef(spatialFDorder);

    this->calcDxf(NX, NY, NZ, dist);
    this->calcDzf(NX, NY, NZ, dist);
    this->calcDyf(NX, NY, NZ, dist);

    HOST_PRINT(comm, "Matrix Dxf, Dyf and Dzf finished.\n");

    Dxf.setContextPtr(ctx);
    Dzf.setContextPtr(ctx);
    Dxb.setContextPtr(ctx);
    Dzb.setContextPtr(ctx);
    Dyf.setContextPtr(ctx);
    Dyb.setContextPtr(ctx);

    Dxb.assignTranspose(Dxf);
    Dxb.scale(-1.0);
    Dzb.assignTranspose(Dzf);
    Dzb.scale(-1.0);
    Dyb.assignTranspose(Dyf);
    Dyb.scale(-1.0);

    HOST_PRINT(comm, "Matrix Dxb, Dyb and Dzb finished.\n");

    Dxf.scale(lama::Scalar(DT / DH));
    Dzf.scale(lama::Scalar(DT / DH));
    Dxb.scale(lama::Scalar(DT / DH));
    Dzb.scale(lama::Scalar(DT / DH));
    Dyf.scale(lama::Scalar(DT / DH));
    Dyb.scale(lama::Scalar(DT / DH));

    HOST_PRINT(comm, "Finished with initialization of the matrices!\n");
}

template class KITGPI::ForwardSolver::Derivatives::FDTD3D<double>;
template class KITGPI::ForwardSolver::Derivatives::FDTD3D<float>;