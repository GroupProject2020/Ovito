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

#include <plugins/particles/Particles.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <plugins/particles/objects/ParticlesObject.h>
#include <core/app/Application.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/utilities/units/UnitsManager.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "CorrelationFunctionModifier.h"

#include <fftw3.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_OVITO_CLASS(CorrelationFunctionModifier);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, sourceProperty1);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, sourceProperty2);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, averagingDirection);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, fftGridSpacing);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, applyWindow);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, doComputeNeighCorrelation);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, neighCutoff);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, numberOfNeighBins);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, normalizeRealSpace);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, normalizeRealSpaceByRDF);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, normalizeRealSpaceByCovariance);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, typeOfRealSpacePlot);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, normalizeReciprocalSpace);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, typeOfReciprocalSpacePlot);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, fixRealSpaceXAxisRange);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, realSpaceXAxisRangeStart);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, realSpaceXAxisRangeEnd);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, fixRealSpaceYAxisRange);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, realSpaceYAxisRangeStart);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, realSpaceYAxisRangeEnd);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, fixReciprocalSpaceXAxisRange);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, reciprocalSpaceXAxisRangeStart);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, reciprocalSpaceXAxisRangeEnd);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, fixReciprocalSpaceYAxisRange);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, reciprocalSpaceYAxisRangeStart);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, reciprocalSpaceYAxisRangeEnd);
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, sourceProperty1, "First property");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, sourceProperty2, "Second property");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, averagingDirection, "Averaging direction");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, fftGridSpacing, "FFT grid spacing");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, applyWindow, "Apply window function to nonperiodic directions");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, doComputeNeighCorrelation, "Direct summation");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, neighCutoff, "Neighbor cutoff radius");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, numberOfNeighBins, "Number of neighbor bins");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, normalizeRealSpace, "Normalize correlation function");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, normalizeRealSpaceByRDF, "Normalize by RDF");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, normalizeRealSpaceByCovariance, "Normalize by covariance");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, normalizeReciprocalSpace, "Normalize correlation function");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CorrelationFunctionModifier, fftGridSpacing, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CorrelationFunctionModifier, neighCutoff, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(CorrelationFunctionModifier, numberOfNeighBins, IntegerParameterUnit, 4, 100000);
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, fixRealSpaceXAxisRange, "Fix x-range");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, realSpaceXAxisRangeStart, "X-range start");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, realSpaceXAxisRangeEnd, "X-range end");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, fixRealSpaceYAxisRange, "Fix y-range");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, realSpaceYAxisRangeStart, "Y-range start");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, realSpaceYAxisRangeEnd, "Y-range end");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, fixReciprocalSpaceXAxisRange, "Fix x-range");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, reciprocalSpaceXAxisRangeStart, "X-range start");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, reciprocalSpaceXAxisRangeEnd, "X-range end");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, fixReciprocalSpaceYAxisRange, "Fix y-range");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, reciprocalSpaceYAxisRangeStart, "Y-range start");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, reciprocalSpaceYAxisRangeEnd, "Y-range end");

