///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
//  Copyright (2017) Lars Pastewka
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once


#include <plugins/particles/Particles.h>
#include <core/dataset/data/simcell/SimulationCell.h>
#include <core/dataset/data/properties/PropertyStorage.h>
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <core/dataset/pipeline/AsynchronousModifier.h>
#include <core/dataset/pipeline/AsynchronousModifierApplication.h>

#include <complex>

#ifdef CorrelationFunctionPlugin_EXPORTS		// This is defined by CMake when building the plugin library.
#  define OVITO_CORRELATION_FUNCTION_EXPORT Q_DECL_EXPORT
#else
#  define OVITO_CORRELATION_FUNCTION_EXPORT Q_DECL_IMPORT
#endif

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief This modifier computes the spatial correlation function between two particle properties.
 */
class OVITO_CORRELATION_FUNCTION_EXPORT CorrelationFunctionModifier : public AsynchronousModifier
{
public:

    enum AveragingDirectionType {
		CELL_VECTOR_1 = 0,
		CELL_VECTOR_2 = 1,
		CELL_VECTOR_3 = 2,
		RADIAL = 3
	};
    Q_ENUMS(AveragingDirectionType);

    enum NormalizationType { 
		DO_NOT_NORMALIZE = 0,
		NORMALIZE_BY_COVARIANCE = 1,
		NORMALIZE_BY_RDF = 2
	};
    Q_ENUMS(NormalizationType);

	/// Constructor.
	Q_INVOKABLE CorrelationFunctionModifier(DataSet* dataset);

	/// \brief Create a new modifier application that refers to this modifier instance.
	virtual OORef<ModifierApplication> createModifierApplication() override;

	/// This method is called by the system after the modifier has been inserted into a data pipeline.
	virtual void initializeModifier(ModifierApplication* modApp) override;

	/// Update plot ranges.
	void updateRanges(FloatType offset, FloatType fac, FloatType reciprocalFac, ModifierApplication* modApp);

public:
	
	/// Give this modifier class its own metaclass.
	class OOMetaClass : public AsynchronousModifier::OOMetaClass 
	{
	public:

		/// Inherit constructor from base metaclass.
		using AsynchronousModifier::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;
	};
		
protected:
	
	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;	

private:

	/// Stores the modifier's results.
	class CorrelationAnalysisResults : public ComputeEngineResults 
	{
	public:

		/// Constructor.
		CorrelationAnalysisResults(int numberOfNeighBins, bool doComputeNeighCorrelation) :
			_neighCorrelation(doComputeNeighCorrelation ? numberOfNeighBins : 0, 0.0),
			_neighCorrelationX(doComputeNeighCorrelation ? numberOfNeighBins : 0) {}

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

		/// Returns the real-space correlation function.
		QVector<FloatType>& realSpaceCorrelation() { return _realSpaceCorrelation; }

		/// Returns the RDF evaluated from an FFT correlation.
		QVector<FloatType>& realSpaceRDF() { return _realSpaceRDF; }

		/// Returns the distances for which the real-space correlation function is tabulAveated.
		QVector<FloatType>& realSpaceCorrelationX() { return _realSpaceCorrelationX; }

		/// Returns the short-ranged real-space correlation function.
		QVector<FloatType>& neighCorrelation() { return _neighCorrelation; }

		/// Returns the RDF evalauted from a direct sum over neighbor shells.
		QVector<FloatType>& neighRDF() { return _neighRDF; }

		/// Returns the distances for which the short-ranged real-space correlation function is tabulated.
		QVector<FloatType>& neighCorrelationX() { return _neighCorrelationX; }

		/// Returns the reciprocal-space correlation function.
		QVector<FloatType>& reciprocalSpaceCorrelation() { return _reciprocalSpaceCorrelation; }

		/// Returns the wavevectors for which the reciprocal-space correlation function is tabulated.
		QVector<FloatType>& reciprocalSpaceCorrelationX() { return _reciprocalSpaceCorrelationX; }

		/// Returns the mean of the first property.
		FloatType mean1() const { return _mean1; }

		/// Returns the mean of the second property.
		FloatType mean2() const { return _mean2; }

		/// Returns the (co)variance.
		FloatType covariance() const { return _covariance; }

		void setLimits(FloatType mean1, FloatType mean2, FloatType covariance) {
			_mean1 = mean1;
			_mean2 = mean2;
			_covariance = covariance;
		}

	private:

