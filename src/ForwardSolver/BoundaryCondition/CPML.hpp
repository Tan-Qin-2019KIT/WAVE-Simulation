#pragma once

#include <scai/dmemo.hpp>
#include <scai/hmemo.hpp>
#include <scai/lama.hpp>

namespace KITGPI
{

    namespace ForwardSolver
    {

        namespace BoundaryCondition
        {

            //! \brief Abstract class for the calculation and application of cpml boundaries
            template <typename ValueType>
            class CPML
            {
              public:
                //! \brief Default constructor
                CPML(){};

                //! \brief Default destructor
                ~CPML(){};

                //! init CPML coefficient vectors and CPML memory variables
                virtual void init(scai::dmemo::DistributionPtr dist, scai::hmemo::ContextPtr ctx, scai::IndexType NX, scai::IndexType NY, scai::IndexType NZ, ValueType DT, scai::IndexType DH, scai::IndexType BoundaryWidth, ValueType NPower, ValueType KMaxCPML, ValueType CenterFrequencyCPML, ValueType VMaxCPML, bool useFreeSurface) = 0;

              protected:
                void resetVector(scai::lama::Vector<ValueType> &vector);

                void initVector(scai::lama::Vector<ValueType> &vector, scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist);

                void SetCoeffCPML(scai::lama::DenseVector<ValueType> &a, scai::lama::DenseVector<ValueType> &b, scai::lama::DenseVector<ValueType> &kInv, scai::lama::DenseVector<ValueType> &a_half, scai::lama::DenseVector<ValueType> &b_half, scai::lama::DenseVector<ValueType> &kInv_half, scai::IndexType coord,
                                  scai::IndexType gdist, scai::IndexType BoundaryWidth, ValueType NPower, ValueType KMaxCPML, ValueType CenterFrequencyCPML, ValueType VMaxCPML, scai::IndexType i, ValueType DT, ValueType DH);

                void ResetCoeffFreeSurface(scai::lama::DenseVector<ValueType> &a, scai::lama::DenseVector<ValueType> &b, scai::lama::DenseVector<ValueType> &kInv,
                                           scai::lama::DenseVector<ValueType> &a_half, scai::lama::DenseVector<ValueType> &b_half, scai::lama::DenseVector<ValueType> &kInv_half,
                                           scai::IndexType i);

                /*inline*/ void applyCPML(scai::lama::Vector<ValueType> &Vec, scai::lama::Vector<ValueType> &Psi, scai::lama::Vector<ValueType> &a, scai::lama::Vector<ValueType> &b, scai::lama::Vector<ValueType> &kInv);

                typedef typename scai::lama::DenseVector<ValueType> VectorType; //!< Define Vector Type as Dense vector. For big models switch to SparseVector

                VectorType temp; //!< temporary vector for pml application
            };
        } /* end namespace BoundaryCondition  */
    }     /* end namespace ForwardSolver */
} /* end namespace KITGPI */
