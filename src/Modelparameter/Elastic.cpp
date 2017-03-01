#include "Elastic.hpp"

using namespace scai;

/*! \brief Prepare modellparameter for modelling
 *
 * Refreshes the module if parameterisation is in terms of velocities
 *
 */
template <typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::prepareForModelling(Configuration::Configuration const &config, scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist, scai::dmemo::CommunicatorPtr comm)
{
    HOST_PRINT(comm, "Preparation of the model parameters…\n");

    refreshModule();
    initializeMatrices(dist, ctx, config, comm);
    this->getInverseDensity();
    calculateAveraging();

    HOST_PRINT(comm, "Model ready!\n\n");
}

/*! \brief Switch the default parameterization of this class to modulus
 *
 * This will recalulcate the modulus vectors from the velocity vectors.
 * Moreover, the parametrisation value will be set to zero.
 *
 */
template <typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::switch2modulus()
{
    if (parametrisation == 1) {
        this->calcModuleFromVelocity(velocityP, density, pWaveModulus);
        this->calcModuleFromVelocity(velocityS, density, sWaveModulus);
        dirtyFlagAveraging = true;
        dirtyFlagModulus = false;
        dirtyFlagVelocity = false;
        parametrisation = 0;
    }
};

/*! \brief Switch the default parameterization of this class to velocity
 *
 * This will recalulcate the velocity vectors from the modulus vectors.
 * Moreover, the parametrisation value will be set to one.
 *
 */
template <typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::switch2velocity()
{
    if (parametrisation == 0) {
        this->calcVelocityFromModule(pWaveModulus, density, velocityP);
        this->calcVelocityFromModule(sWaveModulus, density, velocityS);
        dirtyFlagModulus = false;
        dirtyFlagVelocity = false;
        parametrisation = 1;
    }
};

/*! \brief Refresh the velocity vectors with the module values
 *
 */
template <typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::refreshVelocity()
{
    if (parametrisation == 0) {
        this->calcVelocityFromModule(pWaveModulus, density, velocityP);
        this->calcVelocityFromModule(sWaveModulus, density, velocityS);
        dirtyFlagVelocity = false;
    }
};

/*! \brief Refresh the module vectors with the velocity values
 *
 */
template <typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::refreshModule()
{
    if (parametrisation == 1) {
        this->calcModuleFromVelocity(velocityP, density, pWaveModulus);
        this->calcModuleFromVelocity(velocityS, density, sWaveModulus);
        dirtyFlagModulus = false;
        dirtyFlagAveraging = true;
    }
};

/*! \brief Constructor that is using the Configuration class
 *
 \param config Configuration class
 \param ctx Context for the Calculation
 \param dist Distribution
 */
template <typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType>::Elastic(Configuration::Configuration const &config, scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist)
{
    init(config, ctx, dist);
}

/*! \brief Initialisation that is using the Configuration class
 *
 \param config Configuration class
 \param ctx Context for the Calculation
 \param dist Distribution
 */
