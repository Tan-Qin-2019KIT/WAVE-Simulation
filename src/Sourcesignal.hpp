
#include <scai/lama.hpp>
#include <scai/lama/DenseVector.hpp>
#include <cmath>
#include <valarray>

#pragma once

/*! \brief Abstract class to create sourcesignals
 * This is class holds several methods to generate different sourcesignals.
 * As this class is an abstract class, all methods are protected.
 */
template <typename ValueType>
class Sourcesignal
{

protected:
    
    void Ricker(lama::DenseVector<ValueType>& signal, IndexType NT, ValueType DT, ValueType FC, ValueType AMP, ValueType Tshift);
    void FGaussian(lama::DenseVector<ValueType>& signal, IndexType NT, ValueType DT, ValueType FC, ValueType AMP, ValueType Tshift);
    void Spike(lama::DenseVector<ValueType>& signal, IndexType NT, ValueType DT, ValueType AMP, ValueType Tshift);
    void sinthree(lama::DenseVector<ValueType>& signal, IndexType NT, ValueType DT, ValueType FC, ValueType AMP, ValueType Tshift);
};


/*! \brief Generating a Ricker signal
 *
 \param signal Allocated vector to to store Ricker signal
 \param NT Number of time steps
 \param DT Temporal time step interval
 \param FC Central frequency
 \param AMP Amplitude
 \param Tshift Time to shift wavelet
 */
template<typename ValueType>
void Sourcesignal<ValueType>::Ricker(lama::DenseVector<ValueType>& signal, IndexType NT, ValueType DT, ValueType FC, ValueType AMP, ValueType Tshift )
{
    
    /*
     *  t=0:DT:(NT*DT-DT);
     *  tau=pi*FC*(t-1.5/FC);
     *  signal=AMP*(1-2*tau.^2).*exp(-tau.^2);
     */
    lama::DenseVector<ValueType> t(NT, ValueType(0), DT);
    lama::DenseVector<ValueType> help( t.size(), 1.5 / FC +Tshift);
    lama::DenseVector<ValueType> tau( t - help );
    tau *= M_PI * FC;
    
    /* this is for source[i] = AMP * ( 1.0 - 2.0 * tau[i] * tau[i] * exp( -tau[i] * tau[i] ) ); */
    lama::DenseVector<ValueType> one( signal.size(), 1.0 );
    help = tau * tau;
    tau = -1.0 * help;
    tau.exp();
    help = one - 2.0 * help;
    signal = lama::Scalar(AMP) * help * tau;
    
}

/*! \brief Generating a First derivative of a Gaussian (FGaussian)
 *
 \param signal Allocated vector to to store FGaussian signal
 \param NT Number of time steps
 \param DT Temporal time step interval
 \param FC Central frequency
 \param AMP Amplitude
 \param Tshift Time to shift wavelet
 */
template<typename ValueType>
void Sourcesignal<ValueType>::FGaussian(lama::DenseVector<ValueType>& signal, IndexType NT, ValueType DT, ValueType FC, ValueType AMP, ValueType Tshift )
{
    
    /*
     *  t=0:DT:(NT*DT-DT);
     *  tau=pi*FC*(t-1.2/FC);
     *  signal=AMP*2*tau.*exp(-tau*tau);
     */
    lama::DenseVector<ValueType> t(NT, ValueType(0), DT);
    lama::DenseVector<ValueType> help( t.size(), 1.2 / FC +Tshift);
    lama::DenseVector<ValueType> tau( t - help );
    tau *= M_PI * FC ;
    
    /* this is for source[i] = AMP * (-2) * tau[i].*exp(-tau[i] * tau[i]); */

    help = -2.0 * tau;
    tau = -1.0* tau * tau;
    tau.exp();
    signal = lama::Scalar(AMP) * help * tau;
    
}


/*! \brief Generating a First derivative of a Spike
 *
 \param signal Allocated vector to to store Spike signal
 \param NT Number of time steps
 \param DT Temporal time step interval
 \param AMP Amplitude
 \param Tshift Time to shift wavelet
 */
template<typename ValueType>
void Sourcesignal<ValueType>::Spike(lama::DenseVector<ValueType>& signal, IndexType NT, ValueType DT, ValueType AMP, ValueType Tshift )
{
    
    /*
     *  Spike;
     */
    scai::lama::Scalar temp_spike;
    IndexType time_index;
    lama::DenseVector<ValueType> help( NT, 0.0);
    
    /* this is for source[i] = 1.0 when t=tshift/dt; */
    temp_spike=1.0;
    time_index=floor(Tshift/DT);
    help.setValue(time_index,temp_spike);

    signal = lama::Scalar(AMP) * help;
    
}

/*! \brief Generating a First derivative of a sinus raised to the power of three (sinthree)
 *
 \param signal Allocated vector to to store sinthree signal
 \param NT Number of time steps
 \param DT Temporal time step interval
 \param FC Central frequency
 \param AMP Amplitude
 \param Tshift Time to shift wavelet
 */
template<typename ValueType>
void Sourcesignal<ValueType>::sinthree(lama::DenseVector<ValueType>& signal, IndexType NT, ValueType DT, ValueType FC, ValueType AMP, ValueType Tshift )
{
    
     /*
     *  t=0:DT:(NT*DT-DT);
     *  when t>=tshift && t<=tshift+1.0/FC;
     *  tau=pi*FC*(t-Tshift);
     *  signal=(sin(tau))^3;
     */

    lama::DenseVector<ValueType> zero( NT, 0.0 );
    
    double temp;
    IndexType time_index1,time_index2,i,count;
    
    time_index1 = floor(Tshift/DT);
    time_index2 = time_index1 + floor(1.0/FC/DT);
    
    
    /* this is for source[i] = (sin(PI*(t-Tshift)*FC))^3 when t>=tshift && t<=tshift+1.0/FC; */
    count=0;
    for (i=time_index1; i<=time_index2; i++) {
	  temp=count * DT * M_PI * FC ;
	  temp=sin(temp);
	  temp=pow(temp,3);
          zero.setValue(i,temp);
	  count++;
    }

 
    signal = lama::Scalar(AMP) * zero;
    
}