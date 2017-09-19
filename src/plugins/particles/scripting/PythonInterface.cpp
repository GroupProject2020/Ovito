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

#include <plugins/particles/Particles.h>
#include <plugins/pyscript/binding/PythonBinding.h>
#include <core/dataset/data/properties/PropertyStorage.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/particles/objects/ParticleType.h>
#include <plugins/particles/objects/ParticleDisplay.h>
#include <plugins/particles/objects/VectorDisplay.h>
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/particles/objects/BondsDisplay.h>
#include <plugins/particles/objects/BondProperty.h>
#include <plugins/particles/objects/BondType.h>
#include <plugins/particles/objects/TrajectoryObject.h>
#include <plugins/particles/objects/TrajectoryGenerator.h>
#include <plugins/particles/objects/TrajectoryDisplay.h>
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include <plugins/particles/util/NearestNeighborFinder.h>
#include <core/utilities/io/CompressedTextWriter.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/app/PluginManager.h>
#include "PythonBinding.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace PyScript;

void defineModifiersSubmodule(py::module parentModule);	// Defined in ModifierBinding.cpp
void defineImportersSubmodule(py::module parentModule);	// Defined in ImporterBinding.cpp
void defineExportersSubmodule(py::module parentModule);	// Defined in ExporterBinding.cpp

py::dict BondsObject__array_interface__(const BondsObject& p)
{
	py::dict ai;
	ai["shape"] = py::make_tuple(p.storage()->size(), 2);
	OVITO_STATIC_ASSERT(sizeof(unsigned int) == 4);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
	ai["typestr"] = py::bytes("<u4");
#else
	ai["typestr"] = py::bytes(">u4");
#endif
	const unsigned int* data;
	if(!p.storage()->empty()) {
		data = &p.storage()->front().index1;
	}
	else {
		static const unsigned int null_data = 0;
		data = &null_data;
	}
	ai["data"] = py::make_tuple(reinterpret_cast<std::intptr_t>(data), true);
	ai["strides"] = py::make_tuple(sizeof(Bond), sizeof(unsigned int));
	ai["version"] = py::cast(3);
	return ai;
}

py::dict BondsObject__pbc_vectors(const BondsObject& p)
{
	py::dict ai;
	ai["shape"] = py::make_tuple(p.storage()->size(), 3);
	OVITO_STATIC_ASSERT(sizeof(int8_t) == 1);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
	ai["typestr"] = py::bytes("<i1");
#else
	ai["typestr"] = py::bytes(">i1");
#endif
	const int8_t* data;
	if(!p.storage()->empty()) {
		data = &p.storage()->front().pbcShift.x();
	}
	else {
		static const int8_t null_data = 0;
		data = &null_data;
	}
	ai["data"] = py::make_tuple(reinterpret_cast<std::intptr_t>(data), true);
	ai["strides"] = py::make_tuple(sizeof(Bond), sizeof(int8_t));
	ai["version"] = py::cast(3);
	return ai;
}

