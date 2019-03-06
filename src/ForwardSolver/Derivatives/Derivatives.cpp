#include "Derivatives.hpp"
using namespace scai;

//! \brief Calculate Dxf matrix
/*!
 *
 \param dist Distribution
 */
template <typename ValueType>
void KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::calcDxf(scai::dmemo::DistributionPtr dist)
{
    // Attention: keep in mind topology NZ x NY x NX

    common::Stencil1D<ValueType> stencilId(1);
    common::Stencil3D<ValueType> stencil(stencilId, stencilId, stencilFD);
    // use dist for distribution
    Dxf.define(dist, stencil);
}
//! \brief Calculate Dxf sparse matrix
/*!
 *
 \param modelCoordinates Coordinate class, which eg. maps 3D coordinates to 1D model indices
 \param dist Distribution
 */
template <typename ValueType>
void KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::calcDxf(Acquisition::Coordinates<ValueType> const &modelCoordinates, scai::dmemo::DistributionPtr dist)
{
    hmemo::HArray<IndexType> ownedIndexes; // all (global) points owned by this process
    dist->getOwnedIndexes(ownedIndexes);

    lama::MatrixAssembly<ValueType> assembly;
    assembly.reserve(ownedIndexes.size() * spatialFDorder);
    IndexType X = 0;
    IndexType columnIndex = 0;
    IndexType j = 0;

    for (IndexType ownedIndex : hmemo::hostReadAccess(ownedIndexes)) {

        Acquisition::coordinate3D coordinate = modelCoordinates.index2coordinate(ownedIndex);
        if ((!modelCoordinates.locatedOnInterface(coordinate)) || (coordinate.x % modelCoordinates.getDHFactor(coordinate) == 0)) {
            for (j = 0; j < spatialFDorder; j++) {

                X = coordinate.x + modelCoordinates.getDHFactor(coordinate) * (j - spatialFDorder / 2 + 1);

                if ((X >= 0) && (X < modelCoordinates.getNX())) {
                    columnIndex = modelCoordinates.coordinate2index(X, coordinate.y, coordinate.z);
                    assembly.push(ownedIndex, columnIndex, stencilFD.values()[j] / modelCoordinates.getDH(coordinate));
                }
            }
        }
    }

    DxfSparse = lama::zero<SparseFormat>(dist, dist);
    DxfSparse.fillFromAssembly(assembly);
}

//! \brief Calculate Dyf matrix
/*!
 *
 \param dist Distribution
 */
template <typename ValueType>
void KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::calcDyf(scai::dmemo::DistributionPtr dist)
{
    common::Stencil1D<ValueType> stencilId(1);
    common::Stencil3D<ValueType> stencil(stencilFD, stencilId, stencilId);
    // use dist for distribution
    Dyf.define(dist, stencil);
}

//! \brief Calculate Dyf sparse matrix
/*!
 *
 \param modelCoordinates Coordinate class, which eg. maps 3D coordinates to 1D model indices
 \param dist Distribution
 */
