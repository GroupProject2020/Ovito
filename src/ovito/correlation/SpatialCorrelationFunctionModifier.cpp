////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
//  Copyright 2017 Lars Pastewka
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include "SpatialCorrelationFunctionModifier.h"

#include <kissfft/kiss_fftnd.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_OVITO_CLASS(SpatialCorrelationFunctionModifier);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, sourceProperty1);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, sourceProperty2);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, averagingDirection);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, fftGridSpacing);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, applyWindow);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, doComputeNeighCorrelation);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, neighCutoff);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, numberOfNeighBins);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, normalizeRealSpace);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, normalizeRealSpaceByRDF);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, normalizeRealSpaceByCovariance);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, typeOfRealSpacePlot);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, normalizeReciprocalSpace);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, typeOfReciprocalSpacePlot);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, fixRealSpaceXAxisRange);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, realSpaceXAxisRangeStart);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, realSpaceXAxisRangeEnd);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, fixRealSpaceYAxisRange);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, realSpaceYAxisRangeStart);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, realSpaceYAxisRangeEnd);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, fixReciprocalSpaceXAxisRange);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, reciprocalSpaceXAxisRangeStart);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, reciprocalSpaceXAxisRangeEnd);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, fixReciprocalSpaceYAxisRange);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, reciprocalSpaceYAxisRangeStart);
DEFINE_PROPERTY_FIELD(SpatialCorrelationFunctionModifier, reciprocalSpaceYAxisRangeEnd);
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, sourceProperty1, "First property");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, sourceProperty2, "Second property");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, averagingDirection, "Averaging direction");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, fftGridSpacing, "FFT grid spacing");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, applyWindow, "Apply window function to nonperiodic directions");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, doComputeNeighCorrelation, "Direct summation");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, neighCutoff, "Neighbor cutoff radius");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, numberOfNeighBins, "Number of neighbor bins");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, normalizeRealSpace, "Normalize correlation function");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, normalizeRealSpaceByRDF, "Normalize by RDF");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, normalizeRealSpaceByCovariance, "Normalize by covariance");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, normalizeReciprocalSpace, "Normalize correlation function");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(SpatialCorrelationFunctionModifier, fftGridSpacing, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(SpatialCorrelationFunctionModifier, neighCutoff, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(SpatialCorrelationFunctionModifier, numberOfNeighBins, IntegerParameterUnit, 4, 100000);
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, fixRealSpaceXAxisRange, "Fix x-range");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, realSpaceXAxisRangeStart, "X-range start");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, realSpaceXAxisRangeEnd, "X-range end");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, fixRealSpaceYAxisRange, "Fix y-range");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, realSpaceYAxisRangeStart, "Y-range start");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, realSpaceYAxisRangeEnd, "Y-range end");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, fixReciprocalSpaceXAxisRange, "Fix x-range");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, reciprocalSpaceXAxisRangeStart, "X-range start");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, reciprocalSpaceXAxisRangeEnd, "X-range end");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, fixReciprocalSpaceYAxisRange, "Fix y-range");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, reciprocalSpaceYAxisRangeStart, "Y-range start");
SET_PROPERTY_FIELD_LABEL(SpatialCorrelationFunctionModifier, reciprocalSpaceYAxisRangeEnd, "Y-range end");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
SpatialCorrelationFunctionModifier::SpatialCorrelationFunctionModifier(DataSet* dataset) : AsynchronousModifier(dataset),
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
bool SpatialCorrelationFunctionModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void SpatialCorrelationFunctionModifier::initializeModifier(ModifierApplication* modApp)
{
	AsynchronousModifier::initializeModifier(modApp);

	// Use the first available particle property from the input state as data source when the modifier is newly created.
	if((sourceProperty1().isNull() || sourceProperty2().isNull()) && Application::instance()->executionContext() == Application::ExecutionContext::Interactive) {
		const PipelineFlowState& input = modApp->evaluateInputSynchronous(dataset()->animationSettings()->time());
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
Future<AsynchronousModifier::ComputeEnginePtr> SpatialCorrelationFunctionModifier::createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get the source data.
	if(sourceProperty1().isNull())
		throwException(tr("Please select a first input particle property."));
	if(sourceProperty2().isNull())
		throwException(tr("please select a second input particle property."));

	// Get the current positions.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	particles->verifyIntegrity();
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
std::vector<FloatType> SpatialCorrelationFunctionModifier::CorrelationAnalysisEngine::mapToSpatialGrid(const PropertyStorage* property,
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
		ConstPropertyAccess<Point3> positionsArray(positions());
		if(!property) {
			for(const Point3& pos : positionsArray) {
				Point3 fractionalPos = reciprocalCellMatrix * pos;
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
			ConstPropertyAccess<FloatType,true> propertyArray(*property);
			const Point3* pos = positionsArray.cbegin();
			for(FloatType v : propertyArray.componentRange(vecComponent)) {
				if(!std::isnan(v)) {
					Point3 fractionalPos = reciprocalCellMatrix * (*pos);
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
						gridData[binIndex] += window*(v);
					}
				}
				++pos;
			}
		}
		else if(property->dataType() == PropertyStorage::Int) {
			ConstPropertyAccess<int,true> propertyArray(*property);
			const Point3* pos = positionsArray.cbegin();
			for(int v : propertyArray.componentRange(vecComponent)) {
				Point3 fractionalPos = reciprocalCellMatrix * (*pos);
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
					gridData[binIndex] += window*(v);
				}
				++pos;
			}
		}
		else if(property->dataType() == PropertyStorage::Int64) {
			ConstPropertyAccess<qlonglong,true> propertyArray(*property);
			const Point3* pos = positionsArray.cbegin();
			for(qlonglong v : propertyArray.componentRange(vecComponent)) {
				Point3 fractionalPos = reciprocalCellMatrix * (*pos);
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
					gridData[binIndex] += window*(v);
				}
				++pos;
			}
		}
	}
	return gridData;
}

