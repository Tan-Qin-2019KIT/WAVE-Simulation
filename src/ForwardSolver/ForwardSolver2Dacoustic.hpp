
#pragma once

#include <scai/lama.hpp>
#include <scai/lama/DenseVector.hpp>

#include <iostream>

#include "../Acquisition/Receivers.hpp"
#include "../Acquisition/Sources.hpp"

#include "../Modelparameter/Modelparameter.hpp"
#include "../Wavefields/Wavefields.hpp"
#include "Derivatives/Derivatives.hpp"
#include "BoundaryCondition/FreeSurface2Dacoustic.hpp"
#include "BoundaryCondition/ABS2D.hpp"
#include "BoundaryCondition/CPML2DAcoustic.hpp"
#include "SourceReceiverImpl/FDTD2Dacoustic.hpp"

namespace KITGPI {
    
    namespace ForwardSolver {
        
        //! \brief 3-D Acoustic forward solver
        template<typename ValueType>
        class FD2Dacoustic : public ForwardSolver<ValueType>
        {
            
        public:
            
            /* Default constructor */
            FD2Dacoustic(){};
            
            /* Default destructor */
            ~FD2Dacoustic(){};
            
            void run(Acquisition::Receivers<ValueType>& receiver, Acquisition::Sources<ValueType> const& sources, Modelparameter::Modelparameter<ValueType>& model, Wavefields::Wavefields<ValueType>& wavefield, Derivatives::Derivatives<ValueType>const& derivatives, IndexType NT, ValueType DT) override;
            
            void prepareBoundaryConditions(Configuration::Configuration<ValueType> const& config, Derivatives::Derivatives<ValueType>& derivatives,dmemo::DistributionPtr dist, hmemo::ContextPtr ctx) override;

        private:
            
            /* Boundary Conditions */
            BoundaryCondition::FreeSurface2Dacoustic<ValueType> FreeSurface; //!< Free Surface boundary condition class
            using ForwardSolver<ValueType>::useFreeSurface;
            
            BoundaryCondition::ABS2D<ValueType> DampingBoundary; //!< Damping boundary condition class
            using ForwardSolver<ValueType>::useDampingBoundary;
            
	    BoundaryCondition::CPML2DAcoustic<ValueType> ConvPML; //!< Damping boundary condition class
            using ForwardSolver<ValueType>::useConvPML;
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
void KITGPI::ForwardSolver::FD2Dacoustic<ValueType>::prepareBoundaryConditions(Configuration::Configuration<ValueType> const& config, Derivatives::Derivatives<ValueType>& derivatives,dmemo::DistributionPtr dist, hmemo::ContextPtr ctx){
    
    /* Prepare Free Surface */
    if(config.getFreeSurface()){
        useFreeSurface=true;
        FreeSurface.init(dist,derivatives,config.getNX(),config.getNY(),config.getNZ(),config.getDT(),config.getDH());
    }
    
    /* Prepare Damping Boundary */
    if(config.getDampingBoundary()==1){
        useDampingBoundary=true;
        DampingBoundary.init(dist,ctx,config.getNX(),config.getNY(),config.getNZ(),config.getBoundaryWidth(), config.getDampingCoeff(),useFreeSurface);
    }
    
    if(config.getDampingBoundary()==2){
	useConvPML=true;
	ConvPML.init(dist,ctx,config.getNX(),config.getNY(),config.getNZ(),config.getDT(),config.getDH(),config.getBoundaryWidth(),useFreeSurface,config.getPMLVariables());
    }
}

                                                                                /*! \brief Running the 3-D acoustic foward solver
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
void KITGPI::ForwardSolver::FD2Dacoustic<ValueType>::run(Acquisition::Receivers<ValueType>& receiver, Acquisition::Sources<ValueType> const& sources, Modelparameter::Modelparameter<ValueType>& model, Wavefields::Wavefields<ValueType>& wavefield, Derivatives::Derivatives<ValueType>const& derivatives, IndexType NT, ValueType /*DT*/){
    
    SCAI_REGION( "timestep" )
    
    SCAI_ASSERT_ERROR( NT > 1 , " Number of time steps has to be greater than zero. ");
    
    /* Get references to required modelparameter */
    lama::DenseVector<ValueType> const& inverseDensity=model.getInverseDensity();
    lama::DenseVector<ValueType> const& pWaveModulus=model.getPWaveModulus();
    
    /* Get references to required wavefields */
    lama::DenseVector<ValueType>& vX=wavefield.getVX();
    lama::DenseVector<ValueType>& vY=wavefield.getVY();
    lama::DenseVector<ValueType>& p=wavefield.getP();
    
    /* Get references to required derivatives matrixes */
    lama::CSRSparseMatrix<ValueType>const& Dxf=derivatives.getDxf();
    lama::CSRSparseMatrix<ValueType>const& Dxb=derivatives.getDxb();
    lama::CSRSparseMatrix<ValueType>const& Dyb=derivatives.getDyb();
    lama::CSRSparseMatrix<ValueType>const& Dyf=derivatives.getDyfVelocity();
    
    SourceReceiverImpl::FDTD2Dacoustic<ValueType> SourceReceiver(sources,receiver,wavefield);
    
    common::unique_ptr<lama::Vector> updatePtr( vX.newVector() ); // create new Vector(Pointer) with same configuration as vZ
    lama::Vector& update = *updatePtr; // get Reference of VectorPointer
    
    common::unique_ptr<lama::Vector> update_tempPtr( vX.newVector() ); // create new Vector(Pointer) with same configuration as vZ
    lama::Vector& update_temp = *update_tempPtr; // get Reference of VectorPointer
    
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
        
        
        /* update velocity */
        update= Dxf * p;
	if(useConvPML) { ConvPML.apply_p_x(update);}
        vX += update.scale(inverseDensity);

        update= Dyf * p;
	if(useConvPML) { ConvPML.apply_p_y(update);}
        vY += update.scale(inverseDensity);


        /* pressure update */
        update  =  Dxb * vX;
	if(useConvPML) { ConvPML.apply_vxx(update);}
	
	update_temp =  Dyb * vY;
	if(useConvPML) { ConvPML.apply_vyy(update_temp);}
	update+=update_temp;
	
        p += update.scale(pWaveModulus);

        /* Apply free surface to pressure update */
        if(useFreeSurface){
            FreeSurface.apply(p);
        }

        /* Apply the damping boundary */
        if(useDampingBoundary){
            DampingBoundary.apply(p,vX,vY);
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
