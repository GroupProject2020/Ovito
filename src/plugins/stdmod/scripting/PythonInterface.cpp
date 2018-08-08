///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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

#include <plugins/stdmod/StdMod.h>
#include <plugins/pyscript/binding/PythonBinding.h>
#include <plugins/pyscript/engine/ScriptEngine.h>
#include <plugins/stdobj/scripting/PythonBinding.h>
#include <plugins/stdmod/viewport/ColorLegendOverlay.h>
#include <plugins/stdmod/modifiers/SliceModifier.h>
#include <plugins/stdmod/modifiers/AffineTransformationModifier.h>
#include <plugins/stdmod/modifiers/ClearSelectionModifier.h>
#include <plugins/stdmod/modifiers/InvertSelectionModifier.h>
#include <plugins/stdmod/modifiers/ColorCodingModifier.h>
#include <plugins/stdmod/modifiers/AssignColorModifier.h>
#include <plugins/stdmod/modifiers/DeleteSelectedModifier.h>
#include <plugins/stdmod/modifiers/SelectTypeModifier.h>
#include <plugins/stdmod/modifiers/HistogramModifier.h>
#include <plugins/stdmod/modifiers/ScatterPlotModifier.h>
#include <plugins/stdmod/modifiers/ReplicateModifier.h>
#include <plugins/stdmod/modifiers/ExpressionSelectionModifier.h>
#include <plugins/stdmod/modifiers/FreezePropertyModifier.h>
#include <plugins/stdmod/modifiers/ManualSelectionModifier.h>
#include <plugins/stdmod/modifiers/ComputePropertyModifier.h>
#include <plugins/stdmod/modifiers/CombineDatasetsModifier.h>
#include <core/app/PluginManager.h>

namespace Ovito { namespace StdMod {

using namespace PyScript;

PYBIND11_MODULE(StdMod, m)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();
	
	py::options options;
	options.disable_function_signatures();

	auto SliceModifier_py = ovito_class<SliceModifier, MultiDelegatingModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"Deletes or selects data elements in a semi-infinite region bounded by a plane or in a slab bounded by a pair of parallel planes. "
			"See also the corresponding `user manual page <../../particles.modifiers.slice.html>`__ for this modifier. "
			"The modifier can operate on several classes of data elements: "
			"\n\n"
			"  * Particles (including bonds)\n"
			"  * Surfaces (:py:class:`~ovito.data.SurfaceMesh`) \n"
			"  * Dislocation lines (:py:class:`~ovito.data.DislocationNetwork`)\n"
			"\n\n"
			"The modifier will act on all element classes simultaneously by default. Restricting the slice operation to a subset of classes is possible by setting the :py:attr:`.operate_on` field. ")
		.def_property("distance", &SliceModifier::distance, &SliceModifier::setDistance,
				"The distance of the slicing plane from the origin (along its normal vector)."
				"\n\n"
				":Default: 0.0\n")
		.def_property("normal", &SliceModifier::normal, &SliceModifier::setNormal,
				"The normal vector of the slicing plane. Does not have to be a unit vector."
				"\n\n"
				":Default: ``(1,0,0)``\n")
		.def_property("slab_width", &SliceModifier::slabWidth, &SliceModifier::setSlabWidth,
				"The thicknes of the slab to cut. If zero, the modifier cuts away everything on one "
				"side of the cutting plane."
				"\n\n"
				":Default: 0.0\n")
		// For backward compatibility with OVITO 2.9.0:
		.def_property("slice_width", &SliceModifier::slabWidth, &SliceModifier::setSlabWidth)
		.def_property("inverse", &SliceModifier::inverse, &SliceModifier::setInverse,
				"Reverses the sense of the slicing plane."
				"\n\n"
				":Default: ``False``\n")
		.def_property("select", &SliceModifier::createSelection, &SliceModifier::setCreateSelection,
				"If ``True``, the modifier selects data elements instead of deleting them."
				"\n\n"
				":Default: ``False``\n")
		.def_property("only_selected", &SliceModifier::applyToSelection, &SliceModifier::setApplyToSelection,
				"Controls whether the modifier should act only on currently selected data elements (e.g. selected particles)."
				"\n\n"
				":Default: ``False``\n")
	;
	modifier_operate_on_list(SliceModifier_py, std::mem_fn(&SliceModifier::delegates), "operate_on", 
			"A set of strings specifying the kinds of data elements this modifier should operate on. "
			"By default the set contains all data element types supported by the modifier. "
			"\n\n"
			":Default: ``{'particles', 'surfaces', 'dislocations'}``\n");


