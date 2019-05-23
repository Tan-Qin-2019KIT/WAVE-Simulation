#include <scai/common/Settings.hpp>
#include <scai/common/Walltime.hpp>
#include <scai/dmemo/CommunicatorStack.hpp>
#include <scai/dmemo/GridDistribution.hpp>
#include <scai/lama.hpp>

#include <iostream>
#define _USE_MATH_DEFINES
#include <cmath>

#include "Acquisition/Receivers.hpp"
#include "Acquisition/Sources.hpp"
#include "Acquisition/suHandler.hpp"
#include "Configuration/Configuration.hpp"
#include "ForwardSolver/ForwardSolver.hpp"

#include "ForwardSolver/Derivatives/DerivativesFactory.hpp"
#include "ForwardSolver/ForwardSolverFactory.hpp"
#include "Modelparameter/ModelparameterFactory.hpp"
#include "Wavefields/WavefieldsFactory.hpp"

#include "CheckParameter/CheckParameter.hpp"
#include "Common/HostPrint.hpp"
#include "Partitioning/Partitioning.hpp"

using namespace scai;
using namespace KITGPI;

extern bool verbose; // global variable definition

int main(int argc, const char *argv[])
{
    // parse command line arguments to be set as environment variables, e.g.
    // --SCAI_CONTEXT=CUDA

    common::Settings::parseArgs(argc, argv);

    typedef double ValueType;
    double start_t, end_t; /* For timing */

    /* inter node communicator */
    dmemo::CommunicatorPtr commAll = dmemo::Communicator::getCommunicatorPtr(); // default communicator, set by environment variable SCAI_COMMUNICATOR
    common::Settings::setRank(commAll->getNodeRank());

    /* --------------------------------------- */
    /* Read configuration from file            */
    /* --------------------------------------- */

    if (argc != 2) {
        std::cout << "\n\nNo configuration file given!\n\n"
                  << std::endl;
        return (2);
    }

    Configuration::Configuration config(argv[1]);
    verbose = config.get<bool>("verbose");

    std::string dimension = config.get<std::string>("dimension");
    std::string equationType = config.get<std::string>("equationType");

    HOST_PRINT(commAll, "\nSOFI" << dimension << " " << equationType << " - LAMA Version\n\n");
    if (commAll->getRank() == MASTERGPI) {
        config.print();
    }

    /* --------------------------------------- */
    /* coordinate mapping (3D<->1D)            */
    /* --------------------------------------- */

    Acquisition::Coordinates<ValueType> modelCoordinates(config);

    /* --------------------------------------- */
    /* communicator for shot parallelisation   */
    /* --------------------------------------- */

    IndexType npS = config.get<IndexType>("ProcNS");
    IndexType npM = commAll->getSize() / npS;
    if (commAll->getSize() != npS * npM) {
        HOST_PRINT(commAll, "\n Error: Number of MPI processes (" << commAll->getSize()
                                                                  << ") is not multiple of shots in " << argv[1] << ": ProcNS = " << npS << "\n")
        return (2);
    }

    CheckParameter::checkNumberOfProcesses(config, commAll);

    // Build subsets of processors for the shots
    common::Grid2D procAllGrid(npS, npM);
    IndexType procAllGridRank[2];
    procAllGrid.gridPos(procAllGridRank, commAll->getRank());

    // communicator for set of processors that solve one shot
    dmemo::CommunicatorPtr commShot = commAll->split(procAllGridRank[0]);

    // this communicator is used for reducing the solutions of problems
    dmemo::CommunicatorPtr commInterShot = commAll->split(commShot->getRank());

    SCAI_DMEMO_TASK(commShot)

    /* --------------------------------------- */
    /* Context and Distribution                */
    /* --------------------------------------- */

    /* execution context */
    hmemo::ContextPtr ctx = hmemo::Context::getContextPtr(); // default context, set by environment variable SCAI_CONTEXT

    dmemo::DistributionPtr dist = nullptr;
    if ((config.get<IndexType>("partitioning") == 0) || (config.get<IndexType>("partitioning") == 2)) {
        //Block distribution = starting distribution for graph partitioner
        dist = std::make_shared<dmemo::BlockDistribution>(modelCoordinates.getNGridpoints(), commShot);
    } else if (config.get<IndexType>("partitioning") == 1) {
        SCAI_ASSERT(!config.get<bool>("useVariableGrid"), "Grid distribution is not available for the variable grid");
        dist = Partitioning::gridPartition<ValueType>(config, commShot);
    } else {
        COMMON_THROWEXCEPTION("unknown partioning method");
    }

    /* --------------------------------------- */
    /* Calculate derivative matrizes           */
    /* --------------------------------------- */
    start_t = common::Walltime::get();
    ForwardSolver::Derivatives::Derivatives<ValueType>::DerivativesPtr derivatives(ForwardSolver::Derivatives::Factory<ValueType>::Create(dimension));
    derivatives->init(dist, ctx, config, modelCoordinates, commShot);
    end_t = common::Walltime::get();
    HOST_PRINT(commAll, "", "Finished initializing matrices in " << end_t - start_t << " sec.\n\n");

    /* --------------------------------------- */
    /* Call partioner */
    /* --------------------------------------- */
    if (config.get<IndexType>("partitioning") == 2) {
#ifdef USE_GEOGRAPHER
        start_t = common::Walltime::get();
        auto graph = derivatives->getCombinedMatrix();
        auto &&weights = Partitioning::BoundaryWeights(config, dist, modelCoordinates, config.get<ValueType>("BoundaryWeights"));
        auto &&coords = modelCoordinates.getCoordinates(dist, ctx);

        if (config.get<bool>("coordinateWrite"))
            modelCoordinates.writeCoordinates(dist, ctx, config.get<std::string>("coordinateFilename"));

        end_t = common::Walltime::get();
        HOST_PRINT(commAll, "", "created partioner input  in " << end_t - start_t << " sec.\n\n");
	
	std::string toolStr = config.get<std::string>("graphPartitionTool");
		/*
        
		std::map<std::string,ITI::Tool> toolName = { 
			{"geographer",ITI::Tool::geographer}, {"geoKmeans",ITI::Tool::geoKmeans}, {"geoSFC",ITI::Tool::geoSFC}, {"geoMS", ITI::Tool::geoMS},
			{"geoHierKM", ITI::Tool::geoHierKM}, {"geoHierRepart", ITI::Tool::geoHierRepart},
			{"parMetisSFC",ITI::Tool::parMetisSFC}, {"parMetisGeom",ITI::Tool::parMetisGeom}, {"parMetisGraph",ITI::Tool::parMetisGraph},
			{"zoltanRcb",ITI::Tool::zoltanRCB}, {"zoltanRib",ITI::Tool::zoltanRIB}, {"zoltanMJ",ITI::Tool::zoltanMJ}, {"zoltanHsfc",ITI::Tool::zoltanSFC}
		};

		ITI::Tool tool = toolName[toolStr];
		*/
		//convert string to enum
		//TODO: pass it as string and convert it later to Tool
		ITI::Tool tool = ITI::toTool(toolStr);
		
		start_t = common::Walltime::get();

        dist = Partitioning::graphPartition(config, commShot, coords, graph, weights, tool);

        derivatives->redistributeMatrices(dist);
#else
        HOST_PRINT(commAll, "partitioning=2 or useVariableGrid was set, but geographer was not compiled. \n Use < make prog GEOGRAPHER_ROOT= > to compile the partitioner\n", "\n")
        return (2);
#endif

		end_t = common::Walltime::get();
		HOST_PRINT(commShot, "Partitioning time " << end_t - start_t << std::endl) ;
    }

    /* --------------------------------------- */
    /* Acquisition geometry                    */
    /* --------------------------------------- */
    Acquisition::Sources<ValueType> sources(config, modelCoordinates, ctx, dist);
    Acquisition::Receivers<ValueType> receivers;
    if (!config.get<bool>("useReceiversPerShot")) {
        receivers.init(config, modelCoordinates, ctx, dist);
    }

    /* --------------------------------------- */
    /* Modelparameter                          */
    /* --------------------------------------- */
    Modelparameter::Modelparameter<ValueType>::ModelparameterPtr model(Modelparameter::Factory<ValueType>::Create(equationType));
    if ((config.get<IndexType>("ModelRead") == 2) && (config.get<bool>("useVariableGrid"))){
        HOST_PRINT(commAll, "", "reading regular model ...\n")
        Acquisition::Coordinates<ValueType> regularCoordinates(config.get<IndexType>("NX"), config.get<IndexType>("NY"), config.get<IndexType>("NZ"), config.get<ValueType>("DH"));
        dmemo::DistributionPtr regularDist(new dmemo::BlockDistribution(regularCoordinates.getNGridpoints(), commShot));

        Modelparameter::Modelparameter<ValueType>::ModelparameterPtr regularModel(Modelparameter::Factory<ValueType>::Create(equationType));
        regularModel->init(config, ctx, regularDist);
        HOST_PRINT(commAll, "", "reading regular model finished\n\n")
        
        HOST_PRINT(commAll, "", "initialising model on discontineous grid ...\n")
        model->init(*regularModel, dist, modelCoordinates, regularCoordinates);
        HOST_PRINT(commAll, "", "initialising model on discontineous grid finished\n\n")
    } else {
        model->init(config, ctx, dist);
    }
    model->prepareForModelling(modelCoordinates, ctx, dist, commShot);
    //CheckParameter::checkNumericalArtefeactsAndInstabilities<ValueType>(config, *model, commShot);

    /* --------------------------------------- */
    /* Wavefields                              */
    /* --------------------------------------- */
    Wavefields::Wavefields<ValueType>::WavefieldPtr wavefields(Wavefields::Factory<ValueType>::Create(dimension, equationType));
    wavefields->init(ctx, dist);
    
    /* --------------------------------------- */
    /* Forward solver                          */
    /* --------------------------------------- */

    HOST_PRINT(commAll, "", "ForwardSolver ...\n")
    ForwardSolver::ForwardSolver<ValueType>::ForwardSolverPtr solver(ForwardSolver::Factory<ValueType>::Create(dimension, equationType));
    solver->initForwardSolver(config, *derivatives, *wavefields, *model, modelCoordinates, ctx, config.get<ValueType>("DT"));
    solver->prepareForModelling(*model, config.get<ValueType>("DT"));
    HOST_PRINT(commAll, "", "ForwardSolver prepared\n")

    ValueType DT = config.get<ValueType>("DT");
    IndexType tStepEnd = Common::time2index(config.get<ValueType>("T"), DT);

    dmemo::BlockDistribution shotDist(sources.getNumShots(), commInterShot);

    for (IndexType shotNumber = shotDist.lb(); shotNumber < shotDist.ub(); shotNumber++) {
        /* Update Source */
        if (!config.get<bool>("runSimultaneousShots"))
            sources.init(config, modelCoordinates, ctx, dist, shotNumber);
        if (config.get<bool>("useReceiversPerShot")) {
            receivers.init(config, modelCoordinates, ctx, dist, shotNumber);
        }

        HOST_PRINT(commShot, "Start time stepping for shot " << shotNumber + 1 << " of " << sources.getNumShots() << "\nTotal Number of time steps: " << tStepEnd << "\n");
        wavefields->resetWavefields();

        start_t = common::Walltime::get();

        for (IndexType tStep = 0; tStep < tStepEnd; tStep++) {

            if (tStep % 100 == 0 && tStep != 0) {
                HOST_PRINT(commShot, "Calculating time step " << tStep << "\n");
            }

            solver->run(receivers, sources, *model, *wavefields, *derivatives, tStep);

            if (config.get<IndexType>("snapType") > 0 && tStep >= Common::time2index(config.get<ValueType>("tFirstSnapshot"), DT) && tStep <= Common::time2index(config.get<ValueType>("tlastSnapshot"), DT) && (tStep - Common::time2index(config.get<ValueType>("tFirstSnapshot"), DT)) % Common::time2index(config.get<ValueType>("tincSnapshot"), DT) == 0) {
                wavefields->write(config.get<IndexType>("snapType"), config.get<std::string>("WavefieldFileName"), tStep, *derivatives, *model, config.get<IndexType>("PartitionedOut"));
            }
        }

        end_t = common::Walltime::get();
        HOST_PRINT(commShot, "Finished time stepping (shot " << shotNumber << ") in " << end_t - start_t << " sec.\n");

        receivers.getSeismogramHandler().normalize();

        if (!config.get<bool>("runSimultaneousShots")) {
            receivers.getSeismogramHandler().write(config.get<IndexType>("SeismogramFormat"), config.get<std::string>("SeismogramFilename") + ".shot_" + std::to_string(shotNumber), modelCoordinates);
        } else {
            receivers.getSeismogramHandler().write(config.get<IndexType>("SeismogramFormat"), config.get<std::string>("SeismogramFilename"), modelCoordinates);
        }

        solver->resetCPML();
    }

    std::exit(0); //needed in supermuc
    return 0;
}
