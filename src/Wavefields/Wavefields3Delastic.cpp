#include "Wavefields3Delastic.hpp"

using namespace scai;

/*! \brief Returns hmemo::ContextPtr from this wavefields
 */
template <typename ValueType>
hmemo::ContextPtr KITGPI::Wavefields::FD3Delastic<ValueType>::getContextPtr()
{
    return (VX.getContextPtr());
}

/*! \brief Constructor which will set context, allocate and set the wavefields to zero.
 *
 * Initialisation of 3D elastic wavefields
 *
 \param ctx Context
 \param dist Distribution
 */
template <typename ValueType>
KITGPI::Wavefields::FD3Delastic<ValueType>::FD3Delastic(scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist)
{
    init(ctx, dist);
}

template <typename ValueType>
void KITGPI::Wavefields::FD3Delastic<ValueType>::init(scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist)
{
    this->initWavefield(VX, ctx, dist);
    this->initWavefield(VY, ctx, dist);
    this->initWavefield(VZ, ctx, dist);
    this->initWavefield(Sxx, ctx, dist);
    this->initWavefield(Syy, ctx, dist);
    this->initWavefield(Szz, ctx, dist);
    this->initWavefield(Syz, ctx, dist);
    this->initWavefield(Sxz, ctx, dist);
    this->initWavefield(Sxy, ctx, dist);
}

/*! \brief override Methode tor write Wavefield Snapshot to file
 *
 *
 \param type Type of the Seismogram
 \param t Current Timestep
 */
template <typename ValueType>
void KITGPI::Wavefields::FD3Delastic<ValueType>::write(std::string baseName,std::string type, IndexType t, IndexType partitionedOut)
{
    std::string fileBaseName = baseName + type;
    this->writeWavefield(VX, "VX", fileBaseName, t, partitionedOut);
    this->writeWavefield(VY, "VY", fileBaseName, t, partitionedOut);
    this->writeWavefield(VZ, "VZ", fileBaseName, t, partitionedOut);
    this->writeWavefield(Sxx, "Sxx", fileBaseName, t, partitionedOut);
    this->writeWavefield(Syy, "Syy", fileBaseName, t, partitionedOut);
    this->writeWavefield(Szz, "Szz", fileBaseName, t, partitionedOut);
    this->writeWavefield(Sxy, "Sxy", fileBaseName, t, partitionedOut);
    this->writeWavefield(Sxz, "Sxz", fileBaseName, t, partitionedOut);
    this->writeWavefield(Syz, "Syz", fileBaseName, t, partitionedOut);
}

/*! \brief Wrapper Function to Write Snapshot of the Wavefield
 *
 *
 \param t Current Timestep
 */
template <typename ValueType>
void KITGPI::Wavefields::FD3Delastic<ValueType>::writeSnapshot(std::string baseName,IndexType t, IndexType partitionedOut)
{
    write(baseName, type, t, partitionedOut);
}

/*! \brief Set all wavefields to zero.
 */
template <typename ValueType>
void KITGPI::Wavefields::FD3Delastic<ValueType>::resetWavefields()
{
    this->resetWavefield(VX);
    this->resetWavefield(VY);
    this->resetWavefield(VZ);
    this->resetWavefield(Sxx);
    this->resetWavefield(Syy);
    this->resetWavefield(Szz);
    this->resetWavefield(Syz);
    this->resetWavefield(Sxz);
    this->resetWavefield(Sxy);
}

//! \brief Not valid in the 3D elastic case
template <typename ValueType>
scai::lama::DenseVector<ValueType> &KITGPI::Wavefields::FD3Delastic<ValueType>::getRefP()
{
    COMMON_THROWEXCEPTION("There is no p wavefield in the 3D elastic case.")
    return (P);
}

//! \brief Not valid in the 3D elastic case
template <typename ValueType>
scai::lama::DenseVector<ValueType> &KITGPI::Wavefields::FD3Delastic<ValueType>::getRefRxx()
{
    COMMON_THROWEXCEPTION("There is no Rxx wavefield in the 3D elastic case.")
    return (Rxx);
}

//! \brief Not valid in the 3D elastic case
template <typename ValueType>
scai::lama::DenseVector<ValueType> &KITGPI::Wavefields::FD3Delastic<ValueType>::getRefRyy()
{
    COMMON_THROWEXCEPTION("There is no Ryy wavefield in the 3D elastic case.")
    return (Ryy);
}

//! \brief Not valid in the 3D elastic case
template <typename ValueType>
scai::lama::DenseVector<ValueType> &KITGPI::Wavefields::FD3Delastic<ValueType>::getRefRzz()
{
    COMMON_THROWEXCEPTION("There is no Rzz wavefield in the 3D elastic case.")
    return (Rzz);
}

//! \brief Not valid in the 3D elastic case
template <typename ValueType>
scai::lama::DenseVector<ValueType> &KITGPI::Wavefields::FD3Delastic<ValueType>::getRefRyz()
{
    COMMON_THROWEXCEPTION("There is no Ryz wavefield in the 3D elastic case.")
    return (Ryz);
}

//! \brief Not valid in the 3D elastic case
template <typename ValueType>
scai::lama::DenseVector<ValueType> &KITGPI::Wavefields::FD3Delastic<ValueType>::getRefRxz()
{
    COMMON_THROWEXCEPTION("There is no Rxz wavefield in the 3D elastic case.")
    return (Rxz);
}

//! \brief Not valid in the 3D elastic case
template <typename ValueType>
scai::lama::DenseVector<ValueType> &KITGPI::Wavefields::FD3Delastic<ValueType>::getRefRxy()
{
    COMMON_THROWEXCEPTION("There is no Rxy wavefield in the 3D elastic case.")
    return (Rxy);
}

