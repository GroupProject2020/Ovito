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
#include <plugins/particles/objects/ParticleType.h>
#include <plugins/particles/objects/ParticlesVis.h>
#include <plugins/particles/objects/ParticlesObject.h>
#include <plugins/particles/objects/VectorVis.h>
#include <plugins/particles/objects/BondsVis.h>
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/particles/objects/ParticleBondMap.h>
#include <plugins/particles/objects/BondType.h>
#include <plugins/particles/objects/TrajectoryObject.h>
#include <plugins/particles/objects/TrajectoryVis.h>
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include <plugins/particles/util/NearestNeighborFinder.h>
#include <plugins/stdobj/properties/PropertyStorage.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
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

PYBIND11_MODULE(Particles, m)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();
	
	py::options options;
	options.disable_function_signatures();

	auto Particles_py = ovito_class<ParticlesObject, PropertyContainer>(m,
		":Base class: :py:class:`ovito.data.PropertyContainer`"
		"\n\n"
		".. _particle-types-list:"
		"\n\n"
		"The following standard properties are defined for particles:"
		"\n\n"
		"======================================================= =================================================== ========== ==================================\n"
		"Type constant                                           Property name                                       Data type  Component names\n"
		"======================================================= =================================================== ========== ==================================\n"
		"``Particles.Type.ParticleType``                         :guilabel:`Particle Type`                           int        \n"
		"``Particles.Type.Position``                             :guilabel:`Position`                                float      X, Y, Z\n"
		"``Particles.Type.Selection``                            :guilabel:`Selection`                               int        \n"
		"``Particles.Type.Color``                                :guilabel:`Color`                                   float      R, G, B\n"
		"``Particles.Type.Displacement``                         :guilabel:`Displacement`                            float      X, Y, Z\n"
		"``Particles.Type.DisplacementMagnitude``                :guilabel:`Displacement Magnitude`                  float      \n"
		"``Particles.Type.PotentialEnergy``                      :guilabel:`Potential Energy`                        float      \n"
		"``Particles.Type.KineticEnergy``                        :guilabel:`Kinetic Energy`                          float      \n"
		"``Particles.Type.TotalEnergy``                          :guilabel:`Total Energy`                            float      \n"
		"``Particles.Type.Velocity``                             :guilabel:`Velocity`                                float      X, Y, Z\n"
		"``Particles.Type.Radius``                               :guilabel:`Radius`                                  float      \n"
		"``Particles.Type.Cluster``                              :guilabel:`Cluster`                                 int64      \n"
		"``Particles.Type.Coordination``                         :guilabel:`Coordination`                            int        \n"
		"``Particles.Type.StructureType``                        :guilabel:`Structure Type`                          int        \n"
		"``Particles.Type.Identifier``                           :guilabel:`Particle Identifier`                     int64      \n"
		"``Particles.Type.StressTensor``                         :guilabel:`Stress Tensor`                           float      XX, YY, ZZ, XY, XZ, YZ\n"
		"``Particles.Type.StrainTensor``                         :guilabel:`Strain Tensor`                           float      XX, YY, ZZ, XY, XZ, YZ\n"
		"``Particles.Type.DeformationGradient``                  :guilabel:`Deformation Gradient`                    float      XX, YX, ZX, XY, YY, ZY, XZ, YZ, ZZ\n"
		"``Particles.Type.Orientation``                          :guilabel:`Orientation`                             float      X, Y, Z, W\n"
		"``Particles.Type.Force``                                :guilabel:`Force`                                   float      X, Y, Z\n"
		"``Particles.Type.Mass``                                 :guilabel:`Mass`                                    float      \n"
		"``Particles.Type.Charge``                               :guilabel:`Charge`                                  float      \n"
		"``Particles.Type.PeriodicImage``                        :guilabel:`Periodic Image`                          int        X, Y, Z\n"
		"``Particles.Type.Transparency``                         :guilabel:`Transparency`                            float      \n"
		"``Particles.Type.DipoleOrientation``                    :guilabel:`Dipole Orientation`                      float      X, Y, Z\n"
		"``Particles.Type.DipoleMagnitude``                      :guilabel:`Dipole Magnitude`                        float      \n"
		"``Particles.Type.AngularVelocity``                      :guilabel:`Angular Velocity`                        float      X, Y, Z\n"
		"``Particles.Type.AngularMomentum``                      :guilabel:`Angular Momentum`                        float      X, Y, Z\n"
		"``Particles.Type.Torque``                               :guilabel:`Torque`                                  float      X, Y, Z\n"
		"``Particles.Type.Spin``                                 :guilabel:`Spin`                                    float      \n"
		"``Particles.Type.CentroSymmetry``                       :guilabel:`Centrosymmetry`                          float      \n"
		"``Particles.Type.VelocityMagnitude``                    :guilabel:`Velocity Magnitude`                      float      \n"
		"``Particles.Type.Molecule``                             :guilabel:`Molecule Identifier`                     int64      \n"
		"``Particles.Type.AsphericalShape``                      :guilabel:`Aspherical Shape`                        float      X, Y, Z\n"
		"``Particles.Type.VectorColor``                          :guilabel:`Vector Color`                            float      R, G, B\n"
		"``Particles.Type.ElasticStrainTensor``                  :guilabel:`Elastic Strain`                          float      XX, YY, ZZ, XY, XZ, YZ\n"
		"``Particles.Type.ElasticDeformationGradient``           :guilabel:`Elastic Deformation Gradient`            float      XX, YX, ZX, XY, YY, ZY, XZ, YZ, ZZ\n"
		"``Particles.Type.Rotation``                             :guilabel:`Rotation`                                float      X, Y, Z, W\n"
		"``Particles.Type.StretchTensor``                        :guilabel:`Stretch Tensor`                          float      XX, YY, ZZ, XY, XZ, YZ\n"
		"``Particles.Type.MoleculeType``                         :guilabel:`Molecule Type`                           int        \n"
		"``Particles.Type.User``                                 user-defined properties with non-standard names     int/float  \n"
		"======================================================= =================================================== ========== ==================================\n"
		
		// Python class name:
		,"Particles")

		// For backward compatibility with OVITO 2.9.0:
		.def_property_readonly("position", [](const ParticlesObject& particles) { return particles.getProperty(ParticlesObject::PositionProperty); })
		.def_property_readonly("color", [](const ParticlesObject& particles) { return particles.getProperty(ParticlesObject::ColorProperty); })
		.def_property_readonly("particle_type", [](const ParticlesObject& particles) { return particles.getProperty(ParticlesObject::TypeProperty); })
		.def_property_readonly("displacement", [](const ParticlesObject& particles) { return particles.getProperty(ParticlesObject::DisplacementProperty); })
		.def_property_readonly("structure_type", [](const ParticlesObject& particles) { return particles.getProperty(ParticlesObject::StructureTypeProperty); })
		.def_property_readonly("centrosymmetry", [](const ParticlesObject& particles) { return particles.getProperty(ParticlesObject::CentroSymmetryProperty); })
		.def_property_readonly("cluster", [](const ParticlesObject& particles) { return particles.getProperty(ParticlesObject::ClusterProperty); })
		.def_property_readonly("selection", [](const ParticlesObject& particles) { return particles.getProperty(ParticlesObject::SelectionProperty); })
		.def_property_readonly("coordination", [](const ParticlesObject& particles) { return particles.getProperty(ParticlesObject::CoordinationProperty); })
	;
	createDataSubobjectAccessors(Particles_py, "bonds", &ParticlesObject::bonds, &ParticlesObject::setBonds, 
		"The :py:class:`Bonds` data object, which stores the bond information associated with this particle dataset. ");

	py::enum_<ParticlesObject::Type>(Particles_py, "Type")
		.value("User", ParticlesObject::UserProperty)
		.value("ParticleType", ParticlesObject::TypeProperty)
		.value("Position", ParticlesObject::PositionProperty)
		.value("Selection", ParticlesObject::SelectionProperty)
		.value("Color", ParticlesObject::ColorProperty)
		.value("Displacement", ParticlesObject::DisplacementProperty)
		.value("DisplacementMagnitude", ParticlesObject::DisplacementMagnitudeProperty)
		.value("PotentialEnergy", ParticlesObject::PotentialEnergyProperty)
		.value("KineticEnergy", ParticlesObject::KineticEnergyProperty)
		.value("TotalEnergy", ParticlesObject::TotalEnergyProperty)
		.value("Velocity", ParticlesObject::VelocityProperty)
		.value("Radius", ParticlesObject::RadiusProperty)
		.value("Cluster", ParticlesObject::ClusterProperty)
		.value("Coordination", ParticlesObject::CoordinationProperty)
		.value("StructureType", ParticlesObject::StructureTypeProperty)
		.value("Identifier", ParticlesObject::IdentifierProperty)
		.value("StressTensor", ParticlesObject::StressTensorProperty)
		.value("StrainTensor", ParticlesObject::StrainTensorProperty)
		.value("DeformationGradient", ParticlesObject::DeformationGradientProperty)
		.value("Orientation", ParticlesObject::OrientationProperty)
		.value("Force", ParticlesObject::ForceProperty)
		.value("Mass", ParticlesObject::MassProperty)
		.value("Charge", ParticlesObject::ChargeProperty)
		.value("PeriodicImage", ParticlesObject::PeriodicImageProperty)
		.value("Transparency", ParticlesObject::TransparencyProperty)
		.value("DipoleOrientation", ParticlesObject::DipoleOrientationProperty)
		.value("DipoleMagnitude", ParticlesObject::DipoleMagnitudeProperty)
		.value("AngularVelocity", ParticlesObject::AngularVelocityProperty)
		.value("AngularMomentum", ParticlesObject::AngularMomentumProperty)
		.value("Torque", ParticlesObject::TorqueProperty)
		.value("Spin", ParticlesObject::SpinProperty)
		.value("CentroSymmetry", ParticlesObject::CentroSymmetryProperty)
		.value("VelocityMagnitude", ParticlesObject::VelocityMagnitudeProperty)
		.value("Molecule", ParticlesObject::MoleculeProperty)
		.value("AsphericalShape", ParticlesObject::AsphericalShapeProperty)
		.value("VectorColor", ParticlesObject::VectorColorProperty)
		.value("ElasticStrainTensor", ParticlesObject::ElasticStrainTensorProperty)
		.value("ElasticDeformationGradient", ParticlesObject::ElasticDeformationGradientProperty)
		.value("Rotation", ParticlesObject::RotationProperty)
		.value("StretchTensor", ParticlesObject::StretchTensorProperty)
		.value("MoleculeType", ParticlesObject::MoleculeTypeProperty)
	;		

	auto Bonds_py = ovito_class<BondsObject, PropertyContainer>(m,
		":Base class: :py:class:`ovito.data.PropertyContainer`"
		"\n\n"
		".. _bond-types-list:"
		"\n\n"
		"The following standard properties are defined for bonds:"
		"\n\n"
		"======================================================= =================================================== ========== ==================================\n"
		"Type constant                                           Property name                                       Data type  Component names\n"
		"======================================================= =================================================== ========== ==================================\n"
		"``Bonds.Type.BondType``                                 :guilabel:`Bond Type`                               int         \n"
		"``Bonds.Type.Selection``                                :guilabel:`Selection`                               int         \n"
		"``Bonds.Type.Color``                                    :guilabel:`Color`                                   float      R, G, B\n"
		"``Bonds.Type.Length``                                   :guilabel:`Length`                                  float       \n"
		"``Bonds.Type.Topology``                                 :guilabel:`Topology`                                int64      A, B\n"
		"``Bonds.Type.PeriodicImage``                            :guilabel:`Periodic Image`                          int        X, Y, Z \n"
		"``Bonds.Type.Transparency``                             :guilabel:`Transparency`                            float       \n"
		"``Bonds.Type.User``                                     user-defined properties with non-standard names     int/float   \n"
		"======================================================= =================================================== ========== ==================================\n"
		// Python class name:
		,"Bonds")

		// For backward compatibility with OVITO 2.9.0:
		.def_property_readonly("pbc_vectors", [](const BondsObject& bonds) { return bonds.getProperty(BondsObject::PeriodicImageProperty); })
	;

	py::enum_<BondsObject::Type>(Bonds_py, "Type")
		.value("User", BondsObject::UserProperty)
		.value("BondType", BondsObject::TypeProperty)
		.value("Selection", BondsObject::SelectionProperty)
		.value("Color", BondsObject::ColorProperty)
		.value("Length", BondsObject::LengthProperty)
		.value("Topology", BondsObject::TopologyProperty)
		.value("PeriodicImage", BondsObject::PeriodicImageProperty)
		.value("Transparency", BondsObject::TransparencyProperty)
	;

