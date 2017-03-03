#include "SourceReceiverImpl.hpp"
using namespace scai;

/*! \brief default methode  for Source-Reciever Implementation.
 *
 \param sourceConfig Sources
 \param receiverConfig Recievers
 \param wavefieldIN Wavefield
 */
template <typename ValueType>
KITGPI::ForwardSolver::SourceReceiverImpl::SourceReceiverImpl<ValueType>::SourceReceiverImpl(Acquisition::AcquisitionGeometry<ValueType> const &sourceConfig, Acquisition::AcquisitionGeometry<ValueType> &receiverConfig, Wavefields::Wavefields<ValueType> &wavefieldIN)
    : wavefield(wavefieldIN), sources(sourceConfig.getSeismogramHandler()), receivers(receiverConfig.getSeismogramHandler())
{

    // Set source coordinate to receiver seismogram handler
    if (sources.getNumTracesTotal() == 1) {
        /* If only one source is injected in this simulation, the coordinate of this source is
         set to the receiver seismogram handler */
        lama::Scalar temp = sourceConfig.getCoordinates().getValue(0);
        receivers.setSourceCoordinate(temp.getValue<IndexType>());
    } else {
        /* If more than one source is injected at the same time, the source coordinate is
        set to zero, due to the choice of one coordinate would be subjective */
        receivers.setSourceCoordinate(0);
    }

    // Set Context of temporary variables to context of wavefields
    setContextPtrToTemporary(wavefield.getContextPtr());
}

/*! \brief Gether seismogram.
 *
 \param t Time-step
 */
template <typename ValueType>
void KITGPI::ForwardSolver::SourceReceiverImpl::SourceReceiverImpl<ValueType>::gatherSeismogram(IndexType t)
{
    if (receivers.getNumTracesGlobal(Acquisition::SeismogramType::P) > 0) {
        gatherSeismogramPressure(receivers.getSeismogram(Acquisition::SeismogramType::P), wavefield, t);
    }
    if (receivers.getNumTracesGlobal(Acquisition::SeismogramType::VX) > 0) {
        gatherSeismogramVX(receivers.getSeismogram(Acquisition::SeismogramType::VX), wavefield, t);
    }
    if (receivers.getNumTracesGlobal(Acquisition::SeismogramType::VY) > 0) {
        gatherSeismogramVY(receivers.getSeismogram(Acquisition::SeismogramType::VY), wavefield, t);
    }
    if (receivers.getNumTracesGlobal(Acquisition::SeismogramType::VZ) > 0) {
        gatherSeismogramVZ(receivers.getSeismogram(Acquisition::SeismogramType::VZ), wavefield, t);
    }
}

/*! \brief Gether vx seismogram.
 *
 \param seismo Seismogram
 \param wavefieldIN incoming wavefield
 \param t Time-step
 */
template <typename ValueType>
void KITGPI::ForwardSolver::SourceReceiverImpl::SourceReceiverImpl<ValueType>::gatherSeismogramVX(Acquisition::Seismogram<ValueType> &seismo, Wavefields::Wavefields<ValueType> &wavefieldIN, IndexType t)
{
    gatherSeismogramSingle(seismo, wavefieldIN.getVX(), gatherSeismogram_samplesVX, t);
}

/*! \brief Gether vy seismogram.
 *
 \param seismo Seismogram
 \param wavefieldIN incoming wavefield
 \param t Time-step
 */
template <typename ValueType>
void KITGPI::ForwardSolver::SourceReceiverImpl::SourceReceiverImpl<ValueType>::gatherSeismogramVY(Acquisition::Seismogram<ValueType> &seismo, Wavefields::Wavefields<ValueType> &wavefieldIN, IndexType t)
{
    gatherSeismogramSingle(seismo, wavefieldIN.getVY(), gatherSeismogram_samplesVY, t);
}

/*! \brief Gether vz seismogram.
 *
 \param seismo Seismogram
 \param wavefieldIN incoming wavefield
 \param t Time-step
 */
template <typename ValueType>
void KITGPI::ForwardSolver::SourceReceiverImpl::SourceReceiverImpl<ValueType>::gatherSeismogramVZ(Acquisition::Seismogram<ValueType> &seismo, Wavefields::Wavefields<ValueType> &wavefieldIN, IndexType t)
{
    gatherSeismogramSingle(seismo, wavefieldIN.getVZ(), gatherSeismogram_samplesVZ, t);
}

/*! \brief Gether single seismogram.
 *
 \param seismo Seismogram
 \param wavefieldSingle Wavefield-single
  \param temp temporary Value
 \param t Time-step
 */
template <typename ValueType>
void KITGPI::ForwardSolver::SourceReceiverImpl::SourceReceiverImpl<ValueType>::gatherSeismogramSingle(Acquisition::Seismogram<ValueType> &seismo, scai::lama::DenseVector<ValueType> &wavefieldSingle, scai::lama::DenseVector<ValueType> &temp, IndexType t)
{
    const lama::DenseVector<IndexType> &coordinates = seismo.getCoordinates();
    lama::DenseMatrix<ValueType> &seismogramData = seismo.getData();

    temp.gather(wavefieldSingle, coordinates, utilskernel::binary::BinaryOp::COPY);
    seismogramData.setColumn(temp, t, utilskernel::binary::BinaryOp::COPY);
}

