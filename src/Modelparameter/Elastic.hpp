
#pragma once

#include <scai/lama.hpp>

#include <scai/lama/DenseVector.hpp>
#include <scai/lama/expression/all.hpp>
#include <scai/lama/matrix/all.hpp>
#include <scai/lama/matutils/MatrixCreator.hpp>
#include <scai/lama/norm/L2Norm.hpp>

#include <scai/dmemo/BlockDistribution.hpp>

#include <scai/hmemo/ReadAccess.hpp>
#include <scai/hmemo/WriteAccess.hpp>
#include <scai/hmemo/HArray.hpp>

#include <scai/tracing.hpp>

#include <scai/common/Walltime.hpp>
#include <scai/common/unique_ptr.hpp>
#include <scai/logging.hpp>

#include <iostream>

#include "Modelparameter.hpp"

namespace KITGPI {
    
    //! \brief Modelparameter namespace
    namespace Modelparameter {
        
        //! Class for Modelparameter for elastic simulations (Subsurface properties)
        /*!
         This class handels the modelparameter for the elastic finite-difference simulation.
         */
        template<typename ValueType>
        class Elastic : public Modelparameter<ValueType>
        {
        public:
            
            //! Default constructor.
            Elastic(){};
            
            //! Destructor, releases all allocated resources.
            ~Elastic(){};
            
            explicit Elastic(Configuration::Configuration<ValueType>const& config, hmemo::ContextPtr ctx, dmemo::DistributionPtr dist);
            explicit Elastic(hmemo::ContextPtr ctx, dmemo::DistributionPtr dist, lama::Scalar  pWaveModulus_const,lama::Scalar  sWaveModulus_const, lama::Scalar  rho);
            explicit Elastic(hmemo::ContextPtr ctx, dmemo::DistributionPtr dist, std::string filenamePWaveModulus,std::string filenameSWaveModulus, std::string filenamerho);
            explicit Elastic(hmemo::ContextPtr ctx, dmemo::DistributionPtr dist, std::string filename);
            
            //! Copy Constructor.
            Elastic(const Elastic& rhs);
            
            void init(hmemo::ContextPtr ctx, dmemo::DistributionPtr dist, lama::Scalar  pWaveModulus,lama::Scalar  sWaveModulus, lama::Scalar  rho);
            void init(hmemo::ContextPtr ctx, dmemo::DistributionPtr dist, std::string filename) override;
            void init(hmemo::ContextPtr ctx, dmemo::DistributionPtr dist, std::string filenamePWaveModulus,std::string filenameSWaveModulus, std::string filenamerho);
            
            void initVelocities(hmemo::ContextPtr ctx, dmemo::DistributionPtr dist, std::string filename);
            
            void write(std::string filenamePWaveModulus,std::string filenameSWaveModulus, std::string filenamedensity);
            void write(std::string filename) const override;
            
            /* Getter methods for not requiered parameters */
            lama::DenseVector<ValueType>const& getTauP() override;
            lama::DenseVector<ValueType>const& getTauS() override;
            IndexType getNumRelaxationMechanisms() const override;
            ValueType getRelaxationFrequency() const override;
            
            void switch2velocity() override;
            void switch2modulus() override;
            
            void prepareForModelling() override;
            
            /* Overloading Operators */
            KITGPI::Modelparameter::Elastic<ValueType> operator*(lama::Scalar rhs);
            KITGPI::Modelparameter::Elastic<ValueType> operator*=(lama::Scalar rhs);
            KITGPI::Modelparameter::Elastic<ValueType> operator+(KITGPI::Modelparameter::Elastic<ValueType> rhs);
            KITGPI::Modelparameter::Elastic<ValueType> operator+=(KITGPI::Modelparameter::Elastic<ValueType> rhs);
            KITGPI::Modelparameter::Elastic<ValueType> operator-(KITGPI::Modelparameter::Elastic<ValueType> rhs);
            KITGPI::Modelparameter::Elastic<ValueType> operator-=(KITGPI::Modelparameter::Elastic<ValueType> rhs);
            
        private:
            
