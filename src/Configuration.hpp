#pragma once

#include <scai/lama/Scalar.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

using namespace scai;

//! Class for Configuration of the FD simulation
/*!
 This class handels the configuration for the finite-difference simulation.
 */
template<typename ValueType>
class Configuration
{
public:
    
    /*! \brief Default deconstructor
     */
	~Configuration(){}
    
    Configuration( std::string filename );
    
    void print();
    void printAllRaw();
    
	IndexType getNZ() { return NZ; } ///< Return NZ (Depth)
	IndexType getNX() { return NX; } ///< Return NX
	IndexType getNY() { return NY; } ///< Return NY

	IndexType getDH() { return DH; } ///< Return Grid spacing

	ValueType getDT() { return DT; } ///< Return Time Step
	ValueType getT() { return T; } ///< Return Total propagation time

    IndexType getReadModel() {return ReadModel;} ///< Return Read in Model?
    std::string getFilenameModel() {return FilenameModel;} ///< Return Filename of Model
	ValueType getVelocity() { return velocity; } ///< Return Velocity for homogeneous model
	ValueType getRho() { return rho; } ///< Return Density for homogeneous model

    std::string getSourceFilename() {return SourceFilename;} ///< Return Filename of Model
    
	IndexType getSeismogramZ() { return seismogram_z; } ///< Return seismogram_z
	IndexType getSeismogramX() { return seismogram_x; } ///< Return seismogram_x
	IndexType getSeismogramY() { return seismogram_y; } ///< Return seismogram_y

    IndexType getN() { return N; } ///< Return N

	ValueType getM() { return M; } ///< Return M

	IndexType getNT() { return NT; } ///< Return NT
 
	lama::Scalar& getVfactor() { return v_factor; } ///< Return v_factor
	lama::Scalar& getPfactor() { return p_factor; } ///< Return p_factor

	IndexType getSourceIndex() { return source_index; } ///< Return Source Index
	IndexType getSeismogramIndex() { return seismogram_index; } ///< Return Seismogram Index

private:
    
    
    /*! \brief Routine for calculating the 1D index position from 3-D coordinates
     *
	 */
	IndexType index( IndexType x, IndexType y, IndexType z, IndexType NX, IndexType NY, IndexType NZ )
	{
	    SCAI_REGION( "Index_calculation" )

	    if ( z > NZ || x > NX || y > NY || z < 1 || x < 1 || y < 1 )
	    {
	        COMMON_THROWEXCEPTION ( "Could not map from coordinate to indize!" )
	        return -100;
	    }
	    else
	    {
	        return ( ( z - 1 ) + ( x - 1 ) * NZ + ( y - 1 ) * NZ * NX );
	    }
	}

	/* read parameters */

    // define spatial sampling: number of grid points in direction
    IndexType NZ; ///< Grid points depth
    IndexType NX; ///< Grid points horizontal 1
    IndexType NY; ///< Grid points horizontal 2

    /// define distance between two grid points in meter
    IndexType DH;
    
    // define temporal sampling
    ValueType DT;  ///< temporal sampling in seconds
    ValueType T;   ///< total simulation time

    IndexType ReadModel; ///< Read model from File (1=YES, else=NO)
    std::string FilenameModel; ///< Filename to read model
    ValueType velocity; ///< Density in kilo gramms per cubic meter
    ValueType rho;      ///< P-wave velocity in meter per seconds

    std::string SourceFilename; ///< Filename to read source configuration
    
    IndexType seismogram_z; ///< seismogram position in grid points (depth)
    IndexType seismogram_x; ///< seismogram position in grid points
    IndexType seismogram_y; ///< seismogram position in grid points

    /* calculated parameters */

    IndexType N; ///< Number of total grid points NX*NY*NZ

	ValueType M; ///< P-wave modulus (in case of homogeneous model)

	IndexType NT; ///< Number of time steps

	lama::Scalar v_factor;  ///< factor for update
	lama::Scalar p_factor;  ///< factor for update

	IndexType source_index;  ///< Position of source in 1D coordinates
	IndexType seismogram_index;  ///< Position of receiver in 1D coordinates
};


/*! \brief Constructor
 *
 \param filename of configuration file
 */
