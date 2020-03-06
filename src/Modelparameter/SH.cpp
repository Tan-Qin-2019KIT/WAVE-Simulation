#include "SH.hpp"
#include "../Acquisition/AcquisitionSettings.hpp"
#include "../IO/IO.hpp"

using namespace scai;

/*! \brief estimate sum of the memory of all model parameters
 *
 \param dist Distribution
 */
template <typename ValueType>
ValueType KITGPI::Modelparameter::SH<ValueType>::estimateMemory(dmemo::DistributionPtr dist)
{
    /* 6 Parameter in SH modeling:  rho, Vs, invRho, sWaveModulus, sWaveModulusXZ, sWaveModulus YZ */
    IndexType numParameter = 6;
    return (this->getMemoryUsage(dist, numParameter));
}

/*! \brief Prepare modellparameter for modelling
 *
 * Refreshes the modulus, calculates inverse density and average Values on staggered grid
 *
 \param modelCoordinates Coordinate class object
 \param ctx Context for the Calculation
 \param dist Distribution
 \param comm Communicator pointer
 */
template <typename ValueType>
void KITGPI::Modelparameter::SH<ValueType>::prepareForModelling(Acquisition::Coordinates<ValueType> const &modelCoordinates, scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist, scai::dmemo::CommunicatorPtr comm)
{
    HOST_PRINT(comm, "", "Preparation of the model parameters\n");

    // refreshModulus();
    this->getSWaveModulus();
    //sWaveModulus is not needed after avereging. The memory should be freed after calculating the averaging.
    initializeMatrices(dist, ctx, modelCoordinates, comm);
    this->getInverseDensity();
    calculateAveraging();
    purgeMatrices();
    HOST_PRINT(comm, "", "Model ready!\n\n");
}

/*! \brief Apply thresholds to model parameters
 \param config Configuration class
 */
template <typename ValueType>
void KITGPI::Modelparameter::SH<ValueType>::applyThresholds(Configuration::Configuration const &config)
{
    lama::DenseVector<ValueType> mask(velocityS); //mask to restore vacuum
    mask.unaryOp(mask, common::UnaryOp::SIGN);
    mask.unaryOp(mask, common::UnaryOp::ABS);

    Common::searchAndReplace<ValueType>(velocityS, config.get<ValueType>("lowerVPTh"), config.get<ValueType>("lowerVPTh"), 1);
    Common::searchAndReplace<ValueType>(velocityS, config.get<ValueType>("upperVPTh"), config.get<ValueType>("upperVPTh"), 2);
    dirtyFlagSWaveModulus = true; // the modulus vector is now dirty

    Common::searchAndReplace<ValueType>(density, config.get<ValueType>("lowerDensityTh"), config.get<ValueType>("lowerDensityTh"), 1);
    Common::searchAndReplace<ValueType>(density, config.get<ValueType>("upperDensityTh"), config.get<ValueType>("upperDensityTh"), 2);
    dirtyFlagInverseDensity = true; // If density will be changed, the inverse has to be refreshed if it is accessed
    dirtyFlagAveraging = true;      // If S-Wave velocity will be changed, averaging needs to be redone

    velocityS *= mask;
    density *= mask;
}

/*! \brief If stream configuration is used, get a subset model from the big model
 \param modelSubset subset model
 \param modelCoordinates coordinate class object of the subset
 \param modelCoordinatesBig coordinate class object of the big model
 */
template <typename ValueType>
void KITGPI::Modelparameter::SH<ValueType>::getModelSubset(KITGPI::Modelparameter::Modelparameter<ValueType> &modelSubset, Acquisition::Coordinates<ValueType> const &modelCoordinates, Acquisition::Coordinates<ValueType> const &modelCoordinatesBig, std::vector<Acquisition::coordinate3D> cutCoord, scai::IndexType cutCoordInd)
{
    auto distBig = velocityP.getDistributionPtr();
    auto dist = modelSubset.getVelocityP().getDistributionPtr();
//     auto comm = dist.getCommunicatorPtr();

    scai::lama::CSRSparseMatrix<ValueType> shrinkMatrix = this->getShrinkMatrix(dist,distBig,modelCoordinates,modelCoordinatesBig,cutCoord.at(cutCoordInd));
    
    lama::DenseVector<ValueType> temp;
    temp = modelSubset.getVelocityP();
    
    temp = shrinkMatrix*velocityP;
    modelSubset.setVelocityP(temp);
        
    temp = shrinkMatrix*velocityS;
    modelSubset.setVelocityS(temp);
//    IO::writeVector(temp, "model/usedSubset_" + std::to_string(cutCoordInd) + ".vs", 2);
    
    temp = shrinkMatrix*density;
    modelSubset.setDensity(temp);
}

