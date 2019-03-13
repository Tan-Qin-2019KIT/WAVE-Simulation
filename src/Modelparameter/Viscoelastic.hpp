
#pragma once

#include <scai/lama.hpp>

#include <scai/lama/DenseVector.hpp>
#include <scai/lama/matrix/all.hpp>
#include <scai/lama/matutils/MatrixCreator.hpp>
#include <scai/lama/norm/L2Norm.hpp>

#include <scai/dmemo/BlockDistribution.hpp>

#include <scai/hmemo/HArray.hpp>
#include <scai/hmemo/ReadAccess.hpp>
#include <scai/hmemo/WriteAccess.hpp>

#include <scai/tracing.hpp>

#include <scai/common/Walltime.hpp>
#include <scai/logging.hpp>

#include <iostream>

#include "../PartitionedInOut/PartitionedInOut.hpp"
#include "Modelparameter.hpp"

namespace KITGPI
{

    //! \brief Modelparameter namespace
    namespace Modelparameter
    {

        //! Class for Modelparameter for visco-elastic simulations (Subsurface properties)
        /*!
         This class handels the modelparameter for the visco-elastic finite-difference simulation.
         */
        template <typename ValueType>
        class Viscoelastic : public Modelparameter<ValueType>
        {
          public:
            //! Default constructor.
            Viscoelastic() { equationType = "viscoelastic"; };

            //! Destructor, releases all allocated resources.
            ~Viscoelastic(){};

            explicit Viscoelastic(Configuration::Configuration const &config, scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist);
            explicit Viscoelastic(scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist, ValueType pWaveModulus_const, ValueType sWaveModulus_const, ValueType rho_const, ValueType tauP_const, ValueType tauS_const, scai::IndexType numRelaxationMechanisms_in, ValueType relaxationFrequency_in);
            explicit Viscoelastic(scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist, std::string filename, scai::IndexType partitionedIn);

            //! Copy Constructor.
            Viscoelastic(const Viscoelastic &rhs);

            void init(scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist, ValueType velocityP_const, ValueType velocityS_const, ValueType rho_const, ValueType tauP_const, ValueType tauS_const, scai::IndexType numRelaxationMechanisms_in, ValueType relaxationFrequency_in);
            void init(Configuration::Configuration const &config, scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist) override;
            void init(scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist, std::string filename, scai::IndexType partitionedIn) override;
            void init(KITGPI::Modelparameter::Modelparameter<ValueType> const &model, scai::dmemo::DistributionPtr variableDist,Acquisition::Coordinates<ValueType> const &variableCoordinates,Acquisition::Coordinates<ValueType> const &regularCoordinates) override {COMMON_THROWEXCEPTION("variable grid is not implemented in the viscoelastic case")}; 
            
            void initRelaxationMechanisms(scai::IndexType numRelaxationMechanisms_in, ValueType relaxationFrequency_in);

            void write(std::string filename, scai::IndexType partitionedOut) const override;

            std::string getEquationType() const;

            scai::lama::Vector<ValueType> const &getPWaveModulus() override;
            scai::lama::Vector<ValueType> const &getSWaveModulus() override;

            void prepareForModelling(Acquisition::Coordinates<ValueType> const &modelCoordinates, scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist, scai::dmemo::CommunicatorPtr comm) override;

            void applyThresholds(Configuration::Configuration const &config) override;

            void minusAssign(KITGPI::Modelparameter::Modelparameter<ValueType> const &rhs);
            void plusAssign(KITGPI::Modelparameter::Modelparameter<ValueType> const &rhs);
            void assign(KITGPI::Modelparameter::Modelparameter<ValueType> const &rhs);

            /* Overloading Operators */
            KITGPI::Modelparameter::Viscoelastic<ValueType> operator*(ValueType rhs);
            KITGPI::Modelparameter::Viscoelastic<ValueType> &operator*=(ValueType const &rhs);
            KITGPI::Modelparameter::Viscoelastic<ValueType> operator+(KITGPI::Modelparameter::Viscoelastic<ValueType> const &rhs);
            KITGPI::Modelparameter::Viscoelastic<ValueType> &operator+=(KITGPI::Modelparameter::Viscoelastic<ValueType> const &rhs);
            KITGPI::Modelparameter::Viscoelastic<ValueType> operator-(KITGPI::Modelparameter::Viscoelastic<ValueType> const &rhs);
            KITGPI::Modelparameter::Viscoelastic<ValueType> &operator-=(KITGPI::Modelparameter::Viscoelastic<ValueType> const &rhs);
            KITGPI::Modelparameter::Viscoelastic<ValueType> &operator=(KITGPI::Modelparameter::Viscoelastic<ValueType> const &rhs);

          private:
            void calculateAveraging() override;

            using Modelparameter<ValueType>::equationType;

            using Modelparameter<ValueType>::dirtyFlagInverseDensity;
            using Modelparameter<ValueType>::dirtyFlagPWaveModulus;
            using Modelparameter<ValueType>::dirtyFlagSWaveModulus;
            using Modelparameter<ValueType>::dirtyFlagAveraging;
            using Modelparameter<ValueType>::pWaveModulus;
            using Modelparameter<ValueType>::sWaveModulus;
            using Modelparameter<ValueType>::density;
            using Modelparameter<ValueType>::inverseDensity;
            using Modelparameter<ValueType>::velocityP;
            using Modelparameter<ValueType>::velocityS;

            using Modelparameter<ValueType>::tauS;
            using Modelparameter<ValueType>::tauP;
            using Modelparameter<ValueType>::relaxationFrequency;
            using Modelparameter<ValueType>::numRelaxationMechanisms;

            void initializeMatrices(scai::dmemo::DistributionPtr dist, scai::hmemo::ContextPtr ctx, Acquisition::Coordinates<ValueType> const &modelCoordinates, scai::dmemo::CommunicatorPtr comm) override;

            using Modelparameter<ValueType>::DensityAverageMatrixX;
            using Modelparameter<ValueType>::DensityAverageMatrixY;
            using Modelparameter<ValueType>::DensityAverageMatrixZ;
            using Modelparameter<ValueType>::sWaveModulusAverageMatrixXY;
            using Modelparameter<ValueType>::sWaveModulusAverageMatrixXZ;
            using Modelparameter<ValueType>::sWaveModulusAverageMatrixYZ;

            using Modelparameter<ValueType>::inverseDensityAverageX;
            using Modelparameter<ValueType>::inverseDensityAverageY;
            using Modelparameter<ValueType>::inverseDensityAverageZ;
            using Modelparameter<ValueType>::sWaveModulusAverageXY;
            using Modelparameter<ValueType>::sWaveModulusAverageXZ;
            using Modelparameter<ValueType>::sWaveModulusAverageYZ;
            using Modelparameter<ValueType>::tauSAverageXY;
            using Modelparameter<ValueType>::tauSAverageXZ;
            using Modelparameter<ValueType>::tauSAverageYZ;
        };
    }
}
