///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARxRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/particles/objects/ParticleType.h>
#include <plugins/particles/objects/ParticlesVis.h>
#include <plugins/particles/objects/VectorVis.h>
#include <plugins/particles/objects/BondsVis.h>
#include <plugins/particles/objects/BondProperty.h>
#include <plugins/particles/objects/ParticleBondMap.h>
#include <plugins/particles/objects/BondType.h>
#include <plugins/particles/objects/TrajectoryObject.h>
#include <plugins/particles/objects/TrajectoryGenerator.h>
#include <plugins/particles/objects/TrajectoryVis.h>
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include <plugins/particles/util/NearestNeighborFinder.h>
#include <plugins/stdobj/properties/PropertyStorage.h>
#include <plugins/stdobj/scripting/PythonBinding.h>
#include <plugins/pyscript/binding/PythonBinding.h>
#include <core/utilities/io/CompressedTextWriter.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/app/PluginManager.h>
#include "PythonBinding.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace PyScript;

void defineModifiersSubmodule(py::module parentModule);	// Defined in ModifierBinding.cpp
void defineImportersSubmodule(py::module parentModule);	// Defined in ImporterBinding.cpp
void defineExportersSubmodule(py::module parentModule);	// Defined in ExporterBinding.cpp

PYBIND11_PLUGIN(Particles)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();
	
	py::options options;
	options.disable_function_signatures();

	py::module m("Particles");

	auto ParticleProperty_py = ovito_abstract_class<ParticleProperty, PropertyObject>(m,
			":Base class: :py:class:`ovito.data.Property`\n\n"
			"Stores an array of per-particle values. This class derives from :py:class:`Property`, which provides the "
			"base functionality shared by all property types in OVITO. "
			"\n\n"
			"In OVITO's data model, an arbitrary number of properties can be associated with the particles, "
			"each property being represented by a separate :py:class:`!ParticleProperty` object. A :py:class:`!ParticleProperty` "
			"is basically an array of values whose length matches the number of particles. "
			"\n\n"
			"The set of properties currently associated with all particles is exposed by the "
			":py:attr:`DataCollection.particles` view, which allows accessing them by name "
			"and adding new properties. " 
			"\n\n"
			"**Standard properties**"
			"\n\n"
			"OVITO differentiates between *standard* properties and *user-defined* properties. The former have a "
			"special meaning to OVITO, a prescribed name and data layout. Certain standard properties control the visual representation "
			"of particles. Typical examples are the ``Position`` property, the ``Color`` property and the ``Radius`` property. "
			"User-defined properties, on the other hand, may have arbitrary names (as long as they do not collide with one of the standard names) "
			"and the property values have no special meaning to OVITO, only to you, the user. Whether a :py:class:`!ParticleProperty` is a "
			"standard or a user-defined property is indicated by the value of its :py:attr:`.type` attribute. "
			"\n\n"
			"**Creating particle properties**"
			"\n\n"
			"New properties can be created and assigned to particles with the :py:meth:`ParticlesView.create_property` factory method. "
			"User-defined modifier functions, for example, use this to output their computation results. "
			"\n\n"
			"**Typed particle properties**"
			"\n\n"
			"The standard property ``'Particle Type'`` stores the types of particles encoded as integer values, e.g.: "
			"\n\n"
			"    >>> data = node.compute()\n"
			"    >>> tprop = data.particles['Particle Type']\n"
			"    >>> print(tprop[...])\n"
			"    [2 1 3 ..., 2 1 2]\n"
			"\n\n"
			"Here, each number in the property array refers to a defined particle type (e.g. 1=Cu, 2=Ni, 3=Fe, etc.). The defined particle types, each one represented by "
			"an instance of the :py:class:`ParticleType` auxiliary class, are stored in the :py:attr:`.types` array "
			"of the :py:class:`!ParticleProperty` object. Each type has a unique :py:attr:`~ParticleType.id`, a human-readable :py:attr:`~ParticleType.name` "
			"and other attributes like :py:attr:`~ParticleType.color` and :py:attr:`~ParticleType.radius` that control the "
			"visual appearance of particles belonging to the type:"
			"\n\n"
			"    >>> for type in tprop.types:\n"
			"    ...     print(type.id, type.name, type.color, type.radius)\n"
			"    ... \n"
			"    1 Cu (0.188 0.313 0.972) 0.74\n"
			"    2 Ni (0.564 0.564 0.564) 0.77\n"
			"    3 Fe (1 0.050 0.050) 0.74\n"
			"\n\n"
			"IDs of types typically start at 1 and form a consecutive sequence as in the example above. "
			"Note, however, that the :py:attr:`.types` list may store the :py:class:`ParticleType` objects in an arbitrary order. "
			"Thus, in general, it is not valid to directly use a type ID as an index into the :py:attr:`.types` array. "
			"Instead, the :py:meth:`.type_by_id` method should be used to look up the :py:class:`ParticleType`:: "
			"\n\n"
			"    >>> for i,t in enumerate(tprop): # (loop over the type ID of each particle)\n"
			"    ...     print('Atom', i, 'is of type', tprop.type_by_id(t).name)\n"
			"    ...\n"
			"    Atom 0 is of type Ni\n"
			"    Atom 1 is of type Cu\n"
			"    Atom 2 is of type Fe\n"
			"    Atom 3 is of type Cu\n"
			"\n\n"
			"Similarly, a :py:meth:`.type_by_name` method exists that looks up a :py:attr:`ParticleType` by name. "
			"For example, to count the number of Fe atoms in a system:"
			"\n\n"
			"    >>> Fe_type_id = tprop.type_by_name('Fe').id   # Determine ID of the 'Fe' type\n"
			"    >>> numpy.count_nonzero(tprop == Fe_type_id)   # Count particles having that type ID\n"
			"    957\n"
			"\n\n"
			"Note that OVITO supports multiple type classifications. For example, in addition to the ``'Particle Type'`` standard particle property, "
			"which stores the chemical types of atoms (e.g. C, H, Fe, ...), the ``'Structure Type'`` property may hold the structural types computed for atoms "
			"(e.g. FCC, BCC, ...) maintaining its own list of known structure types in the :py:attr:`.types` array. "
		)
		// Used by ParticlePropertiesView.create(): 
		.def_static("createStandardProperty", [](DataSet& dataset, size_t particleCount, ParticleProperty::Type type, bool initializeMemory) {
			return ParticleProperty::createFromStorage(&dataset, ParticleProperty::createStandardStorage(particleCount, type, initializeMemory));
		})
		.def_static("createUserProperty", [](DataSet& dataset, size_t particleCount, int dataType, size_t componentCount, size_t stride, const QString& name, bool initializeMemory) {
			return ParticleProperty::createFromStorage(&dataset, std::make_shared<PropertyStorage>(particleCount, dataType, componentCount, stride, name, initializeMemory));
		})
		.def_static("standard_property_type_id", [](const QString& name) { return (ParticleProperty::Type)ParticleProperty::OOClass().standardPropertyTypeId(name); })
		.def_property_readonly("type", &ParticleProperty::type,
				".. _particle-types-list:"
				"\n\n"
				"The type of the particle property.\n"
				"One of the following constants:"
				"\n\n"
				"======================================================= =================================================== ========== ==================================\n"
				"Type constant                                           Property name                                       Data type  Component names\n"
				"======================================================= =================================================== ========== ==================================\n"
				"``ParticleProperty.Type.User``                          (a user-defined property with a non-standard name)  int/float  \n"
				"``ParticleProperty.Type.ParticleType``                  :guilabel:`Particle Type`                           int        \n"
				"``ParticleProperty.Type.Position``                      :guilabel:`Position`                                float      X, Y, Z\n"
				"``ParticleProperty.Type.Selection``                     :guilabel:`Selection`                               int        \n"
				"``ParticleProperty.Type.Color``                         :guilabel:`Color`                                   float      R, G, B\n"
				"``ParticleProperty.Type.Displacement``                  :guilabel:`Displacement`                            float      X, Y, Z\n"
				"``ParticleProperty.Type.DisplacementMagnitude``         :guilabel:`Displacement Magnitude`                  float      \n"
				"``ParticleProperty.Type.PotentialEnergy``               :guilabel:`Potential Energy`                        float      \n"
				"``ParticleProperty.Type.KineticEnergy``                 :guilabel:`Kinetic Energy`                          float      \n"
				"``ParticleProperty.Type.TotalEnergy``                   :guilabel:`Total Energy`                            float      \n"
				"``ParticleProperty.Type.Velocity``                      :guilabel:`Velocity`                                float      X, Y, Z\n"
				"``ParticleProperty.Type.Radius``                        :guilabel:`Radius`                                  float      \n"
				"``ParticleProperty.Type.Cluster``                       :guilabel:`Cluster`                                 int        \n"
				"``ParticleProperty.Type.Coordination``                  :guilabel:`Coordination`                            int        \n"
				"``ParticleProperty.Type.StructureType``                 :guilabel:`Structure Type`                          int        \n"
				"``ParticleProperty.Type.Identifier``                    :guilabel:`Particle Identifier`                     int        \n"
				"``ParticleProperty.Type.StressTensor``                  :guilabel:`Stress Tensor`                           float      XX, YY, ZZ, XY, XZ, YZ\n"
				"``ParticleProperty.Type.StrainTensor``                  :guilabel:`Strain Tensor`                           float      XX, YY, ZZ, XY, XZ, YZ\n"
				"``ParticleProperty.Type.DeformationGradient``           :guilabel:`Deformation Gradient`                    float      XX, YX, ZX, XY, YY, ZY, XZ, YZ, ZZ\n"
				"``ParticleProperty.Type.Orientation``                   :guilabel:`Orientation`                             float      X, Y, Z, W\n"
				"``ParticleProperty.Type.Force``                         :guilabel:`Force`                                   float      X, Y, Z\n"
				"``ParticleProperty.Type.Mass``                          :guilabel:`Mass`                                    float      \n"
				"``ParticleProperty.Type.Charge``                        :guilabel:`Charge`                                  float      \n"
				"``ParticleProperty.Type.PeriodicImage``                 :guilabel:`Periodic Image`                          int        X, Y, Z\n"
				"``ParticleProperty.Type.Transparency``                  :guilabel:`Transparency`                            float      \n"
				"``ParticleProperty.Type.DipoleOrientation``             :guilabel:`Dipole Orientation`                      float      X, Y, Z\n"
				"``ParticleProperty.Type.DipoleMagnitude``               :guilabel:`Dipole Magnitude`                        float      \n"
				"``ParticleProperty.Type.AngularVelocity``               :guilabel:`Angular Velocity`                        float      X, Y, Z\n"
				"``ParticleProperty.Type.AngularMomentum``               :guilabel:`Angular Momentum`                        float      X, Y, Z\n"
				"``ParticleProperty.Type.Torque``                        :guilabel:`Torque`                                  float      X, Y, Z\n"
				"``ParticleProperty.Type.Spin``                          :guilabel:`Spin`                                    float      \n"
				"``ParticleProperty.Type.CentroSymmetry``                :guilabel:`Centrosymmetry`                          float      \n"
				"``ParticleProperty.Type.VelocityMagnitude``             :guilabel:`Velocity Magnitude`                      float      \n"
				"``ParticleProperty.Type.Molecule``                      :guilabel:`Molecule Identifier`                     int        \n"
				"``ParticleProperty.Type.AsphericalShape``               :guilabel:`Aspherical Shape`                        float      X, Y, Z\n"
				"``ParticleProperty.Type.VectorColor``                   :guilabel:`Vector Color`                            float      R, G, B\n"
				"``ParticleProperty.Type.ElasticStrainTensor``           :guilabel:`Elastic Strain`                          float      XX, YY, ZZ, XY, XZ, YZ\n"
				"``ParticleProperty.Type.ElasticDeformationGradient``    :guilabel:`Elastic Deformation Gradient`            float      XX, YX, ZX, XY, YY, ZY, XZ, YZ, ZZ\n"
				"``ParticleProperty.Type.Rotation``                      :guilabel:`Rotation`                                float      X, Y, Z, W\n"
				"``ParticleProperty.Type.StretchTensor``                 :guilabel:`Stretch Tensor`                          float      XX, YY, ZZ, XY, XZ, YZ\n"
				"``ParticleProperty.Type.MoleculeType``                  :guilabel:`Molecule Type`                           int        \n"
				"======================================================= =================================================== ========== ==================================\n"
				)
	;
	expose_mutable_subobject_list(ParticleProperty_py,
		std::mem_fn(&ParticleProperty::elementTypes), 
		std::mem_fn(&ParticleProperty::insertElementType), 
		std::mem_fn(&ParticleProperty::removeElementType), "types", "ParticleTypeList",
			  "A (mutable) list of :py:class:`ParticleType` instances. "
			  "\n\n"
			  "Note that the particle types may be stored in arbitrary order in this list. Thus, it is not valid to use a numeric type ID as an index into this list. ");

	py::enum_<ParticleProperty::Type>(ParticleProperty_py, "Type")
		.value("User", ParticleProperty::UserProperty)
		.value("ParticleType", ParticleProperty::TypeProperty)
		.value("Position", ParticleProperty::PositionProperty)
		.value("Selection", ParticleProperty::SelectionProperty)
		.value("Color", ParticleProperty::ColorProperty)
		.value("Displacement", ParticleProperty::DisplacementProperty)
		.value("DisplacementMagnitude", ParticleProperty::DisplacementMagnitudeProperty)
		.value("PotentialEnergy", ParticleProperty::PotentialEnergyProperty)
		.value("KineticEnergy", ParticleProperty::KineticEnergyProperty)
		.value("TotalEnergy", ParticleProperty::TotalEnergyProperty)
		.value("Velocity", ParticleProperty::VelocityProperty)
		.value("Radius", ParticleProperty::RadiusProperty)
		.value("Cluster", ParticleProperty::ClusterProperty)
		.value("Coordination", ParticleProperty::CoordinationProperty)
		.value("StructureType", ParticleProperty::StructureTypeProperty)
		.value("Identifier", ParticleProperty::IdentifierProperty)
		.value("StressTensor", ParticleProperty::StressTensorProperty)
		.value("StrainTensor", ParticleProperty::StrainTensorProperty)
		.value("DeformationGradient", ParticleProperty::DeformationGradientProperty)
		.value("Orientation", ParticleProperty::OrientationProperty)
		.value("Force", ParticleProperty::ForceProperty)
		.value("Mass", ParticleProperty::MassProperty)
		.value("Charge", ParticleProperty::ChargeProperty)
		.value("PeriodicImage", ParticleProperty::PeriodicImageProperty)
		.value("Transparency", ParticleProperty::TransparencyProperty)
		.value("DipoleOrientation", ParticleProperty::DipoleOrientationProperty)
		.value("DipoleMagnitude", ParticleProperty::DipoleMagnitudeProperty)
		.value("AngularVelocity", ParticleProperty::AngularVelocityProperty)
		.value("AngularMomentum", ParticleProperty::AngularMomentumProperty)
		.value("Torque", ParticleProperty::TorqueProperty)
		.value("Spin", ParticleProperty::SpinProperty)
		.value("CentroSymmetry", ParticleProperty::CentroSymmetryProperty)
		.value("VelocityMagnitude", ParticleProperty::VelocityMagnitudeProperty)
		.value("Molecule", ParticleProperty::MoleculeProperty)
		.value("AsphericalShape", ParticleProperty::AsphericalShapeProperty)
		.value("VectorColor", ParticleProperty::VectorColorProperty)
		.value("ElasticStrainTensor", ParticleProperty::ElasticStrainTensorProperty)
		.value("ElasticDeformationGradient", ParticleProperty::ElasticDeformationGradientProperty)
		.value("Rotation", ParticleProperty::RotationProperty)
		.value("StretchTensor", ParticleProperty::StretchTensorProperty)
		.value("MoleculeType", ParticleProperty::MoleculeTypeProperty)
	;								

	py::class_<ParticleBondMap>(m, "BondsEnumerator",
		"Utility class that permits efficient iteration over the bonds connected to specific particles. "
		"\n\n"
    	"The constructor takes a :py:class:`DataCollection` object as input. "
		"From the unordered list of bonds in the data collection, the :py:class:`!BondsEnumerator` will build a lookup table for quick enumeration  "
		"of bonds of particular particles. "
		"\n\n"
		"All bonds connected to a given particle can be subsequently visited using the :py:meth:`.bonds_of_particle` method. "
		"\n\n"
		"Warning: Do not modify the underlying bonds list in the data collection while the :py:class:`!BondsEnumerator` is in use. "
		"Adding or deleting bonds would render the internal lookup table of the :py:class:`!BondsEnumerator` invalid. "
		"\n\n"
		"**Usage example**"
		"\n\n"
		".. literalinclude:: ../example_snippets/bonds_enumerator.py\n")
		// Customized constructor function:
		.def("__init__", [](ParticleBondMap& instance, py::object data_collection) {
				// Get the 'Topology' and the 'Periodic image' bond propertie from the data collection:
				py::object topologyPropertyName = py::cast(BondProperty::OOClass().standardPropertyName(BondProperty::TopologyProperty));
				py::object pbcShiftPropertyName = py::cast(BondProperty::OOClass().standardPropertyName(BondProperty::PeriodicImageProperty));

				py::object DataCollection = py::module::import("ovito.data").attr("DataCollection");
				py::object BondsView = py::module::import("ovito.data").attr("BondsView");
				py::object bonds_view;
				if(py::isinstance(data_collection, DataCollection))
					bonds_view = data_collection.attr("bonds");
				else if(py::isinstance(data_collection, BondsView))
					bonds_view = data_collection;
				else
					throw Exception("BondsEnumerator construction failed. Data collection expected as argument.");
				if(!bonds_view.contains(topologyPropertyName))
					throw Exception("BondsEnumerator construction failed. Data collection doesn't contain any bonds.");
				py::object topology_prop = bonds_view[topologyPropertyName];
				py::object pbd_shift_prop;
				if(bonds_view.contains(pbcShiftPropertyName))
					pbd_shift_prop = bonds_view[pbcShiftPropertyName];
				// Initialize BondsEnumerator instance.
				new (&instance) ParticleBondMap(
						topology_prop.cast<BondProperty*>()->storage(),
						pbd_shift_prop ? pbd_shift_prop.cast<BondProperty*>()->storage() : nullptr);
			}, 
			py::arg("data_collection"))
		.def("bonds_of_particle", [](const ParticleBondMap& bondMap, size_t particleIndex) {
			auto range = bondMap.bondIndicesOfParticle(particleIndex);
			return py::make_iterator<py::return_value_policy::automatic>(range.begin(), range.end());
		},
		py::keep_alive<0, 1>(),
		"Returns an iterator that yields the indices of the bonds connected to the given particle. "
        "The indices can be used to index into the :py:class:`BondProperty` arrays. ");
	;

	ovito_class<ParticleType, ElementType>(m,
			"Represents a particle type or atom type. A :py:class:`!ParticleType` instance is always owned by a :py:class:`ParticleProperty`. ")
		.def_property("id", &ParticleType::id, &ParticleType::setId,
				"The identifier of the particle type.")
		.def_property("color", &ParticleType::color, &ParticleType::setColor,
				"The display color to use for particles of this type.")
		.def_property("radius", &ParticleType::radius, &ParticleType::setRadius,
				"The display radius to use for particles of this type.")
		.def_property("name", &ParticleType::name, &ParticleType::setName,
				"The display name of this particle type.")
	;

	auto ParticlesVis_py = ovito_class<ParticlesVis, DataVis>(m,
			":Base class: :py:class:`ovito.vis.DataVis`\n\n"
			"This element controls the visual appearance of particles. "
			"\n\n"
			"An instance of this class is attached to the :py:class:`~ovito.data.ParticleProperty` named ``Position`` "
			"and can be accessed through its :py:attr:`~ovito.data.DataObject.vis` field: "
			"\n\n"
			".. literalinclude:: ../example_snippets/particles_vis.py\n")
		.def_property("radius", &ParticlesVis::defaultParticleRadius, &ParticlesVis::setDefaultParticleRadius,
				"The standard display radius of particles. "
				"This value is only used if no per-particle or per-type radii have been set. "
				"A per-type radius can be set via :py:attr:`ovito.data.ParticleType.radius`. "
				"An individual display radius can be assigned to particles by creating a ``Radius`` "
				":py:class:`~ovito.data.ParticleProperty`, e.g. using the :py:class:`~ovito.modifiers.ComputePropertyModifier`. "
				"\n\n"
				":Default: 1.2\n")
		.def_property_readonly("default_color", &ParticlesVis::defaultParticleColor)
		.def_property_readonly("selection_color", &ParticlesVis::selectionParticleColor)
		.def_property("rendering_quality", &ParticlesVis::renderingQuality, &ParticlesVis::setRenderingQuality)
		.def_property("shape", &ParticlesVis::particleShape, &ParticlesVis::setParticleShape,
				"The display shape of particles.\n"
				"Possible values are:"
				"\n\n"
				"   * ``ParticlesVis.Shape.Sphere`` (default) \n"
				"   * ``ParticlesVis.Shape.Box``\n"
				"   * ``ParticlesVis.Shape.Circle``\n"
				"   * ``ParticlesVis.Shape.Square``\n"
				"   * ``ParticlesVis.Shape.Cylinder``\n"
				"   * ``ParticlesVis.Shape.Spherocylinder``\n"
				"\n")
		;

	py::enum_<ParticlesVis::ParticleShape>(ParticlesVis_py, "Shape")
		.value("Sphere", ParticlesVis::Sphere)
		.value("Box", ParticlesVis::Box)
		.value("Circle", ParticlesVis::Circle)
		.value("Square", ParticlesVis::Square)
		.value("Cylinder", ParticlesVis::Cylinder)
		.value("Spherocylinder", ParticlesVis::Spherocylinder)
	;

	auto VectorVis_py = ovito_class<VectorVis, DataVis>(m,
			":Base class: :py:class:`ovito.vis.DataVis`\n\n"
			"Controls the visual appearance of vector arrow elements."
			"\n\n"
			"A :py:class:`!VectorVis` element is typically attached to a :py:class:`~ovito.data.ParticleProperty` that represents a three-dimensional "
			"vector quantity, for example the ``Displacement`` property or the ``Force`` property. "
			"\n\n"
			"The parameters of this class control the visual appearance of the arrow elements that are rendered at each particle to "
			"visualize the vector values. If you have imported a vector particle property named either ``Force``, ``Dipole`` or ``Displacement`` "
			"from a simulation file, you can subsequently access the attached :py:class:`!VectorVis` object through the property's :py:attr:`~ovito.data.DataObject.vis` field: "
			"\n\n"
			".. literalinclude:: ../example_snippets/vector_vis.py\n"
			"   :lines: 6-9\n"
			"\n\n"
			"Modifiers that output vector particle properties, e.g. the :py:class:`~ovito.modifiers.CalculateDisplacementsModifier`, "
			"typically manage their own :py:class:`!VectorVis` element, which can be accessed directly: "
			"\n\n"
			".. literalinclude:: ../example_snippets/vector_vis.py\n"
			"   :lines: 14-17\n"
			"\n\n")
		.def_property("shading", &VectorVis::shadingMode, &VectorVis::setShadingMode,
				"The shading style used for the arrows.\n"
				"Possible values:"
				"\n\n"
				"   * ``VectorVis.Shading.Normal`` (default) \n"
				"   * ``VectorVis.Shading.Flat``\n"
				"\n")
		.def_property("rendering_quality", &VectorVis::renderingQuality, &VectorVis::setRenderingQuality)
		.def_property("reverse", &VectorVis::reverseArrowDirection, &VectorVis::setReverseArrowDirection,
				"Boolean flag controlling the reversal of arrow directions."
				"\n\n"
				":Default: ``False``\n")
		.def_property("alignment", &VectorVis::arrowPosition, &VectorVis::setArrowPosition,
				"Controls the positioning of arrows with respect to the particles.\n"
				"Possible values:"
				"\n\n"
				"   * ``VectorVis.Alignment.Base`` (default) \n"
				"   * ``VectorVis.Alignment.Center``\n"
				"   * ``VectorVis.Alignment.Head``\n"
				"\n")
		.def_property("color", &VectorVis::arrowColor, &VectorVis::setArrowColor,
				"The display color of arrows."
				"\n\n"
				":Default: ``(1.0, 1.0, 0.0)``\n")
		.def_property("width", &VectorVis::arrowWidth, &VectorVis::setArrowWidth,
				"Controls the width of arrows (in natural length units)."
				"\n\n"
				":Default: 0.5\n")
		.def_property("scaling", &VectorVis::scalingFactor, &VectorVis::setScalingFactor,
				"The uniform scaling factor applied to vectors."
				"\n\n"
				":Default: 1.0\n")
	;

	py::enum_<VectorVis::ArrowPosition>(VectorVis_py, "Alignment")
		.value("Base", VectorVis::Base)
		.value("Center", VectorVis::Center)
		.value("Head", VectorVis::Head)
	;

	ovito_class<BondsVis, DataVis>(m,
			":Base class: :py:class:`ovito.vis.DataVis`\n\n"
			"A visualization element for rendering bonds between particles. "
			"An instance of this class is attached to the :py:class:`~ovito.data.BondProperty` named ``Topology``. "
			"\n\n"
			"The parameters of this class control the visual appearance of bonds. "
			"If you have imported a simulation file containing bonds, you can subsequently access the :py:class:`!BondsVis` object attached "
			"to the ``Topology`` bond property through its :py:attr:`~ovito.data.DataObject.vis` field:"
			"\n\n"
			".. literalinclude:: ../example_snippets/bonds_vis.py\n"
			"   :lines: 6-8\n"
			"\n\n"
			"If you are creating bonds within OVITO using a modifier, such as the :py:class:`~ovito.modifiers.CreateBondsModifier`, "
			"the :py:class:`!BondsVis` element is typically managed by the modifier:"
			"\n\n"
			".. literalinclude:: ../example_snippets/bonds_vis.py\n"
			"   :lines: 12-14\n")
		.def_property("width", &BondsVis::bondWidth, &BondsVis::setBondWidth,
				"The display width of bonds (in natural length units)."
				"\n\n"
				":Default: 0.4\n")
		.def_property("color", &BondsVis::bondColor, &BondsVis::setBondColor,
				"The uniform display color of bonds. This value is only used if :py:attr:`.use_particle_colors` is false and "
				"if the ``Color`` :py:class:`~ovito.data.BondProperty` is not defined. "
				"\n\n"
				":Default: ``(0.6, 0.6, 0.6)``\n")
		.def_property("shading", &BondsVis::shadingMode, &BondsVis::setShadingMode,
				"Controls the shading style of bonds. "
				"Possible values:"
				"\n\n"
				"   * ``BondsVis.Shading.Normal`` (default) \n"
				"   * ``BondsVis.Shading.Flat``\n"
				"\n")
		.def_property("rendering_quality", &BondsVis::renderingQuality, &BondsVis::setRenderingQuality)
		.def_property("use_particle_colors", &BondsVis::useParticleColors, &BondsVis::setUseParticleColors,
				"If set to ``True``, bonds are rendered in the same color as the particles they are incident to. "
				"Otherwise, a uniform :py:attr:`.color` is used. If the :py:class:`~ovito.data.BondProperty` named ``Color`` is "
				"defined, then the per-bond colors are used in any case. "
				"\n\n"
				":Default: ``True``\n")
	;

	auto CutoffNeighborFinder_py = py::class_<CutoffNeighborFinder>(m, "CutoffNeighborFinder")
		.def(py::init<>())
		.def("prepare", [](CutoffNeighborFinder& finder, FloatType cutoff, ParticleProperty& positions, SimulationCellObject& cell) {
				return finder.prepare(cutoff, *positions.storage(), cell.data(), nullptr, nullptr);
			})
	;

	py::class_<CutoffNeighborFinder::Query>(CutoffNeighborFinder_py, "Query")
		.def(py::init<const CutoffNeighborFinder&, size_t>())
		.def("next", &CutoffNeighborFinder::Query::next)
		.def_property_readonly("at_end", &CutoffNeighborFinder::Query::atEnd)
		.def_property_readonly("index", &CutoffNeighborFinder::Query::current)
		.def_property_readonly("distance_squared", &CutoffNeighborFinder::Query::distanceSquared)
		.def_property_readonly("distance", [](const CutoffNeighborFinder::Query& query) -> FloatType { return sqrt(query.distanceSquared()); })
		.def_property_readonly("delta", &CutoffNeighborFinder::Query::delta)
		.def_property_readonly("pbc_shift", &CutoffNeighborFinder::Query::pbcShift)
	;

	auto NearestNeighborFinder_py = py::class_<NearestNeighborFinder>(m, "NearestNeighborFinder")
		.def(py::init<size_t>())
		.def("prepare", [](NearestNeighborFinder& finder, ParticleProperty& positions, SimulationCellObject& cell) {
			return finder.prepare(*positions.storage(), cell.data(), nullptr, nullptr);
		})
	;

	typedef NearestNeighborFinder::Query<30> NearestNeighborQuery;

	py::class_<NearestNeighborFinder::Neighbor>(NearestNeighborFinder_py, "Neighbor")
		.def_readonly("index", &NearestNeighborFinder::Neighbor::index)
		.def_readonly("distance_squared", &NearestNeighborFinder::Neighbor::distanceSq)
		.def_property_readonly("distance", [](const NearestNeighborFinder::Neighbor& n) -> FloatType { return sqrt(n.distanceSq); })
		.def_readonly("delta", &NearestNeighborFinder::Neighbor::delta)
	;

	py::class_<NearestNeighborQuery>(NearestNeighborFinder_py, "Query")
		.def(py::init<const NearestNeighborFinder&>())
		.def("findNeighbors", static_cast<void (NearestNeighborQuery::*)(size_t)>(&NearestNeighborQuery::findNeighbors))
		.def("findNeighborsAtLocation", static_cast<void (NearestNeighborQuery::*)(const Point3&, bool)>(&NearestNeighborQuery::findNeighbors))
		.def_property_readonly("count", [](const NearestNeighborQuery& q) -> int { return q.results().size(); })
		.def("__getitem__", [](const NearestNeighborQuery& q, int index) -> const NearestNeighborFinder::Neighbor& { return q.results()[index]; },
			py::return_value_policy::reference_internal)
	;

	auto BondProperty_py = ovito_abstract_class<BondProperty, PropertyObject>(m,
			":Base class: :py:class:`ovito.data.Property`\n\n"
			"Stores an array of per-bond values. This class derives from :py:class:`Property`, which provides the "
			"base functionality shared by all property types in OVITO. "
			"\n\n"
			"In OVITO's data model, an arbitrary set of properties can be associated with bonds, "
			"each property being represented by a :py:class:`!BondProperty` object. A :py:class:`!BondProperty` "
			"is basically an array of values whose length matches the numer of bonds in the data collection (see :py:attr:`BondsView.count`). "
			"\n\n"
			":py:class:`!BondProperty` objects have the same fields and behave the same way as :py:class:`ParticleProperty` objects. "
			"Both property classes derives from the common :py:class:`Property` base class. Please see its documentation on how to access per-bond values. "
			"\n\n"
			"The set of properties currently associated with the bonds is exposed by the "
			":py:attr:`DataCollection.bonds` view, which allows accessing them by name and adding new properties. " 
			"\n\n"
			"Note that the topological definition of bonds, i.e. the connectivity between particles, is stored "
			"in the :py:class:`!BondProperty` named ``Topology``. ")
		// Used by BondPropertiesView.create():
		.def_static("createStandardProperty", [](DataSet& dataset, size_t bondCount, BondProperty::Type type, bool initializeMemory) {
			return BondProperty::createFromStorage(&dataset, BondProperty::createStandardStorage(bondCount, type, initializeMemory));
		})
		.def_static("createUserProperty", [](DataSet& dataset, size_t bondCount, int dataType, size_t componentCount, size_t stride, const QString& name, bool initializeMemory) {
			return BondProperty::createFromStorage(&dataset, std::make_shared<PropertyStorage>(bondCount, dataType, componentCount, stride, name, initializeMemory));
		})
		.def_static("standard_property_type_id", [](const QString& name) { return (BondProperty::Type)BondProperty::OOClass().standardPropertyTypeId(name); })
		.def_property_readonly("type", &BondProperty::type, 
				".. _bond-types-list:"
				"\n\n"
				"The type of the bond property (user-defined or one of the standard types).\n"
				"One of the following constants:"
				"\n\n"
				"======================================================= =================================================== ==========\n"
				"Type constant                                           Property name                                       Data type \n"
				"======================================================= =================================================== ==========\n"
				"``BondProperty.Type.User``                              (a user-defined property with a non-standard name)  int/float \n"
				"``BondProperty.Type.BondType``                          :guilabel:`Bond Type`                               int       \n"
				"``BondProperty.Type.Selection``                         :guilabel:`Selection`                               int       \n"
				"``BondProperty.Type.Color``                             :guilabel:`Color`                                   float (3x)\n"
				"``BondProperty.Type.Length``                            :guilabel:`Length`                                  float     \n"
				"``BondProperty.Type.Topology``                          :guilabel:`Topology`                                int (2x)  \n"
				"``BondProperty.Type.PeriodicImage``                     :guilabel:`Periodic Image`                          int (3x)  \n"
				"======================================================= =================================================== ==========\n"
				)
	;
	expose_mutable_subobject_list(BondProperty_py,
		std::mem_fn(&BondProperty::elementTypes), 
		std::mem_fn(&BondProperty::insertElementType), 
		std::mem_fn(&BondProperty::removeElementType), "types", "BondTypeList",
				"A (mutable) list of :py:class:`BondType` instances. "
			  "\n\n"
			  "Note that the bond types may be stored in arbitrary order in this type list.");

	py::enum_<BondProperty::Type>(BondProperty_py, "Type")
		.value("User", BondProperty::UserProperty)
		.value("BondType", BondProperty::TypeProperty)
		.value("Selection", BondProperty::SelectionProperty)
		.value("Color", BondProperty::ColorProperty)
		.value("Length", BondProperty::LengthProperty)
		.value("Topology", BondProperty::TopologyProperty)
		.value("PeriodicImage", BondProperty::PeriodicImageProperty)
	;

	ovito_class<BondType, ElementType>(m,
			"Represents a bond type. A :py:class:`!BondType` instance is always owned by a :py:class:`BondTypeProperty`. ")
		.def_property("id", &BondType::id, &BondType::setId,
				"The identifier of the bond type.")
		.def_property("color", &BondType::color, &BondType::setColor,
				"The display color to use for bonds of this type.")
		.def_property("name", &BondType::name, &BondType::setName,
				"The display name of this bond type.")
	;

	ovito_class<TrajectoryObject, DataObject>(m,
			":Base class: :py:class:`ovito.data.DataObject`"
			"\n\n"
			"This data object type stores the traced trajectory lines of a group of particles. "
			"It is typically generated by a :py:class:`~ovito.pipeline.TrajectoryLineGenerator`. "
			"\n\n"
			"Each :py:class:`!TrajectoryLines` object is associated with a :py:class:`~ovito.vis.TrajectoryVis` "
			"element, which controls the visual appearance of the trajectory lines in rendered images. "
			"It is accessible through the :py:attr:`~DataObject.vis` base class attribute. "
			,
			// Python class name:
			"TrajectoryLines")
	;

	ovito_class<TrajectoryGenerator, StaticSource>(m,
			":Base class: :py:class:`ovito.pipeline.StaticSource`"
			"\n\n"
			"A type of pipeline source that generates trajectory lines by sampling the particle positions of another :py:class:`Pipeline`. "
			"It is used to statically visualize the trajectories of particles. "
			"The trajectory line generation must be explicitly triggered by a call to :py:meth:`.generate`. "
			"The visual appearance of the trajectory lines is controlled by a " 
			":py:class:`~ovito.vis.TrajectoryVis` element attached to the generated :py:class:`~ovito.data.TrajectoryLines` data object. "
			"\n\n"
			"**Usage example:**"
			"\n\n"
			".. literalinclude:: ../example_snippets/trajectory_lines.py",
			// Python class name:
			"TrajectoryLineGenerator")
		.def_property("source_pipeline", &TrajectoryGenerator::source, &TrajectoryGenerator::setSource,
				"The :py:class:`~ovito.pipeline.Pipeline` providing the time-dependent particle positions from which the trajectory lines will be generated. ") 
		// For backward compatibility with OVITO 2.9.0:
		.def_property("source_node", &TrajectoryGenerator::source, &TrajectoryGenerator::setSource)
		.def_property("only_selected", &TrajectoryGenerator::onlySelectedParticles, &TrajectoryGenerator::setOnlySelectedParticles,
				"Controls whether trajectory lines should only by generated for currently selected particles."
				"\n\n"
				":Default: ``True``\n")
		.def_property("unwrap_trajectories", &TrajectoryGenerator::unwrapTrajectories, &TrajectoryGenerator::setUnwrapTrajectories,
				"Controls whether trajectory lines should be automatically unwrapped at the box boundaries when the particles cross a periodic boundary."
				"\n\n"
				":Default: ``True``\n")
		.def_property("sampling_frequency", &TrajectoryGenerator::everyNthFrame, &TrajectoryGenerator::setEveryNthFrame,
				"Length of the animation frame intervals at which the particle positions should be sampled."
				"\n\n"
				":Default: 1\n")
		.def_property("frame_interval", [](TrajectoryGenerator& tgo) -> py::object {
					if(tgo.useCustomInterval()) return py::make_tuple(
						tgo.dataset()->animationSettings()->timeToFrame(tgo.customIntervalStart()),
						tgo.dataset()->animationSettings()->timeToFrame(tgo.customIntervalEnd()));
					else
						return py::none();
				},
				[](TrajectoryGenerator& tgo, py::object arg) {
					if(py::isinstance<py::none>(arg)) {
						tgo.setUseCustomInterval(false);
						return;
					}
					else if(py::isinstance<py::tuple>(arg)) {
						py::tuple tup = py::reinterpret_borrow<py::tuple>(arg);
						if(tup.size() == 2) {
							int a  = tup[0].cast<int>();
							int b  = tup[1].cast<int>();
							tgo.setCustomIntervalStart(tgo.dataset()->animationSettings()->frameToTime(a));
							tgo.setCustomIntervalEnd(tgo.dataset()->animationSettings()->frameToTime(b));
							tgo.setUseCustomInterval(true);
							return;
						}
					}
					throw py::value_error("Tuple of two integers or None expected.");
				},
				"The animation frame interval over which the particle positions are sampled to generate the trajectory lines. "
				"Set this to a tuple of two integers to specify the first and the last animation frame; or use ``None`` to generate trajectory lines "
				"over the entire input sequence."
				"\n\n"
				":Default: ``None``\n")
		.def("generate", [](TrajectoryGenerator& obj) {
				TrajectoryObject* traj = obj.generateTrajectories(ScriptEngine::activeTaskManager());
				if(!traj)
					obj.throwException(ScriptEngine::tr("Trajectory line generation has been canceled by the user."));
				return traj;
			},
			"Generates the trajectory lines by sampling the positions of the particles from the :py:attr:`.source_pipeline` in regular animation time intervals. "
			"The method creates a :py:class:`~ovito.data.TrajectoryLines` data object to store the trajectory line data. The object is inserted into this data collection "
			"and also returned to the caller. ")
	;	

	ovito_class<TrajectoryVis, DataVis>(m,
			":Base class: :py:class:`ovito.vis.DataVis`"
			"\n\n"
			"Controls the visual appearance of particle trajectory lines. An instance of this class is attached "
			"to every :py:class:`~ovito.data.TrajectoryLineGenerator` data object.")
		.def_property("width", &TrajectoryVis::lineWidth, &TrajectoryVis::setLineWidth,
				"The display width of trajectory lines."
				"\n\n"
				":Default: 0.2\n")
		.def_property("color", &TrajectoryVis::lineColor, &TrajectoryVis::setLineColor,
				"The display color of trajectory lines."
				"\n\n"
				":Default: ``(0.6, 0.6, 0.6)``\n")
		.def_property("shading", &TrajectoryVis::shadingMode, &TrajectoryVis::setShadingMode,
				"The shading style used for trajectory lines.\n"
				"Possible values:"
				"\n\n"
				"   * ``TrajectoryVis.Shading.Normal`` \n"
				"   * ``TrajectoryVis.Shading.Flat`` (default)\n"
				"\n")
		.def_property("upto_current_time", &TrajectoryVis::showUpToCurrentTime, &TrajectoryVis::setShowUpToCurrentTime,
				"If ``True``, trajectory lines are only rendered up to the particle positions at the current animation time. "
				"Otherwise, the complete trajectory lines are displayed."
				"\n\n"
				":Default: ``False``\n")
	;

	// Register submodules.
	defineModifiersSubmodule(m);	// Defined in ModifierBinding.cpp
	defineImportersSubmodule(m);	// Defined in ImporterBinding.cpp
	defineExportersSubmodule(m);	// Defined in ExporterBinding.cpp

	return m.ptr();
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(Particles);

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
