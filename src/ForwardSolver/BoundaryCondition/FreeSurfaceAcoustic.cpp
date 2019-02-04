#include "FreeSurfaceAcoustic.hpp"
using namespace scai;

//! Default destructor
template <typename ValueType>
KITGPI::ForwardSolver::BoundaryCondition::FreeSurfaceAcoustic<ValueType>::~FreeSurfaceAcoustic(){};

/*! \brief Initialitation of the free surface
 *
 \param dist Distribution of wavefields
 \param derivatives Derivative class
 \param NX Number of grid points in X-Direction
 \param NY Number of grid points in Y-Direction (Depth)
 \param NZ Number of grid points in Z-Direction
 \param DT Temporal Sampling
 \param DH Distance between grid points
 */
template <typename ValueType>
void KITGPI::ForwardSolver::BoundaryCondition::FreeSurfaceAcoustic<ValueType>::init(scai::dmemo::DistributionPtr dist, Derivatives::Derivatives<ValueType> &derivatives, IndexType NX, IndexType NY, IndexType NZ, ValueType DT, ValueType DH)
{
    dmemo::CommunicatorPtr comm = dist->getCommunicatorPtr();

    HOST_PRINT(comm, "", "Initialization of the free surface...\n");

    active = true;

    derivatives.useFreeSurface = true;
    derivatives.calcDyfFreeSurface(NX, NY, NZ, dist);
    derivatives.DyfFreeSurface *= DT / DH;
    derivatives.Dyf.purge();

    /* Distributed vectors */
    setSurfaceZero.allocate(dist); // Vector to set elements on surface to zero
    setSurfaceZero = 1.0;

    /* Get local "global" indices */
    hmemo::HArray<IndexType> localIndices;
    dist->getOwnedIndexes(localIndices);             /* get local indices based on used distribution */
    IndexType numLocalIndices = localIndices.size(); // Number of local indices

    auto read_localIndices = hostReadAccess(localIndices); // Get read access to localIndices

    /* Get write access to local part of setSurfaceZero */
    auto write_setSurfaceZero = hostWriteAccess(setSurfaceZero.getLocalValues());

    KITGPI::Acquisition::Coordinates coordinateTransformation(NX,NY,NZ);

    IndexType rowGlobal;
    IndexType rowLocal;

    for (IndexType i = 0; i < numLocalIndices; i++) {

        rowGlobal = read_localIndices[i];
        rowLocal = dist->global2Local(rowGlobal);

        /* Determine if the current grid point is located on the surface */
        if (coordinateTransformation.locatedOnSurface(rowGlobal)) {

            /* Set elements at the surface to zero */
            write_setSurfaceZero[rowLocal] = 0.0;
        }
    }

    HOST_PRINT(comm, "", "Finished initializing of the free surface\n\n");
}

template class KITGPI::ForwardSolver::BoundaryCondition::FreeSurfaceAcoustic<float>;
template class KITGPI::ForwardSolver::BoundaryCondition::FreeSurfaceAcoustic<double>;