		QVector<FloatType> _realSpaceCorrelation;
		QVector<FloatType> _realSpaceRDF;
		QVector<FloatType> _realSpaceCorrelationX;
		QVector<FloatType> _neighCorrelation;
		QVector<FloatType> _neighRDF;
		QVector<FloatType> _neighCorrelationX;
		QVector<FloatType> _reciprocalSpaceCorrelation;
		QVector<FloatType> _reciprocalSpaceCorrelationX;
		FloatType _mean1 = 0;
		FloatType _mean2 = 0;
		FloatType _covariance = 0;
	};

	/// Computes the modifier's results.
	class CorrelationAnalysisEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		CorrelationAnalysisEngine(const TimeInterval& validityInterval,
								  ConstPropertyPtr positions,
								  ConstPropertyPtr sourceProperty1,
								  size_t vecComponent1,
								  ConstPropertyPtr sourceProperty2,
								  size_t vecComponent2,
								  const SimulationCell& simCell,
								  FloatType fftGridSpacing,
								  bool applyWindow,
								  bool doComputeNeighCorrelation,
								  FloatType neighCutoff,
								  int numberOfNeighBins,
								  AveragingDirectionType averagingDirection) :
			ComputeEngine(validityInterval), 
			_positions(std::move(positions)),
			_sourceProperty1(std::move(sourceProperty1)), _vecComponent1(vecComponent1),
			_sourceProperty2(std::move(sourceProperty2)), _vecComponent2(vecComponent2),
			_simCell(simCell), _fftGridSpacing(fftGridSpacing),
			_applyWindow(applyWindow), _neighCutoff(neighCutoff),
			_averagingDirection(averagingDirection),
			_results(std::make_shared<CorrelationAnalysisResults>(numberOfNeighBins, doComputeNeighCorrelation)) {}

		/// Compute real and reciprocal space correlation function via FFT.
		void computeFftCorrelation();

		/// Compute real space correlation function via direction summation over neighbors.
		void computeNeighCorrelation();

		/// Compute means and covariance.
		void computeLimits();

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Returns the property storage that contains the input particle positions.
		const ConstPropertyPtr& positions() const { return _positions; }

		/// Returns the property storage that contains the first input particle property.
		const ConstPropertyPtr& sourceProperty1() const { return _sourceProperty1; }

		/// Returns the property storage that contains the second input particle property.
		const ConstPropertyPtr& sourceProperty2() const { return _sourceProperty2; }

		/// Returns the simulation cell data.
		const SimulationCell& cell() const { return _simCell; }

		/// Returns the FFT cutoff radius.
		FloatType fftGridSpacing() const { return _fftGridSpacing; }

		/// Returns the neighbor cutoff radius.
		FloatType neighCutoff() const { return _neighCutoff; }

		/// Returns the real-space correlation function.
		QVector<FloatType>& realSpaceCorrelation() { return _results->realSpaceCorrelation(); }

		/// Returns the RDF evaluated from an FFT correlation.
		QVector<FloatType>& realSpaceRDF() { return _results->realSpaceRDF(); }

		/// Returns the distances for which the real-space correlation function is tabulAveated.
		QVector<FloatType>& realSpaceCorrelationX() { return _results->realSpaceCorrelationX(); }

		/// Returns the short-ranged real-space correlation function.
		QVector<FloatType>& neighCorrelation() { return _results->neighCorrelation(); }

		/// Returns the RDF evalauted from a direct sum over neighbor shells.
		QVector<FloatType>& neighRDF() { return _results->neighRDF(); }

		/// Returns the distances for which the short-ranged real-space correlation function is tabulated.
		QVector<FloatType>& neighCorrelationX() { return _results->neighCorrelationX(); }

		/// Returns the reciprocal-space correlation function.
		QVector<FloatType>& reciprocalSpaceCorrelation() { return _results->reciprocalSpaceCorrelation(); }

		/// Returns the wavevectors for which the reciprocal-space correlation function is tabulated.
		QVector<FloatType>& reciprocalSpaceCorrelationX() { return _results->reciprocalSpaceCorrelationX(); }

	private:

		/// Real-to-complex FFT.
		void r2cFFT(int nX, int nY, int nZ, QVector<FloatType> &rData, QVector<std::complex<FloatType>> &cData);

		/// Complex-to-real inverse FFT
		void c2rFFT(int nX, int nY, int nZ, QVector<std::complex<FloatType>> &cData, QVector<FloatType> &rData);

		/// Map property onto grid.
		void mapToSpatialGrid(const PropertyStorage* property,
							  size_t propertyVectorComponent,
							  const AffineTransformation &reciprocalCell,
							  int nX, int nY, int nZ,
							  QVector<FloatType> &gridData,
							  bool applyWindow);

