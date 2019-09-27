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

#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticleType.h>
#include <ovito/particles/objects/ParticlesVis.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/VectorVis.h>
#include <ovito/particles/objects/BondsVis.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/particles/objects/ParticleBondMap.h>
#include <ovito/particles/objects/BondType.h>
#include <ovito/particles/objects/TrajectoryObject.h>
#include <ovito/particles/objects/TrajectoryVis.h>
#include <ovito/particles/util/CutoffNeighborFinder.h>
#include <ovito/particles/util/NearestNeighborFinder.h>
#include <ovito/stdobj/properties/PropertyStorage.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/scripting/PythonBinding.h>
#include <ovito/pyscript/binding/PythonBinding.h>
#include <ovito/pyscript/engine/ScriptEngine.h>
#include <ovito/core/utilities/io/CompressedTextWriter.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/app/PluginManager.h>
#include "PythonBinding.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace PyScript;

void defineModifiersSubmodule(py::module parentModule);	// Defined in ModifierBinding.cpp
void defineImportersSubmodule(py::module parentModule);	// Defined in ImporterBinding.cpp
void defineExportersSubmodule(py::module parentModule);	// Defined in ExporterBinding.cpp

PYBIND11_MODULE(ParticlesPython, m)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();

	py::options options;
	options.disable_function_signatures();

	auto Particles_py = ovito_class<ParticlesObject, PropertyContainer>(m,
		":Base class: :py:class:`ovito.data.PropertyContainer`"
		"\n\n"
		"This container object stores the information associated with a system of particles. "
		"It is typically accessed through the :py:attr:`DataCollection.particles` "
		"field of a data collection. "
		"The current number of particles is given by the :py:attr:`~PropertyContainer.count` attribute "
		"that is inherited from the :py:class:`~ovito.data.PropertyContainer` base class. "
		"The particles may be associated with a set of properties. Each property is represented by a "
		":py:class:`Property` data object, that is stored in this property container and is basically an "
		"array of numeric values of length *N*, where *N* is the number of particles in the system. "
		"Each property array has a unique name, by which it can be looked up through the dictionary interface of the "
		":py:class:`~ovito.data.PropertyContainer` base class. "
		"While the user is free to define arbitrary particle properties, OVITO predefines a set of *standard properties* "
		"that each have a fixed data layout, meaning and name. They are listed in the table below. "
		"\n\n"
		".. _particle-types-list:"
		"\n\n"
		"=================================================== ========== ==================================\n"
		"Standard property name                              Data type  Component names\n"
		"=================================================== ========== ==================================\n"
		":guilabel:`Particle Type`                           int        \n"
		":guilabel:`Position`                                float      X, Y, Z\n"
		":guilabel:`Selection`                               int        \n"
		":guilabel:`Color`                                   float      R, G, B\n"
		":guilabel:`Displacement`                            float      X, Y, Z\n"
		":guilabel:`Displacement Magnitude`                  float      \n"
		":guilabel:`Potential Energy`                        float      \n"
		":guilabel:`Kinetic Energy`                          float      \n"
		":guilabel:`Total Energy`                            float      \n"
		":guilabel:`Velocity`                                float      X, Y, Z\n"
		":guilabel:`Radius`                                  float      \n"
		":guilabel:`Cluster`                                 int64      \n"
		":guilabel:`Coordination`                            int        \n"
		":guilabel:`Structure Type`                          int        \n"
		":guilabel:`Particle Identifier`                     int64      \n"
		":guilabel:`Stress Tensor`                           float      XX, YY, ZZ, XY, XZ, YZ\n"
		":guilabel:`Strain Tensor`                           float      XX, YY, ZZ, XY, XZ, YZ\n"
		":guilabel:`Deformation Gradient`                    float      XX, YX, ZX, XY, YY, ZY, XZ, YZ, ZZ\n"
		":guilabel:`Orientation`                             float      X, Y, Z, W\n"
		":guilabel:`Force`                                   float      X, Y, Z\n"
		":guilabel:`Mass`                                    float      \n"
		":guilabel:`Charge`                                  float      \n"
		":guilabel:`Periodic Image`                          int        X, Y, Z\n"
		":guilabel:`Transparency`                            float      \n"
		":guilabel:`Dipole Orientation`                      float      X, Y, Z\n"
		":guilabel:`Dipole Magnitude`                        float      \n"
		":guilabel:`Angular Velocity`                        float      X, Y, Z\n"
		":guilabel:`Angular Momentum`                        float      X, Y, Z\n"
		":guilabel:`Torque`                                  float      X, Y, Z\n"
		":guilabel:`Spin`                                    float      \n"
		":guilabel:`Centrosymmetry`                          float      \n"
		":guilabel:`Velocity Magnitude`                      float      \n"
		":guilabel:`Molecule Identifier`                     int64      \n"
		":guilabel:`Aspherical Shape`                        float      X, Y, Z\n"
		":guilabel:`Vector Color`                            float      R, G, B\n"
		":guilabel:`Elastic Strain`                          float      XX, YY, ZZ, XY, XZ, YZ\n"
		":guilabel:`Elastic Deformation Gradient`            float      XX, YX, ZX, XY, YY, ZY, XZ, YZ, ZZ\n"
		":guilabel:`Rotation`                                float      X, Y, Z, W\n"
		":guilabel:`Stretch Tensor`                          float      XX, YY, ZZ, XY, XZ, YZ\n"
		":guilabel:`Molecule Type`                           int        \n"
		"=================================================== ========== ==================================\n"

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
    	"This class is a container for a set of bond :py:class:`Property` objects and typically "
		"part of a :py:class:`Particles` data object (see :py:attr:`~Particles.bonds` field): "
		"\n\n"
		".. literalinclude:: ../example_snippets/bonds_data_object.py\n"
        "  :lines: 9-10\n"
		"\n\n"
	    "The class inherits the :py:attr:`~PropertyContainer.count` attribute from its :py:class:`PropertyContainer` base class. This attribute reports the number of bonds. "
		"\n\n"
    	"**Bond properties**"
		"\n\n"
    	"Bonds can be associated with arbitrary *bond properties*, which are stored in the :py:class:`!Bonds` container "
		"as a set of :py:class:`Property` data arrays. Each bond property has a unique name by which it can be looked up: "
		"\n\n"
		".. literalinclude:: ../example_snippets/bonds_data_object.py\n"
		"  :lines: 15-19\n"
		"\n\n"
		"New bond properties can be added using the :py:meth:`PropertyContainer.create_property` base class method. "
		"\n\n"
		"**Bond topology**"
		"\n\n"
		"The ``Topology`` bond property, which is always present, "
		"defines the connectivity between particles in the form of a *N* x 2 array of indices into the :py:class:`Particles` array. "
		"In other words, each bond is defined by a pair of particle indices. "
		"\n\n"
		".. literalinclude:: ../example_snippets/bonds_data_object.py\n"
		"  :lines: 23-24\n"
		"\n\n"
		"Note that the bonds of a system are not stored in any particular order in the :py:class:`!Bonds` container. "
		"If you need to enumerate all bonds connected to a certain particle, you can use the :py:class:`BondsEnumerator` utility class for that. "
		"\n\n"
		"**Bond display settings**"
		"\n\n"
		"The :py:class:`!Bonds` data object has a :py:class:`~ovito.vis.BondsVis` element attached to it, "
		"which controls the visual appearance of the bonds in rendered images. It can be accessed through the :py:attr:`~DataObject.vis` "
		"attribute inherited from the :py:class:`DataObject` base class: "
		"\n\n"
    	".. literalinclude:: ../example_snippets/bonds_data_object.py\n"
        "  :lines: 30-32\n"
		"\n\n"
		"**Computing bond vectors**"
		"\n\n"
		"Since each bond is defined by two indices into the particles array, we can use this to determine the corresponding spatial "
		"bond *vectors*. They can be computed from the positions of the particles: "
		"\n\n"
		".. literalinclude:: ../example_snippets/bonds_data_object.py\n"
        "  :lines: 37-39\n"
		"\n\n"
		"Here, the first and the second column of the bonds topology array are used to index into the particle positions array. "
		"The subtraction of the two indexed arrays yields the list of bond vectors. Each vector in this list points "
		"from the first particle to the second particle of the corresponding bond. "
		"\n\n"
    	"Finally, we may have to correct for the effect of periodic boundary conditions when a bond "
		"connects two particles on opposite sides of the box. OVITO keeps track of such cases by means of the "
		"the special ``Periodic Image`` bond property. It stores a shift vector for each bond, specifying the directions in which the bond "
		"crosses periodic boundaries. We make use of this information to correct the bond vectors computed above. "
		"This is done by adding the product of the cell matrix and the shift vectors from the ``Periodic Image`` bond property: "
		"\n\n"
		".. literalinclude:: ../example_snippets/bonds_data_object.py\n"
        "  :lines: 43-43\n"
		"\n\n"
		"The shift vectors array is transposed here to facilitate the transformation "
    	"of the entire array of vectors with a single 3x3 cell matrix. "
		"To summarize: In the two code snippets above we have performed "
		"the following calculation for every bond (*a*, *b*) in parallel: "
		"\n\n"
		"   v = x(b) - x(a) + dot(H, pbc)"
		"\n\n"
		"where *H* is the cell matrix and *pbc* is the bond's PBC shift vector of the form (n\\ :sub:`x`, n\\ :sub:`y`, n\\ :sub:`z`). "
		"\n\n"
		".. _bond-types-list:"
		"\n\n"
		"**Standard bond properties**"
		"\n\n"
		"The following standard properties are defined for bonds:"
		"\n\n"
		"=================================================== ========== ==================================\n"
		"Property name                                       Data type  Component names\n"
		"=================================================== ========== ==================================\n"
		":guilabel:`Bond Type`                               int         \n"
		":guilabel:`Selection`                               int         \n"
		":guilabel:`Color`                                   float      R, G, B\n"
		":guilabel:`Length`                                  float       \n"
		":guilabel:`Topology`                                int64      A, B\n"
		":guilabel:`Periodic Image`                          int        X, Y, Z \n"
		":guilabel:`Transparency`                            float       \n"
		"=================================================== ========== ==================================\n"
		// Python class name:
		,"Bonds")
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
			"Represents a particle type or atom type. :py:class:`!ParticleType` instances are typically part of a typed :py:class:`Property`, "
			"but this class is also used in other contexts, for example to define the list of structural types identified by the :py:class:`~ovito.modifiers.PolyhedralTemplateMatchingModifier`. ")
		.def("load_shape", [](ParticleType& ptype, const QString& filepath) {
				ensureDataObjectIsMutable(ptype);
				if(!ptype.loadShapeMesh(filepath, ScriptEngine::currentTask()->createSubTask()))
					ptype.throwException(ParticleType::tr("Loading of the user-defined shape has been canceled by the user."));
			}, py::arg("filepath"),
				"load_shape(filepath)"
				"\n\n"
				"Assigns a user-defined shape to the particle type. Particles of this type will subsequently be rendered "
				"using the polyhedral mesh loaded from the given file. The method will automatically detect the format of the geometry file "
				"and supports standard file formats such as OBJ, STL and VTK that contain triangle meshes, "
				"see the table found :ovitoman:`here <../../usage.import#usage.import.formats>`. "
				"\n\n"
				"The shape loaded from the geometry file will be scaled with the :py:attr:`.radius` value set for this particle type "
				"or the per-particle value stored in the ``Radius`` :ref:`particle property <particle-types-list>` if present. "
				"The shape of each particle will be rendered such that its origin is located at the coordinates of the particle. "
				"\n\n"
				"The following example script demonstrates how to load a user-defined shape for the first particle type (index 0) loaded from "
				"a LAMMPS dump file, which can be accessed through the :py:attr:`Property.types <ovito.data.Property.types>` list "
				"of the ``Particle Type`` :ref:`particle property <particle-types-list>`. "
				"\n\n"
				".. literalinclude:: ../example_snippets/particle_type_load_shape.py\n"
				"  :lines: 4-\n"
				"\n\n")
	;
	createDataPropertyAccessors(ParticleType_py, "radius", &ParticleType::radius, &ParticleType::setRadius,
			"This property controls the display size of the particles of this type. "
			"\n\n"
			"When set to zero, particles of this type will be rendered using the standard size specified "
			"by the :py:attr:`ParticlesVis.radius <ovito.vis.ParticlesVis.radius>` parameter. "
			"Furthermore, precedence is given to any per-particle sizes assigned to the ``Radius`` :ref:`particle property <particle-types-list>` if that property "
			"has been defined. "
			"\n\n"
			":Default: ``0.0``\n"
			"\n\n"
			"The following example script demonstrates how to set the display radii of two particle types loaded from "
			"a simulation file, which can be accessed through the :py:attr:`Property.types <ovito.data.Property.types>` list "
			"of the ``Particle Type`` :ref:`particle property <particle-types-list>`. "
			"\n\n"
			".. literalinclude:: ../example_snippets/particle_type_radius.py\n"
			"  :lines: 4-\n"
			"\n\n");
	createDataPropertyAccessors(ParticleType_py, "mass", &ParticleType::mass, &ParticleType::setMass,
			"The mass of this particle type. "
			"\n\n"
			":Default: ``0.0``\n"
			"\n\n");
	createDataPropertyAccessors(ParticleType_py, "highlight_edges", &ParticleType::highlightShapeEdges, &ParticleType::setHighlightShapeEdges,
			"Activates the highlighting of the polygonal edges of the user-defined particle shape during rendering. "
			"This option only has an effect if a user-defined shape has been assigned to the particle type using the :py:meth:`.load_shape` method. "
			"\n\n"
			":Default: ``False``\n");
	createDataPropertyAccessors(ParticleType_py, "backface_culling", &ParticleType::shapeBackfaceCullingEnabled, &ParticleType::setShapeBackfaceCullingEnabled,
			"Activates back-face culling for the user-defined particle shape mesh "
			"to speed up rendering. If turned on, polygonal sides of the shape mesh facing away from the viewer will not be rendered. "
			"You can turn this option off if the particle's shape is not closed and two-sided rendering is required. "
			"This option only has an effect if a user-defined shape has been assigned to the particle type using the :py:meth:`.load_shape` method. "
			"\n\n"
			":Default: ``True``\n");
	createDataSubobjectAccessors(ParticleType_py, "shape", &ParticleType::shapeMesh, &ParticleType::setShapeMesh);

	auto ParticlesVis_py = ovito_class<ParticlesVis, DataVis>(m,
			":Base class: :py:class:`ovito.vis.DataVis`"
			"\n\n"
			"This type of visual element is responsible for rendering particles and is attached to every :py:class:`~ovito.data.Particles` data object. "
			"You can access the element through the :py:attr:`~ovito.data.DataObject.vis` field of the data object and adjust its parameters to control the visual "
			"appearance of particles in rendered images:  "
			"\n\n"
			".. literalinclude:: ../example_snippets/particles_vis.py\n"
			"\n\n"
			"See also the corresponding :ovitoman:`user manual page <../../display_objects.particles>` for more information on this visual element. ")
		.def_property("radius", &ParticlesVis::defaultParticleRadius, &ParticlesVis::setDefaultParticleRadius,
				"The standard display radius of particles. "
				"This value is only used if no per-particle or per-type radii have been set. "
				"A per-type radius can be set via :py:attr:`ParticleType.radius <ovito.data.ParticleType.radius>`. "
				"An individual display radius can be assigned to each particle by setting the ``Radius`` "
				":ref:`particle property <particle-types-list>`, e.g. using the :py:class:`~ovito.modifiers.ComputePropertyModifier`. "
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
			":Base class: :py:class:`ovito.vis.DataVis`"
			"\n\n"
			"This type of visual element is responsible for rendering arrows to visualize per-particle vector quantities. "
			"An instance of this class is typically attached to a :py:class:`~ovito.data.Property` data object that represents a vectorial quantity, e.g. the ``Force`` and the ``Displacement`` particle properties. "
			"See also the corresponding :ovitoman:`user manual page <../../display_objects.vectors>` for a description of this visual element. "
			"\n\n"
			"The parameters of the vector visual element let you control the visual appearance of the arrows in rendered images. "
			"For the standard particle properties ``Force``, ``Dipole`` and ``Displacement``, OVITO automatically "
			"creates and attaches a :py:class:`!VectorVis` element to these properties and you can access it through their :py:attr:`~ovito.data.DataObject.vis` field: "
			"\n\n"
			".. literalinclude:: ../example_snippets/vector_vis.py\n"
			"   :lines: 6-10\n"
			"\n\n"
			"In the example above, the ``Force`` particle property was loaded from the input simulation file, "
			"and the code accesses the corresponding :py:class:`~ovito.data.Property` data object in the source data collection of the pipeline. "
			"\n\n"
			"Some modifiers dynamically generate new vector particle properties. For instance, the :py:class:`~ovito.modifiers.CalculateDisplacementsModifier` "
			"generates the ``Displacement`` property and will automatically attach a new :py:class:`!VectorVis` element to it. "
			"In this case, the visual element is managed by the modifier and may be configured as needed: "
			"\n\n"
			".. literalinclude:: ../example_snippets/vector_vis.py\n"
			"   :lines: 15-18\n"
			"\n\n"
			"If you write your :ref:`own modifier function <writing_custom_modifiers>` in Python for computing a vector particle property, and you want to visualize these vectors "
			"as arrows, you need to create the :py:class:`!VectorVis` element programmatically and attached it to the :py:class:`~ovito.data.Property` generated "
			"by your user-defined modifier function. For example: "
			"\n\n"
			".. literalinclude:: ../example_snippets/vector_vis.py\n"
			"   :lines: 23-34\n")
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
			"A visualization element that renders cylindrical bonds between particles. "
			"An instance of this class is attached to every :py:class:`~ovito.data.Bonds` data object "
			"and controls the visual appearance of the bonds in rendered images. "
			"\n\n"
			"See also the corresponding :ovitoman:`user manual page <../../display_objects.bonds>` for this visual element. "
			"If you import a simulation file containing bonds, you can subsequently access the :py:class:`!BondsVis` element "
			"through the :py:attr:`~ovito.data.DataObject.vis` field of the bonds data object, which is part in the data collection managed "
			"by the pipeline's :py:attr:`~ovito.pipeline.Pipeline.source` object:"
			"\n\n"
			".. literalinclude:: ../example_snippets/bonds_vis.py\n"
			"   :lines: 6-9\n"
			"\n\n"
			"In cases where the :py:class:`~ovito.data.Bonds` data is dynamically generated by a modifier, e.g. the :py:class:`~ovito.modifiers.CreateBondsModifier`, "
			"the :py:class:`!BondsVis` element is managed by the modifier:"
			"\n\n"
			".. literalinclude:: ../example_snippets/bonds_vis.py\n"
			"   :lines: 13-15\n")
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

	ovito_class<BondType, ElementType>{m,
			":Base class: :py:class:`ovito.data.ElementType`"
			"\n\n"
			"Describes a bond type."}
	;

	ovito_class<TrajectoryObject, PropertyContainer>(m,
			":Base class: :py:class:`ovito.data.PropertyContainer`"
			"\n\n"
			"Data object that stores the trajectory lines of a set of particles, "
			"which have been traced by the :py:class:`~ovito.modifiers.GenerateTrajectoryLinesModifier`. "
			"It is typically part of a pipeline's output data collection, "
			"from where it can be accessed via the :py:attr:`DataCollection.trajectories <ovito.data.DataCollection.trajectories>` field. "
			"\n\n"
			"A :py:class:`!TrajectoryLines` object has an associated :py:class:`~ovito.vis.TrajectoryVis` "
			"element, which controls the visual appearance of the trajectory lines in rendered images. "
			"This visual element is accessible through the :py:attr:`~DataObject.vis` attribute of the base class. "
			,
			// Python class name:
			"TrajectoryLines")
	;

	ovito_class<TrajectoryVis, DataVis>(m,
			":Base class: :py:class:`ovito.vis.DataVis`"
			"\n\n"
			"Controls the visual appearance of particle trajectory lines. An instance of this class is attached "
			"to every :py:class:`~ovito.data.TrajectoryLines` data object. ")
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
		.def_property("wrapped_lines", &TrajectoryVis::wrappedLines, &TrajectoryVis::setWrappedLines,
				"If ``True``, the continuous trajectory lines will automatically be wrapped back into the simulation box during rendering. "
				"Thus, they will be shown as several discontinuous segments if they cross periodic boundaries of the simulation box. "
				"\n\n"
				":Default: ``False``\n")
	;

	// Register submodules.
	defineModifiersSubmodule(m);	// Defined in ModifierBinding.cpp
	defineImportersSubmodule(m);	// Defined in ImporterBinding.cpp
	defineExportersSubmodule(m);	// Defined in ExporterBinding.cpp
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(ParticlesPython);

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