            void refreshModule() override;
            void refreshVelocity() override;
            
            using Modelparameter<ValueType>::dirtyFlagInverseDensity;
            using Modelparameter<ValueType>::dirtyFlagModulus;
            using Modelparameter<ValueType>::dirtyFlagVelocity;
            using Modelparameter<ValueType>::parametrisation;
            using Modelparameter<ValueType>::pWaveModulus;
            using Modelparameter<ValueType>::sWaveModulus;
            using Modelparameter<ValueType>::density;
            using Modelparameter<ValueType>::inverseDensity;
            using Modelparameter<ValueType>::velocityP;
            using Modelparameter<ValueType>::velocityS;
            
            /* Not requiered parameters */
            using Modelparameter<ValueType>::tauP;
            using Modelparameter<ValueType>::tauS;
            using Modelparameter<ValueType>::relaxationFrequency;
            using Modelparameter<ValueType>::numRelaxationMechanisms;
            
        };
    }
}

/*! \brief Prepare modellparameter for modelling
 *
 * Refreshes the module if parameterisation is in terms of velocities
 *
 */
template<typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::prepareForModelling(){
    refreshModule();
}

/*! \brief Switch the default parameterization of this class to modulus
 *
 * This will recalulcate the modulus vectors from the velocity vectors.
 * Moreover, the parametrisation value will be set to zero.
 *
 */
template<typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::switch2modulus(){
    if(parametrisation==1){
        this->calcModuleFromVelocity(velocityP,density,pWaveModulus);
        this->calcModuleFromVelocity(velocityS,density,sWaveModulus);
        dirtyFlagModulus=false;
        dirtyFlagVelocity=false;
        parametrisation=0;
    }
};

/*! \brief Switch the default parameterization of this class to velocity
 *
 * This will recalulcate the velocity vectors from the modulus vectors.
 * Moreover, the parametrisation value will be set to one.
 *
 */
template<typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::switch2velocity(){
    if(parametrisation==0){
        this->calcVelocityFromModule(pWaveModulus,density,velocityP);
        this->calcVelocityFromModule(sWaveModulus,density,velocityS);
        dirtyFlagModulus=false;
        dirtyFlagVelocity=false;
        parametrisation=1;
    }
};

/*! \brief Refresh the velocity vectors with the module values
 *
 */
template<typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::refreshVelocity(){
    if(parametrisation==0){
        this->calcVelocityFromModule(pWaveModulus,density,velocityP);
        this->calcVelocityFromModule(sWaveModulus,density,velocityS);
        dirtyFlagVelocity=false;
    }
};

/*! \brief Refresh the module vectors with the velocity values
 *
 */
template<typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::refreshModule(){
    if(parametrisation==1){
        this->calcModuleFromVelocity(velocityP,density,pWaveModulus);
        this->calcModuleFromVelocity(velocityS,density,sWaveModulus);
        dirtyFlagModulus=false;
    }
};


/*! \brief Constructor that is using the Configuration class
 *
 \param config Configuration class
 \param ctx Context for the Calculation
 \param dist Distribution
 */
template<typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType>::Elastic(Configuration::Configuration<ValueType> const& config, hmemo::ContextPtr ctx, dmemo::DistributionPtr dist)
{
    if(config.getIndex("ModelRead")){
        switch (config.getIndex("ModelParametrisation")) {
            case 1:
                init(ctx,dist,config.getString("ModelFilename"));
                break;
            case 2:
                initVelocities(ctx,dist,config.getString("ModelFilename"));
                break;
            default:
                COMMON_THROWEXCEPTION(" Unkown ModelParametrisation value! ")
                break;
        }
    } else {
        ValueType getPWaveModulus = config.getValue("rho") * config.getValue("velocityP")* config.getValue("velocityP");
        ValueType getSWaveModulus = config.getValue("rho") * config.getValue("velocityS")* config.getValue("velocityS");
        init(ctx,dist,getPWaveModulus,getSWaveModulus,config.getValue("rho"));
    }
    
    if(config.getIndex("ModelWrite")){
        write(config.getString("ModelFilename")+".out");
    }
}