/*! \brief If stream configuration is used, get a subset model from the big model
 \param modelSubset subset model
 \param modelCoordinates coordinate class object of the subset
 \param modelCoordinatesBig coordinate class object of the big model
 */
template <typename ValueType>
void KITGPI::Modelparameter::SH<ValueType>::setModelSubset(KITGPI::Modelparameter::Modelparameter<ValueType> &invertedModelSubset, Acquisition::Coordinates<ValueType> const &modelCoordinates, Acquisition::Coordinates<ValueType> const &modelCoordinatesBig, std::vector<Acquisition::coordinate3D> cutCoord, scai::IndexType cutCoordInd, scai::IndexType smoothRange, scai::IndexType NX, scai::IndexType NY, scai::IndexType NXBig, scai::IndexType NYBig, scai::IndexType boundaryWidth)
{
    auto distBig = velocityP.getDistributionPtr();
    auto dist = invertedModelSubset.getVelocityP().getDistributionPtr();
//     auto comm = dist.getCommunicatorPtr();

    scai::lama::CSRSparseMatrix<ValueType> shrinkMatrix = this->getShrinkMatrix(dist,distBig,modelCoordinates,modelCoordinatesBig,cutCoord.at(cutCoordInd));
    shrinkMatrix.assignTranspose(shrinkMatrix);
    
    scai::lama::SparseVector<ValueType> eraseVector = this->getEraseVector(dist,distBig,modelCoordinates,modelCoordinatesBig,cutCoord.at(cutCoordInd),NX,NYBig,boundaryWidth);
    
    lama::DenseVector<ValueType> temp;
    temp = invertedModelSubset.getVelocityP(); //results of inverted subset model
    //damp the boundary borders
    for (IndexType y = 0; y < NY; y++) {
        for (IndexType i = 0; i < boundaryWidth; i++) {
            temp[modelCoordinates.coordinate2index(i, y, 0)] = temp[modelCoordinates.coordinate2index(i, y, 0)]*(i+1)/boundaryWidth;
            temp[modelCoordinates.coordinate2index(NX-1-i, y, 0)] = temp[modelCoordinates.coordinate2index(NX-1-i, y, 0)]*(i+1)/boundaryWidth;
        }
    }
    temp = shrinkMatrix*temp; //transform subset into big model
    velocityP *= eraseVector;
    velocityP += temp; //take over the values
  
    temp = invertedModelSubset.getVelocityS(); //results of inverted subset model
    //damp the boundary borders
    for (IndexType y = 0; y < NY; y++) {
        for (IndexType i = 0; i < boundaryWidth; i++) {
            temp[modelCoordinates.coordinate2index(i, y, 0)] = temp[modelCoordinates.coordinate2index(i, y, 0)]*(i+1)/boundaryWidth;
            temp[modelCoordinates.coordinate2index(NX-1-i, y, 0)] = temp[modelCoordinates.coordinate2index(NX-1-i, y, 0)]*(i+1)/boundaryWidth;
        }
    }
    temp = shrinkMatrix*temp; //transform subset into big model
    velocityS *= eraseVector;
    velocityS += temp; //take over the values
    
    scai::lama::DenseVector<ValueType> smoothParameter = this->smoothParameter(modelCoordinatesBig, velocityS, cutCoord.at(cutCoordInd), smoothRange, NX, NXBig, NYBig);
     velocityS = smoothParameter;
//    IO::writeVector(velocityS, "model/setSubset_" + std::to_string(cutCoordInd) + ".vs" ,2);
    
    temp = invertedModelSubset.getDensity(); //results of inverted subset model
    //damp the boundary borders
    for (IndexType y = 0; y < NY; y++) {
        for (IndexType i = 0; i < boundaryWidth; i++) {
            temp[modelCoordinates.coordinate2index(i, y, 0)] = temp[modelCoordinates.coordinate2index(i, y, 0)]*(i+1)/boundaryWidth;
            temp[modelCoordinates.coordinate2index(NX-1-i, y, 0)] = temp[modelCoordinates.coordinate2index(NX-1-i, y, 0)]*(i+1)/boundaryWidth;
        }
    }
    temp = shrinkMatrix*temp; //transform subset into big model
    density *= eraseVector;
    density += temp; //take over the values
}