template <typename ValueType>
void KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::calcDyf(Acquisition::Coordinates<ValueType> const &modelCoordinates, scai::dmemo::DistributionPtr dist)
{
    hmemo::HArray<IndexType> ownedIndexes; // all (global) points owned by this process
    dist->getOwnedIndexes(ownedIndexes);

    lama::MatrixAssembly<ValueType> assembly;
    assembly.reserve(ownedIndexes.size() * spatialFDorder);
    IndexType Y = 0;
    IndexType columnIndex = 0;
    IndexType j = 0;
    ValueType DH = 0;
    IndexType dhFactor = 0;
    IndexType layer = 0;

    for (IndexType ownedIndex : hmemo::hostReadAccess(ownedIndexes)) {

        Acquisition::coordinate3D coordinate = modelCoordinates.index2coordinate(ownedIndex);
        layer = modelCoordinates.getLayer(coordinate);

        //Transition from coarse (layer) to fine grid (layer+1) uses fine operator for Dyf
        if ((modelCoordinates.locatedOnInterface(coordinate)) && (!modelCoordinates.getTransition(coordinate))) {
            dhFactor = modelCoordinates.getDHFactor(layer + 1);
            DH = modelCoordinates.getDH(layer + 1);
        } else {
            dhFactor = modelCoordinates.getDHFactor(layer);
            DH = modelCoordinates.getDH(layer);
        }

        if ((!modelCoordinates.locatedOnInterface(coordinate)) || (coordinate.x % dhFactor == 0)) {
            for (j = 0; j < spatialFDorder; j++) {
                Y = coordinate.y + dhFactor * (j - spatialFDorder / 2 + 1);

                if ((Y >= 0) && (Y < modelCoordinates.getNY())) {
                    columnIndex = modelCoordinates.coordinate2index(coordinate.x, Y, coordinate.z);
                    assembly.push(ownedIndex, columnIndex, stencilFD.values()[j] / DH);
                }
            }
        }
    }

    DyfSparse = lama::zero<SparseFormat>(dist, dist);
    DyfSparse.fillFromAssembly(assembly);
}

//! \brief Calculate Dzf matrix
/*!
 *
 \param dist Distribution
 */
template <typename ValueType>
void KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::calcDzf(scai::dmemo::DistributionPtr dist)
{
    common::Stencil1D<ValueType> stencilId(1);
    common::Stencil3D<ValueType> stencil(stencilId, stencilFD, stencilId);
    // use dist for distribution
    Dzf.define(dist, stencil);
}

//! \brief Calculate Dzf sparse matrix
/*!
 *
\param modelCoordinates Coordinate class, which eg. maps 3D coordinates to 1D model indices
 \param dist Distribution
 */
template <typename ValueType>
void KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::calcDzf(Acquisition::Coordinates<ValueType> const &modelCoordinates, scai::dmemo::DistributionPtr dist)
{
    hmemo::HArray<IndexType> ownedIndexes; // all (global) points owned by this process
    dist->getOwnedIndexes(ownedIndexes);

    lama::MatrixAssembly<ValueType> assembly;
    assembly.reserve(ownedIndexes.size() * spatialFDorder);
    IndexType Z = 0;
    IndexType columnIndex = 0;
    IndexType j = 0;

    for (IndexType ownedIndex : hmemo::hostReadAccess(ownedIndexes)) {

        Acquisition::coordinate3D coordinate = modelCoordinates.index2coordinate(ownedIndex);

        for (j = 0; j < spatialFDorder; j++) {

            Z = coordinate.z + modelCoordinates.getDHFactor(coordinate) * (j - spatialFDorder / 2 + 1);

            if ((Z >= 0) && (Z < modelCoordinates.getNZ())) {
                columnIndex = modelCoordinates.coordinate2index(coordinate.x, coordinate.y, Z);
                assembly.push(ownedIndex, columnIndex, stencilFD.values()[j] / modelCoordinates.getDH(coordinate));
            }
        }
    }
    DzfSparse = lama::zero<SparseFormat>(dist, dist);
    DzfSparse.fillFromAssembly(assembly);
}
//! \brief Calculate DyfFreeSurface matrix
/*!
 *
 \param modelCoordinates Coordinate class, which eg. maps 3D coordinates to 1D model indices
 \param dist Distribution
 */