/*! \brief Constructor that is generating a homogeneous model
 *
 *  Generates a homogeneous model, which will be initialized by the two given scalar values.
 \param ctx Context
 \param dist Distribution
 \param pWaveModulus_const P-wave modulus given as Scalar
 \param sWaveModulus_const S-wave modulus given as Scalar
 \param rho Density given as Scalar
 */
template<typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType>::Elastic(hmemo::ContextPtr ctx, dmemo::DistributionPtr dist, lama::Scalar  pWaveModulus_const,lama::Scalar  sWaveModulus_const, lama::Scalar  rho)
{
    init(ctx,dist,pWaveModulus_const,sWaveModulus_const,rho);
}


/*! \brief Initialisation that is generating a homogeneous model
 *
 *  Generates a homogeneous model, which will be initialized by the two given scalar values.
 \param ctx Context
 \param dist Distribution
 \param pWaveModulus_const P-wave modulus given as Scalar
 \param sWaveModulus_const S-wave modulus given as Scalar
 \param rho Density given as Scalar
 */
template<typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::init(hmemo::ContextPtr ctx, dmemo::DistributionPtr dist, lama::Scalar  pWaveModulus_const,lama::Scalar  sWaveModulus_const, lama::Scalar  rho)
{
    parametrisation=0;
    this->initModelparameter(pWaveModulus,ctx,dist,pWaveModulus_const);
    this->initModelparameter(sWaveModulus,ctx,dist,sWaveModulus_const);
    this->initModelparameter(density,ctx,dist,rho);
}


/*! \brief Constructor that is reading models from external files
 *
 *  Reads a model from an external file.
 \param ctx Context
 \param dist Distribution
 \param filenamePWaveModulus Name of file that will be read for the P-wave modulus.
 \param filenameSWaveModulus Name of file that will be read for the S-wave modulus.
 \param filenamerho Name of file that will be read for the Density.
 */
template<typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType>::Elastic(hmemo::ContextPtr ctx, dmemo::DistributionPtr dist, std::string filenamePWaveModulus,std::string filenameSWaveModulus, std::string filenamerho)
{
    init(ctx,dist,filenamePWaveModulus,filenameSWaveModulus,filenamerho);
}


/*! \brief Initialisation that is reading models from external files
 *
 *  Reads a model from an external file.
 \param ctx Context
 \param dist Distribution
 \param filenamePWaveModulus Name of file that will be read for the P-wave modulus.
 \param filenameSWaveModulus Name of file that will be read for the S-wave modulus.
 \param filenamerho Name of file that will be read for the Density.
 */
template<typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::init(hmemo::ContextPtr ctx, dmemo::DistributionPtr dist, std::string filenamePWaveModulus, std::string filenameSWaveModulus, std::string filenamerho)
{
    parametrisation=0;
    this->initModelparameter(pWaveModulus,ctx,dist,filenamePWaveModulus);
    this->initModelparameter(density,ctx,dist,filenamerho);
    this->initModelparameter(sWaveModulus,ctx,dist,filenameSWaveModulus);
}


/*! \brief Constructor that is reading models from external files
 *
 *  Reads a model from an external file.
 \param ctx Context
 \param dist Distribution
 \param filename For the P-wave modulus ".pWaveModulus.mtx" is added, for the second ".sWaveModulus.mtx" and for density ".density.mtx" is added.
 */
template<typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType>::Elastic(hmemo::ContextPtr ctx, dmemo::DistributionPtr dist, std::string filename)
{
    init(ctx,dist,filename);
}


/*! \brief Initialisator that is reading models from external files
 *
 *  Reads a model from an external file.
 \param ctx Context
 \param dist Distribution
 \param filename For the P-wave modulus ".pWaveModulus.mtx" is added, for the second ".sWaveModulus.mtx" and for density ".density.mtx" is added.
 */