/*! \brief Constructor that is using the Configuration class
 *
 \param config Configuration class
 \param ctx Context for the Calculation
 \param dist Distribution
 */
template <typename ValueType>
KITGPI::Modelparameter::SH<ValueType>::SH(Configuration::Configuration const &config, scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist, Acquisition::Coordinates<ValueType> const &modelCoordinates)
{
    equationType = "sh";
    init(config, ctx, dist, modelCoordinates);
}

/*! \brief Initialisation that is using the Configuration class
 *
 \param config Configuration class
 \param ctx Context for the Calculation
 \param dist Distribution
 */
template <typename ValueType>
void KITGPI::Modelparameter::SH<ValueType>::init(Configuration::Configuration const &config, scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist, Acquisition::Coordinates<ValueType> const &modelCoordinates)
{
    SCAI_ASSERT(config.get<IndexType>("ModelRead") != 2, "Read variable model not available for SH, variable grid is not available here!")
    
    if (config.get<IndexType>("ModelRead") == 1) {

        HOST_PRINT(dist->getCommunicatorPtr(), "", "Reading model parameter (SH) from file...\n");

        init(ctx, dist, config.get<std::string>("ModelFilename"), config.get<IndexType>("FileFormat"));

        HOST_PRINT(dist->getCommunicatorPtr(), "", "Finished with reading of the model parameter!\n\n");

    } else {
        init(ctx, dist, config.get<ValueType>("velocityS"), config.get<ValueType>("rho"));
    }

}

/*! \brief Constructor that is generating a homogeneous model
 *
 *  Generates a homogeneous model, which will be initialized by the two given values.
 \param ctx Context
 \param dist Distribution
 \param sWaveModulus_const S-wave modulus given as
 \param rho Density given as ValueType
 */
template <typename ValueType>
KITGPI::Modelparameter::SH<ValueType>::SH(scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist, ValueType velocityS_const, ValueType rho)
{
    equationType = "sh";
    init(ctx, dist, velocityS_const, rho);
}

/*! \brief Initialisation that is generating a homogeneous model
 *
 *  Generates a homogeneous model, which will be initialized by the two given values.
 \param ctx Context
 \param dist Distribution
 \param sWaveModulus_const S-wave modulus given as ValueType
 \param rho Density given as ValueType
 */
template <typename ValueType>
void KITGPI::Modelparameter::SH<ValueType>::init(scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist, ValueType velocityS_const, ValueType rho_const)
{
    this->initModelparameter(velocityS, ctx, dist, velocityS_const);
    this->initModelparameter(density, ctx, dist, rho_const);
}

/*! \brief Constructor that is reading models from external files
 *
 *  Reads a model from an external file.
 \param ctx Context
 \param dist Distribution
 \param filename base filenam of the model
 \param fileFormat Input file format 1=mtx 2=lmf
 */
template <typename ValueType>
KITGPI::Modelparameter::SH<ValueType>::SH(scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist, std::string filename, IndexType fileFormat)
{
    equationType = "sh";
    init(ctx, dist, filename, fileFormat);
}

/*! \brief Initialisator that is reading models from external files
 *
 *  Reads a model from an external file.
 \param ctx Context
 \param dist Distribution
 \param filename base filename of the model
 \param fileFormat Input file format 1=mtx 2=lmf
 */
template <typename ValueType>
void KITGPI::Modelparameter::SH<ValueType>::init(scai::hmemo::ContextPtr ctx, scai::dmemo::DistributionPtr dist, std::string filename, IndexType fileFormat)
{
    this->initModelparameter(velocityS, ctx, dist, filename + ".vs", fileFormat);
    this->initModelparameter(density, ctx, dist, filename + ".density", fileFormat);
}