std::vector<std::complex<FloatType>> SpatialCorrelationFunctionModifier::CorrelationAnalysisEngine::r2cFFT(int nX, int nY, int nZ, std::vector<FloatType> &rData)
{
	OVITO_ASSERT(nX * nY * nZ == rData.size());

	// Convert real-valued input data to complex data type, because KISS FFT expects an array of complex numbers.
	const int dims[3] = { nX, nY, nZ };
	kiss_fftnd_cfg kiss = kiss_fftnd_alloc(dims, 3, false, 0, 0);
	std::vector<kiss_fft_cpx> in(nX * nY * nZ);
	auto rDataIter = rData.begin();
	for(kiss_fft_cpx& c : in) {
		c.r = *rDataIter++;
		c.i = 0;
	}

	// Allocate the output buffer.
	std::vector<std::complex<FloatType>> cData(nX * nY * nZ);
	OVITO_STATIC_ASSERT(sizeof(kiss_fft_cpx) == sizeof(std::complex<FloatType>));

	// Perform FFT calculation.
	kiss_fftnd(kiss, in.data(), reinterpret_cast<kiss_fft_cpx*>(cData.data()));
	kiss_fft_free(kiss);

	return cData;
}

std::vector<FloatType> SpatialCorrelationFunctionModifier::CorrelationAnalysisEngine::c2rFFT(int nX, int nY, int nZ, std::vector<std::complex<FloatType>>& cData)
{
	OVITO_ASSERT(nX * nY * nZ == cData.size());

	const int dims[3] = { nX, nY, nZ };
	kiss_fftnd_cfg kiss = kiss_fftnd_alloc(dims, 3, true, 0, 0);
	std::vector<kiss_fft_cpx> out(nX * nY * nZ);
	OVITO_STATIC_ASSERT(sizeof(kiss_fft_cpx) == sizeof(std::complex<FloatType>));

	// Perform FFT calculation.
	kiss_fftnd(kiss, reinterpret_cast<const kiss_fft_cpx*>(cData.data()), out.data());
	kiss_fft_free(kiss);

	// Convert complex values to real values.
	std::vector<FloatType> rData(nX * nY * nZ);
	auto rDataIter = rData.begin();
	for(const kiss_fft_cpx& c : out) {
		*rDataIter++ = c.r;
	}

	return rData;
}