PYBIND11_PLUGIN(Particles)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();
	
	py::options options;
	options.disable_function_signatures();

	py::module m("Particles");

	auto ParticleProperty_py = ovito_abstract_class<ParticleProperty, PropertyObject>(m,
			":Base class: :py:class:`ovito.data.DataObject`\n\n"
			"Stores an array of per-particle values. "
			"\n\n"
			"In OVITO's data model, an arbitrary number of properties can be associated with the particles, "
			"each property being represented by a separate :py:class:`!ParticleProperty` object. A :py:class:`!ParticleProperty` "
			"is basically an array of values whose length matches the number of particles. "
			"\n\n"
			"The set of properties currently associated with all particles is exposed by the "
			":py:attr:`DataCollection.particle_properties` view, which allows accessing them by name "
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
			"**Data access**"
			"\n\n"
			"A :py:class:`!ParticleProperty` object behaves almost like a Numpy array. For example, you can access the property value for the *i*-th particle using indexing:: "
			"\n\n"
			"     vel_property = data.particle_properties['Velocity']\n"
			"     print('Velocity vector of first particle:', vel_property[0])\n"
			"     print('Z-velocity of second particle:', vel_property[1,2])\n"
			"     for v in vel_property: print(v)\n"
			"\n\n"
			"Particle indices start at zero. Properties can be either vectorial (e.g. velocity vectors are stored as an *N* x 3 array) "
			"or scalar (1-d array of length *N*). Length of the first array dimension is in both cases equal to "
			"the number of particles. Array elements can either be of data type ``float`` or ``int``. "
			"\n\n"
			"If necessary, you can cast a :py:class:`!ParticleProperty` to a standard Numpy array:: "
			"\n\n"
			"     velocities = numpy.asarray(vel_property)\n"
			"\n\n"
			"No data is copied during the conversion; the Numpy array will refer to the same memory as the :py:class:`!ParticleProperty`. "
			"By default, the memory of a :py:class:`!ParticleProperty` is write-protected. Thus, trying to modify property values will raise an error:: "
			"\n\n"
			"    vel_property[0] = (0, 0, -4) # \"TypeError: 'ParticleProperty' object does not\n"
			"                                 # support item assignment\"\n"
			"\n\n"
			"A direct modification is prevented by the system, because OVITO's data pipeline uses shallow data copies and needs to know when data objects are being modified. "
			"Only then results that depend on the changing data can be automatically recalculated. "
			"We need to explicitly announce a modification by using the :py:meth:`ParticleProperty.modify` method and a Python ``with`` statement:: "
			"\n\n"
			"    with vel_property.modify() as arr:\n"
			"        arr[0] = (0, 0, -4)\n"
			"\n\n"
			"Within the ``with`` compound statement, the variable ``arr`` refers to a *modifiable* Numpy array, allowing us to alter "
			"the per-particle data stored in the :py:class:`!ParticleProperty` object. "
			"\n\n"
			"**Creating properties**"
			"\n\n"
			"New properties can be created and assigned to particles with the :py:meth:`ParticlePropertiesView.create` factory method. "
			"User-defined modifier functions, for example, use this to output their computation results. "
			"\n\n"
			"**Particle type property**"
			"\n\n"
			"The standard property ``'Particle Type'`` stores the types of particles encoded as integer values, e.g.: "
			"\n\n"
			"    >>> data = node.compute()\n"
			"    >>> tprop = data.particle_properties['Particle Type']\n"
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
		.def_property("name", &ParticleProperty::name, &ParticleProperty::setName,
				"The name of the particle property.")
		.def_property_readonly("components", &ParticleProperty::componentCount,
				"The number of vector components if this is a vector particle property; otherwise 1 (= scalar property).")				
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

	auto BondsObject_py = ovito_class<BondsObject, DataObject>(m,
			":Base class: :py:class:`ovito.data.DataObject`\n\n"
			"Stores the list of bonds between pairs of particles. "
			"\n\n"
			"Typically, bonds are loaded from a simulation data file or are dynamically created by modifiers in a data pipeline. "
			"The following code example demonstrates how to access the bonds produced by a :py:class:`~.ovito.modifiers.CreateBondsModifier`: "
			"\n\n"
			".. literalinclude:: ../example_snippets/bonds_data_object.py\n"
			"   :lines: 1-12\n"
			"\n"
			"The :py:class:`Bonds` object can be used like a Numpy array of shape *N* x 2, where *N* is the number bonds: "
			"\n\n"
			".. literalinclude:: ../example_snippets/bonds_data_object.py\n"
			"   :lines: 14-15\n"
			"\n"
			"Access to the bonds array is read-only. Each bond in the arrray is defined by a pair of 0-based indices into the particles array. "
			"Note that bonds are stored in the array in an arbitrary order. "
			"If you need to enumerate all bonds of a particular particle, you should use the :py:class:`BondsEnumerator` utility class. "
			"\n\n"
			"**Bond properties**"
			"\n\n"
			"Like particles, bonds may be associated with *bond properties*. These properties are stored "
			"in separate data objects of type :py:class:`BondProperty`, which are accessible through the "
			":py:attr:`DataCollection.bond_properties` field of the data collection. "
			"\n\n"
			"**Bond display settings**"
			"\n\n"
			"Every :py:class:`!Bonds` object is associated with a :py:class:`~ovito.vis.BondsDisplay` instance, "
			"which controls the visual appearance of the bonds in rendered images. It can be accessed through the :py:attr:`~DataObject.display` base class attribute: "
			"\n\n"
			".. literalinclude:: ../example_snippets/bonds_data_object.py\n"
			"   :lines: 20-23\n"
			"\n\n"
			"**Computing bond vectors**"
			"\n\n"
			"Note that the :py:class:`!Bonds` class stores only the indices of the particles connected by bonds (the *topology*). "
			"Sometimes it may be necessary to determine the corresponding spatial bond *vectors*. They can be computed "
			"from the current positions of the particles:\n"
			"\n"
			".. literalinclude:: ../example_snippets/bonds_data_object.py\n"
			"   :lines: 25-29\n"
			"\n\n"
			"Here, the first and the second column of the bonds array are used to index into the particle positions array. "
			"The subtraction of the two indexed arrays yields the list of bond vectors. Each vector in this list points "
			"from the first particle to the second particle of the corresponding bond."
			"\n\n"
			"Finally, we must correct for the effect of periodic boundary conditions when bonds "
			"cross the box boundaries. This is achieved by adding the product of the cell matrix and the stored PBC "
			"shift vectors to the bond vectors:\n"
			"\n"
			".. literalinclude:: ../example_snippets/bonds_data_object.py\n"
			"   :lines: 30-\n"
			"\n\n"
			"The PBC vectors array is transposed here to facilitate the transformation "
			"of the entire array of vectors with a single 3x3 cell matrix. In the code snippet we perform "
			"the following calculation for every bond (*a*, *b*) in parallel:"
			"\n\n"
			"   v = x(b) - x(a) + dot(H, pbc)\n"
			"\n\n"
			"where *H* is the cell matrix and *pbc* is the bond's PBC shift vector of the form (n\\ :sub:`x`, n\\ :sub:`y`, n\\ :sub:`z`). "
			"See the :py:attr:`.pbc_vectors` array for its meaning.\n"
			,
			// Python class name:
			"Bonds")
		.def_property_readonly("__array_interface__", &BondsObject__array_interface__)
		.def_property_readonly("_pbc_vectors", &BondsObject__pbc_vectors)
		.def("clear", &BondsObject::clear)
		.def_property_readonly("size", &BondsObject::size)
		.def("__len__", &BondsObject::size)
	;

	py::class_<ParticleBondMap>(m, "BondsEnumerator",
		"Utility class that allows to efficiently iterate over the bonds connected to specific particles. "
		"\n\n"
    	"The constructor takes a :py:class:`Bonds` object as input, which contains an unordered list of all "
    	"bonds in a system. The :py:class:`!BondsEnumerator` will build a lookup table for quick enumeration  "
		"of bonds of particular particles. "
		"\n\n"
		"All bonds connected to a given particle can be visited using the :py:meth:`.bonds_of_particle` method. "
		"\n\n"
		"Warning: Do not modify the :py:class:`Bonds` instance while using the :py:class:`!BondsEnumerator`. "
		"Adding or deleting bonds would render the internal lookup table of the :py:class:`!BondsEnumerator` invalid. "
		"\n\n"
		"**Usage example**"
		"\n\n"
		".. literalinclude:: ../example_snippets/bonds_enumerator.py\n")
		.def("__init__", [](ParticleBondMap& instance, BondsObject& bonds) {
            	new (&instance) ParticleBondMap(*bonds.storage());
			}, py::arg("bonds"), py::keep_alive<1, 2>())
		.def("bonds_of_particle", [](const ParticleBondMap& bondMap, size_t particleIndex) {
			auto range = bondMap.bondIndicesOfParticle(particleIndex);
			return py::make_iterator<py::return_value_policy::automatic>(range.begin(), range.end());
		},
		py::keep_alive<0, 1>(),
		"Returns an iterator that yields the indices of the bonds connected to the given particle. "
        "They can be used to index into the :py:class:`Bonds` array. ");
	;

	ovito_class<ParticleType, ElementType>(m,
			"Represents a particle or atom type. A :py:class:`!ParticleType` instance is always owned by a :py:class:`ParticleTypeProperty`. ")
		.def_property("id", &ParticleType::id, &ParticleType::setId,
				"The identifier of the particle type.")
		.def_property("color", &ParticleType::color, &ParticleType::setColor,
				"The display color to use for particles of this type.")
		.def_property("radius", &ParticleType::radius, &ParticleType::setRadius,
				"The display radius to use for particles of this type.")
		.def_property("name", &ParticleType::name, &ParticleType::setName,
				"The display name of this particle type.")
	;

	auto ParticleDisplay_py = ovito_class<ParticleDisplay, DisplayObject>(m,
			":Base class: :py:class:`ovito.vis.Display`\n\n"
			"This object controls the visual appearance of particles."
			"\n\n"
			"An instance of this class is attached to the ``Position`` :py:class:`~ovito.data.ParticleProperty` "
			"and can be accessed via its :py:attr:`~ovito.data.DataObject.display` property. "
			"\n\n"
			"For example, the following script demonstrates how to change the display shape of particles to a square:"
			"\n\n"
			".. literalinclude:: ../example_snippets/particle_display.py\n")
		.def_property("radius", &ParticleDisplay::defaultParticleRadius, &ParticleDisplay::setDefaultParticleRadius,
				"The standard display radius of particles. "
				"This value is only used if no per-particle or per-type radii have been set. "
				"A per-type radius can be set via :py:attr:`ovito.data.ParticleType.radius`. "
				"An individual display radius can be assigned to particles by creating a ``Radius`` "
				":py:class:`~ovito.data.ParticleProperty`, e.g. using the :py:class:`~ovito.modifiers.ComputePropertyModifier`. "
				"\n\n"
				":Default: 1.2\n")
		.def_property_readonly("default_color", &ParticleDisplay::defaultParticleColor)
		.def_property_readonly("selection_color", &ParticleDisplay::selectionParticleColor)
		.def_property("rendering_quality", &ParticleDisplay::renderingQuality, &ParticleDisplay::setRenderingQuality)
		.def_property("shape", &ParticleDisplay::particleShape, &ParticleDisplay::setParticleShape,
				"The display shape of particles.\n"
				"Possible values are:"
				"\n\n"
				"   * ``ParticleDisplay.Shape.Sphere`` (default) \n"
				"   * ``ParticleDisplay.Shape.Box``\n"
				"   * ``ParticleDisplay.Shape.Circle``\n"
				"   * ``ParticleDisplay.Shape.Square``\n"
				"   * ``ParticleDisplay.Shape.Cylinder``\n"
				"   * ``ParticleDisplay.Shape.Spherocylinder``\n"
				"\n")
		;

	py::enum_<ParticleDisplay::ParticleShape>(ParticleDisplay_py, "Shape")
		.value("Sphere", ParticleDisplay::Sphere)
		.value("Box", ParticleDisplay::Box)
		.value("Circle", ParticleDisplay::Circle)
		.value("Square", ParticleDisplay::Square)
		.value("Cylinder", ParticleDisplay::Cylinder)
		.value("Spherocylinder", ParticleDisplay::Spherocylinder)
	;

	auto VectorDisplay_py = ovito_class<VectorDisplay, DisplayObject>(m,
			":Base class: :py:class:`ovito.vis.Display`\n\n"
			"Controls the visual appearance of vectors (arrows)."
			"\n\n"
			"An instance of this class is attached to particle properties "
			"like for example the ``Displacement`` property, which represent vector quantities. "
			"It can be accessed via the :py:attr:`~ovito.data.DataObject.display` property of the :py:class:`~ovito.data.ParticleProperty` class. "
			"\n\n"
			"For example, the following script demonstrates how to change the display color of force vectors loaded from an input file:"
			"\n\n"
			".. literalinclude:: ../example_snippets/vector_display.py\n")
		.def_property("shading", &VectorDisplay::shadingMode, &VectorDisplay::setShadingMode,
				"The shading style used for the arrows.\n"
				"Possible values:"
				"\n\n"
				"   * ``VectorDisplay.Shading.Normal`` (default) \n"
				"   * ``VectorDisplay.Shading.Flat``\n"
				"\n")
		.def_property("rendering_quality", &VectorDisplay::renderingQuality, &VectorDisplay::setRenderingQuality)
		.def_property("reverse", &VectorDisplay::reverseArrowDirection, &VectorDisplay::setReverseArrowDirection,
				"Boolean flag controlling the reversal of arrow directions."
				"\n\n"
				":Default: ``False``\n")
		.def_property("alignment", &VectorDisplay::arrowPosition, &VectorDisplay::setArrowPosition,
				"Controls the positioning of arrows with respect to the particles.\n"
				"Possible values:"
				"\n\n"
				"   * ``VectorDisplay.Alignment.Base`` (default) \n"
				"   * ``VectorDisplay.Alignment.Center``\n"
				"   * ``VectorDisplay.Alignment.Head``\n"
				"\n")
		.def_property("color", &VectorDisplay::arrowColor, &VectorDisplay::setArrowColor,
				"The display color of arrows."
				"\n\n"
				":Default: ``(1.0, 1.0, 0.0)``\n")
		.def_property("width", &VectorDisplay::arrowWidth, &VectorDisplay::setArrowWidth,
				"Controls the width of arrows (in natural length units)."
				"\n\n"
				":Default: 0.5\n")
		.def_property("scaling", &VectorDisplay::scalingFactor, &VectorDisplay::setScalingFactor,
				"The uniform scaling factor applied to vectors."
				"\n\n"
				":Default: 1.0\n")
	;

	py::enum_<VectorDisplay::ArrowPosition>(VectorDisplay_py, "Alignment")
		.value("Base", VectorDisplay::Base)
		.value("Center", VectorDisplay::Center)
		.value("Head", VectorDisplay::Head)
	;

	ovito_class<BondsDisplay, DisplayObject>(m,
			":Base class: :py:class:`ovito.vis.Display`\n\n"
			"Controls the visual appearance of particle bonds. An instance of this class is attached to every :py:class:`~ovito.data.Bonds` data object.")
		.def_property("width", &BondsDisplay::bondWidth, &BondsDisplay::setBondWidth,
				"The display width of bonds (in natural length units)."
				"\n\n"
				":Default: 0.4\n")
		.def_property("color", &BondsDisplay::bondColor, &BondsDisplay::setBondColor,
				"The display color of bonds. Used only if :py:attr:`.use_particle_colors` == False."
				"\n\n"
				":Default: ``(0.6, 0.6, 0.6)``\n")
		.def_property("shading", &BondsDisplay::shadingMode, &BondsDisplay::setShadingMode,
				"The shading style used for bonds.\n"
				"Possible values:"
				"\n\n"
				"   * ``BondsDisplay.Shading.Normal`` (default) \n"
				"   * ``BondsDisplay.Shading.Flat``\n"
				"\n")
		.def_property("rendering_quality", &BondsDisplay::renderingQuality, &BondsDisplay::setRenderingQuality)
		.def_property("use_particle_colors", &BondsDisplay::useParticleColors, &BondsDisplay::setUseParticleColors,
				"If ``True``, bonds are assigned the same color as the particles they are adjacent to."
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
			":Base class: :py:class:`ovito.data.DataObject`\n\n"
			"Stores an array of per-bond values. "
			"\n\n"
			"In OVITO's data model, an arbitrary set of properties can be associated with bonds, "
			"each property being represented by a :py:class:`!BondProperty` object. A :py:class:`!BondProperty` "
			"is basically an array of values whose length matches the numer of bonds in the data collection (i.e. the length of the :py:class:`Bonds` array). "
			"\n\n"
			":py:class:`!BondProperty` objects have the same fields and behave the same way as :py:class:`ParticleProperty` objects. "
			"Please see the :py:class:`ParticleProperty` documentation on how to access per-bond values. "
			"\n\n"
			"The set of properties currently associated with the bonds is exposed by the "
			":py:attr:`DataCollection.bond_properties` view, which allows accessing them by name "
			"and adding new properties. " 
			"\n\n"
			"Note that the topological definition of bonds, i.e. the connectivity between particles, is stored separately from the bond properties in "
			"the :py:class:`Bonds` data object. ",
			// Python class name:
			"BondProperty")
		// Used by BondPropertiesView.create():
		.def_static("createStandardProperty", [](DataSet& dataset, size_t bondCount, BondProperty::Type type, bool initializeMemory) {
			return BondProperty::createFromStorage(&dataset, BondProperty::createStandardStorage(bondCount, type, initializeMemory));
		})
		.def_static("createUserProperty", [](DataSet& dataset, size_t bondCount, int dataType, size_t componentCount, size_t stride, const QString& name, bool initializeMemory) {
			return BondProperty::createFromStorage(&dataset, std::make_shared<PropertyStorage>(bondCount, dataType, componentCount, stride, name, initializeMemory));
		})
		.def_static("standard_property_type_id", [](const QString& name) { return (BondProperty::Type)BondProperty::OOClass().standardPropertyTypeId(name); })
		.def_property("name", &BondProperty::name, &BondProperty::setName,
				"The name of this bond property.")
		.def_property_readonly("components", &BondProperty::componentCount,
				"The number of vector components if this is a vector bond property; otherwise 1 (= scalar property).")				
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
				"``BondProperty.Type.Color``                             :guilabel:`Color`                                   float     \n"
				"``BondProperty.Type.Length``                            :guilabel:`Length`                                  float     \n"
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
			"This is a data object that stores the traced trajectory lines of a group of particles. "
			"It is typically generated by a :py:class:`~ovito.pipeline.TrajectoryLinegenerator`."
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
			":py:class:`~ovito.vis.TrajectoryLineDisplay` attached to the generated :py:class:`~ovito.data.TrajectoryLines` data object. "
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

	ovito_class<TrajectoryDisplay, DisplayObject>(m,
			":Base class: :py:class:`ovito.vis.Display`\n\n"
			"Controls the visual appearance of particle trajectory lines. An instance of this class is attached to every :py:class:`~ovito.data.TrajectoryLineGenerator` data object.",
			"TrajectoryLineDisplay")
		.def_property("width", &TrajectoryDisplay::lineWidth, &TrajectoryDisplay::setLineWidth,
				"The display width of trajectory lines."
				"\n\n"
				":Default: 0.2\n")
		.def_property("color", &TrajectoryDisplay::lineColor, &TrajectoryDisplay::setLineColor,
				"The display color of trajectory lines."
				"\n\n"
				":Default: ``(0.6, 0.6, 0.6)``\n")
		.def_property("shading", &TrajectoryDisplay::shadingMode, &TrajectoryDisplay::setShadingMode,
				"The shading style used for trajectory lines.\n"
				"Possible values:"
				"\n\n"
				"   * ``TrajectoryLineDisplay.Shading.Normal`` \n"
				"   * ``TrajectoryLineDisplay.Shading.Flat`` (default)\n"
				"\n")
		.def_property("upto_current_time", &TrajectoryDisplay::showUpToCurrentTime, &TrajectoryDisplay::setShowUpToCurrentTime,
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
