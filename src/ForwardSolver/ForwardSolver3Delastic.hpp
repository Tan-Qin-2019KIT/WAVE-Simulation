
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

            void run(Acquisition::AcquisitionGeometry<ValueType> &receiver, const Acquisition::AcquisitionGeometry<ValueType> &sources, const Modelparameter::Modelparameter<ValueType> &model, Wavefields::Wavefields<ValueType> &wavefield, const Derivatives::Derivatives<ValueType> &derivatives, IndexType TStart, IndexType TEnd) override;

            void prepareBoundaryConditions(Configuration::Configuration const &config, Derivatives::Derivatives<ValueType> &derivatives, scai::dmemo::DistributionPtr dist, scai::hmemo::ContextPtr ctx) override;

	    void initForwardSolver(Configuration::Configuration const &config, Derivatives::Derivatives<ValueType> &derivatives, Wavefields::Wavefields<ValueType> &wavefield, Modelparameter::Modelparameter<ValueType> const &model, scai::hmemo::ContextPtr ctx, ValueType /*DT*/) override;
          private:
            /* Boundary Conditions */
            BoundaryCondition::FreeSurface3Delastic<ValueType> FreeSurface; //!< Free Surface boundary condition class
            using ForwardSolver<ValueType>::useFreeSurface;

            BoundaryCondition::ABS3D<ValueType> DampingBoundary; //!< Damping boundary condition class
            using ForwardSolver<ValueType>::useDampingBoundary;

            BoundaryCondition::CPML3D<ValueType> ConvPML; //!< Damping boundary condition class
            using ForwardSolver<ValueType>::useConvPML;
	    
	    /* Auxiliary Vectors */
	    scai::common::unique_ptr<scai::lama::Vector> updatePtr;
	    scai::common::unique_ptr<scai::lama::Vector> update_tempPtr;
	    scai::common::unique_ptr<scai::lama::Vector> vxxPtr;
	    scai::common::unique_ptr<scai::lama::Vector> vyyPtr;
	    scai::common::unique_ptr<scai::lama::Vector> vzzPtr;
        };
    } /* end namespace ForwardSolver */
} /* end namespace KITGPI */