//! \brief Copy constructor
template <typename ValueType>
KITGPI::Modelparameter::SH<ValueType>::SH(const SH &rhs)
{
    equationType = rhs.equationType;
    sWaveModulus = rhs.sWaveModulus;
    velocityS = rhs.velocityS;
    density = rhs.density;
    inverseDensity = rhs.inverseDensity;
    dirtyFlagInverseDensity = rhs.dirtyFlagInverseDensity;
    dirtyFlagSWaveModulus = rhs.dirtyFlagSWaveModulus;
}

/*! \brief Write model to an external file
 *base filename of the model
 \param fileFormat Output file format 1=mtx 2=lmf
 */
template <typename ValueType>
void KITGPI::Modelparameter::SH<ValueType>::write(std::string filename, IndexType fileFormat) const
{
    IO::writeVector(density, filename + ".density", fileFormat);
    IO::writeVector(velocityS, filename + ".vs", fileFormat);
};

//! \brief Initializsation of the Averaging matrices
/*!
 *
 \param dist Distribution of the wavefield
 \param ctx Context
 \param comm Communicator
 */
template <typename ValueType>
void KITGPI::Modelparameter::SH<ValueType>::initializeMatrices(scai::dmemo::DistributionPtr dist, scai::hmemo::ContextPtr ctx, Acquisition::Coordinates<ValueType> const &modelCoordinates, scai::dmemo::CommunicatorPtr /*comm*/)
{

    SCAI_REGION("Modelparameter.SH.initializeMatrices")
    // reuse of density average for the swavemodulus : sxz and syz are on the same spot as vx and vy in acoustic modeling
    this->calcAverageMatrixX(modelCoordinates, dist);
    this->calcAverageMatrixY(modelCoordinates, dist);

    averageMatrixX.setContextPtr(ctx);
    averageMatrixY.setContextPtr(ctx);
}

//! \brief Purge Averaging matrices to free memory
template <typename ValueType>
void KITGPI::Modelparameter::SH<ValueType>::purgeMatrices()
{
    averageMatrixX.purge();
    averageMatrixY.purge();
}

/*! \brief calculate averaged vectors
 *
 */
template <typename ValueType>
void KITGPI::Modelparameter::SH<ValueType>::calculateAveraging()
{
    if (dirtyFlagAveraging) {
        SCAI_REGION("Modelparameter.SH.calculateAveraging")
        // sxz and syz are on the same spot as vx and vy in acoustic modeling
        this->calculateAveragedSWaveModulus(sWaveModulus, sWaveModulusAverageXZ, averageMatrixX);
        this->calculateAveragedSWaveModulus(sWaveModulus, sWaveModulusAverageYZ, averageMatrixY);
        dirtyFlagAveraging = false;
    }
}

/*! \brief Get equationType (sh)
 */
template <typename ValueType>
std::string KITGPI::Modelparameter::SH<ValueType>::getEquationType() const
{
    return (equationType);
}

/*! \brief Get reference to tauP
 *
 */
template <typename ValueType>
scai::lama::DenseVector<ValueType> const &KITGPI::Modelparameter::SH<ValueType>::getVelocityP() const
{
    COMMON_THROWEXCEPTION("There is no velocityP parameter in an sh modelling")
    return (velocityP);
}

/*! \brief Get reference to tauP
 *
 */
template <typename ValueType>
scai::lama::Vector<ValueType> const &KITGPI::Modelparameter::SH<ValueType>::getPWaveModulus()
{
    COMMON_THROWEXCEPTION("There is no pWaveModulus parameter in an sh modelling")
    return (pWaveModulus);
}

/*! \brief Get reference to tauP
 *
 */
template <typename ValueType>
scai::lama::Vector<ValueType> const &KITGPI::Modelparameter::SH<ValueType>::getPWaveModulus() const
{
    COMMON_THROWEXCEPTION("There is no pWaveModulus parameter in an sh modelling")
    return (pWaveModulus);
}

/*! \brief Get reference to tauP
 *
 */
template <typename ValueType>
scai::lama::Vector<ValueType> const &KITGPI::Modelparameter::SH<ValueType>::getTauP() const
{
    COMMON_THROWEXCEPTION("There is no tau parameter in an sh modelling")
    return (tauP);
}