template <typename ValueType>
void KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::calcDyfFreeSurface(Acquisition::Coordinates<ValueType> const &modelCoordinates, scai::dmemo::DistributionPtr dist)
{
    hmemo::HArray<IndexType> ownedIndexes; // all (global) points owned by this process
    dist->getOwnedIndexes(ownedIndexes);

    lama::MatrixAssembly<ValueType> assembly;
    assembly.reserve(ownedIndexes.size() * spatialFDorder);
    IndexType Y = 0;
    ValueType fdCoeff = 0;

    IndexType ImageIndex = 0;
    IndexType columnIndex = 0;
    for (IndexType ownedIndex : hmemo::hostReadAccess(ownedIndexes)) {

        Acquisition::coordinate3D coordinate = modelCoordinates.index2coordinate(ownedIndex);

        for (IndexType j = 0; j < spatialFDorder; j++) {
            Y = coordinate.y + modelCoordinates.getDHFactor(coordinate) * (j - spatialFDorder / 2 + 1);
            fdCoeff = stencilFD.values()[j];

            ImageIndex = spatialFDorder - 2 - 2 * coordinate.y / modelCoordinates.getDHFactor(coordinate) - j;
            if (ImageIndex >= 0)
                fdCoeff -= stencilFD.values()[ImageIndex];

            if ((Y >= 0) && (Y < modelCoordinates.getNY())) {
                columnIndex = modelCoordinates.coordinate2index(coordinate.x, Y, coordinate.z);
                assembly.push(ownedIndex, columnIndex, fdCoeff / modelCoordinates.getDH(coordinate));
            }
        }
    }

    DyfFreeSurface = lama::zero<SparseFormat>(dist, dist);
    DyfFreeSurface.fillFromAssembly(assembly);
}

//! \brief Calculate DybFreeSurface matrix
/*!
  \param modelCoordinates Coordinate class, which eg. maps 3D coordinates to 1D model indices
 \param dist Distribution
 */
template <typename ValueType>
void KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::calcDybFreeSurface(Acquisition::Coordinates<ValueType> const &modelCoordinates, scai::dmemo::DistributionPtr dist)
{

    hmemo::HArray<IndexType> ownedIndexes; // all (global) points owned by this process
    dist->getOwnedIndexes(ownedIndexes);

    lama::MatrixAssembly<ValueType> assembly;
    assembly.reserve(ownedIndexes.size() * spatialFDorder);
    IndexType Y = 0;
    ValueType fdCoeff = 0;
    IndexType ImageIndex = 0;
    IndexType columnIndex = 0;
    IndexType j = 0;

    for (IndexType ownedIndex : hmemo::hostReadAccess(ownedIndexes)) {

        Acquisition::coordinate3D coordinate = modelCoordinates.index2coordinate(ownedIndex);

        for (j = 0; j < spatialFDorder; j++) {
            Y = coordinate.y + modelCoordinates.getDHFactor(coordinate) * (j - spatialFDorder / 2);

            fdCoeff = stencilFD.values()[j];

            ImageIndex = spatialFDorder - 1 - 2 * coordinate.y / modelCoordinates.getDHFactor(coordinate) - j;

            if (ImageIndex >= 0)
                fdCoeff -= stencilFD.values()[ImageIndex];

            if ((Y >= 0) && (Y < modelCoordinates.getNY())) {
                columnIndex = modelCoordinates.coordinate2index(coordinate.x, Y, coordinate.z);
                assembly.push(ownedIndex, columnIndex, fdCoeff / modelCoordinates.getDH(coordinate));
            }
        }
    }

    DybFreeSurface = lama::zero<SparseFormat>(dist, dist);
    DybFreeSurface.fillFromAssembly(assembly);
}

//! \brief Calculate Dxb sparse matrix
/*!
 *
 \param modelCoordinates Coordinate class, which eg. maps 3D coordinates to 1D model indices
 \param dist Distribution
 */
template <typename ValueType>
void KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::calcDxb(Acquisition::Coordinates<ValueType> const &modelCoordinates, scai::dmemo::DistributionPtr dist)
{
    hmemo::HArray<IndexType> ownedIndexes; // all (global) points owned by this process
    dist->getOwnedIndexes(ownedIndexes);

    lama::MatrixAssembly<ValueType> assembly;
    assembly.reserve(ownedIndexes.size() * spatialFDorder);
    IndexType X = 0;
    IndexType columnIndex = 0;
    IndexType j = 0;

    for (IndexType ownedIndex : hmemo::hostReadAccess(ownedIndexes)) {

        Acquisition::coordinate3D coordinate = modelCoordinates.index2coordinate(ownedIndex);
        if ((!modelCoordinates.locatedOnInterface(coordinate)) || (coordinate.x % modelCoordinates.getDHFactor(coordinate) == 0)) {
            for (j = 0; j < spatialFDorder; j++) {

                X = coordinate.x + modelCoordinates.getDHFactor(coordinate) * (j - spatialFDorder / 2);

                if ((X >= 0) && (X < modelCoordinates.getNX())) {
                    columnIndex = modelCoordinates.coordinate2index(X, coordinate.y, coordinate.z);
                    assembly.push(ownedIndex, columnIndex, stencilFD.values()[j] / modelCoordinates.getDH(coordinate));
                }
            }
        }
    }

    DxbSparse = lama::zero<SparseFormat>(dist, dist);
    DxbSparse.fillFromAssembly(assembly);
}

