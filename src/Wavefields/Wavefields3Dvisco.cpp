#include "Wavefields3Dvisco.hpp"

using namespace scai;

/*! \brief Returns hmemo::ContextPtr from this wavefields
 */
template <typename ValueType>
scai::hmemo::ContextPtr KITGPI::Wavefields::FD3Dvisco<ValueType>::getContextPtr()
{
    return (VX.getContextPtr());
}

/*! \brief Constructor which will set context, allocate and set the wavefields to zero.
 *
 * Initialisation of 3D viscoelastic wavefields
 *
 /param ctx Context
 /param dist Distribution
 */
template <typename ValueType>
KITGPI::Wavefields::FD3Dvisco<ValueType>::FD3Dvisco(scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist)
{
    init(ctx, dist);
}

template <typename ValueType>
void KITGPI::Wavefields::FD3Dvisco<ValueType>::init(scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist)
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
    this->initWavefield(Rxx, ctx, dist);
    this->initWavefield(Ryy, ctx, dist);
    this->initWavefield(Rzz, ctx, dist);
    this->initWavefield(Ryz, ctx, dist);
    this->initWavefield(Rxz, ctx, dist);
    this->initWavefield(Rxy, ctx, dist);
}

/*! \brief Set all wavefields to zero.
 */
template <typename ValueType>
void KITGPI::Wavefields::FD3Dvisco<ValueType>::reset()
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
    this->resetWavefield(Rxx);
    this->resetWavefield(Ryy);
    this->resetWavefield(Rzz);
    this->resetWavefield(Ryz);
    this->resetWavefield(Rxz);
    this->resetWavefield(Rxy);
}

//! \brief Not valid in the 3D visco-elastic case
template <typename ValueType>
scai::lama::DenseVector<ValueType> &KITGPI::Wavefields::FD3Dvisco<ValueType>::getP()
{
    COMMON_THROWEXCEPTION("There is no p wavefield in the 3D visco-elastic case.")
    return (P);
}

template class KITGPI::Wavefields::FD3Dvisco<float>;
template class KITGPI::Wavefields::FD3Dvisco<double>;
