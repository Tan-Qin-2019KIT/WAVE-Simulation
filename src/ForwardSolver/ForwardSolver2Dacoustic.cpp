#include "ForwardSolver2Dacoustic.hpp"
using namespace scai;

/*! \brief Initialitation of the boundary conditions
 *
 *
 \param config Configuration
 \param derivatives Derivatives matrices
 \param dist Distribution of the wave fields
 \param ctx Context
 */
template <typename ValueType>
void KITGPI::ForwardSolver::FD2Dacoustic<ValueType>::prepareBoundaryConditions(Configuration::Configuration const &config, Derivatives::Derivatives<ValueType> &derivatives, scai::dmemo::DistributionPtr dist, scai::hmemo::ContextPtr ctx)
{

    /* Prepare Free Surface */
    if (config.get<IndexType>("FreeSurface")) {
        useFreeSurface = true;
        FreeSurface.init(dist, derivatives, config.get<IndexType>("NX"), config.get<IndexType>("NY"), config.get<IndexType>("NZ"), config.get<ValueType>("DT"), config.get<ValueType>("DH"));
    }

    /* Prepare Damping Boundary */
    if (config.get<IndexType>("DampingBoundary") == 1) {
        if (config.get<IndexType>("DampingBoundaryType") == 1) {
            useDampingBoundary = true;
            DampingBoundary.init(dist, ctx, config.get<IndexType>("NX"), config.get<IndexType>("NY"), config.get<IndexType>("NZ"), config.get<IndexType>("BoundaryWidth"), config.get<ValueType>("DampingCoeff"), useFreeSurface);
        }

        if (config.get<IndexType>("DampingBoundaryType") == 2) {
            useConvPML = true;
            ConvPML.init(dist, ctx, config.get<IndexType>("NX"), config.get<IndexType>("NY"), config.get<IndexType>("NZ"), config.get<ValueType>("DT"), config.get<IndexType>("DH"), config.get<IndexType>("BoundaryWidth"), config.get<ValueType>("NPower"), config.get<ValueType>("KMaxCPML"), config.get<ValueType>("CenterFrequencyCPML"), config.get<ValueType>("VMaxCPML"), useFreeSurface);
        }
    }
}

/*! \brief Initialitation of the ForwardSolver
 *
 *
 \param config Configuration
 \param derivatives Derivatives matrices
 \param wavefield Wavefields for the modelling
 \param ctx Context
 */
template <typename ValueType>
void KITGPI::ForwardSolver::FD2Dacoustic<ValueType>::initForwardSolver(Configuration::Configuration const &config, Derivatives::Derivatives<ValueType> &derivatives, Wavefields::Wavefields<ValueType> &wavefield, Modelparameter::Modelparameter<ValueType> const &model, scai::hmemo::ContextPtr ctx, ValueType /*DT*/)
{
    /* Check if distributions of wavefields and models are the same */
    SCAI_ASSERT_ERROR(wavefield.getRefVX().getDistributionPtr()==model.getDensity().getDistributionPtr(),"Distributions of wavefields and models are not the same");
    
    /* Get distribibution of the wavefields */
    dmemo::DistributionPtr dist;
    lama::Vector &vX = wavefield.getRefVX();
    dist=vX.getDistributionPtr();
    
    /* Initialisation of Boundary Conditions */
    if (config.get<IndexType>("FreeSurface") && config.get<IndexType>("DampingBoundaryType")) {
	this->prepareBoundaryConditions(config, derivatives, dist, ctx);
    }
    
    /* Initialisation of auxiliary vectors*/
    common::unique_ptr<lama::Vector> tmp1(vX.newVector());  	// create new Vector(Pointer) with same configuration as vX (goes out of scope at functions end)
    updatePtr=std::move(tmp1); 					// assign tmp1 to updatePtr
    common::unique_ptr<lama::Vector> tmp2(vX.newVector()); 
    update_tempPtr=std::move(tmp2); 	
}

