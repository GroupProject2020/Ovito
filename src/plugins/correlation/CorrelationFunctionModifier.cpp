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
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <core/app/Application.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/utilities/units/UnitsManager.h>
#include "CorrelationFunctionModifier.h"

#include <QtConcurrent>
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

IMPLEMENT_OVITO_CLASS(CorrelationFunctionModifierApplication);

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
	_normalizeRealSpace(DO_NOT_NORMALIZE), 
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
	return input.findObject<ParticleProperty>() != nullptr;
}

/******************************************************************************
* Create a new modifier application that refers to this modifier instance.
******************************************************************************/
OORef<ModifierApplication> CorrelationFunctionModifier::createModifierApplication()
{
	OORef<ModifierApplication> modApp = new CorrelationFunctionModifierApplication(dataset());
	modApp->setModifier(this);
	return modApp;
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void CorrelationFunctionModifier::initializeModifier(ModifierApplication* modApp)
{
	AsynchronousModifier::initializeModifier(modApp);

	// Use the first available particle property from the input state as data source when the modifier is newly created.
	if(sourceProperty1().isNull() || sourceProperty2().isNull()) {
		PipelineFlowState input = modApp->evaluateInputPreliminary();
		ParticlePropertyReference bestProperty;
		for(DataObject* o : input.objects()) {
			ParticleProperty* property = dynamic_object_cast<ParticleProperty>(o);
			if(property && (property->dataType() == qMetaTypeId<int>() || property->dataType() == qMetaTypeId<FloatType>())) {
				bestProperty = ParticlePropertyReference(property, (property->componentCount() > 1) ? 0 : -1);
			}
		}
		if(!bestProperty.isNull()) {
			if (sourceProperty1().isNull())
				setSourceProperty1(bestProperty);
			if (sourceProperty2().isNull())
				setSourceProperty2(bestProperty);
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
		throwException(tr("Select a first particle property first."));
	if(sourceProperty2().isNull())
		throwException(tr("Select a second particle property first."));

	// Get the current positions.
	ParticleInputHelper pih(dataset(), input);
	ParticleProperty* posProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);

	// Get the current selected properties.
	ParticleProperty* property1 = sourceProperty1().findInState(input);
	ParticleProperty* property2 = sourceProperty2().findInState(input);
	if(!property1)
		throwException(tr("The selected particle property with the name '%1' does not exist.").arg(sourceProperty1().name()));
	if(!property2)
		throwException(tr("The selected particle property with the name '%1' does not exist.").arg(sourceProperty2().name()));

	// Get simulation cell.
	SimulationCellObject* inputCell = pih.expectSimulationCell();

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
void CorrelationFunctionModifier::CorrelationAnalysisEngine::mapToSpatialGrid(const PropertyStorage* property,
																			  size_t propertyVectorComponent,
																			  const AffineTransformation &reciprocalCellMatrix,
																			  int nX, int nY, int nZ,
																			  QVector<FloatType> &gridData,
																			  bool applyWindow)
{
	size_t vecComponent = std::max(size_t(0), propertyVectorComponent);
	size_t vecComponentCount = property ? property->componentCount() : 0;

	int numberOfGridPoints = nX*nY*nZ;

	// Resize real space grid.
	gridData.fill(0.0, numberOfGridPoints);

	// Get periodic boundary flag.
	std::array<bool, 3> pbc = cell().pbcFlags();

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
		else if(property->dataType() == qMetaTypeId<FloatType>()) {
			const FloatType* v = property->constDataFloat() + vecComponent;
			const FloatType* v_end = v + (property->size() * vecComponentCount);
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
					if (!applyWindow) window = 1;
					if(binIndexX >= 0 && binIndexX < nX && binIndexY >= 0 && binIndexY < nY && binIndexZ >= 0 && binIndexZ < nZ) {
						// Store in row-major format.
						size_t binIndex = binIndexZ+nZ*(binIndexY+nY*binIndexX);
						gridData[binIndex] += window*(*v);
					}
				}
			}
		}
		else if(property->dataType() == qMetaTypeId<int>()) {
			const int* v = property->constDataInt() + vecComponent;
			const int* v_end = v + (property->size() * vecComponentCount);
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
				if (!applyWindow) window = 1;
				if(binIndexX >= 0 && binIndexX < nX && binIndexY >= 0 && binIndexY < nY && binIndexZ >= 0 && binIndexZ < nZ) {
					// Store in row-major format.
					size_t binIndex = binIndexZ+nZ*(binIndexY+nY*binIndexX);
					gridData[binIndex] += window*(*v);
				}
			}
		}
	}
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