	auto AffineTransformationModifier_py = ovito_class<AffineTransformationModifier, MultiDelegatingModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"This modifier applies an affine transformation to data elements in order to move, rotate, shear or scale them. "
			"See also the corresponding `user manual page <../../particles.modifiers.affine_transformation.html>`__ for this modifier. "
			"The transformation modifier can operate on several types of elements: "
			"\n\n"
			"  * Particle positions\n"
			"  * Particle vector properties (``'Velocity'``, ``'Force'``, ``'Displacement'``)\n"
			"  * Simulation cells (:py:class:`~ovito.data.SimulationCell`) \n"
			"  * Surfaces (:py:class:`~ovito.data.SurfaceMesh`) \n"
			"\n\n"
			"The modifier will act on all of them simultaneously by default. Restricting the modifier to a subset is possible by setting the :py:attr:`.operate_on` field. "
			"Example::"
			"\n\n"
			"    xy_shear = 0.05\n"
			"    mod = AffineTransformationModifier(\n"
			"              operate_on = {'particles'},  # Transform particles but not simulation box\n"
			"              transformation = [[1,xy_shear,0,0],\n"
			"                                [0,       1,0,0],\n"
			"                                [0,       0,1,0]])\n"
			"\n\n")
		.def_property("transformation", MatrixGetter<AffineTransformationModifier, AffineTransformation, &AffineTransformationModifier::transformationTM>(), 
										MatrixSetter<AffineTransformationModifier, AffineTransformation, &AffineTransformationModifier::setTransformationTM>(),
				"The 3x4 transformation matrix being applied to input elements. "
				"The first three matrix columns define the linear part of the transformation, while the fourth "
				"column specifies the translation vector. "
				"\n\n"
				"This matrix describes a relative transformation and is used only if :py:attr:`.relative_mode` == ``True``."
				"\n\n"
				":Default: ``[[ 1.  0.  0.  0.] [ 0.  1.  0.  0.] [ 0.  0.  1.  0.]]``\n")
		.def_property("target_cell", MatrixGetter<AffineTransformationModifier, AffineTransformation, &AffineTransformationModifier::targetCell>(), 
									 MatrixSetter<AffineTransformationModifier, AffineTransformation, &AffineTransformationModifier::setTargetCell>(),
				"This 3x4 matrix specifies the target cell shape. It is used when :py:attr:`.relative_mode` == ``False``. "
				"\n\n"
				"The first three columns of the matrix specify the three edge vectors of the target cell. "
				"The fourth column defines the origin vector of the target cell.")
		.def_property("relative_mode", &AffineTransformationModifier::relativeMode, &AffineTransformationModifier::setRelativeMode,
				"Selects the operation mode of the modifier."
				"\n\n"
				"If ``relative_mode==True``, the modifier transforms elements "
				"by applying the matrix given by the :py:attr:`.transformation` parameter."
				"\n\n"
				"If ``relative_mode==False``, the modifier transforms elements "
				"such that the old simulation cell will have the shape given by the the :py:attr:`.target_cell` parameter after the transformation."
				"\n\n"
				":Default: ``True``\n")
		.def_property("only_selected", &AffineTransformationModifier::selectionOnly, &AffineTransformationModifier::setSelectionOnly,
				"Controls whether the modifier should affect only on currently selected elements (e.g. selected particles)."
				"\n\n"
				":Default: ``False``\n")
	;
	modifier_operate_on_list(AffineTransformationModifier_py, std::mem_fn(&AffineTransformationModifier::delegates), "operate_on", 
			"A set of strings specifying the kinds of data elements this modifier should operate on. "
			"By default the set contains all data element types supported by the modifier. "
			"\n\n"
			":Default: ``{'particles', 'vector_properties', 'cell', 'surfaces'}``\n");


	auto ReplicateModifier_py = ovito_class<ReplicateModifier, MultiDelegatingModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"This modifier replicates all particles and bonds to generate periodic images. "
			"See also the corresponding `user manual page <../../particles.modifiers.show_periodic_images.html>`__ for this modifier. "
			"The modifier can operate on several classes of data elements: "
			"\n\n"
			"  * Particles (including bonds)\n"
			"  * Surfaces (:py:class:`~ovito.data.SurfaceMesh`) \n"
			"  * Dislocation lines (:py:class:`~ovito.data.DislocationNetwork`)\n"
			"  * Voxel data grids\n"
			"\n\n"
			"The modifier will act on all element classes simultaneously by default. Restricting the replicate operation to a subset of classes is possible by setting the :py:attr:`.operate_on` field. ")
		.def_property("num_x", &ReplicateModifier::numImagesX, &ReplicateModifier::setNumImagesX,
				"A positive integer specifying the number of copies to generate in the *x* direction (including the existing primary image)."
				"\n\n"
				":Default: 1\n")
		.def_property("num_y", &ReplicateModifier::numImagesY, &ReplicateModifier::setNumImagesY,
				"A positive integer specifying the number of copies to generate in the *y* direction (including the existing primary image)."
				"\n\n"
				":Default: 1\n")
		.def_property("num_z", &ReplicateModifier::numImagesZ, &ReplicateModifier::setNumImagesZ,
				"A positive integer specifying the number of copies to generate in the *z* direction (including the existing primary image)."
				"\n\n"
				":Default: 1\n")
		.def_property("adjust_box", &ReplicateModifier::adjustBoxSize, &ReplicateModifier::setAdjustBoxSize,
				"Controls whether the simulation cell is resized. "
				"If ``True``, the simulation cell is accordingly extended to fit the replicated data. "
				"If ``False``, the original simulation cell size (containing only the primary image of the system) is maintained. "
				"\n\n"
				":Default: ``True``\n")
		.def_property("unique_ids", &ReplicateModifier::uniqueIdentifiers, &ReplicateModifier::setUniqueIdentifiers,
				"If ``True``, the modifier automatically generates new unique IDs for each copy of particles. "
				"Otherwise, the replica will keep the same IDs as the original particles, which is typically not what you want. "
				"\n\n"
				"Note: This option has no effect if the input particles do not already have numeric IDs (i.e. the ``'Particle Identifier'`` property does not exist). "
				"\n\n"
				":Default: ``True``\n")
	;
	modifier_operate_on_list(ReplicateModifier_py, std::mem_fn(&ReplicateModifier::delegates), "operate_on", 
			"A set of strings specifying the kinds of data elements this modifier should operate on. "
			"By default the set contains all data element types supported by the modifier. "
			"\n\n"
			":Default: ``{'particles', 'voxels', 'surfaces', 'dislocations'}``\n");

	ovito_class<ClearSelectionModifier, GenericPropertyModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"This modifier clears the current selection. It can operate on several kinds of data elements: "
			"\n\n"
			"  * Particles (removing the ``'Selection'`` :ref:`particle property <particle-types-list>`)\n"
			"  * Bonds (removing the ``'Selection'`` :ref:`bond property <bond-types-list>`)\n"
			"\n\n"
			"The modifier will act on particles only by default. This can be changed by setting the :py:attr:`.operate_on` field. "
			"See also the corresponding `user manual page <../../particles.modifiers.clear_selection.html>`__ for this modifier. "
			)
		.def_property("operate_on", modifierPropertyClassGetter(), modifierPropertyClassSetter(),
				"Selects the kind of data elements this modifier should operate on. "
				"Supported values are: ``'particles'``, ``'bonds'``, ``'voxels'``. "
				"\n\n"
				":Default: ``'particles'``\n")		
	;

	ovito_class<InvertSelectionModifier, GenericPropertyModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"This modifier inverts the current selection. It can operate on several kinds of data elements: "
			"\n\n"
			"  * Particles (inverting the ``'Selection'`` :ref:`particle property <particle-types-list>`)\n"
			"  * Bonds (inverting the ``'Selection'`` :ref:`bond property <bond-types-list>`)\n"
			"\n\n"
			"The modifier will act on particles only by default. This can be changed by setting the :py:attr:`.operate_on` field. "
			"See also the corresponding `user manual page <../../particles.modifiers.invert_selection.html>`__ for this modifier. "
			)
		.def_property("operate_on", modifierPropertyClassGetter(), modifierPropertyClassSetter(),
				"Selects the kind of data elements this modifier should operate on. "
				"Supported values are: ``'particles'``, ``'bonds'``, ``'voxels'``. "
				"\n\n"
				":Default: ``'particles'``\n")		
	;
			
	auto ColorCodingModifier_py = ovito_class<ColorCodingModifier, DelegatingModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"Assigns colors to elements based on some scalar input property to visualize the property values. "
			"See also the corresponding `user manual page <../../particles.modifiers.color_coding.html>`__ for this modifier. "
			"The modifier can operate on several kinds of data elements: "
			"\n\n"
			"  * Particles (setting the ``'Color'`` :ref:`particle property <particle-types-list>`)\n"
			"  * Particle vectors (setting the ``'Vector Color'`` :ref:`particle property <particle-types-list>`)\n"
			"  * Bonds (setting the ``'Color'`` :ref:`bond property <bond-types-list>`)\n"
			"\n\n"
			"The modifier will act on particles by default. You can change this by setting the modifier's :py:attr:`.operate_on` field. "
			"\n\n"
			"Usage example:"
			"\n\n"
			".. literalinclude:: ../example_snippets/color_coding.py\n"
			"   :lines: 6-\n"
			"\n"
			"If the :py:attr:`.start_value` and :py:attr:`.end_value` parameters are not explicitly specified during modifier construction, "
			"then the modifier will automatically adjust them to the minimum and maximum values of the input property at the time it "
			"is inserted into a data pipeline. "
			"\n\n"
			"The :py:class:`~ovito.vis.ColorLegendOverlay` may be used in conjunction with a :py:class:`ColorCodingModifier` "
			"to insert a color legend into rendered images. "
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``Color`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The computed particle colors if :py:attr:`.operate_on` is set to ``'particles'``.\n"
			" * ``Vector Color`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The computed arrow colors if :py:attr:`.operate_on` is set to ``'vectors'``.\n"
			" * ``Color`` (:py:class:`~ovito.data.BondProperty`):\n"
			"   The compute bond colors if :py:attr:`.operate_on` is set to ``'bonds'``.\n"
			"\n")
		
		.def_property("property", &ColorCodingModifier::sourceProperty, [](ColorCodingModifier& mod, py::object val) {
					auto* delegate = static_object_cast<ColorCodingModifierDelegate>(mod.delegate());
					mod.setSourceProperty(convertPythonPropertyReference(val, delegate ? &delegate->propertyClass() : nullptr));
				},
				"The name of the input property that should be used to color elements. "
				"\n\n"
				"If :py:attr:`.operate_on` is set to ``'particles'`` or ``'vectors'``, this can be one of the :ref:`standard particle properties <particle-types-list>` "
				"or a name of a user-defined :py:class:`~ovito.data.ParticleProperty`. "
				"If :py:attr:`.operate_on` is set to ``'bonds'``, this can be one of the :ref:`standard bond properties <bond-types-list>` "
				"or a name of a user-defined :py:class:`~ovito.data.BondProperty`. "
				"\n\n"
				"When the input property has multiple components, then a component name must be appended to the property base name, e.g. ``\"Velocity.X\"``. "
				"\n\n"
				"Note: Make sure that :py:attr:`.operate_on` is set to the desired value *before* setting this attribute, "
				"because changing :py:attr:`.operate_on` will implicitly reset the :py:attr:`!property` attribute. ")
		.def_property("start_value", &ColorCodingModifier::startValue, &ColorCodingModifier::setStartValue,
				"This parameter defines, together with the :py:attr:`.end_value` parameter, the normalization range for mapping the input property values to colors.")
		.def_property("end_value", &ColorCodingModifier::endValue, &ColorCodingModifier::setEndValue,
				"This parameter defines, together with the :py:attr:`.start_value` parameter, the normalization range for mapping the input property values to colors.")
		.def_property("gradient", &ColorCodingModifier::colorGradient, &ColorCodingModifier::setColorGradient,
				"The color gradient object, which is responsible for mapping normalized property values to colors. "
				"Available gradient types are:\n"
				" * ``ColorCodingModifier.BlueWhiteRed()``\n"
				" * ``ColorCodingModifier.Grayscale()``\n"
				" * ``ColorCodingModifier.Hot()``\n"
				" * ``ColorCodingModifier.Jet()``\n"
				" * ``ColorCodingModifier.Magma()``\n"
				" * ``ColorCodingModifier.Rainbow()`` [default]\n"
				" * ``ColorCodingModifier.Viridis()``\n"
				" * ``ColorCodingModifier.Custom(\"<image file>\")``\n"
				"\n"
				"The last color map constructor expects the path to an image file on disk, "
				"which will be used to create a custom color gradient from a row of pixels in the image.")
		.def_property("only_selected", &ColorCodingModifier::colorOnlySelected, &ColorCodingModifier::setColorOnlySelected,
				"If ``True``, only selected elements will be affected by the modifier and the existing colors "
				"of unselected elements will be preserved; if ``False``, all elements will be colored."
				"\n\n"
				":Default: ``False``\n")
		.def_property("operate_on", modifierDelegateGetter<ColorCodingModifier>(), modifierDelegateSetter<ColorCodingModifier>(),
				"Selects the kind of data elements this modifier should operate on. "
				"Supported values are: ``'particles'``, ``'bonds'``, ``'vectors'``. "
				"\n\n"
				"Note: Assigning a new value to this attribute resets the :py:attr:`.property` field. "
				"\n\n"
				":Default: ``'particles'``\n")
	;

	ovito_abstract_class<ColorCodingGradient, RefTarget>(ColorCodingModifier_py)
		.def("valueToColor", &ColorCodingGradient::valueToColor)
	;

	ovito_class<ColorCodingHSVGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "Rainbow")
	;
	ovito_class<ColorCodingGrayscaleGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "Grayscale")
	;
	ovito_class<ColorCodingHotGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "Hot")
	;
	ovito_class<ColorCodingJetGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "Jet")
	;
	ovito_class<ColorCodingBlueWhiteRedGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "BlueWhiteRed")
	;
	ovito_class<ColorCodingViridisGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "Viridis")
	;
	ovito_class<ColorCodingMagmaGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "Magma")
	;
	ovito_class<ColorCodingImageGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "Image")
		.def("load_image", &ColorCodingImageGradient::loadImage)
	;
	ColorCodingModifier_py.def_static("Custom", [](const QString& filename) {
	    OORef<ColorCodingImageGradient> gradient(new ColorCodingImageGradient(ScriptEngine::activeDataset()));
    	gradient->loadImage(filename);
    	return gradient;
	});

	ovito_class<SelectTypeModifier, GenericPropertyModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"Selects all elements of a certain type (e.g. atoms of a chemical type). "
			"See also the corresponding `user manual page <../../particles.modifiers.select_particle_type.html>`__ for this modifier. "
			"The modifier can operate on different kinds of data elements: "
			"\n\n"
			"  * Particles\n"
			"  * Bonds\n"
			"\n\n"
			"The modifier will act on particles by default. You can change this by setting the modifier's :py:attr:`.operate_on` field. "
			"\n\n"
			"Usage example:"
			"\n\n"
			".. literalinclude:: ../example_snippets/select_type_modifier.py\n"
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``Selection`` (:py:class:`~ovito.data.ParticleProperty` or :py:class:`~ovito.data.BondProperty`):\n"
			"   The output property will be set to 1 for particles/bonds whose type is contained in :py:attr:`.types` and 0 for others.\n"
			" * ``SelectType.num_selected`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of elements that were selected by the modifier.\n"
			"\n")
		.def_property("property", &SelectTypeModifier::sourceProperty, [](SelectTypeModifier& mod, py::object val) {
					mod.setSourceProperty(convertPythonPropertyReference(val, mod.propertyClass()));
				},
				"The name of the property to use as input; must be an integer property. "
				"\n\n"
				"When selecting particles, possible input properties are ``\'Particle Type\'`` and ``\'Structure Type\'``, for example. "
				"When selecting bonds, ``'Bond Type'`` is a typical input property for this modifier. "
				"\n\n"
				"Note: Make sure that :py:attr:`.operate_on` is set to the desired value *before* setting this attribute, "
				"because changing :py:attr:`.operate_on` will implicitly reset the :py:attr:`!property` attribute. "
				"\n\n"
				":Default: ``''``\n")
		.def_property("operate_on", modifierPropertyClassGetter(), modifierPropertyClassSetter(),
				"Selects the kind of data elements this modifier should select. "
				"Supported values are: ``'particles'``, ``'bonds'``. "
				"\n\n"
				"Note: Assigning a new value to this attribute resets the :py:attr:`.property` field. "
				"\n\n"
				":Default: ``'particles'``\n")
		// Required by implementation of SelectTypeModifier.types attribute:
		.def_property("_selected_type_ids", &SelectTypeModifier::selectedTypeIDs, &SelectTypeModifier::setSelectedTypeIDs)
		.def_property("_selected_type_names", &SelectTypeModifier::selectedTypeNames, &SelectTypeModifier::setSelectedTypeNames)
	;

	auto HistogramModifier_py = ovito_class<HistogramModifier, GenericPropertyModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"Generates a histogram from the values of a property. "
			"See also the corresponding `user manual page <../../particles.modifiers.histogram.html>`__ for this modifier. "
			"The modifier can operate on properties of different kinds of elements: "
			"\n\n"
			"  * Particles (:py:class:`~ovito.data.ParticleProperty`)\n"
			"  * Bonds (:py:class:`~ovito.data.BondProperty`)\n"
			"  * Voxel grids (:py:class:`~ovito.data.VoxelProperty`)\n"
			"\n\n"
			"The modifier will act on particles by default. You can change this by setting the modifier's :py:attr:`.operate_on` field. "
			"\n\n"
			"The value range of the histogram is determined automatically from the minimum and maximum values of the selected property "
			"unless :py:attr:`.fix_xrange` is set to ``True``. In this case the range of the histogram is controlled by the "
			":py:attr:`.xrange_start` and :py:attr:`.xrange_end` parameters."
			"\n\n"
			"Usage example:"
			"\n\n"
			".. literalinclude:: ../example_snippets/histogram_modifier.py\n"
			"\n\n")
		.def_property("operate_on", modifierPropertyClassGetter(), modifierPropertyClassSetter(),
				"Selects the kind of data elements this modifier should operate on. "
				"Supported values are: ``'particles'``, ``'bonds'``, ``'voxels'``. "
				"\n\n"
				"Note: Assigning a new value to this attribute resets the :py:attr:`.property` field. "
				"\n\n"
				":Default: ``'particles'``\n")		
		.def_property("property", &HistogramModifier::sourceProperty, [](HistogramModifier& mod, py::object val) {					
					mod.setSourceProperty(convertPythonPropertyReference(val, mod.propertyClass()));
				},
				"The name of the input property for which to compute the histogram. "
				"For vector properties a component name must be appended in the string, e.g. ``\"Velocity.X\"``. "
				"\n\n"
				"Note: Make sure that :py:attr:`.operate_on` is set to the desired value *before* setting this attribute, "
				"because changing :py:attr:`.operate_on` will implicitly reset the :py:attr:`!property` attribute. "
				"\n\n"
				":Default: ``''``\n")
		.def_property("bin_count", &HistogramModifier::numberOfBins, &HistogramModifier::setNumberOfBins,
				"The number of histogram bins."
				"\n\n"
				":Default: 200\n")
		.def_property("fix_xrange", &HistogramModifier::fixXAxisRange, &HistogramModifier::setFixXAxisRange,
				"Controls how the value range of the histogram is determined. If false, the range is chosen automatically by the modifier to include "
				"all input values. If true, the range is specified manually using the :py:attr:`.xrange_start` and :py:attr:`.xrange_end` attributes."
				"\n\n"
				":Default: ``False``\n")
		.def_property("xrange_start", &HistogramModifier::xAxisRangeStart, &HistogramModifier::setXAxisRangeStart,
				"If :py:attr:`.fix_xrange` is true, then this specifies the lower end of the value range covered by the histogram."
				"\n\n"
				":Default: 0.0\n")
		.def_property("xrange_end", &HistogramModifier::xAxisRangeEnd, &HistogramModifier::setXAxisRangeEnd,
				"If :py:attr:`.fix_xrange` is true, then this specifies the upper end of the value range covered by the histogram."
				"\n\n"
				":Default: 0.0\n")
		.def_property("only_selected", &HistogramModifier::onlySelected, &HistogramModifier::setOnlySelected,
				"If ``True``, the histogram is computed only on the basis of currently selected particles or bonds. "
				"You can use this to restrict histogram calculation to a subset of particles/bonds. "
				"\n\n"
				":Default: ``False``\n")
		.def_property_readonly("_histogram_data", py::cpp_function([](HistogramModifier& mod) {
				HistogramModifierApplication* modApp = dynamic_object_cast<HistogramModifierApplication>(mod.someModifierApplication());
				if(!modApp || !modApp->binCounts()) mod.throwException(HistogramModifier::tr("Modifier has not been evaluated yet. Histogram data is not yet available."));
				py::array_t<qlonglong> array(modApp->binCounts()->size(), modApp->binCounts()->constDataInt64(), py::cast(modApp));
				// Mark array as read-only.
				reinterpret_cast<py::detail::PyArray_Proxy*>(array.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
				return array;
			}))
		.def_property_readonly("_interval_start", py::cpp_function([](HistogramModifier& mod) {
				HistogramModifierApplication* modApp = dynamic_object_cast<HistogramModifierApplication>(mod.someModifierApplication());
				if(!modApp || !modApp->binCounts()) mod.throwException(HistogramModifier::tr("Modifier has not been evaluated yet. Histogram data is not yet available."));
				return modApp->histogramInterval().first;
			}))
		.def_property_readonly("_interval_end", py::cpp_function([](HistogramModifier& mod) {
				HistogramModifierApplication* modApp = dynamic_object_cast<HistogramModifierApplication>(mod.someModifierApplication());
				if(!modApp || !modApp->binCounts()) mod.throwException(HistogramModifier::tr("Modifier has not been evaluated yet. Histogram data is not yet available."));
				return modApp->histogramInterval().second;
			}))
	;
	ovito_class<HistogramModifierApplication, ModifierApplication>{m};
	
	ovito_class<ScatterPlotModifier, GenericPropertyModifier>{m};
	ovito_class<ScatterPlotModifierApplication, ModifierApplication>{m};
	
	ovito_class<AssignColorModifier, DelegatingModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"Assigns a uniform color to all selected elements. "
			"See also the corresponding `user manual page <../../particles.modifiers.assign_color.html>`__ for this modifier. "
			"The modifier can operate on several kinds of data elements: "
			"\n\n"
			"  * Particles (setting the ``'Color'`` :ref:`particle property <particle-types-list>`)\n"
			"  * Particle vectors (setting the ``'Vector Color'`` :ref:`particle property <particle-types-list>`)\n"
			"  * Bonds (setting the ``'Color'`` :ref:`bond property <bond-types-list>`)\n"
			"\n\n"
			"The modifier will act on particles by default. You can change this by setting the modifier's :py:attr:`.operate_on` field. "
			"\n\n"
			"The modifier uses the ``'Selection'`` property as input to decide which elements "
			"are being assigned the color. If the  ``'Selection'`` property does not exist in the modifier's input, "
			"the color will be assigned to all elements. ")
		.def_property("color", VectorGetter<AssignColorModifier, Color, &AssignColorModifier::color>(), 
							   VectorSetter<AssignColorModifier, Color, &AssignColorModifier::setColor>(),
				"The uniform RGB color that will be assigned to elements by the modifier."
				"\n\n"
				":Default: ``(0.3, 0.3, 1.0)``\n")
		.def_property("keep_selection", &AssignColorModifier::keepSelection, &AssignColorModifier::setKeepSelection)
		.def_property("operate_on", modifierDelegateGetter<AssignColorModifier>(), modifierDelegateSetter<AssignColorModifier>(),
				"Selects the kind of data elements this modifier should operate on. "
				"Supported values are: ``'particles'``, ``'bonds'``, ``'vectors'``. "
				"\n\n"
				":Default: ``'particles'``\n")
	;

	auto DeleteSelectedModifier_py = ovito_class<DeleteSelectedModifier, MultiDelegatingModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"This modifier deletes the currently selected elements of the following kinds: "
			"\n\n"
			"  * Particles (deleting particles whose ``'Selection'`` :ref:`property <particle-types-list>` is non-zero)\n"
			"  * Bonds (deleting bonds whose ``'Selection'`` :ref:`property <bond-types-list>` is non-zero)\n"
			"\n\n"
			"The modifier will act on all of them simultaneously by default. Restricting the delete operation to a subset is possible by setting the :py:attr:`.operate_on` field. "
			"See also the corresponding `user manual page <../../particles.modifiers.delete_selected_particles.html>`__ for this modifier. "
			)
	;	
	modifier_operate_on_list(DeleteSelectedModifier_py, std::mem_fn(&DeleteSelectedModifier::delegates), "operate_on", 
			"A set of strings specifying the kinds of data elements this modifier should operate on. "
			"By default the set contains all data element types supported by the modifier. "
			"\n\n"
			":Default: ``{'particles', 'bonds'}``\n");	
	
	ovito_class<ColorLegendOverlay, ViewportOverlay>(m,
			"Renders a color legend for a :py:class:`~ovito.modifiers.ColorCodingModifier` on top of the three-dimensional "
			"scene. You can attach an instance of this class to a :py:class:`~ovito.vis.Viewport` by adding it to the viewport's "
			":py:attr:`~ovito.vis.Viewport.overlays` collection:"
			"\n\n"
			".. literalinclude:: ../example_snippets/color_legend_overlay.py"
			"\n")
		.def_property("behind_scene", &ViewportOverlay::renderBehindScene, &ViewportOverlay::setRenderBehindScene,
				"This option puts the overlay behind the three-dimensional scene, i.e. as an \"underlay\" instead of an \"overlay\". "
				"If set to to true, objects in the scene will occclude the overlay content. "
				"\n\n"
				":Default: ``False``")
		.def_property("alignment", &ColorLegendOverlay::alignment, &ColorLegendOverlay::setAlignment,
				"Selects the corner of the viewport where the color bar is displayed (anchor position). This must be a valid `Qt.Alignment value <http://doc.qt.io/qt-5/qt.html#AlignmentFlag-enum>`__ as shown in the code example above. "
				"\n\n"
				":Default: ``PyQt5.QtCore.Qt.AlignHCenter ^ PyQt5.QtCore.Qt.AlignBottom``")
		.def_property("orientation", &ColorLegendOverlay::orientation, &ColorLegendOverlay::setOrientation,
				"Selects the orientation of the color bar. This must be a valid `Qt.Orientation value <http://doc.qt.io/qt-5/qt.html#Orientation-enum>`__ as shown in the code example above. "
				"\n\n"
				":Default: ``PyQt5.QtCore.Qt.Horizontal``")
		.def_property("offset_x", &ColorLegendOverlay::offsetX, &ColorLegendOverlay::setOffsetX,
				"This parameter allows to displace the color bar horizontally from its anchor position. The offset is specified as a fraction of the output image width."
				"\n\n"
				":Default: 0.0\n")
		.def_property("offset_y", &ColorLegendOverlay::offsetY, &ColorLegendOverlay::setOffsetY,
				"This parameter allows to displace the color bar vertically from its anchor position. The offset is specified as a fraction of the output image height."
				"\n\n"
				":Default: 0.0\n")
		.def_property("legend_size", &ColorLegendOverlay::legendSize, &ColorLegendOverlay::setLegendSize,
				"Controls the overall size of the color bar relative to the output image size. "
				"\n\n"
				":Default: 0.3\n")
		.def_property("aspect_ratio", &ColorLegendOverlay::aspectRatio, &ColorLegendOverlay::setAspectRatio,
				"The aspect ratio of the color bar. Larger values make it more narrow. "
				"\n\n"
				":Default: 8.0\n")
		.def_property("font_size", &ColorLegendOverlay::fontSize, &ColorLegendOverlay::setFontSize,
				"The relative size of the font used for text labels."
				"\n\n"
				":Default: 0.1\n")
		.def_property("format_string", &ColorLegendOverlay::valueFormatString, &ColorLegendOverlay::setValueFormatString,
				"The format string used with the `sprintf() <http://en.cppreference.com/w/cpp/io/c/fprintf>`__ function to "
				"generate the text representation of floating-point values. You can change this format string to control the "
				"number of decimal places or add units to the numeric values, for example. "
				"\n\n"
				":Default: '%g'\n")
		.def_property("title", &ColorLegendOverlay::title, &ColorLegendOverlay::setTitle,
				"The text displayed next to the color bar. If empty, the name of the input property selected in the :py:class:`~ovito.modifiers.ColorCodingModifier` "
				"is used. "
				"\n\n"
				":Default: ''")
		.def_property("label1", &ColorLegendOverlay::label1, &ColorLegendOverlay::setLabel1,
				"Sets the text string displayed at the upper end of the bar. If empty, the :py:attr:`~ovito.modifiers.ColorCodingModifier.end_value` of the "
				":py:class:`~ovito.modifiers.ColorCodingModifier` is used. "
				"\n\n"
				":Default: ''")
		.def_property("label2", &ColorLegendOverlay::label2, &ColorLegendOverlay::setLabel2,
				"Sets the text string displayed at the lower end of the bar. If empty, the :py:attr:`~ovito.modifiers.ColorCodingModifier.start_value` of the "
				":py:class:`~ovito.modifiers.ColorCodingModifier` is used. "
				"\n\n"
				":Default: ''")
		.def_property("modifier", &ColorLegendOverlay::modifier, &ColorLegendOverlay::setModifier,
				"The :py:class:`~ovito.modifiers.ColorCodingModifier` for which the color legend should be rendered.")
		.def_property("text_color", &ColorLegendOverlay::textColor, &ColorLegendOverlay::setTextColor,
				"The RGB color used for text labels."
				"\n\n"
				":Default: ``(0.0,0.0,0.0)``\n")
		.def_property("outline_color", &ColorLegendOverlay::outlineColor, &ColorLegendOverlay::setOutlineColor,
				"The text outline color. This is used only if :py:attr:`.outline_enabled` is set."
				"\n\n"
				":Default: ``(1.0,1.0,1.0)``\n")
		.def_property("outline_enabled", &ColorLegendOverlay::outlineEnabled, &ColorLegendOverlay::setOutlineEnabled,
				"Enables the painting of a font outline to make the text easier to read."
				"\n\n"
				":Default: ``False``\n")
	;

	ovito_class<ExpressionSelectionModifier, DelegatingModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"Selects elements based on a user-defined Boolean expression. "
			"See also the corresponding `user manual page <../../particles.modifiers.expression_select.html>`__ for this modifier. "
			"The modifier can operate on different classes of elements: "
			"\n\n"
			"  * Particles (setting the ``'Selection'`` :ref:`particle property <particle-types-list>`)\n"
			"  * Bonds (setting the ``'Selection'`` :ref:`bond property <bond-types-list>`)\n"
			"\n\n"
			"The modifier will act on particles by default. You can change this by setting the modifier's :py:attr:`.operate_on` field. "			
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``Selection`` (:py:class:`~ovito.data.Property`):\n"
			"   This property is set to 1 for selected elements and 0 for others.\n"
			" * ``SelectExpression.num_selected`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of particles selected by the modifier.\n"
			"\n\n"
			"**Example:**"
			"\n\n"
			".. literalinclude:: ../example_snippets/select_expression_modifier.py\n"
			"   :lines: 6-\n"
			"\n")
		.def_property("expression", &ExpressionSelectionModifier::expression, &ExpressionSelectionModifier::setExpression,
				"A string containing the Boolean expression to be evaluated for every element. "
				"The expression syntax is documented in `OVITO's user manual <../../particles.modifiers.expression_select.html>`__.")
		.def_property("operate_on", modifierDelegateGetter<ExpressionSelectionModifier>(), modifierDelegateSetter<ExpressionSelectionModifier>(),
				"Selects the kind of data elements this modifier should operate on. "
				"Supported values are: ``'particles'``, ``'bonds'``. "
				"\n\n"
				":Default: ``'particles'``\n")
	;

	ovito_class<FreezePropertyModifier, GenericPropertyModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`"
			"\n\n"
			"This modifier obtains the value of a property by evaluating the data pipeline at a fixed animation time (frame 0 by default), "
			"and injects it back into the pipeline, optionally under a different name than the original property. "
			"Thus, the :py:class:`!FreezePropertyModifier` allows you to *freeze* a dynamically changing property and overwrite its values with those from a fixed point in time. "
			"See also the corresponding `user manual page <../../particles.modifiers.freeze_property.html>`__ for this modifier. "
			"\n\n"
			"The modifier can operate on properties of different kinds of elements: "
			"\n\n"
			"  * Particles (:py:class:`~ovito.data.ParticleProperty`)\n"
			"  * Bonds (:py:class:`~ovito.data.BondProperty`)\n"
			"  * Voxel grids (:py:class:`~ovito.data.VoxelProperty`)\n"
			"\n\n"
			"The modifier will operate on particle properties by default. You can change this by setting the modifier's :py:attr:`.operate_on` field. "
			"\n\n"
			"**Example:**"
			"\n\n"
			".. literalinclude:: ../example_snippets/freeze_property_modifier.py\n"
			"   :emphasize-lines: 12-14\n"
			"\n")
		.def_property("source_property", &FreezePropertyModifier::sourceProperty, [](FreezePropertyModifier& mod, py::object val) {					
					mod.setSourceProperty(convertPythonPropertyReference(val, mod.propertyClass()));
				},
				"The name of the input property that should be evaluated by the modifier on the animation frame specified by :py:attr:`.freeze_at`. "
				"\n\n"
				"Note: Make sure that :py:attr:`.operate_on` is set to the desired value *before* setting this attribute, "
				"because changing :py:attr:`.operate_on` will implicitly reset the :py:attr:`!source_property` attribute. ")
		.def_property("destination_property", &FreezePropertyModifier::destinationProperty, [](FreezePropertyModifier& mod, py::object val) {					
					mod.setDestinationProperty(convertPythonPropertyReference(val, mod.propertyClass()));
				},
				"The name of the output property that should be created by the modifier. "
				"It may be the same as :py:attr:`.source_property`. If the destination property already exists in the modifier's input, the values are overwritten. "
				"\n\n"
				"Note: Make sure that :py:attr:`.operate_on` is set to the desired value *before* setting this attribute, "
				"because changing :py:attr:`.operate_on` will implicitly reset the :py:attr:`!destination_property` attribute. ")
		.def_property("freeze_at", 
				[](FreezePropertyModifier& mod) {
					return mod.dataset()->animationSettings()->timeToFrame(mod.freezeTime());
				},
				[](FreezePropertyModifier& mod, int frame) {
					mod.setFreezeTime(mod.dataset()->animationSettings()->frameToTime(frame));
				},
				"The animation frame number at which to freeze the input property's values. "
				"\n\n"
				":Default: 0\n")
		.def_property("operate_on", modifierPropertyClassGetter(), modifierPropertyClassSetter(),
				"Selects the kind of properties this modifier should operate on. "
				"Supported values are: ``'particles'``, ``'bonds'``, ``'voxels'``. "
				"\n\n"
				"Note: Assigning a new value to this attribute resets the :py:attr:`.source_property` and :py:attr:`.destination_property` fields. "
				"\n\n"
				":Default: ``'particles'``\n")
	;
	ovito_class<FreezePropertyModifierApplication, ModifierApplication>{m};

	ovito_class<ManualSelectionModifier, Modifier>(m)
		.def("reset_selection", &ManualSelectionModifier::resetSelection)
		.def("select_all", &ManualSelectionModifier::selectAll)
		.def("clear_selection", &ManualSelectionModifier::clearSelection)
		.def("toggle_selection", &ManualSelectionModifier::toggleElementSelection)
	;
	ovito_class<ManualSelectionModifierApplication, ModifierApplication>{m};

	ovito_abstract_class<ComputePropertyModifierDelegate, AsynchronousModifierDelegate>{m}
	;
	ovito_class<ComputePropertyModifier, AsynchronousDelegatingModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"Evaluates a user-defined math expression for every particle or bond and assigns the computed values to the selected output property. "
			"See also the corresponding `user manual page <../../particles.modifiers.compute_property.html>`__ for this modifier. "
			"\n\n"
			"The modifier can compute properties of different kinds of elements: "
			"\n\n"
			"  * Particles (:py:class:`~ovito.data.ParticleProperty`)\n"
			"  * Bonds (:py:class:`~ovito.data.BondProperty`)\n"
			"\n\n"
			"The modifier will operate on particles by default. You can change this by setting the modifier's :py:attr:`.operate_on` field. "
			"\n\n"
			"Usage example:"
			"\n\n"
			".. literalinclude:: ../example_snippets/compute_property_modifier.py\n"
			"   :lines: 6-\n"
			"\n"
			"Note that, as an alternative to this modifier, a :py:class:`PythonScriptModifier` with a user-defined modifier function may be the better choice to "
			"set properties, in particular when it comes to problems that involve complex element indexing or conditional computations. "
		)
		.def_property("operate_on", modifierDelegateGetter<ComputePropertyModifier>(), modifierDelegateSetter<ComputePropertyModifier>(),
				"Selects the kind of data elements this modifier should operate on. "
				"Supported values are: ``'particles'``, ``'bonds'``. "
				"\n\n"
				":Default: ``'particles'``\n")
		.def_property("expressions", &ComputePropertyModifier::expressions, &ComputePropertyModifier::setExpressions,
				"A list of strings containing the math expressions to compute, one for each vector component of the selected output property. "
				"If the output property is scalar, the list must comprise one expression string. "
				"\n\n"
				"Note: Before setting this field, make sure that :py:attr:`.output_property` is already set to the desired value, "
				"because changing the :py:attr:`.output_property` will implicitly resize the :py:attr:`!expressions` list. "
				"\n\n"
				"See the corresponding `user manual page <../../particles.modifiers.compute_property.html>`__ for a description of the expression syntax. "
				"\n\n"
				":Default: ``[\"0\"]``\n")		
		.def_property("output_property", &ComputePropertyModifier::outputProperty, [](ComputePropertyModifier& mod, py::object val) {					
					auto* delegate = static_object_cast<ComputePropertyModifierDelegate>(mod.delegate());
					mod.setOutputProperty(convertPythonPropertyReference(val, delegate ? &delegate->propertyClass() : nullptr));
				},
				"The output property that will receive the computed values. "
				"This can be one of the :ref:`standard property names <particle-types-list>` defined by OVITO or a user-defined property name. "
				"\n\n"
				"If :py:attr:`.operate_on` is set to ``'particles'``, this can be one of the :ref:`standard particle properties <particle-types-list>` "
				"or a name of a new user-defined :py:class:`~ovito.data.ParticleProperty`. "
				"If :py:attr:`.operate_on` is set to ``'bonds'``, this can be one of the :ref:`standard bond properties <bond-types-list>` "
				"or a name of a new user-defined :py:class:`~ovito.data.BondProperty`. "
				"\n\n"
				"Note: Make sure that the :py:attr:`.operate_on` field is set to the desired value *before* setting this field, "
				"because changing :py:attr:`.operate_on` will implicitly reset :py:attr:`!output_property` to its default value. "
				"\n\n"
				":Default: ``\"My property\"``\n")
		.def_property("component_count", &ComputePropertyModifier::propertyComponentCount, &ComputePropertyModifier::setPropertyComponentCount)
		.def_property("only_selected", &ComputePropertyModifier::onlySelectedElements, &ComputePropertyModifier::setOnlySelectedElements,
				"If ``True``, the property is only computed for currently selected elements. "
				"In this case, the property values of unselected elements will be preserved if the output property already exists. "
				"\n\n"
				":Default: ``False``\n")
	;
	ovito_class<ComputePropertyModifierApplication, AsynchronousModifierApplication>{m};

	ovito_class<CombineDatasetsModifier, MultiDelegatingModifier>(m,
			":Base class: :py:class:`ovito.pipeline.Modifier`\n\n"
			"This modifier loads a set of particles from a separate simulation file and merges them into the current dataset. "
			"See also the corresponding `user manual page <../../particles.modifiers.combine_particle_sets.html>`__ for this modifier. "
			"\n\n"
			"Example:"
			"\n\n"
			".. literalinclude:: ../example_snippets/combine_datasets_modifier.py")
		.def_property("source", &CombineDatasetsModifier::secondaryDataSource, &CombineDatasetsModifier::setSecondaryDataSource,
				"A :py:class:`~ovito.pipeline.FileSource` that provides the set of particles to be merged. "
				"You can call its :py:meth:`~ovito.pipeline.FileSource.load` function to load a data file "
				"as shown in the code example above.")
	;
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(StdMod);

}	// End of namespace
}	// End of namespace