template <typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::init(Configuration::Configuration const &config, scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist)
{
    if (config.get<IndexType>("ModelRead")) {

        HOST_PRINT(dist->getCommunicatorPtr(), "Reading model parameter from file...\n");

        switch (config.get<IndexType>("ModelParametrisation")) {
        case 1:
            init(ctx, dist, config.get<std::string>("ModelFilename"), config.get<IndexType>("PartitionedIn"));
            break;
        case 2:
            initVelocities(ctx, dist, config.get<std::string>("ModelFilename"), config.get<IndexType>("PartitionedIn"));
            break;
        default:
            COMMON_THROWEXCEPTION(" Unkown ModelParametrisation value! ")
            break;
        }

        HOST_PRINT(dist->getCommunicatorPtr(), "Finished with reading of the model parameter!\n\n");

    } else {
        ValueType getPWaveModulus = config.get<ValueType>("rho") * config.get<ValueType>("velocityP") * config.get<ValueType>("velocityP");
        ValueType getSWaveModulus = config.get<ValueType>("rho") * config.get<ValueType>("velocityS") * config.get<ValueType>("velocityS");
        init(ctx, dist, getPWaveModulus, getSWaveModulus, config.get<ValueType>("rho"));
    }

    if (config.get<IndexType>("ModelWrite")) {
        write(config.get<std::string>("ModelFilename") + ".out", config.get<IndexType>("PartitionedOut"));
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
template <typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType>::Elastic(scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist, scai::lama::Scalar pWaveModulus_const, scai::lama::Scalar sWaveModulus_const, scai::lama::Scalar rho)
{
    init(ctx, dist, pWaveModulus_const, sWaveModulus_const, rho);
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
template <typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::init(scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist, scai::lama::Scalar pWaveModulus_const, scai::lama::Scalar sWaveModulus_const, scai::lama::Scalar rho)
{
    parametrisation = 0;
    this->initModelparameter(pWaveModulus, ctx, dist, pWaveModulus_const);
    this->initModelparameter(sWaveModulus, ctx, dist, sWaveModulus_const);
    this->initModelparameter(density, ctx, dist, rho);
}

/*! \brief Constructor that is reading models from external files
 *
 *  Reads a model from an external file.
 \param ctx Context
 \param dist Distribution
 \param filenamePWaveModulus Name of file that will be read for the P-wave modulus.
 \param filenameSWaveModulus Name of file that will be read for the S-wave modulus.
 \param filenamerho Name of file that will be read for the Density.
 \param partitionedIn Partitioned input
 */
template <typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType>::Elastic(scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist, std::string filenamePWaveModulus, std::string filenameSWaveModulus, std::string filenamerho, IndexType partitionedIn)
{
    init(ctx, dist, filenamePWaveModulus, filenameSWaveModulus, filenamerho, partitionedIn);
}

/*! \brief Initialisation that is reading models from external files
 *
 *  Reads a model from an external file.
 \param ctx Context
 \param dist Distribution
 \param filenamePWaveModulus Name of file that will be read for the P-wave modulus.
 \param filenameSWaveModulus Name of file that will be read for the S-wave modulus.
 \param filenamerho Name of file that will be read for the Density.
 \param partitionedIn Partitioned input
 */
template <typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::init(scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist, std::string filenamePWaveModulus, std::string filenameSWaveModulus, std::string filenamerho, IndexType partitionedIn)
{
    parametrisation = 0;
    this->initModelparameter(pWaveModulus, ctx, dist, filenamePWaveModulus, partitionedIn);
    this->initModelparameter(density, ctx, dist, filenamerho, partitionedIn);
    this->initModelparameter(sWaveModulus, ctx, dist, filenameSWaveModulus, partitionedIn);
}

/*! \brief Constructor that is reading models from external files
 *
 *  Reads a model from an external file.
 \param ctx Context
 \param dist Distribution
 \param filename For the P-wave modulus ".pWaveModulus.mtx" is added, for the second ".sWaveModulus.mtx" and for density ".density.mtx" is added.
 \param partitionedIn Partitioned input
 */
template <typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType>::Elastic(scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist, std::string filename, IndexType partitionedIn)
{
    init(ctx, dist, filename, partitionedIn);
}

/*! \brief Initialisator that is reading models from external files
 *
 *  Reads a model from an external file.
 \param ctx Context
 \param dist Distribution
 \param filename For the P-wave modulus ".pWaveModulus.mtx" is added, for the second ".sWaveModulus.mtx" and for density ".density.mtx" is added.
 \param partitionedIn Partitioned input
 */
template <typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::init(scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist, std::string filename, IndexType partitionedIn)
{
    parametrisation = 0;
    std::string filenamePWaveModulus = filename + ".pWaveModulus.mtx";
    std::string filenameSWaveModulus = filename + ".sWaveModulus.mtx";
    std::string filenamedensity = filename + ".density.mtx";

    this->initModelparameter(pWaveModulus, ctx, dist, filenamePWaveModulus, partitionedIn);
    this->initModelparameter(sWaveModulus, ctx, dist, filenameSWaveModulus, partitionedIn);
    this->initModelparameter(density, ctx, dist, filenamedensity, partitionedIn);
}

//! \brief Copy constructor
template <typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType>::Elastic(const Elastic &rhs)
{
    pWaveModulus = rhs.pWaveModulus;
    sWaveModulus = rhs.sWaveModulus;
    velocityP = rhs.velocityP;
    velocityS = rhs.velocityS;
    density = rhs.density;
    dirtyFlagInverseDensity = rhs.dirtyFlagInverseDensity;
    dirtyFlagModulus = rhs.dirtyFlagModulus;
    dirtyFlagVelocity = rhs.dirtyFlagVelocity;
    parametrisation = rhs.parametrisation;
    inverseDensity = rhs.inverseDensity;
}

/*! \brief Initialisator that is reading Velocity-Vector
 *
 *  Reads a model from an external file.
 \param ctx Context
 \param dist Distribution
 \param filename For the Velocity-Vector "filename".vp.mtx" and "filename".vs.mtx" is added and for density "filename+".density.mtx" is added.
 \param partitionedIn Partitioned input
 *
 */
template <typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::initVelocities(scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist, std::string filename, IndexType partitionedIn)
{
    parametrisation = 1;
    std::string filenameVelocityP = filename + ".vp.mtx";
    std::string filenameVelocityS = filename + ".vs.mtx";
    std::string filenamedensity = filename + ".density.mtx";

    this->initModelparameter(velocityP, ctx, dist, filenameVelocityP, partitionedIn);
    this->initModelparameter(velocityS, ctx, dist, filenameVelocityS, partitionedIn);
    this->initModelparameter(density, ctx, dist, filenamedensity, partitionedIn);
}

/*! \brief Write model to an external file
 *
 \param filenameP Filename for P-wave modulus / P-wave velocity model
 \param filenameS Filename for S-wave modulus / S-wave velocity model
 \param filenamedensity Filename for Density model
 \param partitionedOut Partitioned output
 */
template <typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::write(std::string filenameP, std::string filenameS, std::string filenamedensity, IndexType partitionedOut) const
{
    SCAI_ASSERT_DEBUG(parametrisation == 0 || parametrisation == 1, "Unkown parametrisation");

    this->writeModelparameter(density, filenamedensity, partitionedOut);

    switch (parametrisation) {
    case 0:
        this->writeModelparameter(pWaveModulus, filenameP, partitionedOut);
        this->writeModelparameter(sWaveModulus, filenameS, partitionedOut);
        break;
    case 1:
        this->writeModelparameter(velocityP, filenameP, partitionedOut);
        this->writeModelparameter(velocityS, filenameS, partitionedOut);
        break;
    }
};

/*! \brief Write model to an external file
 *
 \param filename For the P-wave modulus ".pWaveModulus.mtx" is added, for the second ".sWaveModulus.mtx" and for density ".density.mtx" is added.
 \param partitionedOut Partitioned output
 */
template <typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::write(std::string filename, IndexType partitionedOut) const
{
    SCAI_ASSERT_DEBUG(parametrisation == 0 || parametrisation == 1, "Unkown parametrisation");

    std::string filenameP;
    std::string filenameS;
    std::string filenamedensity = filename + ".density.mtx";

    switch (parametrisation) {
    case 0:
        filenameP = filename + ".pWaveModulus.mtx";
        filenameS = filename + ".sWaveModulus.mtx";
        break;
    case 1:
        filenameP = filename + ".vp.mtx";
        filenameS = filename + ".vs.mtx";
        break;
    }

    write(filenameP, filenameS, filenamedensity, partitionedOut);
};

//! \brief Wrapper to support configuration
/*!
 *
 \param dist Distribution of the wavefield
 \param ctx Context
 \param config Configuration
 \param comm Communicator
 */
template <typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::initializeMatrices(scai::dmemo::DistributionPtr dist, scai::hmemo::ContextPtr ctx, Configuration::Configuration config, scai::dmemo::CommunicatorPtr comm)
{
    initializeMatrices(dist, ctx, config.get<IndexType>("NX"), config.get<IndexType>("NY"), config.get<IndexType>("NZ"), config.get<ValueType>("DH"), config.get<ValueType>("DT"), comm);
}

//! \brief Initializsation of the Averaging matrices
/*!
 *
 \param dist Distribution of the wavefield
 \param ctx Context
 \param NX Total number of grid points in X
 \param NY Total number of grid points in Y
 \param NZ Total number of grid points in Z
 \param DH Grid spacing (equidistant)
 \param DT Temporal sampling interval
 \param comm Communicator
 */
template <typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::initializeMatrices(scai::dmemo::DistributionPtr dist, scai::hmemo::ContextPtr ctx, IndexType NX, IndexType NY, IndexType NZ, ValueType /*DH*/, ValueType /*DT*/, scai::dmemo::CommunicatorPtr /*comm*/)
{

    SCAI_REGION("initializeMatrices")

    this->calcDensityAverageMatrixX(NX, NY, NZ, dist);
    this->calcDensityAverageMatrixY(NX, NY, NZ, dist);
    this->calcDensityAverageMatrixZ(NX, NY, NZ, dist);
    this->calcSWaveModulusAverageMatrixXY(NX, NY, NZ, dist);
    this->calcSWaveModulusAverageMatrixXZ(NX, NY, NZ, dist);
    this->calcSWaveModulusAverageMatrixYZ(NX, NY, NZ, dist);

    DensityAverageMatrixX.setContextPtr(ctx);
    DensityAverageMatrixY.setContextPtr(ctx);
    DensityAverageMatrixZ.setContextPtr(ctx);
    sWaveModulusAverageMatrixXY.setContextPtr(ctx);
    sWaveModulusAverageMatrixXZ.setContextPtr(ctx);
    sWaveModulusAverageMatrixYZ.setContextPtr(ctx);
}

/*! \brief calculate averaged vectors
 *
 */
template <typename ValueType>
void KITGPI::Modelparameter::Elastic<ValueType>::calculateAveraging()
{

    this->calculateInverseAveragedDensity(density, inverseDensityAverageX, DensityAverageMatrixX);
    this->calculateInverseAveragedDensity(density, inverseDensityAverageY, DensityAverageMatrixY);
    this->calculateInverseAveragedDensity(density, inverseDensityAverageZ, DensityAverageMatrixZ);
    this->calculateAveragedSWaveModulus(sWaveModulus, sWaveModulusAverageXY, sWaveModulusAverageMatrixXY);
    this->calculateAveragedSWaveModulus(sWaveModulus, sWaveModulusAverageXZ, sWaveModulusAverageMatrixXZ);
    this->calculateAveragedSWaveModulus(sWaveModulus, sWaveModulusAverageYZ, sWaveModulusAverageMatrixYZ);
    dirtyFlagAveraging = false;
}

/*! \brief Get reference to tauP
 *
 */
template <typename ValueType>
scai::lama::Vector const &KITGPI::Modelparameter::Elastic<ValueType>::getTauP()
{
    COMMON_THROWEXCEPTION("There is no tau parameter in an elastic modelling")
    return (tauP);
}

/*! \brief Get reference to tauS
 */
template <typename ValueType>
scai::lama::Vector const &KITGPI::Modelparameter::Elastic<ValueType>::getTauS()
{
    COMMON_THROWEXCEPTION("There is no tau parameter in an elastic modelling")
    return (tauS);
}

/*! \brief Getter method for relaxation frequency */
template <typename ValueType>
ValueType KITGPI::Modelparameter::Elastic<ValueType>::getRelaxationFrequency() const
{
    COMMON_THROWEXCEPTION("There is no relaxationFrequency parameter in an elastic modelling")
    return (relaxationFrequency);
}

/*! \brief Getter method for number of relaxation mechanisms */
template <typename ValueType>
IndexType KITGPI::Modelparameter::Elastic<ValueType>::getNumRelaxationMechanisms() const
{
    COMMON_THROWEXCEPTION("There is no numRelaxationMechanisms parameter in an elastic modelling")
    return (numRelaxationMechanisms);
}

/*! \brief Get reference to tauS xy-plane
 */
template <typename ValueType>
scai::lama::Vector const &KITGPI::Modelparameter::Elastic<ValueType>::getTauSAverageXY()
{
    COMMON_THROWEXCEPTION("There is no averaged tau parameter in an elastic modelling")
    return (tauSAverageXY);
}

/*! \brief Get reference to tauS xz-plane
 */
template <typename ValueType>
scai::lama::Vector const &KITGPI::Modelparameter::Elastic<ValueType>::getTauSAverageXZ()
{
    COMMON_THROWEXCEPTION("There is no averaged tau parameter in an elastic modelling")
    return (tauSAverageXZ);
}

/*! \brief Get reference to tauS yz-plane
 */
template <typename ValueType>
scai::lama::Vector const &KITGPI::Modelparameter::Elastic<ValueType>::getTauSAverageYZ()
{
    COMMON_THROWEXCEPTION("There is no averaged tau parameter in an elastic modelling")
    return (tauSAverageYZ);
}

/*! \brief Overloading * Operation
 *
 \param rhs Scalar factor with which the vectors are multiplied.
 */
template <typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType> KITGPI::Modelparameter::Elastic<ValueType>::operator*(scai::lama::Scalar rhs)
{
    KITGPI::Modelparameter::Elastic<ValueType> result;
    result.density = this->density * rhs;
    if (parametrisation == 0) {
        result.pWaveModulus = this->pWaveModulus * rhs;
        result.sWaveModulus = this->sWaveModulus * rhs;
        return result;
    }
    if (parametrisation == 1) {
        result.velocityP = this->velocityP * rhs;
        result.velocityS = this->velocityS * rhs;
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
template <typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType> operator*(scai::lama::Scalar lhs, KITGPI::Modelparameter::Elastic<ValueType> rhs)
{
    return rhs * lhs;
}

/*! \brief Overloading *= Operation
 *
 \param rhs Scalar factor with which the vectors are multiplied.
 */
template <typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType> KITGPI::Modelparameter::Elastic<ValueType>::operator*=(scai::lama::Scalar rhs)
{
    return *this * rhs;
}

/*! \brief Overloading + Operation
 *
 \param rhs Model which is added.
 */
template <typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType> KITGPI::Modelparameter::Elastic<ValueType>::operator+(KITGPI::Modelparameter::Elastic<ValueType> rhs)
{
    KITGPI::Modelparameter::Elastic<ValueType> result;
    result.density = this->density + rhs.density;
    if (parametrisation == 0) {
        result.pWaveModulus = this->pWaveModulus + rhs.pWaveModulus;
        result.sWaveModulus = this->sWaveModulus + rhs.sWaveModulus;
        return result;
    }
    if (parametrisation == 1) {
        result.velocityP = this->velocityP + rhs.velocityP;
        result.velocityS = this->velocityS + rhs.velocityS;
        return result;
    } else {
        COMMON_THROWEXCEPTION(" Unknown parametrisation! ");
    }
}

/*! \brief Overloading += Operation
 *
 \param rhs Model which is added.
 */
template <typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType> KITGPI::Modelparameter::Elastic<ValueType>::operator+=(KITGPI::Modelparameter::Elastic<ValueType> rhs)
{
    return *this + rhs;
}

/*! \brief Overloading - Operation
 *
 \param rhs Model which is subtractet.
 */
template <typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType> KITGPI::Modelparameter::Elastic<ValueType>::operator-(KITGPI::Modelparameter::Elastic<ValueType> rhs)
{
    KITGPI::Modelparameter::Elastic<ValueType> result;
    result.density = this->density - rhs.density;
    if (parametrisation == 0) {
        result.pWaveModulus = this->pWaveModulus - rhs.pWaveModulus;
        result.sWaveModulus = this->sWaveModulus - rhs.sWaveModulus;
        return result;
    }
    if (parametrisation == 1) {
        result.velocityP = this->velocityP - rhs.velocityP;
        result.velocityS = this->velocityS - rhs.velocityS;
        return result;
    } else {
        COMMON_THROWEXCEPTION(" Unknown parametrisation! ");
    }
}

/*! \brief Overloading -= Operation
 *
 \param rhs Model which is subtractet.
 */
template <typename ValueType>
KITGPI::Modelparameter::Elastic<ValueType> KITGPI::Modelparameter::Elastic<ValueType>::operator-=(KITGPI::Modelparameter::Elastic<ValueType> rhs)
{
    return *this - rhs;
}

template class KITGPI::Modelparameter::Elastic<float>;
template class KITGPI::Modelparameter::Elastic<double>;