#if 0
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
	;
						
#endif

	py::class_<ParticleBondMap>(m, "BondsEnumerator",
		"Utility class that permits efficient iteration over the bonds connected to specific particles. "
		"\n\n"
    	"The constructor takes a :py:class:`Bonds` object as input. "
		"From the generally unordered list of bonds, the :py:class:`!BondsEnumerator` will build a lookup table for quick enumeration  "
		"of bonds of particular particles. "
		"\n\n"
		"All bonds connected to a specific particle can be subsequently visited using the :py:meth:`.bonds_of_particle` method. "
		"\n\n"
		"Warning: Do not modify the underlying :py:class:`Bonds` object while the :py:class:`!BondsEnumerator` is in use. "
		"Adding or deleting bonds would render the internal lookup table of the :py:class:`!BondsEnumerator` invalid. "
		"\n\n"
		"**Usage example**"
		"\n\n"
		".. literalinclude:: ../example_snippets/bonds_enumerator.py\n")
		.def(py::init<const BondsObject&>(), py::arg("bonds"))
		.def("bonds_of_particle", [](const ParticleBondMap& bondMap, size_t particleIndex) {
			auto range = bondMap.bondIndicesOfParticle(particleIndex);
			return py::make_iterator<py::return_value_policy::automatic>(range.begin(), range.end());
		},
		py::keep_alive<0, 1>(),
		"Returns an iterator that yields the indices of the bonds connected to the given particle. "
        "The indices can be used to index into the :py:class:`~ovito.data.Property` arrays of the :py:class:`Bonds` object. ");
	;

	auto ParticleType_py = ovito_class<ParticleType, ElementType>(m,
			":Base class: :py:class:`ovito.data.ElementType`"
			"\n\n"
			"Represents a particle type or atom type. :py:class:`!ParticleType` instances are typically part of a typed :py:class:`ParticleProperty`, "
			"but this class is also used in other contexts, for example to define the list of structural types identified by the :py:class:`~ovito.modifiers.PolyhedralTemplateMatchingModifier`. ")
	;
	createDataPropertyAccessors(ParticleType_py, "radius", &ParticleType::radius, &ParticleType::setRadius,
			"The display radius to use for particles of this type.");

	auto ParticlesVis_py = ovito_class<ParticlesVis, DataVis>(m,
			":Base class: :py:class:`ovito.vis.DataVis`\n\n"
			"This element controls the visual appearance of particles. "
			"See also the corresponding `user manual page <../../display_objects.particles.html>`__ for this visual element. "
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
			"Controls the visual appearance of vector arrow elements. "
			"\n\n"
			"A :py:class:`!VectorVis` element is typically attached to a :py:class:`~ovito.data.ParticleProperty` that represents a three-dimensional "
			"vector quantity, for example the ``Displacement`` property or the ``Force`` property. "
			"See also the corresponding `user manual page <../../display_objects.vectors.html>`__ for this visual element. "
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
			"See also the corresponding `user manual page <../../display_objects.bonds.html>`__ for this visual element. "
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
		.def("prepare", [](CutoffNeighborFinder& finder, FloatType cutoff, const PropertyObject& positions, const SimulationCellObject& cell) {
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
		.def(py::init<int>())
		.def("prepare", [](NearestNeighborFinder& finder, const PropertyObject& positions, const SimulationCellObject& cell) {
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
		.def("findNeighbors", py::overload_cast<size_t>(&NearestNeighborQuery::findNeighbors))
		.def("findNeighborsAtLocation", py::overload_cast<const Point3&, bool>(&NearestNeighborQuery::findNeighbors))
		.def_property_readonly("count", [](const NearestNeighborQuery& q) -> int { return q.results().size(); })
		.def("__getitem__", [](const NearestNeighborQuery& q, int index) -> const NearestNeighborFinder::Neighbor& { return q.results()[index]; },
			py::return_value_policy::reference_internal)
	;

#if 0
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
		
	;
#endif

	ovito_class<BondType, ElementType>{m,
			":Base class: :py:class:`ovito.data.ElementType`"
			"\n\n"
			"Describes a bond type."}
	;

	ovito_class<TrajectoryObject, DataObject>(m,
			":Base class: :py:class:`ovito.data.DataObject`"
			"\n\n"
			"This data object type stores the traced trajectory lines of a group of particles. "
			"It is typically generated by a :py:class:`~ovito.modifiers.GenerateTrajectoryLinesModifier`. "
			"\n\n"
			"Each :py:class:`!TrajectoryLines` object is associated with a :py:class:`~ovito.vis.TrajectoryVis` "
			"element, which controls the visual appearance of the trajectory lines in rendered images. "
			"It is accessible through the :py:attr:`~DataObject.vis` base class attribute. "
			,
			// Python class name:
			"TrajectoryLines")
	;

	ovito_class<TrajectoryVis, DataVis>(m,
			":Base class: :py:class:`ovito.vis.DataVis`"
			"\n\n"
			"Controls the visual appearance of particle trajectory lines. An instance of this class is attached "
			"to every :py:class:`~ovito.data.TrajectoryLines` data object.")
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
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(Particles);

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