void CorrelationFunctionModifier::CorrelationAnalysisEngine::r2cFFT(int nX, int nY, int nZ,
																	QVector<FloatType> &rData,
																	QVector<std::complex<FloatType>> &cData)
{
	cData.resize(nX*nY*(nZ/2+1));
	auto plan = fftw_plan_dft_r2c_3d(
		nX, nY, nZ,
		rData.data(),
		reinterpret_cast<fftw_complex*>(cData.data()),
		FFTW_ESTIMATE);
	fftw_execute(plan);
	fftw_destroy_plan(plan);
}

void CorrelationFunctionModifier::CorrelationAnalysisEngine::c2rFFT(int nX, int nY, int nZ,
																	QVector<std::complex<FloatType>> &cData,
																	QVector<FloatType> &rData)
{
	rData.resize(nX*nY*nZ);
	auto plan = fftw_plan_dft_c2r_3d(
		nX, nY, nZ,
		reinterpret_cast<fftw_complex*>(cData.data()),
		rData.data(),
		FFTW_ESTIMATE);
	fftw_execute(plan);
	fftw_destroy_plan(plan);
}

/******************************************************************************
* Compute real and reciprocal space correlation function via FFT.
******************************************************************************/
void CorrelationFunctionModifier::CorrelationAnalysisEngine::computeFftCorrelation()
{
	// Get reciprocal cell.
	AffineTransformation cellMatrix = cell().matrix();
	AffineTransformation reciprocalCellMatrix = cell().inverseMatrix();

	// Note: Cell vectors are in columns. Those are 3-vectors.
	int nX = cellMatrix.column(0).length()/fftGridSpacing();
	int nY = cellMatrix.column(1).length()/fftGridSpacing();
	int nZ = cellMatrix.column(2).length()/fftGridSpacing();

	// Map all quantities onto a spatial grid.
	QVector<FloatType> gridProperty1, gridProperty2;
	mapToSpatialGrid(sourceProperty1().get(),
					 _vecComponent1,
					 reciprocalCellMatrix,
					 nX, nY, nZ,
					 gridProperty1,
					 _applyWindow);

	incrementProgressValue();
	if (isCanceled())
		return;

	mapToSpatialGrid(sourceProperty2().get(),
					 _vecComponent2,
					 reciprocalCellMatrix,
					 nX, nY, nZ,
					 gridProperty2,
					 _applyWindow);

	QVector<FloatType> gridDensity;
	mapToSpatialGrid(nullptr,
					 _vecComponent1,
					 reciprocalCellMatrix,
					 nX, nY, nZ,
					 gridDensity,
					 _applyWindow);
	incrementProgressValue();
	if (isCanceled())
		return;

	// FIXME. Apply windowing function in nonperiodic directions here.

	// Compute reciprocal-space correlation function from a product in Fourier space.

	// Compute Fourier transform of spatial grid.
	QVector<std::complex<FloatType>> ftProperty1;
	r2cFFT(nX, nY, nZ, gridProperty1, ftProperty1);

	incrementProgressValue();
	if (isCanceled())
		return;

	QVector<std::complex<FloatType>> ftProperty2;
	r2cFFT(nX, nY, nZ, gridProperty2, ftProperty2);

	incrementProgressValue();
	if (isCanceled())
		return;

	QVector<std::complex<FloatType>> ftDensity;
	r2cFFT(nX, nY, nZ, gridDensity, ftDensity);

	incrementProgressValue();
	if (isCanceled())
		return;

	// Note: Reciprocal cell vectors are in rows. Those are 4-vectors.
	Vector_4<FloatType> recCell1 = reciprocalCellMatrix.row(0);
	Vector_4<FloatType> recCell2 = reciprocalCellMatrix.row(1);
	Vector_4<FloatType> recCell3 = reciprocalCellMatrix.row(2);

	// Compute distance of cell faces.
	FloatType cellFaceDistance1 = 1/sqrt(recCell1.x()*recCell1.x() + recCell1.y()*recCell1.y() + recCell1.z()*recCell1.z());
	FloatType cellFaceDistance2 = 1/sqrt(recCell2.x()*recCell2.x() + recCell2.y()*recCell2.y() + recCell2.z()*recCell2.z());
	FloatType cellFaceDistance3 = 1/sqrt(recCell3.x()*recCell3.x() + recCell3.y()*recCell3.y() + recCell3.z()*recCell3.z());

	FloatType minCellFaceDistance = std::min({cellFaceDistance1, cellFaceDistance2, cellFaceDistance3});

	// Minimum reciprocal space vector is given by the minimum distance of cell faces.
	FloatType minReciprocalSpaceVector = 1/minCellFaceDistance;
	int numberOfWavevectorBins, dir1, dir2;
	Vector_3<int> n(nX, nY, nZ);

	if (_averagingDirection == RADIAL) {
		numberOfWavevectorBins = 1/(2*minReciprocalSpaceVector*fftGridSpacing());
	}
	else {
		dir1 = (_averagingDirection+1)%3;
		dir2 = (_averagingDirection+2)%3;
		numberOfWavevectorBins = n[dir1]*n[dir2];
	}

	// Averaged reciprocal space correlation function.
	reciprocalSpaceCorrelation().fill(0.0, numberOfWavevectorBins);
	reciprocalSpaceCorrelationX().resize(numberOfWavevectorBins);
	QVector<int> numberOfValues(numberOfWavevectorBins, 0);

	// Populate array with reciprocal space vectors.
	for (int wavevectorBinIndex = 0; wavevectorBinIndex < numberOfWavevectorBins; wavevectorBinIndex++) {
		reciprocalSpaceCorrelationX()[wavevectorBinIndex] = 2*FLOATTYPE_PI*(wavevectorBinIndex+0.5)*minReciprocalSpaceVector;
	}

	// Compute Fourier-transformed correlation function and put it on a radial
	// grid.
	int binIndex = 0;
	for (int binIndexX = 0; binIndexX < nX; binIndexX++) {
		for (int binIndexY = 0; binIndexY < nY; binIndexY++) {
			for (int binIndexZ = 0; binIndexZ < nZ/2+1; binIndexZ++, binIndex++) {
				// Compute correlation function.
				std::complex<FloatType> corr = ftProperty1[binIndex]*std::conj(ftProperty2[binIndex]);

				// Store correlation function to property1 for back transform.
				ftProperty1[binIndex] = corr;

				// Compute structure factor/radial distribution function.
				ftDensity[binIndex] = ftDensity[binIndex]*std::conj(ftDensity[binIndex]);

				int wavevectorBinIndex;
				if (_averagingDirection == RADIAL) {
					// Ignore Gamma-point for radial average.
					if (binIndex == 0 && binIndexY == 0 && binIndexZ == 0)
						continue;

					// Compute wavevector.
					int iX = SimulationCell::modulo(binIndexX+nX/2, nX)-nX/2;
					int iY = SimulationCell::modulo(binIndexY+nY/2, nY)-nY/2;
					int iZ = SimulationCell::modulo(binIndexZ+nZ/2, nZ)-nZ/2;
					// This is the reciprocal space vector (without a factor of 2*pi).
					Vector_4<FloatType> wavevector = FloatType(iX)*reciprocalCellMatrix.row(0) +
							 		   	 			 FloatType(iY)*reciprocalCellMatrix.row(1) +
							 			 			 FloatType(iZ)*reciprocalCellMatrix.row(2);
					wavevector.w() = 0.0;

					// Compute bin index.
					wavevectorBinIndex = int(std::floor(wavevector.length()/minReciprocalSpaceVector));
				}
				else {
					Vector_3<int> binIndexXYZ(binIndexX, binIndexY, binIndexZ);
					wavevectorBinIndex = binIndexXYZ[dir2]+n[dir2]*binIndexXYZ[dir1];
				}

				if (wavevectorBinIndex >= 0 && wavevectorBinIndex < numberOfWavevectorBins) {
					reciprocalSpaceCorrelation()[wavevectorBinIndex] += std::real(corr);
					numberOfValues[wavevectorBinIndex]++;
				}
			}
		}
	}

	// Compute averages and normalize reciprocal-space correlation function.
	FloatType normalizationFactor = cell().volume3D()/(sourceProperty1()->size()*sourceProperty2()->size());
	for(int wavevectorBinIndex = 0; wavevectorBinIndex < numberOfWavevectorBins; wavevectorBinIndex++) {
		if(numberOfValues[wavevectorBinIndex] > 0) {
			reciprocalSpaceCorrelation()[wavevectorBinIndex] *= normalizationFactor/numberOfValues[wavevectorBinIndex];
		}
	}

	incrementProgressValue();
	if(isCanceled())
		return;

	// Compute long-ranged part of the real-space correlation function from the FFT convolution.

	// Computer inverse Fourier transform of correlation function.
	c2rFFT(nX, nY, nZ, ftProperty1, gridProperty1);

	incrementProgressValue();
	if(isCanceled())
		return;

	c2rFFT(nX, nY, nZ, ftDensity, gridDensity);

	incrementProgressValue();
	if(isCanceled())
		return;

	// Determine number of grid points for reciprocal-spacespace correlation function.
	int numberOfDistanceBins = minCellFaceDistance/(2*fftGridSpacing());
	FloatType gridSpacing = minCellFaceDistance/(2*numberOfDistanceBins);

	// Radially averaged real space correlation function.
	realSpaceCorrelation().fill(0.0, numberOfDistanceBins);
	realSpaceRDF().fill(0.0, numberOfDistanceBins);
	realSpaceCorrelationX().resize(numberOfDistanceBins);
	numberOfValues.fill(0, numberOfDistanceBins);

	// Populate array with real space vectors.
	for(int distanceBinIndex = 0; distanceBinIndex < numberOfDistanceBins; distanceBinIndex++) {
		realSpaceCorrelationX()[distanceBinIndex] = (distanceBinIndex+0.5)*gridSpacing;
	}

	// Put real-space correlation function on a radial grid.
	binIndex = 0;
	for (int binIndexX = 0; binIndexX < nX; binIndexX++) {
		for (int binIndexY = 0; binIndexY < nY; binIndexY++) {
			for (int binIndexZ = 0; binIndexZ < nZ; binIndexZ++, binIndex++) {
				// Ignore origin for radial average (which is just the covariance of the quantities).
				if (binIndex == 0 && binIndexY == 0 && binIndexZ == 0)
					continue;

				// Compute distance. (FIXME! Check that this is actually correct for even and odd numbers of grid points.)
				FloatType fracX = FloatType(SimulationCell::modulo(binIndexX+nX/2, nX)-nX/2)/nX;
				FloatType fracY = FloatType(SimulationCell::modulo(binIndexY+nY/2, nY)-nY/2)/nY;
				FloatType fracZ = FloatType(SimulationCell::modulo(binIndexZ+nZ/2, nZ)-nZ/2)/nZ;
				// This is the real space vector.
				Vector_3<FloatType> distance = fracX*cellMatrix.column(0) +
						 		   	 	       fracY*cellMatrix.column(1) +
						 			 		   fracZ*cellMatrix.column(2);

				// Length of real space vector.
				int distanceBinIndex = int(std::floor(distance.length()/gridSpacing));
				if (distanceBinIndex >= 0 && distanceBinIndex < numberOfDistanceBins) {
					realSpaceCorrelation()[distanceBinIndex] += gridProperty1[binIndex];
					realSpaceRDF()[distanceBinIndex] += gridDensity[binIndex];
					numberOfValues[distanceBinIndex]++;
				}
			}
		}
	}

	// Compute averages and normalize real-space correlation function. Note FFTW computes an unnormalized transform.
	normalizationFactor = 1.0/(sourceProperty1()->size()*sourceProperty2()->size());
	for (int distanceBinIndex = 0; distanceBinIndex < numberOfDistanceBins; distanceBinIndex++) {
		if (numberOfValues[distanceBinIndex] > 0) {
			realSpaceCorrelation()[distanceBinIndex] *= normalizationFactor/numberOfValues[distanceBinIndex];
			realSpaceRDF()[distanceBinIndex] *= normalizationFactor/numberOfValues[distanceBinIndex];
		}
	}

	incrementProgressValue();
}

