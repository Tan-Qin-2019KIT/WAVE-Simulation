#include "FreeSurface3Dvisco.hpp"
using namespace scai;

/*! \brief Apply free surface condition during time stepping
 *
 * THIS METHOD IS CALLED DURING TIME STEPPING
 * DO NOT WASTE RUNTIME HERE
 *
 \param sumHorizonatlDerivative Sum of horizontal velocity updates
 \param temp DenseVector which is used for temporary storage
 \param Sxx Sxx wavefield (stress)
 \param Syy Syy wavefield (stress)
 \param Szz Szz wavefield (stress)
 \param Rxx Rxx wavefield (relaxation)
 \param Ryy Ryy wavefield (relaxation)
 \param Rzz Rzz wavefield (relaxation)
 */
template <typename ValueType>
void KITGPI::ForwardSolver::BoundaryCondition::FreeSurface3Dvisco<ValueType>::apply(scai::lama::Vector<ValueType> &sumHorizonatlDerivative, scai::lama::Vector<ValueType> &temp, scai::lama::Vector<ValueType> &Sxx, scai::lama::Vector<ValueType> &Syy, scai::lama::Vector<ValueType> &Szz, scai::lama::Vector<ValueType> &Rxx, scai::lama::Vector<ValueType> &Ryy, scai::lama::Vector<ValueType> &Rzz)
{

    SCAI_ASSERT_DEBUG(active, " FreeSurface is not active ");

    /* Update the stress parameter at the free surface */
    temp = sumHorizonatlDerivative;
    temp *= scaleStressHorizontalUpdate;
    Sxx += temp; // Apply horizontal update
    Szz += temp; // Apply horizontal update

    Syy *= setSurfaceZero; // Set the free surface to zero

    /* Update relaxation parameter at the free surface */
    sumHorizonatlDerivative *= scaleRelaxationHorizontalUpdate;
    Rxx += sumHorizonatlDerivative; // Apply horizontal update
    Rzz += sumHorizonatlDerivative; // Apply horizontal update

    Ryy *= setSurfaceZero; // Set the free surface to zero
}

template class KITGPI::ForwardSolver::BoundaryCondition::FreeSurface3Dvisco<float>;
template class KITGPI::ForwardSolver::BoundaryCondition::FreeSurface3Dvisco<double>;