//! \brief Calculate Dyb matrix
/*!
  \param modelCoordinates Coordinate class, which eg. maps 3D coordinates to 1D model indices
 \param dist Distribution
 */
template <typename ValueType>
void KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::calcDyb(Acquisition::Coordinates<ValueType> const &modelCoordinates, scai::dmemo::DistributionPtr dist)
{

    hmemo::HArray<IndexType> ownedIndexes; // all (global) points owned by this process
    dist->getOwnedIndexes(ownedIndexes);

    lama::MatrixAssembly<ValueType> assembly;
    assembly.reserve(ownedIndexes.size() * spatialFDorder);
    IndexType Y = 0;
    IndexType columnIndex = 0;
    IndexType j = 0;

    IndexType layer;
    IndexType dhFactor;

    for (IndexType ownedIndex : hmemo::hostReadAccess(ownedIndexes)) {

        Acquisition::coordinate3D coordinate = modelCoordinates.index2coordinate(ownedIndex);
        layer = modelCoordinates.getLayer(coordinate);
        dhFactor = modelCoordinates.getDHFactor(layer);

        if ((!modelCoordinates.locatedOnInterface(coordinate)) || (coordinate.x % dhFactor == 0)) {
            for (j = 0; j < spatialFDorder; j++) {

                Y = coordinate.y + dhFactor * (j - spatialFDorder / 2);

                // apply coordinate correction in the fine staggered grid (coordinates are only correct for full grid points)
                if ((modelCoordinates.locatedOnInterface(coordinate)) && (j != modelCoordinates.getTransition(coordinate))) {
                    // fG<->cG transotion=1 -> layer-1, cG<->fG transotion=0  -> layer+1
                    Y += modelCoordinates.getDHFactor(layer + 1 - 2 * modelCoordinates.getTransition(coordinate));
                }

                if ((Y >= 0) && (Y < modelCoordinates.getNY())) {
                    columnIndex = modelCoordinates.coordinate2index(coordinate.x, Y, coordinate.z);
                    assembly.push(ownedIndex, columnIndex, stencilFD.values()[j] / modelCoordinates.getDH(coordinate));
                }
            }
        }
    }

    DybSparse = lama::zero<SparseFormat>(dist, dist);
    DybSparse.fillFromAssembly(assembly);
}

//! \brief Calculate Dzb sparse matrix
/*!
 *
\param modelCoordinates Coordinate class, which eg. maps 3D coordinates to 1D model indices
 \param dist Distribution
 */
template <typename ValueType>
void KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::calcDzb(Acquisition::Coordinates<ValueType> const &modelCoordinates, scai::dmemo::DistributionPtr dist)
{
    hmemo::HArray<IndexType> ownedIndexes; // all (global) points owned by this process
    dist->getOwnedIndexes(ownedIndexes);

    lama::MatrixAssembly<ValueType> assembly;
    assembly.reserve(ownedIndexes.size() * spatialFDorder);
    IndexType Z = 0;
    IndexType columnIndex = 0;
    IndexType j = 0;

    for (IndexType ownedIndex : hmemo::hostReadAccess(ownedIndexes)) {

        Acquisition::coordinate3D coordinate = modelCoordinates.index2coordinate(ownedIndex);

        for (j = 0; j < spatialFDorder; j++) {

            Z = coordinate.z + modelCoordinates.getDHFactor(coordinate) * (j - spatialFDorder / 2);

            if ((Z >= 0) && (Z < modelCoordinates.getNZ())) {
                columnIndex = modelCoordinates.coordinate2index(coordinate.x, coordinate.y, Z);
                assembly.push(ownedIndex, columnIndex, stencilFD.values()[j] / modelCoordinates.getDH(coordinate));
            }
        }
    }
    DzbSparse = lama::zero<SparseFormat>(dist, dist);
    DzbSparse.fillFromAssembly(assembly);
}

