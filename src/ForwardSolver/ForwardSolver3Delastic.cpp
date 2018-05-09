#include "ForwardSolver3Delastic.hpp"
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
void KITGPI::ForwardSolver::FD3Delastic<ValueType>::prepareBoundaryConditions(Configuration::Configuration const &config, Derivatives::Derivatives<ValueType> &derivatives, scai::dmemo::DistributionPtr dist, scai::hmemo::ContextPtr ctx)
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

/*! \brief Running the 3-D elastic foward solver
 *
 * Start the 3-D forward solver as defined by the given parameters
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
void KITGPI::ForwardSolver::FD3Delastic<ValueType>::run(Acquisition::AcquisitionGeometry<ValueType> &receiver, Acquisition::AcquisitionGeometry<ValueType> const &sources, Modelparameter::Modelparameter<ValueType> const &model, Wavefields::Wavefields<ValueType> &wavefield, Derivatives::Derivatives<ValueType> const &derivatives, IndexType tStart, IndexType tEnd, ValueType /*DT*/)
{

    SCAI_REGION("timestep")

    SCAI_ASSERT_ERROR((tEnd - tStart) >= 1, " Number of time steps has to be greater than zero. ");

    /* Get references to required modelparameter */
    lama::Vector<ValueType> const &inverseDensity = model.getInverseDensity();
    lama::Vector<ValueType> const &pWaveModulus = model.getPWaveModulus();
    lama::Vector<ValueType> const &sWaveModulus = model.getSWaveModulus();
    lama::Vector<ValueType> const &inverseDensityAverageX = model.getInverseDensityAverageX();
    lama::Vector<ValueType> const &inverseDensityAverageY = model.getInverseDensityAverageY();
    lama::Vector<ValueType> const &inverseDensityAverageZ = model.getInverseDensityAverageZ();
    lama::Vector<ValueType> const &sWaveModulusAverageXY = model.getSWaveModulusAverageXY();
    lama::Vector<ValueType> const &sWaveModulusAverageXZ = model.getSWaveModulusAverageXZ();
    lama::Vector<ValueType> const &sWaveModulusAverageYZ = model.getSWaveModulusAverageYZ();

    /* Get references to required wavefields */
    lama::Vector<ValueType> &vX = wavefield.getRefVX();
    lama::Vector<ValueType> &vY = wavefield.getRefVY();
    lama::Vector<ValueType> &vZ = wavefield.getRefVZ();

    lama::Vector<ValueType> &Sxx = wavefield.getRefSxx();
    lama::Vector<ValueType> &Syy = wavefield.getRefSyy();
    lama::Vector<ValueType> &Szz = wavefield.getRefSzz();

    lama::Vector<ValueType> &Syz = wavefield.getRefSyz();
    lama::Vector<ValueType> &Sxz = wavefield.getRefSxz();
    lama::Vector<ValueType> &Sxy = wavefield.getRefSxy();

    /* Get references to required derivatives matrixes */
    lama::Matrix<ValueType> const &Dxf = derivatives.getDxf();
    lama::Matrix<ValueType> const &Dzf = derivatives.getDzf();
    lama::Matrix<ValueType> const &Dxb = derivatives.getDxb();
    lama::Matrix<ValueType> const &Dzb = derivatives.getDzb();

    lama::Matrix<ValueType> const &DybPressure = derivatives.getDybPressure();
    lama::Matrix<ValueType> const &DybVelocity = derivatives.getDybVelocity();
    lama::Matrix<ValueType> const &DyfPressure = derivatives.getDyfPressure();
    lama::Matrix<ValueType> const &DyfVelocity = derivatives.getDyfVelocity();

    SourceReceiverImpl::FDTD3Delastic<ValueType> SourceReceiver(sources, receiver, wavefield);

    std::unique_ptr<lama::Vector<ValueType>> updatePtr(vX.newVector()); // create new Vector(Pointer) with same configuration as vZ
    lama::Vector<ValueType> &update = *updatePtr;                          // get Reference of VectorPointer

    std::unique_ptr<lama::Vector<ValueType>> update_tempPtr(vX.newVector()); // create new Vector(Pointer) with same configuration as vZ
    lama::Vector<ValueType> &update_temp = *update_tempPtr;                     // get Reference of VectorPointer

    std::unique_ptr<lama::Vector<ValueType>> vxxPtr(vX.newVector());
    std::unique_ptr<lama::Vector<ValueType>> vyyPtr(vX.newVector());
    std::unique_ptr<lama::Vector<ValueType>> vzzPtr(vX.newVector());

    lama::Vector<ValueType> &vxx = *vxxPtr;
    lama::Vector<ValueType> &vyy = *vyyPtr;
    lama::Vector<ValueType> &vzz = *vzzPtr;

    if (useFreeSurface) {
        FreeSurface.setModelparameter(model);
    }

    dmemo::CommunicatorPtr comm = inverseDensity.getDistributionPtr()->getCommunicatorPtr();

    /* --------------------------------------- */
    /* Start runtime critical part             */
    /* --------------------------------------- */

    for (IndexType t = tStart; t < tEnd; t++) {

        if (t % 100 == 0 && t != 0) {
            HOST_PRINT(comm, "Calculating time step " << t << "\n");
        }

        /* ----------------*/
        /* update velocity */
        /* ----------------*/
        update = Dxf * Sxx;
        if (useConvPML) {
            ConvPML.apply_sxx_x(update);
        }

        update_temp = DybVelocity * Sxy;
        if (useConvPML) {
            ConvPML.apply_sxy_y(update_temp);
        }
        update += update_temp;

        update_temp = Dzb * Sxz;
        if (useConvPML) {
            ConvPML.apply_sxz_z(update_temp);
        }
        update += update_temp;
        update *= inverseDensityAverageX;
        vX += update;

        update = Dxb * Sxy;
        if (useConvPML) {
            ConvPML.apply_sxy_x(update);
        }

        update_temp = DyfVelocity * Syy;
        if (useConvPML) {
            ConvPML.apply_syy_y(update_temp);
        }
        update += update_temp;

        update_temp = Dzb * Syz;
        if (useConvPML) {
            ConvPML.apply_syz_z(update_temp);
        }
        update += update_temp;

        update *= inverseDensityAverageY;
        vY += update;

        update = Dxb * Sxz;
        if (useConvPML) {
            ConvPML.apply_sxz_x(update);
        }

        update_temp = DybVelocity * Syz;
        if (useConvPML) {
            ConvPML.apply_syz_y(update_temp);
        }
        update += update_temp;

        update_temp = Dzf * Szz;
        if (useConvPML) {
            ConvPML.apply_szz_z(update_temp);
        }
        update += update_temp;

        update *= inverseDensityAverageZ;
        vZ += update;

        /* ----------------*/
        /* pressure update */
        /* ----------------*/
        vxx = Dxb * vX;
        vyy = DybPressure * vY;
        vzz = Dzb * vZ;
        if (useConvPML) {
            ConvPML.apply_vxx(vxx);
            ConvPML.apply_vyy(vyy);
            ConvPML.apply_vzz(vzz);
        }

        update = vxx;
        update += vyy;
        update += vzz;
        update *= pWaveModulus;

        Sxx += update;
        Syy += update;
        Szz += update;

        update = vyy + vzz;
        update *= sWaveModulus;
        Sxx -= 2.0 * update;
        update = vxx + vzz;
        update *= sWaveModulus;
        Syy -= 2.0 * update;
        update = vxx + vyy;
        update *= sWaveModulus;
        Szz -= 2.0 * update;

        update = DyfPressure * vX;
        if (useConvPML) {
            ConvPML.apply_vxy(update);
        }
        update_temp = Dxf * vY;
        if (useConvPML) {
            ConvPML.apply_vyx(update_temp);
        }
        update += update_temp;
        update *= sWaveModulusAverageXY;
        Sxy += update;

        update = Dzf * vX;
        if (useConvPML) {
            ConvPML.apply_vxz(update);
        }
        update_temp = Dxf * vZ;
        if (useConvPML) {
            ConvPML.apply_vzx(update_temp);
        }
        update += update_temp;

        update *= sWaveModulusAverageXZ;
        Sxz += update;

        update = Dzf * vY;
        if (useConvPML) {
            ConvPML.apply_vyz(update);
        }
        update_temp = DyfPressure * vZ;
        if (useConvPML) {
            ConvPML.apply_vzy(update_temp);
        }
        update += update_temp;
        update *= sWaveModulusAverageYZ;
        Syz += update;

        /* Apply free surface to stress update */
        if (useFreeSurface) {
            update = vxx + vzz;
            FreeSurface.apply(update, Sxx, Syy, Szz);
        }

        /* Apply the damping boundary */
        if (useDampingBoundary) {
            DampingBoundary.apply(Sxx, Syy, Szz, Sxy, Sxz, Syz, vX, vY, vZ);
        }

        /* Apply source and save seismogram */
        SourceReceiver.applySource(t);
        SourceReceiver.gatherSeismogram(t);
    }

    /* --------------------------------------- */
    /* Stop runtime critical part             */
    /* --------------------------------------- */
}

template class KITGPI::ForwardSolver::FD3Delastic<float>;
template class KITGPI::ForwardSolver::FD3Delastic<double>;
