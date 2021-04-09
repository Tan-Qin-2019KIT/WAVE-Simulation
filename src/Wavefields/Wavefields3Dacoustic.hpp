

#pragma once
#include <scai/lama.hpp>
#include <scai/lama/DenseVector.hpp>

#include <scai/dmemo/BlockDistribution.hpp>
#include <scai/hmemo/HArray.hpp>

#include "../ForwardSolver/Derivatives/Derivatives.hpp"
#include "Wavefields.hpp"

namespace KITGPI
{

    namespace Wavefields
    {

        /*! \brief The class FD3Dacoustic holds the wavefields for 3D acoustic simulation
         *
         */
        template <typename ValueType>
        class FD3Dacoustic : public Wavefields<ValueType>
        {

          public:
            //! Default constructor
            FD3Dacoustic():EquationType("acoustic"),NumDimension(3)
            {
                equationType = EquationType;
                numDimension = NumDimension;
            };

            //! Default destructor
            ~FD3Dacoustic(){};

            explicit FD3Dacoustic(scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist);

            void resetWavefields() override;

            int getNumDimension() const;
            std::string getEquationType() const;

            /* Getter routines for non-required wavefields: Will throw an error */
            scai::lama::DenseVector<ValueType> &getRefSxx() override;
            scai::lama::DenseVector<ValueType> &getRefSyy() override;
            scai::lama::DenseVector<ValueType> &getRefSzz() override;
            scai::lama::DenseVector<ValueType> &getRefSyz() override;
            scai::lama::DenseVector<ValueType> &getRefSxz() override;
            scai::lama::DenseVector<ValueType> &getRefSxy() override;
            scai::lama::DenseVector<ValueType> &getRefRxx() override;
            scai::lama::DenseVector<ValueType> &getRefRyy() override;
            scai::lama::DenseVector<ValueType> &getRefRzz() override;
            scai::lama::DenseVector<ValueType> &getRefRyz() override;
            scai::lama::DenseVector<ValueType> &getRefRxz() override;
            scai::lama::DenseVector<ValueType> &getRefRxy() override;

            scai::hmemo::ContextPtr getContextPtr() override;

            void init(scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist) override;

            ValueType estimateMemory(scai::dmemo::DistributionPtr dist) override;

            /* Overloading Operators */
            KITGPI::Wavefields::FD3Dacoustic<ValueType> operator*(ValueType rhs);
            KITGPI::Wavefields::FD3Dacoustic<ValueType> operator*=(ValueType rhs);
            KITGPI::Wavefields::FD3Dacoustic<ValueType> operator*(KITGPI::Wavefields::FD3Dacoustic<ValueType> rhs);
            KITGPI::Wavefields::FD3Dacoustic<ValueType> operator*=(KITGPI::Wavefields::FD3Dacoustic<ValueType> rhs);

            void write(scai::IndexType snapType, std::string baseName, scai::IndexType t, KITGPI::ForwardSolver::Derivatives::Derivatives<ValueType> const &derivatives, Modelparameter::Modelparameter<ValueType> const &model, scai::IndexType fileFormat) override;

            void minusAssign(KITGPI::Wavefields::Wavefields<ValueType> &rhs);
            void plusAssign(KITGPI::Wavefields::Wavefields<ValueType> &rhs);
            void assign(KITGPI::Wavefields::Wavefields<ValueType> &rhs);
            void timesAssign(ValueType rhs);
            void applyWavefieldTransform(scai::lama::CSRSparseMatrix<ValueType> lhs, KITGPI::Wavefields::Wavefields<ValueType> &rhs);

          private:
              std::string EquationType;
              int NumDimension;
            using Wavefields<ValueType>::equationType; 
            using Wavefields<ValueType>::numDimension;


            /* required wavefields */
            using Wavefields<ValueType>::VX;
            using Wavefields<ValueType>::VY;
            using Wavefields<ValueType>::VZ;
            using Wavefields<ValueType>::P;

            /* non-required wavefields */
            using Wavefields<ValueType>::Sxx;
            using Wavefields<ValueType>::Syy;
            using Wavefields<ValueType>::Szz;
            using Wavefields<ValueType>::Syz;
            using Wavefields<ValueType>::Sxz;
            using Wavefields<ValueType>::Sxy;
            using Wavefields<ValueType>::Rxx;
            using Wavefields<ValueType>::Ryy;
            using Wavefields<ValueType>::Rzz;
            using Wavefields<ValueType>::Ryz;
            using Wavefields<ValueType>::Rxz;
            using Wavefields<ValueType>::Rxy;

            std::string type = EquationType+std::to_string(NumDimension)+"D";
        };
    }
}