// This global mutex is used to serialize access to the FFTW3 planner 
// routines, which are not thread-safe.
QMutex CorrelationFunctionModifier::_fftwMutex;

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CorrelationFunctionModifier::CorrelationFunctionModifier(DataSet* dataset) : AsynchronousModifier(dataset),
	_averagingDirection(RADIAL), 
	_fftGridSpacing(3.0), 
	_applyWindow(true), 
	_doComputeNeighCorrelation(false), 
	_neighCutoff(5.0), 
	_numberOfNeighBins(50),
	_normalizeRealSpace(VALUE_CORRELATION), 
	_normalizeRealSpaceByRDF(false), 
	_normalizeRealSpaceByCovariance(false), 
	_typeOfRealSpacePlot(0), 
	_normalizeReciprocalSpace(false), 
	_typeOfReciprocalSpacePlot(0),
	_fixRealSpaceXAxisRange(false), 
	_realSpaceXAxisRangeStart(0.0), 
	_realSpaceXAxisRangeEnd(1.0),
	_fixRealSpaceYAxisRange(false), 
	_realSpaceYAxisRangeStart(0.0), 
	_realSpaceYAxisRangeEnd(1.0),
	_fixReciprocalSpaceXAxisRange(false), 
	_reciprocalSpaceXAxisRangeStart(0.0), 
	_reciprocalSpaceXAxisRangeEnd(1.0),
	_fixReciprocalSpaceYAxisRange(false), 
	_reciprocalSpaceYAxisRangeStart(0.0), 
	_reciprocalSpaceYAxisRangeEnd(1.0)
{
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool CorrelationFunctionModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void CorrelationFunctionModifier::initializeModifier(ModifierApplication* modApp)
{
	AsynchronousModifier::initializeModifier(modApp);

	// Use the first available particle property from the input state as data source when the modifier is newly created.
	if((sourceProperty1().isNull() || sourceProperty2().isNull()) && !Application::instance()->scriptMode()) {
		const PipelineFlowState& input = modApp->evaluateInputPreliminary();
		if(const ParticlesObject* container = input.getObject<ParticlesObject>()) {
			ParticlePropertyReference bestProperty;
			for(const PropertyObject* property : container->properties()) {
				bestProperty = ParticlePropertyReference(property, (property->componentCount() > 1) ? 0 : -1);
			}
			if(!bestProperty.isNull()) {
				if(sourceProperty1().isNull())
					setSourceProperty1(bestProperty);
				if(sourceProperty2().isNull())
					setSourceProperty2(bestProperty);
			}
		}
	}
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> CorrelationFunctionModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get the source data.
	if(sourceProperty1().isNull())
		throwException(tr("Please select a first input particle property."));
	if(sourceProperty2().isNull())
		throwException(tr("please select a second input particle property."));

	// Get the current positions.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);

	// Get the current selected properties.
	const PropertyObject* property1 = sourceProperty1().findInContainer(particles);
	const PropertyObject* property2 = sourceProperty2().findInContainer(particles);
	if(!property1)
		throwException(tr("The selected input particle property with the name '%1' does not exist.").arg(sourceProperty1().name()));
	if(!property2)
		throwException(tr("The selected input particle property with the name '%1' does not exist.").arg(sourceProperty2().name()));

	// Get simulation cell.
	const SimulationCellObject* inputCell = input.expectObject<SimulationCellObject>();
	if(inputCell->is2D())
		throwException(tr("Correlation function modifier does not support two-dimensional systems."));
	if(inputCell->volume3D() < FLOATTYPE_EPSILON)
		throwException(tr("Simulation cell is degenerate. Cannot compute correlation function."));

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<CorrelationAnalysisEngine>(posProperty->storage(),
													   property1->storage(),
													   std::max(0, sourceProperty1().vectorComponent()),
													   property2->storage(),
													   std::max(0, sourceProperty2().vectorComponent()),
													   inputCell->data(),
													   fftGridSpacing(),
													   applyWindow(),
													   doComputeNeighCorrelation(),
													   neighCutoff(),
													   numberOfNeighBins(),
													   averagingDirection());
}

/******************************************************************************
* Map property onto grid.
******************************************************************************/
std::vector<FloatType> CorrelationFunctionModifier::CorrelationAnalysisEngine::mapToSpatialGrid(const PropertyStorage* property,
																			  size_t propertyVectorComponent,
																			  const AffineTransformation& reciprocalCellMatrix,
																			  int nX, int nY, int nZ,
																			  bool applyWindow)
{
	size_t vecComponent = std::max(size_t(0), propertyVectorComponent);
	size_t vecComponentCount = property ? property->componentCount() : 0;
	int numberOfGridPoints = nX * nY * nZ;

	// Alloocate real space grid.
	std::vector<FloatType> gridData(numberOfGridPoints, 0);

	// Get periodic boundary flag.
	const std::array<bool, 3> pbc = cell().pbcFlags();

	if(!property || property->size() > 0) {
		const Point3* pos = positions()->constDataPoint3();
		const Point3* pos_end = pos + positions()->size();

		if(!property) {
			for(; pos != pos_end; ++pos) {
				Point3 fractionalPos = reciprocalCellMatrix*(*pos);
				int binIndexX = int( fractionalPos.x() * nX );
				int binIndexY = int( fractionalPos.y() * nY );
				int binIndexZ = int( fractionalPos.z() * nZ );
				FloatType window = 1;
				if(pbc[0]) binIndexX = SimulationCell::modulo(binIndexX, nX);
				else window *= sqrt(2./3)*(1-cos(2*FLOATTYPE_PI*fractionalPos.x()));
				if(pbc[1]) binIndexY = SimulationCell::modulo(binIndexY, nY);
				else window *= sqrt(2./3)*(1-cos(2*FLOATTYPE_PI*fractionalPos.y()));
				if(pbc[2]) binIndexZ = SimulationCell::modulo(binIndexZ, nZ);
				else window *= sqrt(2./3)*(1-cos(2*FLOATTYPE_PI*fractionalPos.z()));
				if (!applyWindow) window = 1;
				if(binIndexX >= 0 && binIndexX < nX && binIndexY >= 0 && binIndexY < nY && binIndexZ >= 0 && binIndexZ < nZ) {
					// Store in row-major format.
					size_t binIndex = binIndexZ+nZ*(binIndexY+nY*binIndexX);
					gridData[binIndex] += window;
				}
			}
		}
		else if(property->dataType() == PropertyStorage::Float) {
			auto v = property->constDataFloat() + vecComponent;
			auto v_end = v + (property->size() * vecComponentCount);
			for(; v != v_end; v += vecComponentCount, ++pos) {
				if(!std::isnan(*v)) {
					Point3 fractionalPos = reciprocalCellMatrix*(*pos);
					int binIndexX = int( fractionalPos.x() * nX );
					int binIndexY = int( fractionalPos.y() * nY );
					int binIndexZ = int( fractionalPos.z() * nZ );
					FloatType window = 1;
					if(pbc[0]) binIndexX = SimulationCell::modulo(binIndexX, nX);
					else window *= sqrt(2./3)*(1-cos(2*FLOATTYPE_PI*fractionalPos.x()));
					if(pbc[1]) binIndexY = SimulationCell::modulo(binIndexY, nY);
					else window *= sqrt(2./3)*(1-cos(2*FLOATTYPE_PI*fractionalPos.y()));
					if(pbc[2]) binIndexZ = SimulationCell::modulo(binIndexZ, nZ);
					else window *= sqrt(2./3)*(1-cos(2*FLOATTYPE_PI*fractionalPos.z()));
					if(!applyWindow) window = 1;
					if(binIndexX >= 0 && binIndexX < nX && binIndexY >= 0 && binIndexY < nY && binIndexZ >= 0 && binIndexZ < nZ) {
						// Store in row-major format.
						size_t binIndex = binIndexZ+nZ*(binIndexY+nY*binIndexX);
						gridData[binIndex] += window*(*v);
					}
				}
			}
		}
		else if(property->dataType() == PropertyStorage::Int) {
			auto v = property->constDataInt() + vecComponent;
			auto v_end = v + (property->size() * vecComponentCount);
			for(; v != v_end; v += vecComponentCount, ++pos) {
				Point3 fractionalPos = reciprocalCellMatrix*(*pos);
				int binIndexX = int( fractionalPos.x() * nX );
				int binIndexY = int( fractionalPos.y() * nY );
				int binIndexZ = int( fractionalPos.z() * nZ );
				FloatType window = 1;
				if(pbc[0]) binIndexX = SimulationCell::modulo(binIndexX, nX);
				else window *= sqrt(2./3)*(1-cos(2*FLOATTYPE_PI*fractionalPos.x()));
				if(pbc[1]) binIndexY = SimulationCell::modulo(binIndexY, nY);
				else window *= sqrt(2./3)*(1-cos(2*FLOATTYPE_PI*fractionalPos.y()));
				if(pbc[2]) binIndexZ = SimulationCell::modulo(binIndexZ, nZ);
				else window *= sqrt(2./3)*(1-cos(2*FLOATTYPE_PI*fractionalPos.z()));
				if(!applyWindow) window = 1;
				if(binIndexX >= 0 && binIndexX < nX && binIndexY >= 0 && binIndexY < nY && binIndexZ >= 0 && binIndexZ < nZ) {
					// Store in row-major format.
					size_t binIndex = binIndexZ+nZ*(binIndexY+nY*binIndexX);
					gridData[binIndex] += window*(*v);
				}
			}
		}
		else if(property->dataType() == PropertyStorage::Int64) {
			auto v = property->constDataInt64() + vecComponent;
			auto v_end = v + (property->size() * vecComponentCount);
			for(; v != v_end; v += vecComponentCount, ++pos) {
				Point3 fractionalPos = reciprocalCellMatrix*(*pos);
				int binIndexX = int( fractionalPos.x() * nX );
				int binIndexY = int( fractionalPos.y() * nY );
				int binIndexZ = int( fractionalPos.z() * nZ );
				FloatType window = 1;
				if(pbc[0]) binIndexX = SimulationCell::modulo(binIndexX, nX);
				else window *= sqrt(2./3)*(1-cos(2*FLOATTYPE_PI*fractionalPos.x()));
				if(pbc[1]) binIndexY = SimulationCell::modulo(binIndexY, nY);
				else window *= sqrt(2./3)*(1-cos(2*FLOATTYPE_PI*fractionalPos.y()));
				if(pbc[2]) binIndexZ = SimulationCell::modulo(binIndexZ, nZ);
				else window *= sqrt(2./3)*(1-cos(2*FLOATTYPE_PI*fractionalPos.z()));
				if(!applyWindow) window = 1;
				if(binIndexX >= 0 && binIndexX < nX && binIndexY >= 0 && binIndexY < nY && binIndexZ >= 0 && binIndexZ < nZ) {
					// Store in row-major format.
					size_t binIndex = binIndexZ+nZ*(binIndexY+nY*binIndexX);
					gridData[binIndex] += window*(*v);
				}
			}
		}
	}
	return gridData;
}

// Use single precision FFTW if Ovito is compiled with single precision
// floating point type.
#ifdef FLOATTYPE_FLOAT
#define fftw_complex fftwf_complex
#define fftw_plan_dft_r2c_3d fftwf_plan_dft_r2c_3d
#define fftw_plan_dft_c2r_3d fftwf_plan_dft_c2r_3d
#define fftw_execute fftwf_execute
#define fftw_destroy_plan fftwf_destroy_plan
#endif

std::vector<std::complex<FloatType>> CorrelationFunctionModifier::CorrelationAnalysisEngine::r2cFFT(int nX, int nY, int nZ, std::vector<FloatType> &rData)
{
	std::vector<std::complex<FloatType>> cData(nX * nY * (nZ/2+1));

	// Only serial access to FFTW3 functions allowed because they are not thread-safe.
	QMutexLocker locker(&_fftwMutex);		
	auto plan = fftw_plan_dft_r2c_3d(
		nX, nY, nZ,
		rData.data(),
		reinterpret_cast<fftw_complex*>(cData.data()),
		FFTW_ESTIMATE);
	locker.unlock();
	fftw_execute(plan);
	locker.relock();
	fftw_destroy_plan(plan);

	return cData;
}

std::vector<FloatType> CorrelationFunctionModifier::CorrelationAnalysisEngine::c2rFFT(int nX, int nY, int nZ, std::vector<std::complex<FloatType>>& cData)
{
	std::vector<FloatType> rData(nX * nY * nZ);
	// Only serial access to FFTW3 functions allowed because they are not thread-safe.
	QMutexLocker locker(&_fftwMutex);		
	auto plan = fftw_plan_dft_c2r_3d(
		nX, nY, nZ,
		reinterpret_cast<fftw_complex*>(cData.data()),
		rData.data(),
		FFTW_ESTIMATE);
	locker.unlock();
	fftw_execute(plan);
	locker.relock();
	fftw_destroy_plan(plan);

	return rData;
}

/******************************************************************************
* Compute real and reciprocal space correlation function via FFT.
******************************************************************************/
void CorrelationFunctionModifier::CorrelationAnalysisEngine::computeFftCorrelation()
{
	// Get reciprocal cell.
	const AffineTransformation& cellMatrix = cell().matrix();
	const AffineTransformation& reciprocalCellMatrix = cell().inverseMatrix();

	// Note: Cell vectors are in columns. Those are 3-vectors.
	int nX = std::max(1, (int)(cellMatrix.column(0).length() / fftGridSpacing()));
	int nY = std::max(1, (int)(cellMatrix.column(1).length() / fftGridSpacing()));
	int nZ = std::max(1, (int)(cellMatrix.column(2).length() / fftGridSpacing()));

	// Map all quantities onto a spatial grid.
	std::vector<FloatType> gridProperty1 = mapToSpatialGrid(sourceProperty1().get(),
					 _vecComponent1,
					 reciprocalCellMatrix,
					 nX, nY, nZ,
					 _applyWindow);

	task()->nextProgressSubStep();
	if(task()->isCanceled())
		return;

	std::vector<FloatType> gridProperty2 = mapToSpatialGrid(sourceProperty2().get(),
					 _vecComponent2,
					 reciprocalCellMatrix,
					 nX, nY, nZ,
					 _applyWindow);
	task()->nextProgressSubStep();
	if(task()->isCanceled())
		return;

	std::vector<FloatType> gridDensity = mapToSpatialGrid(nullptr,
					 _vecComponent1,
					 reciprocalCellMatrix,
					 nX, nY, nZ,
					 _applyWindow);	
	task()->nextProgressSubStep();
	if(task()->isCanceled())
		return;

	// FIXME. Apply windowing function in nonperiodic directions here.

	// Compute reciprocal-space correlation function from a product in Fourier space.

	// Compute Fourier transform of spatial grid.
	std::vector<std::complex<FloatType>> ftProperty1 = r2cFFT(nX, nY, nZ, gridProperty1);
	task()->nextProgressSubStep();
	if(task()->isCanceled())
		return;

	std::vector<std::complex<FloatType>> ftProperty2 = r2cFFT(nX, nY, nZ, gridProperty2);
	task()->nextProgressSubStep();
	if(task()->isCanceled())
		return;

	std::vector<std::complex<FloatType>> ftDensity = r2cFFT(nX, nY, nZ, gridDensity);
	task()->nextProgressSubStep();
	if(task()->isCanceled())
		return;

	// Note: Reciprocal cell vectors are in rows. Those are 4-vectors.
	Vector4 recCell1 = reciprocalCellMatrix.row(0);
	Vector4 recCell2 = reciprocalCellMatrix.row(1);
	Vector4 recCell3 = reciprocalCellMatrix.row(2);

	// Compute distance of cell faces.
	FloatType cellFaceDistance1 = 1 / sqrt(recCell1.x()*recCell1.x() + recCell1.y()*recCell1.y() + recCell1.z()*recCell1.z());
	FloatType cellFaceDistance2 = 1 / sqrt(recCell2.x()*recCell2.x() + recCell2.y()*recCell2.y() + recCell2.z()*recCell2.z());
	FloatType cellFaceDistance3 = 1 / sqrt(recCell3.x()*recCell3.x() + recCell3.y()*recCell3.y() + recCell3.z()*recCell3.z());

	FloatType minCellFaceDistance = std::min({cellFaceDistance1, cellFaceDistance2, cellFaceDistance3});

	// Minimum reciprocal space vector is given by the minimum distance of cell faces.
	FloatType minReciprocalSpaceVector = 1 / minCellFaceDistance;
	int numberOfWavevectorBins, dir1, dir2;
	Vector3I n(nX, nY, nZ);

	if(_averagingDirection == RADIAL) {
		numberOfWavevectorBins = 1 / (2 * minReciprocalSpaceVector * fftGridSpacing());
	}
	else {
		dir1 = (_averagingDirection+1) % 3;
		dir2 = (_averagingDirection+2) % 3;
		numberOfWavevectorBins = n[dir1] * n[dir2];
	}

	// Averaged reciprocal space correlation function.
	_reciprocalSpaceCorrelation = std::make_shared<PropertyStorage>(numberOfWavevectorBins, PropertyStorage::Float, 1, 0, tr("C(q)"), true, DataSeriesObject::YProperty);
	_reciprocalSpaceCorrelationRange = 2 * FLOATTYPE_PI * minReciprocalSpaceVector * numberOfWavevectorBins;

	std::vector<int> numberOfValues(numberOfWavevectorBins, 0);
	FloatType* reciprocalSpaceCorrelationData = reciprocalSpaceCorrelation()->dataFloat();

	// Compute Fourier-transformed correlation function and put it on a radial grid.
	int binIndex = 0;
	for(int binIndexX = 0; binIndexX < nX; binIndexX++) {
		for(int binIndexY = 0; binIndexY < nY; binIndexY++) {
			for(int binIndexZ = 0; binIndexZ < nZ/2+1; binIndexZ++, binIndex++) {
				// Compute correlation function.
				std::complex<FloatType> corr = ftProperty1[binIndex] * std::conj(ftProperty2[binIndex]);

				// Store correlation function to property1 for back transform.
				ftProperty1[binIndex] = corr;

				// Compute structure factor/radial distribution function.
				ftDensity[binIndex] = ftDensity[binIndex] * std::conj(ftDensity[binIndex]);

				int wavevectorBinIndex;
				if(_averagingDirection == RADIAL) {
					// Ignore Gamma-point for radial average.
					if(binIndex == 0 && binIndexY == 0 && binIndexZ == 0)
						continue;

					// Compute wavevector.
					int iX = SimulationCell::modulo(binIndexX+nX/2, nX)-nX/2;
					int iY = SimulationCell::modulo(binIndexY+nY/2, nY)-nY/2;
					int iZ = SimulationCell::modulo(binIndexZ+nZ/2, nZ)-nZ/2;
					// This is the reciprocal space vector (without a factor of 2*pi).
					Vector4 wavevector = FloatType(iX)*reciprocalCellMatrix.row(0) +
										 FloatType(iY)*reciprocalCellMatrix.row(1) +
							 			 FloatType(iZ)*reciprocalCellMatrix.row(2);
					wavevector.w() = 0.0;

					// Compute bin index.
					wavevectorBinIndex = int(std::floor(wavevector.length() / minReciprocalSpaceVector));
				}
				else {
					Vector3I binIndexXYZ(binIndexX, binIndexY, binIndexZ);
					wavevectorBinIndex = binIndexXYZ[dir2] + n[dir2]*binIndexXYZ[dir1];
				}

				if(wavevectorBinIndex >= 0 && wavevectorBinIndex < numberOfWavevectorBins) {
					reciprocalSpaceCorrelationData[wavevectorBinIndex] += std::real(corr);
					numberOfValues[wavevectorBinIndex]++;
				}
			}
		}
		if(task()->isCanceled()) return;
	}

	// Compute averages and normalize reciprocal-space correlation function.
	FloatType normalizationFactor = cell().volume3D() / (sourceProperty1()->size() * sourceProperty2()->size());
	for(int wavevectorBinIndex = 0; wavevectorBinIndex < numberOfWavevectorBins; wavevectorBinIndex++) {
		if(numberOfValues[wavevectorBinIndex] != 0)
			reciprocalSpaceCorrelationData[wavevectorBinIndex] *= normalizationFactor / numberOfValues[wavevectorBinIndex];
	}
	task()->nextProgressSubStep();
	if(task()->isCanceled())
		return;

	// Compute long-ranged part of the real-space correlation function from the FFT convolution.

	// Computer inverse Fourier transform of correlation function.
	gridProperty1 = c2rFFT(nX, nY, nZ, ftProperty1);
	task()->nextProgressSubStep();
	if(task()->isCanceled())
		return;

	gridDensity = c2rFFT(nX, nY, nZ, ftDensity);
	task()->nextProgressSubStep();
	if(task()->isCanceled())
		return;

	// Determine number of grid points for reciprocal-spacespace correlation function.
	int numberOfDistanceBins = minCellFaceDistance / (2 * fftGridSpacing());
	FloatType gridSpacing = minCellFaceDistance / (2 * numberOfDistanceBins);

	// Radially averaged real space correlation function.
	_realSpaceCorrelation = std::make_shared<PropertyStorage>(numberOfDistanceBins, PropertyStorage::Float, 1, 0, tr("C(r)"), true, DataSeriesObject::YProperty);
	_realSpaceCorrelationRange = minCellFaceDistance / 2;
	_realSpaceRDF = std::make_shared<PropertyStorage>(numberOfDistanceBins, PropertyStorage::Float, 1, 0, tr("g(r)"), true, DataSeriesObject::YProperty);

	numberOfValues = std::vector<int>(numberOfDistanceBins, 0);
	FloatType* realSpaceCorrelationData = realSpaceCorrelation()->dataFloat();
	FloatType* realSpaceRDFData = realSpaceRDF()->dataFloat();

	// Put real-space correlation function on a radial grid.
	binIndex = 0;
	for(int binIndexX = 0; binIndexX < nX; binIndexX++) {
		for(int binIndexY = 0; binIndexY < nY; binIndexY++) {
			for(int binIndexZ = 0; binIndexZ < nZ; binIndexZ++, binIndex++) {
				// Ignore origin for radial average (which is just the covariance of the quantities).
				if(binIndex == 0 && binIndexY == 0 && binIndexZ == 0)
					continue;

				// Compute distance. (FIXME! Check that this is actually correct for even and odd numbers of grid points.)
				FloatType fracX = FloatType(SimulationCell::modulo(binIndexX+nX/2, nX)-nX/2)/nX;
				FloatType fracY = FloatType(SimulationCell::modulo(binIndexY+nY/2, nY)-nY/2)/nY;
				FloatType fracZ = FloatType(SimulationCell::modulo(binIndexZ+nZ/2, nZ)-nZ/2)/nZ;
				// This is the real space vector.
				Vector3 distance = fracX*cellMatrix.column(0) +
						 		   fracY*cellMatrix.column(1) +
						 		   fracZ*cellMatrix.column(2);

				// Length of real space vector.
				int distanceBinIndex = int(std::floor(distance.length()/gridSpacing));
				if(distanceBinIndex >= 0 && distanceBinIndex < numberOfDistanceBins) {
					realSpaceCorrelationData[distanceBinIndex] += gridProperty1[binIndex];
					realSpaceRDFData[distanceBinIndex] += gridDensity[binIndex];
					numberOfValues[distanceBinIndex]++;
				}
			}
		}
		if(task()->isCanceled()) return;
	}

	// Compute averages and normalize real-space correlation function. Note FFTW computes an unnormalized transform.
	normalizationFactor = 1.0 / (sourceProperty1()->size() * sourceProperty2()->size());
	for(int distanceBinIndex = 0; distanceBinIndex < numberOfDistanceBins; distanceBinIndex++) {
		if(numberOfValues[distanceBinIndex] != 0) {
			realSpaceCorrelationData[distanceBinIndex] *= normalizationFactor / numberOfValues[distanceBinIndex];
			realSpaceRDFData[distanceBinIndex] *= normalizationFactor / numberOfValues[distanceBinIndex];
		}
	}

	task()->nextProgressSubStep();
}

/******************************************************************************
* Compute real space correlation function via direction summation over neighbors.
******************************************************************************/
void CorrelationFunctionModifier::CorrelationAnalysisEngine::computeNeighCorrelation()
{
	// Get number of particles.
	size_t particleCount = positions()->size();

	// Get pointers to data.
	const FloatType* floatData1 = nullptr;
	const FloatType* floatData2 = nullptr;
	const int* intData1 = nullptr;
	const int* intData2 = nullptr; 
	const qlonglong* int64Data1 = nullptr;
	const qlonglong* int64Data2 = nullptr; 
	size_t componentCount1 = sourceProperty1()->componentCount();
	size_t componentCount2 = sourceProperty2()->componentCount();
	if(sourceProperty1()->dataType() == PropertyStorage::Float) {
		floatData1 = sourceProperty1()->constDataFloat();
	}
	else if(sourceProperty1()->dataType() == PropertyStorage::Int) {
		intData1 = sourceProperty1()->constDataInt();
	}
	else if(sourceProperty1()->dataType() == PropertyStorage::Int64) {
		int64Data1 = sourceProperty1()->constDataInt64();
	}
	if(sourceProperty2()->dataType() == PropertyStorage::Float) {
		floatData2 = sourceProperty2()->constDataFloat();
	}
	else if(sourceProperty2()->dataType() == PropertyStorage::Int) {
		intData2 = sourceProperty2()->constDataInt();
	}
	else if(sourceProperty2()->dataType() == PropertyStorage::Int64) {
		int64Data2 = sourceProperty2()->constDataInt64();
	}

	// Allocate neighbor RDF.
	_neighRDF = std::make_shared<PropertyStorage>(neighCorrelation()->size(), PropertyStorage::Float, 1, 0, tr("Neighbor g(r)"), true, DataSeriesObject::YProperty);

	// Prepare the neighbor list.
	CutoffNeighborFinder neighborListBuilder;
	if(!neighborListBuilder.prepare(neighCutoff(), *positions(), cell(), nullptr, task().get()))
		return;

	// Perform analysis on each particle in parallel.
	size_t vecComponent1 = _vecComponent1;
	size_t vecComponent2 = _vecComponent2;
	task()->setProgressValue(0);
	task()->setProgressMaximum(particleCount);
	std::mutex mutex;
	parallelForChunks(particleCount, *task(), [&,this](size_t startIndex, size_t chunkSize, PromiseState& promise) {
		FloatType gridSpacing = (neighCutoff() + FLOATTYPE_EPSILON) / neighCorrelation()->size();
		std::vector<FloatType> threadLocalCorrelation(neighCorrelation()->size(), 0);
		std::vector<int> threadLocalRDF(neighCorrelation()->size(), 0);
		size_t endIndex = startIndex + chunkSize;
		for(size_t i = startIndex; i < endIndex; i++) {
			FloatType data1;
			if(floatData1) data1 = floatData1[i*componentCount1 + vecComponent1];
			else if(intData1) data1 = intData1[i*componentCount1 + vecComponent1];
			else if(int64Data1) data1 = int64Data1[i*componentCount1 + vecComponent1];
			else data1 = 0;
			for(CutoffNeighborFinder::Query neighQuery(neighborListBuilder, i); !neighQuery.atEnd(); neighQuery.next()) {
				size_t distanceBinIndex = (size_t)(sqrt(neighQuery.distanceSquared()) / gridSpacing);
				distanceBinIndex = std::min(distanceBinIndex, threadLocalCorrelation.size() - 1);
				FloatType data2;
				if(floatData2) data2 = floatData2[neighQuery.current() * componentCount2 + vecComponent2];
				else if(intData2) data2 = intData2[neighQuery.current() * componentCount2 + vecComponent2];
				else if(int64Data2) data2 = int64Data2[neighQuery.current() * componentCount2 + vecComponent2];
				else data2 = 0;
				threadLocalCorrelation[distanceBinIndex] += data1 * data2;
				threadLocalRDF[distanceBinIndex]++;
			}
			// Update progress indicator.
			if((i % 1024ll) == 0)
				promise.incrementProgressValue(1024);
			// Abort loop when operation was canceled by the user.
			if(promise.isCanceled())
				return;
		}
		std::lock_guard<std::mutex> lock(mutex);
		auto iter_corr_out = neighCorrelation()->dataFloat();
		for(auto iter_corr = threadLocalCorrelation.cbegin(); iter_corr != threadLocalCorrelation.cend(); ++iter_corr, ++iter_corr_out)
			*iter_corr_out += *iter_corr;
		auto iter_rdf_out = neighRDF()->dataFloat();
		for(auto iter_rdf = threadLocalRDF.cbegin(); iter_rdf != threadLocalRDF.cend(); ++iter_rdf, ++iter_rdf_out)
			*iter_rdf_out += *iter_rdf;
	});
	if(task()->isCanceled())
		return;
	task()->nextProgressSubStep();

	// Normalize short-ranged real-space correlation function.
	FloatType gridSpacing = (neighCutoff() + FLOATTYPE_EPSILON) / neighCorrelation()->size();
	FloatType normalizationFactor = 3 * cell().volume3D() / (4 * FLOATTYPE_PI * sourceProperty1()->size() * sourceProperty2()->size());
	for(int distanceBinIndex = 0; distanceBinIndex < neighCorrelation()->size(); distanceBinIndex++) {
		FloatType distance = distanceBinIndex * gridSpacing;
		FloatType distance2 = distance + gridSpacing;
		neighCorrelation()->dataFloat()[distanceBinIndex] *= normalizationFactor / (distance2*distance2*distance2 - distance*distance*distance);
		neighRDF()->dataFloat()[distanceBinIndex] *= normalizationFactor / (distance2*distance2*distance2 - distance*distance*distance);
	}

	task()->nextProgressSubStep();
}

/******************************************************************************
* Compute means and covariance.
******************************************************************************/
void CorrelationFunctionModifier::CorrelationAnalysisEngine::computeLimits()
{
	// Get pointers to data.
	const FloatType* floatData1 = nullptr;
	const FloatType* floatData2 = nullptr;
	const int* intData1 = nullptr;
	const int* intData2 = nullptr; 
	const qlonglong* int64Data1 = nullptr;
	const qlonglong* int64Data2 = nullptr; 
	size_t componentCount1 = sourceProperty1()->componentCount();
	size_t componentCount2 = sourceProperty2()->componentCount();
	if(sourceProperty1()->dataType() == PropertyStorage::Float) {
		floatData1 = sourceProperty1()->constDataFloat();
	}
	else if(sourceProperty1()->dataType() == PropertyStorage::Int) {
		intData1 = sourceProperty1()->constDataInt();
	}
	else if(sourceProperty1()->dataType() == PropertyStorage::Int64) {
		int64Data1 = sourceProperty1()->constDataInt64();
	}
	if(sourceProperty2()->dataType() == PropertyStorage::Float) {
		floatData2 = sourceProperty2()->constDataFloat();
	}
	else if(sourceProperty2()->dataType() == PropertyStorage::Int) {
		intData2 = sourceProperty2()->constDataInt();
	}
	else if(sourceProperty2()->dataType() == PropertyStorage::Int64) {
		int64Data2 = sourceProperty2()->constDataInt64();
	}

	// Compute mean and covariance values.
	FloatType mean1 = 0;
	FloatType mean2 = 0;
	FloatType variance1 = 0;
	FloatType variance2 = 0;
	FloatType covariance = 0;
	size_t particleCount = sourceProperty1()->size();
	for(size_t particleIndex = 0; particleIndex < particleCount; particleIndex++) {
		FloatType data1, data2;
		if(floatData1) data1 = floatData1[particleIndex*componentCount1 + _vecComponent1];
		else if(intData1) data1 = intData1[particleIndex*componentCount1 + _vecComponent1];
		else if(int64Data1) data1 = int64Data1[particleIndex*componentCount1 + _vecComponent1];
		else data1 = 0;
		if(floatData2) data2 = floatData2[particleIndex*componentCount2 + _vecComponent2];
		else if(intData2) data2 = intData2[particleIndex*componentCount2 + _vecComponent2];
		else if(int64Data2) data2 = int64Data2[particleIndex*componentCount2 + _vecComponent2];
		else data2 = 0;
		mean1 += data1;
		mean2 += data2;
		variance1 += data1*data1;
		variance2 += data2*data2;
		covariance += data1*data2;
		if(task()->isCanceled()) return;
	}
	mean1 /= particleCount;
	mean2 /= particleCount;
	variance1 /= particleCount;
	variance2 /= particleCount;
	covariance /= particleCount;
	setMoments(mean1, mean2, variance1, variance2, covariance);
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void CorrelationFunctionModifier::CorrelationAnalysisEngine::perform()
{
	task()->setProgressText(tr("Computing correlation function"));
	task()->beginProgressSubSteps(neighCorrelation() ? 13 : 11);

	// Compute reciprocal space correlation function and long-ranged part of
	// the real-space correlation function from an FFT.
	computeFftCorrelation();
	if(task()->isCanceled())
		return;

	// Compute short-ranged part of the real-space correlation function from a direct loop over particle neighbors.
	if(neighCorrelation())
		computeNeighCorrelation();
	if(task()->isCanceled())
		return;

	computeLimits();
	task()->endProgressSubSteps();
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
PipelineFlowState CorrelationFunctionModifier::CorrelationAnalysisEngine::emitResults(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	PipelineFlowState output = input;

	// Output real-space correlation function to the pipeline as a data series. 
	DataSeriesObject* realSpaceCorrelationObj = output.createObject<DataSeriesObject>(QStringLiteral("correlation/real-space"), modApp, tr("Real-space correlation"), realSpaceCorrelation());
	realSpaceCorrelationObj->setAxisLabelX(tr("Distance r"));
	realSpaceCorrelationObj->setIntervalStart(0);
	realSpaceCorrelationObj->setIntervalEnd(_realSpaceCorrelationRange);

	// Output real-space RDF to the pipeline as a data series. 
	DataSeriesObject* realSpaceRDFObj = output.createObject<DataSeriesObject>(QStringLiteral("correlation/real-space/rdf"), modApp, tr("Real-space RDF"), realSpaceRDF());
	realSpaceRDFObj->setAxisLabelX(tr("Distance r"));
	realSpaceRDFObj->setIntervalStart(0);
	realSpaceRDFObj->setIntervalEnd(_realSpaceCorrelationRange);

	// Output short-ranged part of the real-space correlation function to the pipeline as a data series. 
	if(neighCorrelation()) {
		DataSeriesObject* neighCorrelationObj = output.createObject<DataSeriesObject>(QStringLiteral("correlation/neighbor"), modApp, tr("Neighbor correlation"), neighCorrelation());
		neighCorrelationObj->setAxisLabelX(tr("Distance r"));
		neighCorrelationObj->setIntervalStart(0);
		neighCorrelationObj->setIntervalEnd(neighCutoff());
	}

	// Output short-ranged part of the RDF to the pipeline as a data series. 
	if(neighRDF()) {
		DataSeriesObject* neighRDFObj = output.createObject<DataSeriesObject>(QStringLiteral("correlation/neighbor/rdf"), modApp, tr("Neighbor RDF"), neighRDF());
		neighRDFObj->setAxisLabelX(tr("Distance r"));
		neighRDFObj->setIntervalStart(0);
		neighRDFObj->setIntervalEnd(neighCutoff());
	}

	// Output reciprocal-space correlation function to the pipeline as a data series. 
	DataSeriesObject* reciprocalSpaceCorrelationObj = output.createObject<DataSeriesObject>(QStringLiteral("correlation/reciprocal-space"), modApp, tr("Reciprocal-space correlation"), reciprocalSpaceCorrelation());
	reciprocalSpaceCorrelationObj->setAxisLabelX(tr("Wavevector q"));
	reciprocalSpaceCorrelationObj->setIntervalStart(0);
	reciprocalSpaceCorrelationObj->setIntervalEnd(_reciprocalSpaceCorrelationRange);

	// Output global attributes.
	output.addAttribute(QStringLiteral("CorrelationFunction.mean1"), QVariant::fromValue(mean1()), modApp);
	output.addAttribute(QStringLiteral("CorrelationFunction.mean2"), QVariant::fromValue(mean2()), modApp);
	output.addAttribute(QStringLiteral("CorrelationFunction.variance1"), QVariant::fromValue(variance1()), modApp);
	output.addAttribute(QStringLiteral("CorrelationFunction.variance2"), QVariant::fromValue(variance2()), modApp);
	output.addAttribute(QStringLiteral("CorrelationFunction.covariance"), QVariant::fromValue(covariance()), modApp);

	return output;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