		const size_t _vecComponent1;
		const size_t _vecComponent2;
		const FloatType _fftGridSpacing;
		const bool _applyWindow;
		const FloatType _neighCutoff;
		const AveragingDirectionType _averagingDirection;
		const SimulationCell _simCell;
		const ConstPropertyPtr _positions;
		const ConstPropertyPtr _sourceProperty1;
		const ConstPropertyPtr _sourceProperty2;
		std::shared_ptr<CorrelationAnalysisResults> _results;
	};

private:

	/// The particle property that serves as the first data source for the correlation function.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticlePropertyReference, sourceProperty1, setSourceProperty1);
	/// The particle property that serves as the second data source for the correlation function.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticlePropertyReference, sourceProperty2, setSourceProperty2);
	/// Controls the cutoff radius for the FFT grid.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, fftGridSpacing, setFFTGridSpacing);
	/// Controls if a windowing function should be applied in nonperiodic directions.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, applyWindow, setApplyWindow);
	/// Controls whether the real-space correlation should be computed by direct summation.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, doComputeNeighCorrelation, setComputeNeighCorrelation);
	/// Controls the cutoff radius for the neighbor lists.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, neighCutoff, setNeighCutoff);
	/// Controls the number of bins for the neighbor part of the real-space correlation function.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, numberOfNeighBins, setNumberOfNeighBins);
	/// Controls the averaging direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(AveragingDirectionType, averagingDirection, setAveragingDirection);
	/// Controls the normalization of the real-space correlation function.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(NormalizationType, normalizeRealSpace, setNormalizeRealSpace);
	/// Type of real-space plot (lin-lin, log-lin or log-log)
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, typeOfRealSpacePlot, setTypeOfRealSpacePlot);
	/// Controls the whether the range of the x-axis of the plot should be fixed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, fixRealSpaceXAxisRange, setFixRealSpaceXAxisRange);
	/// Controls the start value of the x-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, realSpaceXAxisRangeStart, setRealSpaceXAxisRangeStart);
	/// Controls the end value of the x-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, realSpaceXAxisRangeEnd, setRealSpaceXAxisRangeEnd);
	/// Controls the whether the range of the y-axis of the plot should be fixed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, fixRealSpaceYAxisRange, setFixRealSpaceYAxisRange);
	/// Controls the start value of the y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, realSpaceYAxisRangeStart, setRealSpaceYAxisRangeStart);
	/// Controls the end value of the y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, realSpaceYAxisRangeEnd, setRealSpaceYAxisRangeEnd);
	/// Controls the normalization of the reciprocal-space correlation function.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, normalizeReciprocalSpace, setNormalizeReciprocalSpace);
	/// Type of reciprocal-space plot (lin-lin, log-lin or log-log)
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, typeOfReciprocalSpacePlot, setTypeOfReciprocalSpacePlot);
	/// Controls the whether the range of the x-axis of the plot should be fixed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, fixReciprocalSpaceXAxisRange, setFixReciprocalSpaceXAxisRange);
	/// Controls the start value of the x-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, reciprocalSpaceXAxisRangeStart, setReciprocalSpaceXAxisRangeStart);
	/// Controls the end value of the x-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, reciprocalSpaceXAxisRangeEnd, setReciprocalSpaceXAxisRangeEnd);
	/// Controls the whether the range of the y-axis of the plot should be fixed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, fixReciprocalSpaceYAxisRange, setFixReciprocalSpaceYAxisRange);
	/// Controls the start value of the y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, reciprocalSpaceYAxisRangeStart, setReciprocalSpaceYAxisRangeStart);
	/// Controls the end value of the y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, reciprocalSpaceYAxisRangeEnd, setReciprocalSpaceYAxisRangeEnd);

	Q_OBJECT
	OVITO_CLASS

	Q_CLASSINFO("DisplayName", "Correlation function");
	Q_CLASSINFO("ModifierCategory", "Analysis");
};