/*! \brief Applying source.
 *
 \param t Time-step
 */
template <typename ValueType>
void KITGPI::ForwardSolver::SourceReceiverImpl::SourceReceiverImpl<ValueType>::applySource(IndexType t)
{
    if (sources.getNumTracesGlobal(Acquisition::SeismogramType::P) > 0) {
        applySourcePressure(sources.getSeismogram(Acquisition::SeismogramType::P), wavefield, t);
    }
    if (sources.getNumTracesGlobal(Acquisition::SeismogramType::VX) > 0) {
        applySourceVX(sources.getSeismogram(Acquisition::SeismogramType::VX), wavefield, t);
    }
    if (sources.getNumTracesGlobal(Acquisition::SeismogramType::VY) > 0) {
        applySourceVY(sources.getSeismogram(Acquisition::SeismogramType::VY), wavefield, t);
    }
    if (sources.getNumTracesGlobal(Acquisition::SeismogramType::VZ) > 0) {
        applySourceVZ(sources.getSeismogram(Acquisition::SeismogramType::VZ), wavefield, t);
    }
}

/*! \brief Applying vx source.
 *
 \param seismo Seismogram
 \param wavefieldIN incoming Wavefield
 \param t Time-step
 */
template <typename ValueType>
void KITGPI::ForwardSolver::SourceReceiverImpl::SourceReceiverImpl<ValueType>::applySourceVX(Acquisition::Seismogram<ValueType> const &seismo, Wavefields::Wavefields<ValueType> &wavefieldIN, IndexType t)
{
    applySourceSingle(seismo, wavefieldIN.getVX(), applySource_samplesVX, t);
}

/*! \brief Applying vy source.
 *
 \param seismo Seismogram
 \param wavefieldIN incoming Wavefield
 \param t Time-step
 */
template <typename ValueType>
void KITGPI::ForwardSolver::SourceReceiverImpl::SourceReceiverImpl<ValueType>::applySourceVY(Acquisition::Seismogram<ValueType> const &seismo, Wavefields::Wavefields<ValueType> &wavefieldIN, IndexType t)
{
    applySourceSingle(seismo, wavefieldIN.getVY(), applySource_samplesVY, t);
}

/*! \brief Applying vz source.
 *
 \param seismo Seismogram
 \param wavefieldIN incoming Wavefield
 \param t Time-step
 */
template <typename ValueType>
void KITGPI::ForwardSolver::SourceReceiverImpl::SourceReceiverImpl<ValueType>::applySourceVZ(Acquisition::Seismogram<ValueType> const &seismo, Wavefields::Wavefields<ValueType> &wavefieldIN, IndexType t)
{
    applySourceSingle(seismo, wavefieldIN.getVZ(), applySource_samplesVZ, t);
}

/*! \brief Applying single source.
 *
 \param seismo Seismogram
 \param wavefieldSingle Wavefields
 \param temp temporary Value
 \param t Time-step
 */
template <typename ValueType>
void KITGPI::ForwardSolver::SourceReceiverImpl::SourceReceiverImpl<ValueType>::applySourceSingle(Acquisition::Seismogram<ValueType> const &seismo, scai::lama::DenseVector<ValueType> &wavefieldSingle, scai::lama::DenseVector<ValueType> &temp, IndexType t)
{
    /* Get reference to sourcesignal storing seismogram */
    const lama::DenseMatrix<ValueType> &sourcesSignals = seismo.getData();
    const lama::DenseVector<IndexType> &coordinates = seismo.getCoordinates();

    sourcesSignals.getColumn(temp, t);
    wavefieldSingle.scatter(coordinates, temp, utilskernel::binary::BinaryOp::ADD);
}

/*! \brief Setting context pointer to temporary source and seismogram.
 *
 \param ctx Context
 */
template <typename ValueType>
void KITGPI::ForwardSolver::SourceReceiverImpl::SourceReceiverImpl<ValueType>::setContextPtrToTemporary(scai::hmemo::ContextPtr ctx)
{
    applySource_samplesVX.setContextPtr(ctx);
    applySource_samplesVY.setContextPtr(ctx);
    applySource_samplesVZ.setContextPtr(ctx);
    gatherSeismogram_samplesVX.setContextPtr(ctx);
    gatherSeismogram_samplesVY.setContextPtr(ctx);
    gatherSeismogram_samplesVZ.setContextPtr(ctx);
    applySource_samplesPressure.setContextPtr(ctx);
    gatherSeismogram_samplesPressure.setContextPtr(ctx);
}

template class KITGPI::ForwardSolver::SourceReceiverImpl::SourceReceiverImpl<float>;
template class KITGPI::ForwardSolver::SourceReceiverImpl::SourceReceiverImpl<double>;
