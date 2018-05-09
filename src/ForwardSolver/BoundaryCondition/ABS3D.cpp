#include "ABS3D.hpp"
using namespace scai;

/*! \brief Application of the damping boundary
 *
 * THIS METHOD IS CALLED DURING TIME STEPPING
 * DO NOT WASTE RUNTIME HERE
 *
 \param v1 DenseVector to apply damping boundary
 \param v2 DenseVector to apply damping boundary
 \param v3 DenseVector to apply damping boundary
 \param v4 DenseVector to apply damping boundary
 */
template <typename ValueType>
void KITGPI::ForwardSolver::BoundaryCondition::ABS3D<ValueType>::apply(lama::Vector<ValueType> &v1, lama::Vector<ValueType> &v2, lama::Vector<ValueType> &v3, lama::Vector<ValueType> &v4)
{

    SCAI_ASSERT_DEBUG(active, " ABS is not active ");

    v1 *= damping;
    v2 *= damping;
    v3 *= damping;
    v4 *= damping;
}

/*! \brief Application of the damping boundary
 *
 * THIS METHOD IS CALLED DURING TIME STEPPING
 * DO NOT WASTE RUNTIME HERE
 *
 \param v1 DenseVector to apply damping boundary
 \param v2 DenseVector to apply damping boundary
 \param v3 DenseVector to apply damping boundary
 \param v4 DenseVector to apply damping boundary
 \param v5 DenseVector to apply damping boundary
 \param v6 DenseVector to apply damping boundary
 \param v7 DenseVector to apply damping boundary
 \param v8 DenseVector to apply damping boundary
 \param v9 DenseVector to apply damping boundary
 */
template <typename ValueType>
void KITGPI::ForwardSolver::BoundaryCondition::ABS3D<ValueType>::apply(
    lama::Vector<ValueType> &v1, lama::Vector<ValueType> &v2, lama::Vector<ValueType> &v3, 
    lama::Vector<ValueType> &v4, lama::Vector<ValueType> &v5, lama::Vector<ValueType> &v6, 
    lama::Vector<ValueType> &v7, lama::Vector<ValueType> &v8, lama::Vector<ValueType> &v9)
{

    SCAI_ASSERT_DEBUG(active, " ABS is not active ");

    v1 *= damping;
    v2 *= damping;
    v3 *= damping;
    v4 *= damping;
    v5 *= damping;
    v6 *= damping;
    v7 *= damping;
    v8 *= damping;
    v9 *= damping;
}

//! \brief Initializsation of the absorbing coefficient matrix
/*!
 *
 \param dist Distribution of the wavefield
 \param ctx Context
 \param NX Total number of grid points in X
 \param NY Total number of grid points in Y
 \param NZ Total number of grid points in Z
 \param BoundaryWidth Width of damping boundary
 \param DampingCoeff Damping coefficient
 \param useFreeSurface Bool if free surface is in use
 */
template <typename ValueType>
void KITGPI::ForwardSolver::BoundaryCondition::ABS3D<ValueType>::init(dmemo::DistributionPtr dist, hmemo::ContextPtr ctx, IndexType NX, IndexType NY, IndexType NZ, IndexType BoundaryWidth, ValueType DampingCoeff, bool useFreeSurface)
{

    HOST_PRINT(dist->getCommunicatorPtr(), "Initialization of the Damping Boundary...\n");

    active = true;

    dmemo::CommunicatorPtr comm = dist->getCommunicatorPtr();

    /* Get local "global" indices */
    hmemo::HArray<IndexType> localIndices;
    dist->getOwnedIndexes(localIndices);

    IndexType numLocalIndices = localIndices.size(); // Number of local indices

    hmemo::ReadAccess<IndexType> read_localIndices(localIndices); // Get read access to localIndices
    IndexType read_localIndices_temp;                             // Temporary storage, so we do not have to access the array

    /* Distributed vectors */
    damping.allocate(dist); // Vector to set elements on surface to zero
    damping = 1.0;

    /* Get write access to local part of setSurfaceZero */
    hmemo::HArray<ValueType> *damping_LA = &damping.getLocalValues();
    hmemo::WriteAccess<ValueType> write_damping(*damping_LA);

    // calculate damping function
    ValueType amp = 0;
    ValueType coeff[BoundaryWidth];
    ValueType a = 0;

    amp = 1.0 - DampingCoeff / 100.0;
    a = sqrt(-log(amp) / ((BoundaryWidth) * (BoundaryWidth)));

    for (IndexType j = 0; j < BoundaryWidth; j++) {
        coeff[j] = exp(-(a * a * (BoundaryWidth - j) * (BoundaryWidth - j)));
    }

    Acquisition::Coordinates coordTransform;
    SCAI_ASSERT_DEBUG(coordTransform.index2coordinate(2, 100, 100, 100).x == 2, "")
    SCAI_ASSERT_DEBUG(coordTransform.index2coordinate(102, 100, 100, 100).y == 1, "")
    SCAI_ASSERT_DEBUG(coordTransform.index2coordinate(2, 100, 100, 1).z == 0, "")

    Acquisition::coordinate3D coordinate;
    Acquisition::coordinate3D coordinatedist;

    IndexType coordinateMin = 0;
    IndexType coordinatexzMin = 0;

    /* Set the values into the indice arrays and the value array */
    for (IndexType i = 0; i < numLocalIndices; i++) {

        read_localIndices_temp = read_localIndices[i];

        coordinate = coordTransform.index2coordinate(read_localIndices_temp, NX, NY, NZ);
        coordinatedist = coordTransform.edgeDistance(coordinate, NX, NY, NZ);

        coordinateMin = coordinatedist.min();
        if (coordinateMin < BoundaryWidth) {
            write_damping[i] = coeff[coordinateMin];
        }

        if (useFreeSurface) {
            coordinatexzMin = !((coordinatedist.x) < (coordinatedist.z)) ? (coordinatedist.z) : (coordinatedist.x);
            if (coordinate.y < BoundaryWidth) {
                write_damping[i] = 1.0;

                if ((coordinatedist.z < BoundaryWidth) || (coordinatedist.x < BoundaryWidth)) {
                    write_damping[i] = coeff[coordinatexzMin];
                }
            }
        }
    }

    /* Release all read and write access */
    read_localIndices.release();
    write_damping.release();

    damping.setContextPtr(ctx);

    HOST_PRINT(dist->getCommunicatorPtr(), "Finished with initialization of the Damping Boundary!\n\n");
}

template class KITGPI::ForwardSolver::BoundaryCondition::ABS3D<float>;
template class KITGPI::ForwardSolver::BoundaryCondition::ABS3D<double>;