template<typename ValueType>
Configuration<ValueType>::Configuration( std::string filename )
{
    // read all lines in file
    
    std::string line;
    std::map<std::string,std::string> map;
    std::ifstream input( filename.c_str() );
    
    while ( std::getline( input, line ) )
    {
        size_t lineEnd = line.size();
        std::string::size_type commentPos1 = line.find_first_of( "#", 0 );
        if ( std::string::npos != commentPos1 )
        {
            std::string::size_type commentPos2 = line.find_first_of( "#", commentPos1 );
            if ( std::string::npos != commentPos2 )
            {
                if( commentPos1 == 0 )
                {
                    continue;
                }
                lineEnd = commentPos1;
            }
        }
        
        std::string::size_type equalPos = line.find_first_of( "=", 0 );
        
        if ( std::string::npos != equalPos )
        {
            // tokenize it  name = val
            std::string name = line.substr( 0, equalPos );
            size_t len = lineEnd - ( equalPos + 1);
            std::string val  = line.substr( equalPos + 1, len);
            map.insert( std::pair<std::string,std::string>( name, val ) );
        }
    }
    input.close();
    
    // check map and assign all values with right "cast" to members
    size_t nArgs = 14;
    if ( map.size() != nArgs )
    {
        std::cout << filename << " does not include a valid configutation with " << nArgs << " arguments." << std::endl;
        }
        
        std::istringstream( map[ "NZ" ] ) >> NZ; // IndexType
        std::istringstream( map[ "NX" ] ) >> NX; // IndexType
        std::istringstream( map[ "NY" ] ) >> NY; // IndexType
        
        std::istringstream( map[ "DH" ] ) >> DH; // IndexType
        
        std::istringstream( map[ "DT" ] ) >> DT; // ValueType
        std::istringstream( map[ "T" ] ) >> T;  // ValueType
        
        std::istringstream( map[ "ReadModel" ] ) >> ReadModel; // IndexType
        std::istringstream( map[ "FilenameModel" ] ) >> FilenameModel; // std::string
    
        std::istringstream( map[ "velocity" ] ) >> velocity; // ValueType
        std::istringstream( map[ "rho" ] ) >> rho; // ValueType

        std::istringstream( map[ "SourceFilename" ] ) >> SourceFilename; // std::string
    
        std::istringstream( map[ "seismogram_z" ] ) >> seismogram_z; // IndexType
        std::istringstream( map[ "seismogram_x" ] ) >> seismogram_x; // IndexType
        std::istringstream( map[ "seismogram_y" ] ) >> seismogram_y; // IndexType
        
        // calculate other parameters
        
        N = NZ * NX * NY;
        
        M = velocity * velocity * rho; // P-wave modulus
        
        NT = static_cast<IndexType>( ( T / DT ) + 0.5 ); // MATLAB round(T/DT)
        
        v_factor = lama::Scalar(DT / DH);
        p_factor = lama::Scalar(DT);
        
        seismogram_index = index( seismogram_x, seismogram_y, seismogram_z, NX, NY, NZ );
        
}


/*! \brief Print all parameter (for debugging)
 */
template<typename ValueType>
void Configuration<ValueType>::printAllRaw()
{
    std::cout << "NZ=" << getNZ() << std::endl;
    std::cout << "NX=" << getNX() << std::endl;
    std::cout << "NY=" << getNY() << std::endl;
    std::cout << "DH=" << getDH() << std::endl;
    std::cout << "DT=" << getDT() << std::endl;
    std::cout << "T=" << getT() << std::endl;
    std::cout << "velocity=" << getVelocity() << std::endl;
    std::cout << "rho=" << getRho() << std::endl;
    std::cout << "seismogram_z=" << getSeismogramZ() << std::endl;
    std::cout << "seismogram_x=" << getSeismogramX() << std::endl;
    std::cout << "seismogram_y=" << getSeismogramX() << std::endl;
    std::cout << "N=" << getN() << std::endl;
    std::cout << "M=" << getM() << std::endl;
    std::cout << "NT=" << getNT() << std::endl;
    std::cout << "v_factor=" << getVfactor() << std::endl;
    std::cout << "p_factor=" << getPfactor() << std::endl;
    std::cout << "source_index=" << getSourceIndex() << std::endl;
    std::cout << "seismogram_index=" << getSeismogramIndex() << std::endl;
}

/*! \brief Print configuration
 */
template<typename ValueType>
void Configuration<ValueType>::print()
{
    IndexType velocity_max = velocity; // TODO: is velocity a vector?
    double courant = velocity_max * DT / DH;
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "Criteriums:" << std::endl;
    std::cout << "    Courant-number: " << courant << std::endl;
    if ( courant >= 0.8 )
    {
        std::cout << "Simulation will be UNSTABLE" << std::endl;
        std::cout << "Choose smaller DT, eg.: " << DH * 0.3 / velocity_max << std::endl;
        exit(0);
    }
    std::cout << "Modelling-domain:" << std::endl;
    std::cout << "    Z: " << DH * NZ << " m (Depth)" << std::endl;
    std::cout << "    X: " << DH * NX << " m (Horizontal)" << std::endl;
    std::cout << "    Y: " << DH * NY << " m (Horizontal)" << std::endl;
    std::cout << "Acquisition:" << std::endl;
    std::cout << "    Source acquisition will be read in from " << SourceFilename << std::endl;
    std::cout << "Material:" << std::endl;
    if(ReadModel==1) {
        std::cout << "    Model will be read in from disk" << std::endl;
        std::cout << "    First Lame-Parameter: " << FilenameModel << ".pi.mtx" << std::endl;
        std::cout << "    Density: " << FilenameModel << ".density.mtx" << std::endl;
    } else {
        std::cout << "    A homogeneous model will be generated" << std::endl;
        std::cout << "    Velocity:" << velocity << " m/s" << std::endl;
        std::cout << "    Density:" << rho << " g/cm3" << std::endl;
    }
    std::cout << std::endl;
}

