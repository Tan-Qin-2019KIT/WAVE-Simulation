
#pragma once

#include <scai/lama.hpp>
#include <scai/lama/DenseVector.hpp>

#include <iostream>

#include "../Acquisition/Receivers.hpp"
#include "../Acquisition/Sources.hpp"
#include "../Acquisition/Seismogram.hpp"

#include "../Modelparameter/Modelparameter.hpp"
#include "../Wavefields/Wavefields.hpp"
#include "Derivatives/Derivatives.hpp"
#include "BoundaryCondition/FreeSurface2Delastic.hpp"
#include "BoundaryCondition/ABS2D.hpp"
#include "SourceReceiverImpl/FDTD2Delastic.hpp"

namespace KITGPI {
    
    namespace ForwardSolver {
        
        //! \brief 3-D elastic forward solver
        template<typename ValueType>
        class FD2Delastic : public ForwardSolver<ValueType>
        {
            
        public:
            
            /* Default constructor */
            FD2Delastic(){};
            
            /* Default destructor */
            ~FD2Delastic(){};
            
            void run(Acquisition::Receivers<ValueType>& receiver, Acquisition::Sources<ValueType> const& sources, Modelparameter::Modelparameter<ValueType>& model, Wavefields::Wavefields<ValueType>& wavefield, Derivatives::Derivatives<ValueType>const& derivatives, IndexType NT, ValueType DT) override;
            
            void prepareBoundaryConditions(Configuration::Configuration<ValueType> const& config, Derivatives::Derivatives<ValueType>& derivatives,dmemo::DistributionPtr dist, hmemo::ContextPtr ctx) override;
            
        private:
            
            /* Boundary Conditions */
            BoundaryCondition::FreeSurface2Delastic<ValueType> FreeSurface; //!< Free Surface boundary condition class
            using ForwardSolver<ValueType>::useFreeSurface;
            
            BoundaryCondition::ABS2D<ValueType> DampingBoundary; //!< Damping boundary condition class
            using ForwardSolver<ValueType>::useDampingBoundary;
            
        };
    } /* end namespace ForwardSolver */
} /* end namespace KITGPI */


/*! \brief Initialitation of the boundary conditions
 *
 *
 \param config Configuration
 \param derivatives Derivatives matrices
 \param dist Distribution of the wave fields
 \param ctx Context
 */
template<typename ValueType>
void KITGPI::ForwardSolver::FD2Delastic<ValueType>::prepareBoundaryConditions(Configuration::Configuration<ValueType> const& config, Derivatives::Derivatives<ValueType>& derivatives,dmemo::DistributionPtr dist, hmemo::ContextPtr ctx){
    
    /* Prepare Free Surface */
    if(config.getIndex("FreeSurface")){
        useFreeSurface=true;
        FreeSurface.init(dist,derivatives,config.getIndex("NX"),config.getIndex("NY"),config.getIndex("NZ"),config.getValue("DT"),config.getValue("DH"));
    }
    
    /* Prepare Damping Boundary */
    if(config.getIndex("DampingBoundary")){
        useDampingBoundary=true;
        DampingBoundary.init(dist,ctx,config.getIndex("NX"),config.getIndex("NY"),config.getIndex("NZ"),config.getIndex("BoundaryWidth"), config.getValue("DampingCoeff"),useFreeSurface);
    }
    
}

/*! \brief Running the 3-D elastic foward solver
 *
 * Start the 3-D forward solver as defined by the given parameters
 *
 \param receiver Configuration of the receivers
 \param sources Configuration of the sources
 \param model Configuration of the modelparameter
 \param wavefield Wavefields for the modelling
 \param derivatives Derivations matrices to calculate the spatial derivatives
 \param NT Total number of time steps
 \param DT Temporal Sampling intervall in seconds
 */
