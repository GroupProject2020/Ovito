"""
This module provides functions for interfacing with the ASE (`Atomistic Simulation Environment <https://wiki.fysik.dtu.dk/ase/>`__).
It contains two high-level functions for converting atomistic data back and forth between
OVITO and ASE:

    * :py:func:`ovito_to_ase`
    * :py:func:`ase_to_ovito`

.. note::

    Using the functions of this module will raise an ``ImportError`` if the ASE package
    is not installed in the current Python interpreter. Note that the built-in
    Python interpreter of OVITO does *not* include the ASE package by default.
    You can install the ASE module by running ``ovitos -m pip install ase``.
    Alternatively, you can install the ``ovito`` module in your :ref:`own Python interpreter <use_ovito_with_system_interpreter>`.

"""

import numpy as np

from ...data import DataCollection, SimulationCell, ParticleType, Particles

__all__ = ['ovito_to_ase', 'ase_to_ovito']

def ovito_to_ase(data_collection):
    """
    Constructs an `ASE Atoms object <https://wiki.fysik.dtu.dk/ase/ase/atoms.html>`__ from the
    particle data in an OVITO :py:class:`~ovito.data.DataCollection`.

    :param: data_collection: The OVITO :py:class:`~ovito.data.DataCollection` to convert.
    :return: An `ASE Atoms object <https://wiki.fysik.dtu.dk/ase/ase/atoms.html>`__ containing the
             converted particle data from the source :py:class:`~ovito.data.DataCollection`.

    Usage example:

    .. literalinclude:: ../example_snippets/ovito_to_ase.py
       :lines: 6-

    """

    from ase.atoms import Atoms
    from ase.data import chemical_symbols
    assert(isinstance(data_collection, DataCollection))

    # Extract basic data: pbc, cell, positions, particle types
    cell_obj = data_collection.cell
    pbc = cell_obj.pbc if cell_obj is not None else None
    cell = cell_obj[:, :3].T if cell_obj is not None else None
    info = {'cell_origin': cell_obj[:, 3] } if cell_obj is not None else None
    positions = np.array(data_collection.particles.positions)
    if data_collection.particles.particle_types is not None:
        # ASE only accepts chemical symbols as atom type names.
        # If our atom type names are not chemical symbols, pass the numerical atom type to ASE instead.
        type_names = {}
        for t in data_collection.particles.particle_types.types:
            if t.name in chemical_symbols:
                type_names[t.id] = t.name
            else:
                type_names[t.id] = t.id
        symbols = [type_names[id] for id in data_collection.particles.particle_types]
    else:
        symbols = None

    # Construct ase.Atoms object
    atoms = Atoms(symbols,
                  positions=positions,
                  cell=cell,
                  pbc=pbc,
                  info=info)

    # Convert any other particle properties to additional arrays
    for name, prop in data_collection.particles.items():
        if name in ['Position',
                    'Particle Type']:
            continue
        prop_name = prop.name
        i = 1
        while prop_name in atoms.arrays:
            prop_name = '{0}_{1}'.format(prop.name, i)
            i += 1
        atoms.new_array(prop_name, np.asanyarray(prop))

    return atoms

def ase_to_ovito(atoms):
    """
    Converts an `ASE Atoms object <https://wiki.fysik.dtu.dk/ase/ase/atoms.html>`__ to an OVITO :py:class:`~ovito.data.DataCollection`.

    :param atoms: The `ASE Atoms object <https://wiki.fysik.dtu.dk/ase/ase/atoms.html>`__ to be converted.
    :return: A new :py:class:`~ovito.data.DataCollection` containing the converted data.

    Usage example:

    .. literalinclude:: ../example_snippets/ase_to_ovito.py
       :lines: 6-

    """
    data_collection = DataCollection()

    # Set the unit cell and origin (if specified in atoms.info)
    cell = SimulationCell()
    cell.pbc = [bool(p) for p in atoms.get_pbc()]
    cell[:, :3] = atoms.get_cell().T
    cell[:, 3]  = atoms.info.get('cell_origin', [0., 0., 0.])
    data_collection.objects.append(cell)

    # Create particle property from atomic positions
    data_collection.objects.append(Particles())
    data_collection.particles.create_property('Position', data=atoms.get_positions())

    # Set particle types from chemical symbols
    types = data_collection.particles.create_property('Particle Type')
    symbols = atoms.get_chemical_symbols()
    type_list = list(set(symbols))
    for i, sym in enumerate(type_list):
        ovito_ptype = ParticleType(id=i+1, name=sym)
        ovito_ptype.load_defaults()
        types.type_list.append(ovito_ptype)
    with types as tarray:
        for i,sym in enumerate(symbols):
            tarray[i] = type_list.index(sym)+1

    # Check for computed properties - forces, energies, stresses
    calc = atoms.get_calculator()
    if calc is not None:
        for name, ptype in [('forces', 'Force'),
                            ('energies', 'Potential Energy'),
                            ('stresses', 'Stress Tensor'),
                            ('charges', 'Charge')]:
            try:
                array = calc.get_property(name,
                                          atoms,
                                          allow_calculation=False)
                if array is None:
                    continue
            except NotImplementedError:
                continue

            # Create a corresponding OVITO standard property.
            data_collection.particles.create_property(ptype, data=array)

    # Create user-defined properties
    for name, array in atoms.arrays.items():
        if name in ['positions', 'numbers']:
            continue
        data_collection.particles.create_property(name, data=array)

    return data_collection