/******************************************************************************
* Compute real space correlation function via direction summation over neighbors.
******************************************************************************/
void CorrelationFunctionModifier::CorrelationAnalysisEngine::computeNeighCorrelation()
{
	// Get number of particles.
	size_t particleCount = positions()->size();

	// Get pointers to data.
	const FloatType *floatData1 = nullptr, *floatData2 = nullptr;
	const int *intData1 = nullptr, *intData2 = nullptr; 
	size_t componentCount1 = sourceProperty1()->componentCount();
	size_t componentCount2 = sourceProperty2()->componentCount();
	if(sourceProperty1()->dataType() == qMetaTypeId<FloatType>()) {
		floatData1 = sourceProperty1()->constDataFloat();
	}
	else if (sourceProperty1()->dataType() == qMetaTypeId<int>()) {
		intData1 = sourceProperty1()->constDataInt();
	}
	if(sourceProperty2()->dataType() == qMetaTypeId<FloatType>()) {
		floatData2 = sourceProperty2()->constDataFloat();
	}
	else if (sourceProperty2()->dataType() == qMetaTypeId<int>()) {
		intData2 = sourceProperty2()->constDataInt();
	}

	// Resize neighbor RDF and fill with zeros.
	neighRDF().fill(0.0, neighCorrelation().size());

	// Prepare the neighbor list.
	CutoffNeighborFinder neighborListBuilder;
	if (!neighborListBuilder.prepare(_neighCutoff, *positions(), cell(), nullptr, this))
		return;

	// Perform analysis on each particle in parallel.
	std::vector<QFuture<void>> workers;
	size_t num_threads = Application::instance()->idealThreadCount();
	size_t chunkSize = particleCount / num_threads;
	size_t startIndex = 0;
	size_t endIndex = chunkSize;
	size_t vecComponent1 = _vecComponent1;
	size_t vecComponent2 = _vecComponent2;
	std::mutex mutex;
	for (size_t t = 0; t < num_threads; t++) {
		if (t == num_threads - 1) {
			endIndex += particleCount % num_threads;
		}
		workers.push_back(QtConcurrent::run([&neighborListBuilder, startIndex, endIndex,
								   	   floatData1, intData1, componentCount1, vecComponent1,
								   	   floatData2, intData2, componentCount2, vecComponent2,
									   &mutex, this]() {
			FloatType gridSpacing = (_neighCutoff + FLOATTYPE_EPSILON) / neighCorrelation().size();
			std::vector<double> threadLocalCorrelation(neighCorrelation().size(), 0);
			std::vector<int> threadLocalRDF(neighCorrelation().size(), 0);
			for (size_t i = startIndex; i < endIndex;) {
					for (CutoffNeighborFinder::Query neighQuery(neighborListBuilder, i); !neighQuery.atEnd(); neighQuery.next()) {
					size_t distanceBinIndex = (size_t)(sqrt(neighQuery.distanceSquared()) / gridSpacing);
					distanceBinIndex = std::min(distanceBinIndex, threadLocalCorrelation.size() - 1);
					FloatType data1 = 0.0, data2 = 0.0;
					if (floatData1)
						data1 = floatData1[i*componentCount1 + vecComponent1];
					else if (intData1)
						data1 = intData1[i*componentCount1 + vecComponent1];
					if (floatData2)
						data2 = floatData2[neighQuery.current() * componentCount2 + vecComponent2];
					else if (intData2)
						data2 = intData2[neighQuery.current() * componentCount2 + vecComponent2];
					threadLocalCorrelation[distanceBinIndex] += data1*data2;
					threadLocalRDF[distanceBinIndex] += 1;
				}
					i++;
					// Abort loop when operation was canceled by the user.
				if (isCanceled())
					return;
			}
			std::lock_guard<std::mutex> lock(mutex);
			auto iter_corr_out = neighCorrelation().begin();
			for (auto iter_corr = threadLocalCorrelation.cbegin(); iter_corr != threadLocalCorrelation.cend(); ++iter_corr, ++iter_corr_out)
				*iter_corr_out += *iter_corr;
			auto iter_rdf_out = neighRDF().begin();
			for (auto iter_rdf = threadLocalRDF.cbegin(); iter_rdf != threadLocalRDF.cend(); ++iter_rdf, ++iter_rdf_out)
				*iter_rdf_out += *iter_rdf;
		}));
		startIndex = endIndex;
		endIndex += chunkSize;
	}

	for (auto& t: workers)
		t.waitForFinished();
	incrementProgressValue();

	// Normalize short-ranged real-space correlation function and populate x-array.
	FloatType gridSpacing = (_neighCutoff + FLOATTYPE_EPSILON) / neighCorrelation().size();
	FloatType normalizationFactor = 3.0*cell().volume3D()/(4.0*FLOATTYPE_PI*sourceProperty1()->size()*sourceProperty2()->size());
	for (int distanceBinIndex = 0; distanceBinIndex < neighCorrelation().size(); distanceBinIndex++) {
		FloatType distance = distanceBinIndex*gridSpacing;
		FloatType distance2 = (distanceBinIndex+1)*gridSpacing;
		neighCorrelationX()[distanceBinIndex] = (distance+distance2)/2;
		neighCorrelation()[distanceBinIndex] *= normalizationFactor/(distance2*distance2*distance2-distance*distance*distance);
		neighRDF()[distanceBinIndex] *= normalizationFactor/(distance2*distance2*distance2-distance*distance*distance);
	}

	incrementProgressValue();
}