/******************************************************************************
* Compute real and reciprocal space correlation function via FFT.
******************************************************************************/
void SpatialCorrelationFunctionModifier::CorrelationAnalysisEngine::computeFftCorrelation()
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
	_reciprocalSpaceCorrelation = std::make_shared<PropertyStorage>(numberOfWavevectorBins, PropertyStorage::Float, 1, 0, tr("C(q)"), true, DataTable::YProperty);
	_reciprocalSpaceCorrelationRange = 2 * FLOATTYPE_PI * minReciprocalSpaceVector * numberOfWavevectorBins;

	std::vector<int> numberOfValues(numberOfWavevectorBins, 0);
	PropertyAccess<FloatType> reciprocalSpaceCorrelationData(_reciprocalSpaceCorrelation);

	// Compute Fourier-transformed correlation function and put it on a radial grid.
	int binIndex = 0;
	for(int binIndexX = 0; binIndexX < nX; binIndexX++) {
		for(int binIndexY = 0; binIndexY < nY; binIndexY++) {
			for(int binIndexZ = 0; binIndexZ < nZ; binIndexZ++, binIndex++) {
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
	_realSpaceCorrelation = std::make_shared<PropertyStorage>(numberOfDistanceBins, PropertyStorage::Float, 1, 0, tr("C(r)"), true, DataTable::YProperty);
	_realSpaceCorrelationRange = minCellFaceDistance / 2;
	_realSpaceRDF = std::make_shared<PropertyStorage>(numberOfDistanceBins, PropertyStorage::Float, 1, 0, tr("g(r)"), true, DataTable::YProperty);

	numberOfValues = std::vector<int>(numberOfDistanceBins, 0);
	PropertyAccess<FloatType> realSpaceCorrelationData(_realSpaceCorrelation);
	PropertyAccess<FloatType> realSpaceRDFData(_realSpaceRDF);

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

	// Compute averages and normalize real-space correlation function. Note KISS FFT computes an unnormalized transform.
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
void SpatialCorrelationFunctionModifier::CorrelationAnalysisEngine::computeNeighCorrelation()
{
	// Get number of particles.
	size_t particleCount = positions()->size();

	// Get pointers to data.
	ConstPropertyAccess<FloatType,true> floatData1;
	ConstPropertyAccess<FloatType,true> floatData2;
	ConstPropertyAccess<int,true> intData1;
	ConstPropertyAccess<int,true> intData2;
	ConstPropertyAccess<qlonglong,true> int64Data1;
	ConstPropertyAccess<qlonglong,true> int64Data2;
	if(sourceProperty1()->dataType() == PropertyStorage::Float) {
		floatData1 = sourceProperty1();
	}
	else if(sourceProperty1()->dataType() == PropertyStorage::Int) {
		intData1 = sourceProperty1();
	}
	else if(sourceProperty1()->dataType() == PropertyStorage::Int64) {
		int64Data1 = sourceProperty1();
	}
	if(sourceProperty2()->dataType() == PropertyStorage::Float) {
		floatData2 = sourceProperty2();
	}
	else if(sourceProperty2()->dataType() == PropertyStorage::Int) {
		intData2 = sourceProperty2();
	}
	else if(sourceProperty2()->dataType() == PropertyStorage::Int64) {
		int64Data2 = sourceProperty2();
	}

	// Allocate neighbor RDF.
	_neighRDF = std::make_shared<PropertyStorage>(neighCorrelation()->size(), PropertyStorage::Float, 1, 0, tr("Neighbor g(r)"), true, DataTable::YProperty);

	// Prepare the neighbor list.
	CutoffNeighborFinder neighborListBuilder;
	if(!neighborListBuilder.prepare(neighCutoff(), positions(), cell(), {}, task().get()))
		return;

	// Perform analysis on each particle in parallel.
	size_t vecComponent1 = _vecComponent1;
	size_t vecComponent2 = _vecComponent2;
	task()->setProgressValue(0);
	task()->setProgressMaximum(particleCount);
	std::mutex mutex;
	parallelForChunks(particleCount, *task(), [&,this](size_t startIndex, size_t chunkSize, Task& promise) {
		FloatType gridSpacing = (neighCutoff() + FLOATTYPE_EPSILON) / neighCorrelation()->size();
		std::vector<FloatType> threadLocalCorrelation(neighCorrelation()->size(), 0);
		std::vector<int> threadLocalRDF(neighCorrelation()->size(), 0);
		size_t endIndex = startIndex + chunkSize;
		for(size_t i = startIndex; i < endIndex; i++) {
			FloatType data1;
			if(floatData1) data1 = floatData1.get(i, vecComponent1);
			else if(intData1) data1 = intData1.get(i, vecComponent1);
			else if(int64Data1) data1 = int64Data1.get(i, vecComponent1);
			else data1 = 0;
			for(CutoffNeighborFinder::Query neighQuery(neighborListBuilder, i); !neighQuery.atEnd(); neighQuery.next()) {
				size_t distanceBinIndex = (size_t)(sqrt(neighQuery.distanceSquared()) / gridSpacing);
				distanceBinIndex = std::min(distanceBinIndex, threadLocalCorrelation.size() - 1);
				FloatType data2;
				if(floatData2) data2 = floatData2.get(neighQuery.current(), vecComponent2);
				else if(intData2) data2 = intData2.get(neighQuery.current(), vecComponent2);
				else if(int64Data2) data2 = int64Data2.get(neighQuery.current(), vecComponent2);
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
		PropertyAccess<FloatType> neighCorrelationArray(neighCorrelation());
		auto iter_corr_out = neighCorrelationArray.begin();
		for(auto iter_corr = threadLocalCorrelation.cbegin(); iter_corr != threadLocalCorrelation.cend(); ++iter_corr, ++iter_corr_out)
			*iter_corr_out += *iter_corr;
		PropertyAccess<FloatType> neighRDFArray(neighRDF());
		auto iter_rdf_out = neighRDFArray.begin();
		for(auto iter_rdf = threadLocalRDF.cbegin(); iter_rdf != threadLocalRDF.cend(); ++iter_rdf, ++iter_rdf_out)
			*iter_rdf_out += *iter_rdf;
	});
	if(task()->isCanceled())
		return;
	task()->nextProgressSubStep();

	// Normalize short-ranged real-space correlation function.
	FloatType gridSpacing = (neighCutoff() + FLOATTYPE_EPSILON) / neighCorrelation()->size();
	FloatType normalizationFactor = 3 * cell().volume3D() / (4 * FLOATTYPE_PI * sourceProperty1()->size() * sourceProperty2()->size());
	PropertyAccess<FloatType> neighCorrelationArray(neighCorrelation());
	PropertyAccess<FloatType> neighRDFArray(neighRDF());
	for(size_t distanceBinIndex = 0; distanceBinIndex < neighCorrelation()->size(); distanceBinIndex++) {
		FloatType distance = distanceBinIndex * gridSpacing;
		FloatType distance2 = distance + gridSpacing;
		neighCorrelationArray[distanceBinIndex] *= normalizationFactor / (distance2*distance2*distance2 - distance*distance*distance);
		neighRDFArray[distanceBinIndex] *= normalizationFactor / (distance2*distance2*distance2 - distance*distance*distance);
	}

	task()->nextProgressSubStep();
}

/******************************************************************************
* Compute means and covariance.
******************************************************************************/
void SpatialCorrelationFunctionModifier::CorrelationAnalysisEngine::computeLimits()
{
	// Get pointers to data.
	ConstPropertyAccess<FloatType,true> floatData1;
	ConstPropertyAccess<FloatType,true> floatData2;
	ConstPropertyAccess<int,true> intData1;
	ConstPropertyAccess<int,true> intData2;
	ConstPropertyAccess<qlonglong,true> int64Data1;
	ConstPropertyAccess<qlonglong,true> int64Data2;
	if(sourceProperty1()->dataType() == PropertyStorage::Float) {
		floatData1 = sourceProperty1();
	}
	else if(sourceProperty1()->dataType() == PropertyStorage::Int) {
		intData1 = sourceProperty1();
	}
	else if(sourceProperty1()->dataType() == PropertyStorage::Int64) {
		int64Data1 = sourceProperty1();
	}
	if(sourceProperty2()->dataType() == PropertyStorage::Float) {
		floatData2 = sourceProperty2();
	}
	else if(sourceProperty2()->dataType() == PropertyStorage::Int) {
		intData2 = sourceProperty2();
	}
	else if(sourceProperty2()->dataType() == PropertyStorage::Int64) {
		int64Data2 = sourceProperty2();
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
		if(floatData1) data1 = floatData1.get(particleIndex, _vecComponent1);
		else if(intData1) data1 = intData1.get(particleIndex, _vecComponent1);
		else if(int64Data1) data1 = int64Data1.get(particleIndex, _vecComponent1);
		else data1 = 0;
		if(floatData2) data2 = floatData2.get(particleIndex, _vecComponent2);
		else if(intData2) data2 = intData2.get(particleIndex, _vecComponent2);
		else if(int64Data2) data2 = int64Data2.get(particleIndex, _vecComponent2);
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
void SpatialCorrelationFunctionModifier::CorrelationAnalysisEngine::perform()
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
void SpatialCorrelationFunctionModifier::CorrelationAnalysisEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	// Output real-space correlation function to the pipeline as a data table.
	DataTable* realSpaceCorrelationObj = state.createObject<DataTable>(QStringLiteral("correlation-real-space"), modApp, DataTable::Line, tr("Real-space correlation"), realSpaceCorrelation());
	realSpaceCorrelationObj->setAxisLabelX(tr("Distance r"));
	realSpaceCorrelationObj->setIntervalStart(0);
	realSpaceCorrelationObj->setIntervalEnd(_realSpaceCorrelationRange);

	// Output real-space RDF to the pipeline as a data table.
	DataTable* realSpaceRDFObj = state.createObject<DataTable>(QStringLiteral("correlation-real-space-rdf"), modApp, DataTable::Line, tr("Real-space RDF"), realSpaceRDF());
	realSpaceRDFObj->setAxisLabelX(tr("Distance r"));
	realSpaceRDFObj->setIntervalStart(0);
	realSpaceRDFObj->setIntervalEnd(_realSpaceCorrelationRange);

	// Output short-ranged part of the real-space correlation function to the pipeline as a data table.
	if(neighCorrelation()) {
		DataTable* neighCorrelationObj = state.createObject<DataTable>(QStringLiteral("correlation-neighbor"), modApp, DataTable::Line, tr("Neighbor correlation"), neighCorrelation());
		neighCorrelationObj->setAxisLabelX(tr("Distance r"));
		neighCorrelationObj->setIntervalStart(0);
		neighCorrelationObj->setIntervalEnd(neighCutoff());
	}

	// Output short-ranged part of the RDF to the pipeline as a data table.
	if(neighRDF()) {
		DataTable* neighRDFObj = state.createObject<DataTable>(QStringLiteral("correlation-neighbor-rdf"), modApp, DataTable::Line, tr("Neighbor RDF"), neighRDF());
		neighRDFObj->setAxisLabelX(tr("Distance r"));
		neighRDFObj->setIntervalStart(0);
		neighRDFObj->setIntervalEnd(neighCutoff());
	}

	// Output reciprocal-space correlation function to the pipeline as a data table.
	DataTable* reciprocalSpaceCorrelationObj = state.createObject<DataTable>(QStringLiteral("correlation-reciprocal-space"), modApp, DataTable::Line, tr("Reciprocal-space correlation"), reciprocalSpaceCorrelation());
	reciprocalSpaceCorrelationObj->setAxisLabelX(tr("Wavevector q"));
	reciprocalSpaceCorrelationObj->setIntervalStart(0);
	reciprocalSpaceCorrelationObj->setIntervalEnd(_reciprocalSpaceCorrelationRange);

	// Output global attributes.
	state.addAttribute(QStringLiteral("CorrelationFunction.mean1"), QVariant::fromValue(mean1()), modApp);
	state.addAttribute(QStringLiteral("CorrelationFunction.mean2"), QVariant::fromValue(mean2()), modApp);
	state.addAttribute(QStringLiteral("CorrelationFunction.variance1"), QVariant::fromValue(variance1()), modApp);
	state.addAttribute(QStringLiteral("CorrelationFunction.variance2"), QVariant::fromValue(variance2()), modApp);
	state.addAttribute(QStringLiteral("CorrelationFunction.covariance"), QVariant::fromValue(covariance()), modApp);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