template <typename ValueType>
void KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::calcInterpolationP(Acquisition::Coordinates<ValueType> const &modelCoordinates, scai::dmemo::DistributionPtr dist)
{
    hmemo::HArray<IndexType> ownedIndexes; // all (global) points owned by this process

    dist->getOwnedIndexes(ownedIndexes);

    lama::MatrixAssembly<ValueType> assembly;
    assembly.reserve(ownedIndexes.size() * 2);

    ValueType third = ValueType(1) / ValueType(3);

    for (IndexType ownedIndex : hmemo::hostReadAccess(ownedIndexes)) {
        Acquisition::coordinate3D coordinate = modelCoordinates.index2coordinate(ownedIndex);

        const IndexType &x = coordinate.x;
        const IndexType &y = coordinate.y;
        const IndexType &z = coordinate.z;

        if (!modelCoordinates.locatedOnInterface(coordinate) || (x % 3 == 0)) {
            assembly.push(ownedIndex, ownedIndex, ValueType(1));
        } else if ((x % 3 == 1) && (x < modelCoordinates.getNX() - 2)) {
            IndexType leftIndex = modelCoordinates.coordinate2index(x - 1, y, z);
            IndexType rightIndex = modelCoordinates.coordinate2index(x + 2, y, z);
            assembly.push(ownedIndex, leftIndex, 2 * third);
            assembly.push(ownedIndex, rightIndex, 1 * third);
        } else if (x < modelCoordinates.getNX() - 1) {
            IndexType leftIndex = modelCoordinates.coordinate2index(x - 2, y, z);
            IndexType rightIndex = modelCoordinates.coordinate2index(x + 1, y, z);
            assembly.push(ownedIndex, leftIndex, 1 * third);
            assembly.push(ownedIndex, rightIndex, 2 * third);
        }
    }

    // auto colDist = std::make_shared<dmemo::NoDistribution( dist.getGlobalSize() );
    InterpolationP = lama::zero<SparseFormat>(dist, dist);

    InterpolationP.fillFromAssembly(assembly);

    std::cout << "InterpolationP computed: " << std::endl;
}

//! \brief Getter method for the spatial FD-order
template <typename ValueType>
IndexType KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::getSpatialFDorder() const
{
    return (spatialFDorder);
}

//! \brief Getter method for derivative matrix DybFreeSurface
template <typename ValueType>
scai::lama::Matrix<ValueType> const &KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::getDybFreeSurface() const
{
    return (DybFreeSurface);
}

//! \brief Getter method for derivative matrix DyfFreeSurface
template <typename ValueType>
scai::lama::Matrix<ValueType> const &KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::getDyfFreeSurface() const
{
    return (DyfFreeSurface);
}

//! \brief Getter method for derivative matrix Dxf
template <typename ValueType>
scai::lama::Matrix<ValueType> const &KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::getDxf() const
{
    if (useSparse)
        return (DxfSparse);
    else
        return (Dxf);
}

//! \brief Getter method for derivative matrix Dyf
template <typename ValueType>
scai::lama::Matrix<ValueType> const &KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::getDyf() const
{
    if (useSparse)
        return (DyfSparse);
    else
        return (Dyf);
}

//! \brief Getter method for derivative matrix Dzf
template <typename ValueType>
scai::lama::Matrix<ValueType> const &KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::getDzf() const
{
    if (useSparse)
        return (DzfSparse);
    else
        return (Dzf);
}