/******************************************************************************
* Compute means and covariance.
******************************************************************************/
void CorrelationFunctionModifier::CorrelationAnalysisEngine::computeLimits()
{
	// Get pointers to data.
	const FloatType *floatData1 = nullptr, *floatData2 = nullptr;
	const int *intData1 = nullptr, *intData2 = nullptr; 
	size_t componentCount1 = sourceProperty1()->componentCount();
	size_t componentCount2 = sourceProperty2()->componentCount();
	if(sourceProperty1()->dataType() == qMetaTypeId<FloatType>()) {
		floatData1 = sourceProperty1()->constDataFloat();
	}
	else if (sourceProperty1()->dataType() == qMetaTypeId<int>()) {
		intData1 = sourceProperty1()->constDataInt();
	}
	if(sourceProperty2()->dataType() == qMetaTypeId<FloatType>()) {
		floatData2 = sourceProperty2()->constDataFloat();
	}
	else if (sourceProperty2()->dataType() == qMetaTypeId<int>()) {
		intData2 = sourceProperty2()->constDataInt();
	}

	// Compute mean and covariance values.
	if (sourceProperty1()->size() != sourceProperty2()->size())
		return;

	FloatType mean1 = 0;
	FloatType mean2 = 0;
	FloatType covariance = 0;
	for (int particleIndex = 0; particleIndex < sourceProperty1()->size(); particleIndex++) {
		FloatType data1, data2;
		if (floatData1)
			data1 = floatData1[particleIndex*componentCount1 + _vecComponent1];
		else if (intData1)
			data1 = intData1[particleIndex*componentCount1 + _vecComponent1];
		if (floatData2)
			data2 = floatData2[particleIndex*componentCount2 + _vecComponent2];
		else if (intData2)
			data2 = intData2[particleIndex*componentCount2 + _vecComponent2];
		mean1 += data1;
		mean2 += data2;
		covariance += data1*data2;
	}
	mean1 /= sourceProperty1()->size();
	mean2 /= sourceProperty2()->size();
	covariance /= sourceProperty1()->size();
	_results->setLimits(mean1, mean2, covariance);

	incrementProgressValue();
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void CorrelationFunctionModifier::CorrelationAnalysisEngine::perform()
{
	setProgressText(tr("Computing correlation function"));
	setProgressValue(0);
	int range = 12;
	if(!neighCorrelation().empty())
		range += 2;
	setProgressMaximum(range);

	// Compute reciprocal space correlation function and long-ranged part of
	// the real-space correlation function from an FFT.
	computeFftCorrelation();
	if(isCanceled())
		return;

	// Compute short-ranged part of the real-space correlation function from a direct loop over particle neighbors.
	if(!neighCorrelation().empty())
		computeNeighCorrelation();

	if(isCanceled())
		return;

	computeLimits();

	// Return the results of the compute engine.
	setResult(std::move(_results));
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
PipelineFlowState CorrelationFunctionModifier::CorrelationAnalysisResults::apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Store the computed data in the ModifierApplication.
	static_object_cast<CorrelationFunctionModifierApplication>(modApp)->setResults(
		realSpaceCorrelation(), realSpaceRDF(), realSpaceCorrelationX(),
		neighCorrelation(), neighRDF(), neighCorrelationX(), 
		reciprocalSpaceCorrelation(), reciprocalSpaceCorrelationX(), mean1(), mean2(), covariance());
		
	return input;
}

/******************************************************************************
* Update plot ranges.
******************************************************************************/
void CorrelationFunctionModifier::updateRanges(FloatType offset, FloatType fac, FloatType reciprocalFac, ModifierApplication* modApp)
{
	CorrelationFunctionModifierApplication* myModApp = dynamic_object_cast<CorrelationFunctionModifierApplication>(modApp);
	if(!myModApp) return;

	// Compute data ranges
	if (!fixRealSpaceXAxisRange()) {
		if (!myModApp->realSpaceCorrelationX().empty() && !myModApp->neighCorrelationX().empty() && doComputeNeighCorrelation()) {
			setRealSpaceXAxisRangeStart(std::min(myModApp->realSpaceCorrelationX().first(), myModApp->neighCorrelationX().first()));
			setRealSpaceXAxisRangeEnd(std::max(myModApp->realSpaceCorrelationX().last(), myModApp->neighCorrelationX().last()));
		}
		else if (!myModApp->realSpaceCorrelationX().empty()) {
			setRealSpaceXAxisRangeStart(myModApp->realSpaceCorrelationX().first());
			setRealSpaceXAxisRangeEnd(myModApp->realSpaceCorrelationX().last());
		}
		else if (!myModApp->neighCorrelationX().empty() && doComputeNeighCorrelation()) {
			setRealSpaceXAxisRangeStart(myModApp->neighCorrelationX().first());
			setRealSpaceXAxisRangeEnd(myModApp->neighCorrelationX().last());
		}
	}
	if (!fixRealSpaceYAxisRange()) {
		if (!myModApp->realSpaceCorrelation().empty() && !myModApp->neighCorrelation().empty() && doComputeNeighCorrelation()) {
			auto realSpace = std::minmax_element(myModApp->realSpaceCorrelation().begin(), myModApp->realSpaceCorrelation().end());
			auto neigh = std::minmax_element(myModApp->neighCorrelation().begin(), myModApp->neighCorrelation().end());
			setRealSpaceYAxisRangeStart(fac*(std::min(*realSpace.first, *neigh.first)-offset));
			setRealSpaceYAxisRangeEnd(fac*(std::max(*realSpace.second, *neigh.second)-offset));
		}
		else if (!myModApp->realSpaceCorrelation().empty()) {
			auto realSpace = std::minmax_element(myModApp->realSpaceCorrelation().begin(), myModApp->realSpaceCorrelation().end());
			setRealSpaceYAxisRangeStart(fac*(*realSpace.first-offset));
			setRealSpaceYAxisRangeEnd(fac*(*realSpace.second-offset));
		}
		else if (!myModApp->neighCorrelation().empty() && doComputeNeighCorrelation()) {
			auto neigh = std::minmax_element(myModApp->neighCorrelation().begin(), myModApp->neighCorrelation().end());
			setRealSpaceYAxisRangeStart(fac*(*neigh.first-offset));
			setRealSpaceYAxisRangeEnd(fac*(*neigh.second-offset));
		}	
	}
	if (!fixReciprocalSpaceXAxisRange() && !myModApp->reciprocalSpaceCorrelationX().empty()) {
		setReciprocalSpaceXAxisRangeStart(myModApp->reciprocalSpaceCorrelationX().first());
		setReciprocalSpaceXAxisRangeEnd(myModApp->reciprocalSpaceCorrelationX().last());
	}
	if (!fixReciprocalSpaceYAxisRange() && !myModApp->reciprocalSpaceCorrelation().empty()) {
		auto reciprocalSpace = std::minmax_element(myModApp->reciprocalSpaceCorrelation().begin(), myModApp->reciprocalSpaceCorrelation().end());
		setReciprocalSpaceYAxisRangeStart(reciprocalFac*(*reciprocalSpace.first));
		setReciprocalSpaceYAxisRangeEnd(reciprocalFac*(*reciprocalSpace.second));
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