/*! \brief Get reference to tauS
 */
template <typename ValueType>
scai::lama::Vector<ValueType> const &KITGPI::Modelparameter::SH<ValueType>::getTauS() const
{
    COMMON_THROWEXCEPTION("There is no tau parameter in an sh modelling")
    return (tauS);
}

/*! \brief Getter method for relaxation frequency */
template <typename ValueType>
ValueType KITGPI::Modelparameter::SH<ValueType>::getRelaxationFrequency() const
{
    COMMON_THROWEXCEPTION("There is no relaxationFrequency parameter in an sh modelling")
    return (relaxationFrequency);
}

/*! \brief Getter method for number of relaxation mechanisms */
template <typename ValueType>
IndexType KITGPI::Modelparameter::SH<ValueType>::getNumRelaxationMechanisms() const
{
    COMMON_THROWEXCEPTION("There is no numRelaxationMechanisms parameter in an sh modelling")
    return (numRelaxationMechanisms);
}

/*! \brief Get reference to tauS xy-plane
 */
template <typename ValueType>
scai::lama::Vector<ValueType> const &KITGPI::Modelparameter::SH<ValueType>::getTauSAverageXY()
{
    COMMON_THROWEXCEPTION("There is no averaged tau parameter in an sh modelling")
    return (tauSAverageXY);
}

/*! \brief Get reference to tauS xy-plane
 */
template <typename ValueType>
scai::lama::Vector<ValueType> const &KITGPI::Modelparameter::SH<ValueType>::getTauSAverageXY() const
{
    COMMON_THROWEXCEPTION("There is no averaged tau parameter in an sh modelling")
    return (tauSAverageXY);
}

/*! \brief Get reference to tauS xz-plane
 */
template <typename ValueType>
scai::lama::Vector<ValueType> const &KITGPI::Modelparameter::SH<ValueType>::getTauSAverageXZ()
{
    COMMON_THROWEXCEPTION("There is no averaged tau parameter in an sh modelling")
    return (tauSAverageXZ);
}

/*! \brief Get reference to tauS xz-plane
 */
template <typename ValueType>
scai::lama::Vector<ValueType> const &KITGPI::Modelparameter::SH<ValueType>::getTauSAverageXZ() const
{
    COMMON_THROWEXCEPTION("There is no averaged tau parameter in an sh modelling")
    return (tauSAverageXZ);
}

/*! \brief Get reference to tauS yz-plane
 */
template <typename ValueType>
scai::lama::Vector<ValueType> const &KITGPI::Modelparameter::SH<ValueType>::getTauSAverageYZ()
{
    COMMON_THROWEXCEPTION("There is no averaged tau parameter in an sh modelling")
    return (tauSAverageYZ);
}

/*! \brief Get reference to tauS yz-plane
 */
template <typename ValueType>
scai::lama::Vector<ValueType> const &KITGPI::Modelparameter::SH<ValueType>::getTauSAverageYZ() const
{
    COMMON_THROWEXCEPTION("There is no averaged tau parameter in an sh modelling")
    return (tauSAverageYZ);
}
/*! \brief Overloading * Operation
 *
 \param rhs ValueType factor with which the vectors are multiplied.
 */
template <typename ValueType>
KITGPI::Modelparameter::SH<ValueType> KITGPI::Modelparameter::SH<ValueType>::operator*(ValueType rhs)
{
    KITGPI::Modelparameter::SH<ValueType> result(*this);
    result *= rhs;
    return result;
}

/*! \brief free function to multiply
 *
 \param lhs ValueType factor with which the vectors are multiplied.
 \param rhs Vector
 */
template <typename ValueType>
KITGPI::Modelparameter::SH<ValueType> operator*(ValueType lhs, KITGPI::Modelparameter::SH<ValueType> const &rhs)
{
    return rhs * lhs;
}

/*! \brief Overloading *= Operation
 *
 \param rhs valueType factor with which the vectors are multiplied.
 */
template <typename ValueType>
KITGPI::Modelparameter::SH<ValueType> &KITGPI::Modelparameter::SH<ValueType>::operator*=(ValueType const &rhs)
{
    density *= rhs;
    velocityS *= rhs;

    dirtyFlagInverseDensity = true;
    dirtyFlagSWaveModulus = true;
    dirtyFlagAveraging = true;
    return *this;
}