//! \brief Getter method for derivative matrix Dxb
template <typename ValueType>
scai::lama::Matrix<ValueType> const &KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::getDxb() const
{
    if (useSparse)
        return (DxbSparse);
    else
        return (Dxb);
}

//! \brief Getter method for derivative matrix Dyb
template <typename ValueType>
scai::lama::Matrix<ValueType> const &KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::getDyb() const
{
    if (useSparse)
        return (DybSparse);
    else
        return (Dyb);
}

//! \brief Getter method for derivative matrix Dzb
template <typename ValueType>
scai::lama::Matrix<ValueType> const &KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::getDzb() const
{
    if (useSparse)
        return (DzbSparse);
    else
        return (Dzb);
}

//! \brief Getter method for derivative interpolation matrix of P
template <typename ValueType>
scai::lama::Matrix<ValueType> const *KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::getInterP() const
{
    if (InterpolationP.getNumRows() > 0) {
        return &InterpolationP;
    } else {
        return NULL;
    }
}

//! \brief Set FD coefficients for each order
/*!
 *
 \param spFDo Order of spatial FD-coefficient
 */
template <typename ValueType>
void KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType>::setFDCoef(IndexType spFDo)
{

    const ValueType FD2[] = {-1.0, 1.0};

    const ValueType FD4[] = {1.0 / 24.0, -9.0 / 8.0, 9.0 / 8.0, -1.0 / 24.0};

    const ValueType FD6[] = {-3.0 / 640.0, 25.0 / 384.0, -75.0 / 64.0,
                             75.0 / 64.0, -25.0 / 384.0, 3.0 / 640.0};

    const ValueType FD8[] = {5.0 / 7168.0, -49.0 / 5120.0, 245.0 / 3072.0, -1225.0 / 1024.0,
                             1225.0 / 1024.0, -245.0 / 3072.0, 49.0 / 5120.0, -5.0 / 7168.0};

    const ValueType FD10[] = {-8756999275442633.0 / 73786976294838206464.0,
                              8142668969129685.0 / 4611686018427387904.0,
                              -567.0 / 40960.0,
                              735.0 / 8192.0,
                              -19845.0 / 16384.0,
                              19845.0 / 16384.0,
                              -735.0 / 8192.0,
                              567.0 / 40960.0,
                              -8142668969129685.0 / 4611686018427387904.0,
                              8756999275442633.0 / 73786976294838206464.0};

    const ValueType FD12[] = {6448335830095439.0 / 295147905179352825856.0,
                              -1655620175512543.0 / 4611686018427387904.0,
                              6842103786556949.0 / 2305843009213693952.0,
                              -628618285389933.0 / 36028797018963968.0,
                              436540475965291.0 / 4503599627370496.0,
                              -2750204998582123.0 / 2251799813685248.0,
                              2750204998582123.0 / 2251799813685248.0,
                              -436540475965291.0 / 4503599627370496.0,
                              628618285389933.0 / 36028797018963968.0,
                              -6842103786556949.0 / 2305843009213693952.0,
                              1655620175512543.0 / 4611686018427387904.0,
                              -6448335830095439.0 / 295147905179352825856.0};

    switch (spFDo) {
    case 2:
        stencilFD = common::Stencil1D<ValueType>(2, FD2);
        break;
    case 4:
        stencilFD = common::Stencil1D<ValueType>(4, FD4);
        break;
    case 6:
        stencilFD = common::Stencil1D<ValueType>(6, FD6);
        break;
    case 8:
        stencilFD = common::Stencil1D<ValueType>(8, FD8);
        break;
    case 10:
        stencilFD = common::Stencil1D<ValueType>(10, FD10);
        break;
    case 12:
        stencilFD = common::Stencil1D<ValueType>(12, FD12);
        break;
    default:
        COMMON_THROWEXCEPTION(" Unkown spatialFDorder value.");
        break;
    }
}

template class KITGPI::ForwardSolver::Derivatives::Derivatives<float>;
template class KITGPI::ForwardSolver::Derivatives::Derivatives<double>;
