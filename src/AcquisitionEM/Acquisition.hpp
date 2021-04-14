#pragma once

#include "../Acquisition/Acquisition.hpp"

namespace KITGPI
{

    /*! \brief Acquisition namespace
     *
     * This namespace handles the seismic acquisition, e.g. Sources, Receivers, Seismogram and SourceSignals.
     */
    namespace Acquisition
    {

        /*! \brief List of seismogram types
         *
         * Types of seismograms, which are used in this programm.
         */
        enum SeismogramTypeEM { EZ, //!< Electric intensity in z direction
                              EX, //!< Electric intensity in x direction
                              EY, //!< Electric intensity in y direction
                              HZ  //!< Electric intensity in z direction
        };

        /*! \brief Map to transfer a SeismogramTypeEM to the corresponding string
         * 
         * This map can be used to map a SeismogramTypeEM to the corresponding char.
         * This can be usefull to add the SeismogramTypeEM to a filename etc.
         */
        static std::map<SeismogramTypeEM, const char *> SeismogramTypeStringEM = {
            {EZ, "ez"},
            {EX, "ex"},
            {EY, "ey"},
            {HZ, "hz"}};
    }
}