/**
 * \brief The type of ModifierApplication created for a CorrelationFunctionModifier 
 *        when it is inserted into in a data pipeline. Its stores results computed by the
 *        modifier's compute engine so that they can be displayed in the modifier's UI panel.
 */
 class OVITO_CORRELATION_FUNCTION_EXPORT CorrelationFunctionModifierApplication : public AsynchronousModifierApplication
 {
 public:
 
	 /// Constructor.
	 Q_INVOKABLE CorrelationFunctionModifierApplication(DataSet* dataset) : AsynchronousModifierApplication(dataset) {}
 
	/// Returns the Y coordinates of the real-space correlation function.
	const QVector<FloatType>& realSpaceCorrelation() const { return _realSpaceCorrelation; }

	/// Returns the RDF evaluated from an FFT correlation.
	const QVector<FloatType>& realSpaceRDF() const { return _realSpaceRDF; }

	/// Returns the X coordinates of the real-space correlation function.
	const QVector<FloatType>& realSpaceCorrelationX() const { return _realSpaceCorrelationX; }

	/// Returns the Y coordinates of the short-ranged part of the real-space correlation function.
	const QVector<FloatType>& neighCorrelation() const { return _neighCorrelation; }

	/// Returns the RDF evalauted from a direct sum over neighbor shells.
	const QVector<FloatType>& neighRDF() const { return _neighRDF; }

	/// Returns the X coordinates of the short-ranged part of the real-space correlation function.
	const QVector<FloatType>& neighCorrelationX() const { return _neighCorrelationX; }

	/// Returns the Y coordinates of the reciprocal-space correlation function.
	const QVector<FloatType>& reciprocalSpaceCorrelation() const { return _reciprocalSpaceCorrelation; }

	/// Returns the X coordinates of the reciprocal-space correlation function.
	const QVector<FloatType>& reciprocalSpaceCorrelationX() const { return _reciprocalSpaceCorrelationX; }

	/// Returns the mean of the first property.
	FloatType mean1() const { return _mean1; }

	/// Returns the mean of the second property.
	FloatType mean2() const { return _mean2; }

	/// Returns the (co)variance.
	FloatType covariance() const { return _covariance; }
	 
	/// Replaces the stored data.
	void setResults(QVector<FloatType> realSpaceCorrelation, QVector<FloatType> realSpaceRDF, QVector<FloatType> realSpaceCorrelationX,
		QVector<FloatType> neighCorrelation, QVector<FloatType> neighRDF, QVector<FloatType> neighCorrelationX, 
		QVector<FloatType> reciprocalSpaceCorrelation, QVector<FloatType> reciprocalSpaceCorrelationX, FloatType mean1, FloatType mean2, FloatType covariance) 
	{
		_realSpaceCorrelation = std::move(realSpaceCorrelation);
		_realSpaceRDF = std::move(realSpaceRDF);
		_realSpaceCorrelationX = std::move(realSpaceCorrelationX);
		_neighCorrelation = std::move(neighCorrelation);
		_neighRDF = std::move(neighRDF);
		_neighCorrelationX = std::move(neighCorrelationX);
		_reciprocalSpaceCorrelation = std::move(reciprocalSpaceCorrelation);
		_reciprocalSpaceCorrelationX = std::move(reciprocalSpaceCorrelationX);
		_mean1 = mean1;
		_mean2 = mean2;
		_covariance = covariance;
		notifyDependents(ReferenceEvent::ObjectStatusChanged);
	}
 
private:
 

	/// The real-space correlation function.
	QVector<FloatType> _realSpaceCorrelation;

	/// The radial distribution function computed from an FFT convolution.
	QVector<FloatType> _realSpaceRDF;

	/// The distances for which the real-space correlation function is tabulated.
	QVector<FloatType> _realSpaceCorrelationX;

	/// The short-ranged part of the real-space correlation function.
	QVector<FloatType> _neighCorrelation;

	/// The radial distribution function computed from a direct sum over neighbor shells.
	QVector<FloatType> _neighRDF;

	/// The distances for which short-ranged part of the real-space correlation function is tabulated.
	QVector<FloatType> _neighCorrelationX;

	/// The reciprocal-space correlation function.
	QVector<FloatType> _reciprocalSpaceCorrelation;

	/// The wavevevtors for which the reciprocal-space correlation function is tabulated.
	QVector<FloatType> _reciprocalSpaceCorrelationX;

	/// Mean of the first property.
	FloatType _mean1;

	/// Mean of the second property.
	FloatType _mean2;

	/// (Co)variance.
	FloatType _covariance;

	Q_OBJECT
	OVITO_CLASS
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::CorrelationFunctionModifier::AveragingDirectionType);
Q_DECLARE_METATYPE(Ovito::Particles::CorrelationFunctionModifier::NormalizationType);
Q_DECLARE_TYPEINFO(Ovito::Particles::CorrelationFunctionModifier::AveragingDirectionType, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Ovito::Particles::CorrelationFunctionModifier::NormalizationType, Q_PRIMITIVE_TYPE);