template<typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::init(hmemo::ContextPtr ctx, dmemo::DistributionPtr dist, std::string filename)
{
    parametrisation=0;
    std::string filenamePWaveModulus=filename+".pWaveModulus.mtx";
    std::string filenameSWaveModulus=filename+".sWaveModulus.mtx";
    std::string filenamedensity=filename+".density.mtx";
    
    this->initModelparameter(pWaveModulus,ctx,dist,filenamePWaveModulus);
    this->initModelparameter(sWaveModulus,ctx,dist,filenameSWaveModulus);
    this->initModelparameter(density,ctx,dist,filenamedensity);
}


//! \brief Copy constructor
template<typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType>::Elastic(const Elastic& rhs)
{
    pWaveModulus=rhs.pWaveModulus;
    sWaveModulus=rhs.sWaveModulus;
    velocityP=rhs.velocityP;
    velocityS=rhs.velocityS;
    density=rhs.density;
    dirtyFlagInverseDensity=rhs.dirtyFlagInverseDensity;
    dirtyFlagModulus=rhs.dirtyFlagModulus;
    dirtyFlagVelocity=rhs.dirtyFlagVelocity;
    parametrisation=rhs.parametrisation;
    inverseDensity=rhs.inverseDensity;
}

/*! \brief Initialisator that is reading Velocity-Vector
 *
 *  Reads a model from an external file.
 \param ctx Context
 \param dist Distribution
 \param filename For the Velocity-Vector "filename".vp.mtx" and "filename".vs.mtx" is added and for density "filename+".density.mtx" is added.
 *
 */
template<typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::initVelocities(hmemo::ContextPtr ctx, dmemo::DistributionPtr dist, std::string filename)
{
    parametrisation=1;
    std::string filenameVelocityP=filename+".vp.mtx";
    std::string filenameVelocityS=filename+".vs.mtx";
    std::string filenamedensity=filename+".density.mtx";
    
    this->initModelparameter(velocityP,ctx,dist,filenameVelocityP);
    this->initModelparameter(velocityS,ctx,dist,filenameVelocityS);
    this->initModelparameter(density,ctx,dist,filenamedensity);
    
}


/*! \brief Write model to an external file
 *
 \param filenamePWaveModulus Filename for P-wave modulus model
 \param filenameSWaveModulus Name of file that will be read for the S-wave modulus.
 \param filenamedensity Filename for Density model
 */
template<typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::write( std::string filenamePWaveModulus, std::string filenameSWaveModulus, std::string filenamedensity)
{
    this->writeModelparameter(pWaveModulus,filenamePWaveModulus);
    this->writeModelparameter(density,filenamedensity);
    this->writeModelparameter(sWaveModulus,filenameSWaveModulus);
};


/*! \brief Write model to an external file
 *
 \param filename For the P-wave modulus ".pWaveModulus.mtx" is added, for the second ".sWaveModulus.mtx" and for density ".density.mtx" is added.
 */
template<typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::write(std::string filename) const
{
    std::string filenamePWaveModulus=filename+".pWaveModulus.mtx";
    std::string filenameSWaveModulus=filename+".sWaveModulus.mtx";
    std::string filenamedensity=filename+".density.mtx";
    this->writeModelparameter(pWaveModulus,filenamePWaveModulus);
    this->writeModelparameter(sWaveModulus,filenameSWaveModulus);
    this->writeModelparameter(density,filenamedensity);
};

/*! \brief Get reference to tauP
 *
 */
template<typename ValueType>
lama::DenseVector<ValueType>const& KITGPI::Modelparameter::Elastic<ValueType>::getTauP(){
    COMMON_THROWEXCEPTION("There is no tau parameter in an elastic modelling")
    return(tauP);
}

/*! \brief Get reference to tauS
 */
template<typename ValueType>
lama::DenseVector<ValueType>const& KITGPI::Modelparameter::Elastic<ValueType>::getTauS(){
    COMMON_THROWEXCEPTION("There is no tau parameter in an elastic modelling")
    return(tauS);
}

/*! \brief Getter method for relaxation frequency */
template<typename ValueType>
ValueType KITGPI::Modelparameter::Elastic<ValueType>::getRelaxationFrequency() const
{
    COMMON_THROWEXCEPTION("There is no relaxationFrequency parameter in an elastic modelling")
    return(relaxationFrequency);
}