template<typename ValueType>
void KITGPI::ForwardSolver::FD2Delastic<ValueType>::run(Acquisition::Receivers<ValueType>& receiver, Acquisition::Sources<ValueType> const& sources, Modelparameter::Modelparameter<ValueType>& model, Wavefields::Wavefields<ValueType>& wavefield, Derivatives::Derivatives<ValueType>const& derivatives, IndexType NT, ValueType /*DT*/){
    
    SCAI_REGION( "timestep" )
    
    SCAI_ASSERT_ERROR( NT > 0 , " Number of time steps has to be greater than zero. ");
    
    /* Get references to required modelparameter */
    lama::DenseVector<ValueType>const& inverseDensity=model.getInverseDensity();
    lama::DenseVector<ValueType>const& pWaveModulus=model.getPWaveModulus();
    lama::DenseVector<ValueType>const& sWaveModulus=model.getSWaveModulus();
    
    /* Get references to required wavefields */
    lama::DenseVector<ValueType>& vX=wavefield.getVX();
    lama::DenseVector<ValueType>& vY=wavefield.getVY();
    
    lama::DenseVector<ValueType>& Sxx=wavefield.getSxx();
    lama::DenseVector<ValueType>& Syy=wavefield.getSyy();
    
    lama::DenseVector<ValueType>& Sxy=wavefield.getSxy();
    
    /* Get references to required derivatives matrixes */
    lama::CSRSparseMatrix<ValueType>const& Dxf=derivatives.getDxf();
    lama::CSRSparseMatrix<ValueType>const& Dxb=derivatives.getDxb();
    
    lama::CSRSparseMatrix<ValueType>const& DybPressure=derivatives.getDybPressure();
    lama::CSRSparseMatrix<ValueType>const& DybVelocity=derivatives.getDybVelocity();
    lama::CSRSparseMatrix<ValueType>const& DyfPressure=derivatives.getDyfPressure();
    lama::CSRSparseMatrix<ValueType>const& DyfVelocity=derivatives.getDyfVelocity();
    
    SourceReceiverImpl::FDTD2Delastic<ValueType> SourceReceiver(sources,receiver,wavefield);
    
    common::unique_ptr<lama::Vector> updatePtr( vX.newVector() ); // create new Vector(Pointer) with same configuration as vZ
    lama::Vector& update = *updatePtr; // get Reference of VectorPointer
    
    common::unique_ptr<lama::Vector> vxxPtr( vX.newVector() );
    common::unique_ptr<lama::Vector> vyyPtr( vX.newVector() );
    
    lama::Vector& vxx = *vxxPtr;
    lama::Vector& vyy = *vyyPtr;
    
    if(useFreeSurface){
        FreeSurface.setModelparameter(model);
    }
    
    dmemo::CommunicatorPtr comm=inverseDensity.getDistributionPtr()->getCommunicatorPtr();

    
    /* --------------------------------------- */
    /* Start runtime critical part             */
    /* --------------------------------------- */
    
    HOST_PRINT( comm, "Start time stepping\n" );
    ValueType start_t = common::Walltime::get();
    for ( IndexType t = 0; t < NT; t++ ){
        
        
        if( t % 100 == 0 && t != 0){
            HOST_PRINT( comm, "Calculating time step " << t << " from " << NT << "\n" );
        }
        
        /* ----------------*/
        /* update velocity */
        /* ----------------*/
        update = Dxf * Sxx;
        update += DybVelocity * Sxy;
        vX += update.scale(inverseDensity);
        
        update = Dxb * Sxy;
        update += DyfVelocity * Syy;
        vY += update.scale(inverseDensity);
        
        
        /* ----------------*/
        /* pressure update */
        /* ----------------*/
        vxx = Dxb * vX;
        vyy = DybPressure * vY;
        
        update = vxx;
        update += vyy;
        update.scale(pWaveModulus);
        
        Sxx += update;
        Syy += update;
        
        Sxx -= 2.0 * vyy.scale(sWaveModulus);
        Syy -= 2.0 * vxx.scale(sWaveModulus);
        
        update = DyfPressure * vX;
        update += Dxf * vY;
        Sxy += update.scale(sWaveModulus);
        
        
        /* Apply free surface to stress update */
        if(useFreeSurface){
            FreeSurface.apply(vxx,Sxx,Syy);
        }
        
        /* Apply the damping boundary */
        if(useDampingBoundary){
            DampingBoundary.apply(Sxx,Syy,Sxy,vX,vY);
        }
        
        /* Apply source and save seismogram */
        SourceReceiver.applySource(t);
        SourceReceiver.gatherSeismogram(t);
        
    }
    ValueType end_t = common::Walltime::get();
    HOST_PRINT( comm, "Finished time stepping in " << end_t - start_t << " sec.\n\n" );
    
    /* --------------------------------------- */
    /* Stop runtime critical part             */
    /* --------------------------------------- */
}