/*! \brief Overloading * Operation
 *
 \param rhs Scalar factor with which the vectors are multiplied.
 */
template <typename ValueType>
KITGPI::Wavefields::FD3Delastic<ValueType> KITGPI::Wavefields::FD3Delastic<ValueType>::operator*(ValueType rhs)
{
    KITGPI::Wavefields::FD3Delastic<ValueType> result;
    result.VX = this->VX * rhs;
    result.VY = this->VY * rhs;
    result.VZ = this->VZ * rhs;
    result.P = this->P * rhs;
    result.Sxx = this->Sxx * rhs;
    result.Syy = this->Syy * rhs;
    result.Szz = this->Szz * rhs;
    result.Sxy = this->Sxy * rhs;
    result.Sxz = this->Sxz * rhs;
    result.Syz = this->Syz * rhs;
    return result;
}

/*! \brief free function to multiply
 *
 \param lhs Scalar factor with which the vectors are multiplied.
 \param rhs Vector
 */
template <typename ValueType>
KITGPI::Wavefields::FD3Delastic<ValueType> operator*(ValueType lhs, KITGPI::Wavefields::FD3Delastic<ValueType> rhs)
{
    return rhs * lhs;
}

/*! \brief Overloading *= Operation
 *
 \param rhs Scalar factor with which the vectors are multiplied.
 */
template <typename ValueType>
KITGPI::Wavefields::FD3Delastic<ValueType> KITGPI::Wavefields::FD3Delastic<ValueType>::operator*=(ValueType rhs)
{
    return rhs * *this;
}

/*! \brief Overloading * Operation
 *
 \param rhs seperate Wavefield whith which the components of the current wavefield are multiplied.
 */
template <typename ValueType>
KITGPI::Wavefields::FD3Delastic<ValueType> KITGPI::Wavefields::FD3Delastic<ValueType>::operator*(KITGPI::Wavefields::FD3Delastic<ValueType> rhs)
{
    KITGPI::Wavefields::FD3Delastic<ValueType> result;
    result.VX = this->VX * rhs.VX;
    result.VY = this->VY * rhs.VY;
    result.VZ = this->VZ * rhs.VZ;
    result.P = this->P * rhs.P;
    result.Sxx = this->Sxx * rhs.Sxx;
    result.Syy = this->Syy * rhs.Syy;
    result.Szz = this->Szz * rhs.Szz;
    result.Sxy = this->Sxy * rhs.Sxy;
    result.Sxz = this->Sxz * rhs.Sxz;
    result.Syz = this->Syz * rhs.Syz;
    return result;
}

/*! \brief Overloading *= Operation
 *
 \param rhs seperate Wavefield whith which the components of the current wavefield are multiplied.
 */
template <typename ValueType>
KITGPI::Wavefields::FD3Delastic<ValueType> KITGPI::Wavefields::FD3Delastic<ValueType>::operator*=(KITGPI::Wavefields::FD3Delastic<ValueType> rhs)
{
    return rhs * *this;
}

/*! \brief function for overloading -= Operation (called in base class)
 *
 \param rhs Abstract wavefield which is assigned.
 */
template <typename ValueType>
void KITGPI::Wavefields::FD3Delastic<ValueType>::assign(KITGPI::Wavefields::Wavefields<ValueType> &rhs)
{
    VX = rhs.getRefVX();
    VY = rhs.getRefVY();
    VZ = rhs.getRefVZ();
    Sxx = rhs.getRefSxx();
    Syy = rhs.getRefSyy();
    Szz = rhs.getRefSzz();
    Sxy = rhs.getRefSxy();
    Sxz = rhs.getRefSxz();
    Syz = rhs.getRefSyz();
}

/*! \brief function for overloading -= Operation (called in base class)
 *
 \param rhs Abstract wavefield which is substracted.
 */
template <typename ValueType>
void KITGPI::Wavefields::FD3Delastic<ValueType>::minusAssign(KITGPI::Wavefields::Wavefields<ValueType> &rhs)
{
    VX -= rhs.getRefVX();
    VY -= rhs.getRefVY();
    VZ -= rhs.getRefVZ();
    Sxx -= rhs.getRefSxx();
    Syy -= rhs.getRefSyy();
    Szz -= rhs.getRefSzz();
    Sxy -= rhs.getRefSxy();
    Sxz -= rhs.getRefSxz();
    Syz -= rhs.getRefSyz();
}

/*! \brief function for overloading += Operation (called in base class)
 *
 \param rhs Abstarct wavefield which is added.
 */
template <typename ValueType>
void KITGPI::Wavefields::FD3Delastic<ValueType>::plusAssign(KITGPI::Wavefields::Wavefields<ValueType> &rhs)
{
    VX += rhs.getRefVX();
    VY += rhs.getRefVY();
    VZ += rhs.getRefVZ();
    Sxx += rhs.getRefSxx();
    Syy += rhs.getRefSyy();
    Szz += rhs.getRefSzz();
    Sxy += rhs.getRefSxy();
    Sxz += rhs.getRefSxz();
    Syz += rhs.getRefSyz();
}

/*! \brief function for overloading *= Operation (called in base class)
 *
 \param rhs Scalar is multiplied.
 */
template <typename ValueType>
void KITGPI::Wavefields::FD3Delastic<ValueType>::timesAssign(ValueType rhs)
{
    VX *= rhs;
    VY *= rhs;
    VZ *= rhs;
    Sxx *= rhs;
    Syy *= rhs;
    Szz *= rhs;
    Sxy *= rhs;
    Sxz *= rhs;
    Syz *= rhs;
}
template class KITGPI::Wavefields::FD3Delastic<float>;
template class KITGPI::Wavefields::FD3Delastic<double>;