/*! \brief Getter method for number of relaxation mechanisms */
template<typename ValueType>
IndexType KITGPI::Modelparameter::Elastic<ValueType>::getNumRelaxationMechanisms() const
{
    COMMON_THROWEXCEPTION("There is no numRelaxationMechanisms parameter in an elastic modelling")
    return(numRelaxationMechanisms);
}


/*! \brief Overloading * Operation
 *
 \param rhs Scalar factor with which the vectors are multiplied.
 */
template<typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType> KITGPI::Modelparameter::Elastic<ValueType>::operator*(lama::Scalar rhs)
{
    KITGPI::Modelparameter::Elastic<ValueType> result;
    result.density = this->density * rhs;
    if (parametrisation==0) {
        result.pWaveModulus= this->pWaveModulus * rhs;
        result.sWaveModulus= this->sWaveModulus * rhs;
        return result;
    } if (parametrisation==1) {
        result.velocityP= this->velocityP * rhs;
        result.velocityS= this->velocityS * rhs;
        return result;
    } else {
        COMMON_THROWEXCEPTION(" Unknown parametrisation! ");
    }
}


/*! \brief free function to multiply
 *
 \param lhs Scalar factor with which the vectors are multiplied.
 \param rhs Vector
 */
template<typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType> operator*(lama::Scalar lhs, KITGPI::Modelparameter::Elastic<ValueType> rhs)
{
    return rhs * lhs;
}


/*! \brief Overloading *= Operation
 *
 \param rhs Scalar factor with which the vectors are multiplied.
 */
template<typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType> KITGPI::Modelparameter::Elastic<ValueType>::operator*=(lama::Scalar rhs)
{
    return this * rhs;
}


/*! \brief Overloading + Operation
 *
 \param rhs Model which is added.
 */
template<typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType> KITGPI::Modelparameter::Elastic<ValueType>::operator+(KITGPI::Modelparameter::Elastic<ValueType> rhs)
{
    KITGPI::Modelparameter::Elastic<ValueType> result;
    result.density = this->density + rhs.density;
    if (parametrisation==0) {
        result.pWaveModulus= this->pWaveModulus + rhs.pWaveModulus;
        result.sWaveModulus= this->sWaveModulus + rhs.sWaveModulus;
        return result;
    } if (parametrisation==1) {
        result.velocityP= this->velocityP + rhs.velocityP;
        result.velocityS= this->velocityS + rhs.velocityS;
        return result;
    } else {
        COMMON_THROWEXCEPTION(" Unknown parametrisation! ");
    }
}


/*! \brief Overloading += Operation
 *
 \param rhs Model which is added.
 */
template<typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType> KITGPI::Modelparameter::Elastic<ValueType>::operator+=(KITGPI::Modelparameter::Elastic<ValueType> rhs)
{
    return this + rhs;
}


/*! \brief Overloading - Operation
 *
 \param rhs Model which is subtractet.
 */
template<typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType> KITGPI::Modelparameter::Elastic<ValueType>::operator-(KITGPI::Modelparameter::Elastic<ValueType> rhs)
{
    KITGPI::Modelparameter::Elastic<ValueType> result;
    result.density = this->density - rhs.density;
    if (parametrisation==0) {
        result.pWaveModulus= this->pWaveModulus - rhs.pWaveModulus;
        result.sWaveModulus= this->sWaveModulus - rhs.sWaveModulus;
        return result;
    } if (parametrisation==1) {
        result.velocityP= this->velocityP - rhs.velocityP;
        result.velocityS= this->velocityS - rhs.velocityS;
        return result;
    } else {
        COMMON_THROWEXCEPTION(" Unknown parametrisation! ");
    }
}


/*! \brief Overloading -= Operation
 *
 \param rhs Model which is subtractet.
 */
template<typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType> KITGPI::Modelparameter::Elastic<ValueType>::operator-=(KITGPI::Modelparameter::Elastic<ValueType> rhs)
{
    return this - rhs; 
}