/*! \brief Overloading + Operation
 *
 \param rhs Model which is added.
 */
template <typename ValueType>
KITGPI::Modelparameter::SH<ValueType> KITGPI::Modelparameter::SH<ValueType>::operator+(KITGPI::Modelparameter::SH<ValueType> const &rhs)
{
    KITGPI::Modelparameter::SH<ValueType> result(*this);
    result += rhs;
    return result;
}

/*! \brief Overloading += Operation
 *
 \param rhs Model which is added.
 */
template <typename ValueType>
KITGPI::Modelparameter::SH<ValueType> &KITGPI::Modelparameter::SH<ValueType>::operator+=(KITGPI::Modelparameter::SH<ValueType> const &rhs)
{
    density += rhs.density;
    velocityS += rhs.velocityS;

    dirtyFlagInverseDensity = true;
    dirtyFlagSWaveModulus = true;
    dirtyFlagAveraging = true;
    return *this;
}

/*! \brief Overloading - Operation
 *
 \param rhs Model which is subtractet.
 */
template <typename ValueType>
KITGPI::Modelparameter::SH<ValueType> KITGPI::Modelparameter::SH<ValueType>::operator-(KITGPI::Modelparameter::SH<ValueType> const &rhs)
{
    KITGPI::Modelparameter::SH<ValueType> result(*this);
    result -= rhs;
    return result;
}

/*! \brief Overloading -= Operation
 *
 \param rhs Model which is subtractet.
 */
template <typename ValueType>
KITGPI::Modelparameter::SH<ValueType> &KITGPI::Modelparameter::SH<ValueType>::operator-=(KITGPI::Modelparameter::SH<ValueType> const &rhs)
{
    density = density -= rhs.density;
    velocityS -= rhs.velocityS;

    dirtyFlagInverseDensity = true;
    dirtyFlagSWaveModulus = true;
    dirtyFlagAveraging = true;
    return *this;
}

/*! \brief Overloading = Operation
 *
 \param rhs Model which is copied.
 */
template <typename ValueType>
KITGPI::Modelparameter::SH<ValueType> &KITGPI::Modelparameter::SH<ValueType>::operator=(KITGPI::Modelparameter::SH<ValueType> const &rhs)
{
    velocityS = rhs.velocityS;
    density = rhs.density;
    dirtyFlagInverseDensity = true;
    dirtyFlagSWaveModulus = true;
    dirtyFlagAveraging = true;
    return *this;
}

/*! \brief function for overloading = Operation (called in base class)
 *
 \param rhs Abstract model which is assigned.
 */
template <typename ValueType>
void KITGPI::Modelparameter::SH<ValueType>::assign(KITGPI::Modelparameter::Modelparameter<ValueType> const &rhs)
{
    velocityS = rhs.getVelocityS();
    density = rhs.getDensity();
    dirtyFlagInverseDensity = true;
    dirtyFlagSWaveModulus = true;
    dirtyFlagAveraging = true;
}

/*! \brief function for overloading -= Operation (called in base class)
 *
 \param rhs Abstract model which is substracted.
 */
template <typename ValueType>
void KITGPI::Modelparameter::SH<ValueType>::minusAssign(KITGPI::Modelparameter::Modelparameter<ValueType> const &rhs)
{
    velocityS -= rhs.getVelocityS();
    density -= rhs.getDensity();
    dirtyFlagInverseDensity = true;
    dirtyFlagSWaveModulus = true;
    dirtyFlagAveraging = true;
}

/*! \brief function for overloading += Operation (called in base class)
 *
 \param rhs Abstract model which is added.
 */
template <typename ValueType>
void KITGPI::Modelparameter::SH<ValueType>::plusAssign(KITGPI::Modelparameter::Modelparameter<ValueType> const &rhs)
{
    velocityS += rhs.getVelocityS();
    density += rhs.getDensity();
    dirtyFlagInverseDensity = true;
    dirtyFlagSWaveModulus = true;
    dirtyFlagAveraging = true;
}
template class KITGPI::Modelparameter::SH<float>;
template class KITGPI::Modelparameter::SH<double>;
