
#pragma once

#include <scai/dmemo.hpp>
#include <scai/hmemo.hpp>
#include <scai/lama.hpp>

#include <iostream>

#include "ForwardSolver.hpp"

#include "BoundaryCondition/ABS3D.hpp"
#include "BoundaryCondition/CPML3D.hpp"
#include "BoundaryCondition/FreeSurface3Delastic.hpp"
#include "SourceReceiverImpl/FDTD3Delastic.hpp"

namespace KITGPI
{

    namespace ForwardSolver
    {

        //! \brief 3-D elastic forward solver
        template <typename ValueType>
        class FD3Delastic : public ForwardSolver<ValueType>
        {

          public:
            //! Default constructor
            FD3Delastic(){};

            //! Default destructor
            ~FD3Delastic(){};

            void run(Acquisition::AcquisitionGeometry<ValueType> &receiver, const Acquisition::AcquisitionGeometry<ValueType> &sources, const Modelparameter::Modelparameter<ValueType> &model, Wavefields::Wavefields<ValueType> &wavefield, const Derivatives::Derivatives<ValueType> &derivatives, scai::IndexType TStart, scai::IndexType TEnd, ValueType) override;

            void prepareBoundaryConditions(Configuration::Configuration const &config, Derivatives::Derivatives<ValueType> &derivatives, scai::dmemo::DistributionPtr dist, scai::hmemo::ContextPtr ctx) override;

          private:
            /* Boundary Conditions */
            BoundaryCondition::FreeSurface3Delastic<ValueType> FreeSurface; //!< Free Surface boundary condition class
            using ForwardSolver<ValueType>::useFreeSurface;

            BoundaryCondition::ABS3D<ValueType> DampingBoundary; //!< Damping boundary condition class
            using ForwardSolver<ValueType>::useDampingBoundary;

            BoundaryCondition::CPML3D<ValueType> ConvPML; //!< Damping boundary condition class
            using ForwardSolver<ValueType>::useConvPML;
        };
    } /* end namespace ForwardSolver */
} /* end namespace KITGPI */