/*! \brief Running the 2-D acoustic foward solver
 *
 * Start the 2-D forward solver as defined by the given parameters
 *
 \param receiver Configuration of the receivers
 \param sources Configuration of the sources
 \param model Configuration of the modelparameter
 \param wavefield Wavefields for the modelling
 \param derivatives Derivations matrices to calculate the spatial derivatives
 \param tStart Counter start in for loop over time steps
 \param tEnd Counter end  in for loop over time steps
 \param DT Temporal Sampling intervall in seconds
 */
template <typename ValueType>
void KITGPI::ForwardSolver::FD2Dacoustic<ValueType>::run(Acquisition::AcquisitionGeometry<ValueType> &receiver, Acquisition::AcquisitionGeometry<ValueType> const &sources, Modelparameter::Modelparameter<ValueType> const &model, Wavefields::Wavefields<ValueType> &wavefield, Derivatives::Derivatives<ValueType> const &derivatives, IndexType tStart, IndexType tEnd)
{

    SCAI_REGION("timestep")

    SCAI_ASSERT_ERROR((tEnd - tStart) >= 1, " Number of time steps has to be greater than zero. ");

    /* Get references to required modelparameter */
    lama::Vector const &inverseDensity = model.getInverseDensity();
    lama::Vector const &pWaveModulus = model.getPWaveModulus();
    lama::Vector const &inverseDensityAverageX = model.getInverseDensityAverageX();
    lama::Vector const &inverseDensityAverageY = model.getInverseDensityAverageY();

    /* Get references to required wavefields */
    lama::Vector &vX = wavefield.getRefVX();
    lama::Vector &vY = wavefield.getRefVY();
    lama::Vector &p = wavefield.getRefP();

    /* Get references to required derivatives matrixes */
    lama::Matrix const &Dxf = derivatives.getDxf();
    lama::Matrix const &Dxb = derivatives.getDxb();
    lama::Matrix const &Dyb = derivatives.getDyb();
    lama::Matrix const &Dyf = derivatives.getDyfVelocity();
    
    /* Get reference to auxiliary vector */
    scai::lama::Vector &update=*updatePtr;
    scai::lama::Vector &update_temp=*update_tempPtr;
    
    SourceReceiverImpl::FDTD2Dacoustic<ValueType> SourceReceiver(sources, receiver, wavefield);

    dmemo::CommunicatorPtr comm = inverseDensity.getDistributionPtr()->getCommunicatorPtr();

    /* --------------------------------------- */
    /* Start runtime critical part             */
    /* --------------------------------------- */

    for (IndexType t = tStart; t < tEnd; t++) {

        if (t % 100 == 0 && t != 0) {
            HOST_PRINT(comm, "Calculating time step " << t << "\n");
        }

        /* update velocity */
        update = Dxf * p;
        if (useConvPML) {
            ConvPML.apply_p_x(update);
        }
        update *= inverseDensityAverageX;
        vX += update;

        update = Dyf * p;
        if (useConvPML) {
            ConvPML.apply_p_y(update);
        }
        update *= inverseDensityAverageY;
        vY += update;

        /* pressure update */
        update = Dxb * vX;
        if (useConvPML) {
            ConvPML.apply_vxx(update);
        }

        update_temp = Dyb * vY;
        if (useConvPML) {
            ConvPML.apply_vyy(update_temp);
        }
        update += update_temp;

        update *= pWaveModulus;
        p += update;

        /* Apply free surface to pressure update */
        if (useFreeSurface) {
            FreeSurface.apply(p);
        }

        /* Apply the damping boundary */
        if (useDampingBoundary) {
            DampingBoundary.apply(p, vX, vY);
        }

        /* Apply source and save seismogram */
        SourceReceiver.applySource(t);
        SourceReceiver.gatherSeismogram(t);
    }

    /* --------------------------------------- */
    /* Stop runtime critical part             */
    /* --------------------------------------- */
}

template class KITGPI::ForwardSolver::FD2Dacoustic<float>;
template class KITGPI::ForwardSolver::FD2Dacoustic<double>;
