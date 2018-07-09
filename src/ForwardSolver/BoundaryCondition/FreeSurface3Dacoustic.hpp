#pragma once

#include "../../Common/HostPrint.hpp"
#include "../Derivatives/Derivatives.hpp"
#include "FreeSurfaceAcoustic.hpp"

namespace KITGPI
{

    namespace ForwardSolver
    {

        //! \brief BoundaryCondition namespace
        namespace BoundaryCondition
        {

            //! \brief 3-D acoustic free surface
            template <typename ValueType>
            class FreeSurface3Dacoustic : public FreeSurfaceAcoustic<ValueType>
            {
              public:
                //! Default constructor
                FreeSurface3Dacoustic(){};

                //! Default destructor
                ~FreeSurface3Dacoustic(){};
            };
        } /* end namespace BoundaryCondition */
    }     /* end namespace ForwardSolver */
} /* end namespace KITGPI */
