# Load the native modules and other dependencies.
from ..plugins.PyScript import FileExporter, AttributeFileExporter, PipelineStatus, PipelineObject
from ..data import DataCollection, DataObject
from ..pipeline import Pipeline, StaticSource
import ovito

def export_file(data, file, format, **params):
    """ High-level function that exports data to a file.
        See the :ref:`file_output_overview` section for an overview of this topic.

        :param data: The object to be exported. See below for options.
        :param str file: The output file path.
        :param str format: The type of file to write. See below for options.

        **Data to export**

        Various kinds of objects are accepted by the function as *data* argument:

           * :py:class:`~ovito.pipeline.Pipeline`:
             Exports the dynamically generated output of a data pipeline. Since pipelines can be evaluated at different animation times,
             multi-frame sequences can be produced when passing a :py:class:`~ovito.pipeline.Pipeline` object to the :py:func:`!export_file` function.

           * :py:class:`~ovito.data.DataCollection`:
             Exports the static data of a data collection. Data objects contained in the collection that are not compatible
             with the chosen output format are ignored.

           * :py:class:`~ovito.data.DataObject`:
             Exports just the data object as if it were the only part of a :py:class:`~ovito.data.DataCollection`.
             The provided data object must be compatible with the selected output format. For example, when exporting
             to the ``"txt/series"`` format (see below), a :py:class:`~ovito.data.DataSeries` object may be passed
             to the :py:func:`!export_file` function.

           * ``None``:
             All pipelines that are part of the current scene (see :py:attr:`ovito.Scene.pipelines`) are
             exported. This option makes sense for scene description formats such as the POV-Ray format.

        **Output format**

        The *format* parameter determines the type of file to write; the filename suffix is ignored.
        However, for filenames that end with ``.gz``, automatic gzip compression is activated if the selected format is text-based.
        The following *format* strings are supported:

            * ``"txt/attr"`` -- Export global attributes to a text file (see below)
            * ``"txt/series"`` -- Export a :py:class:`~ovito.data.DataSeries` to a text file
            * ``"lammps/dump"`` -- LAMMPS text-based dump format
            * ``"lammps/data"`` -- LAMMPS data format
            * ``"imd"`` -- IMD format
            * ``"vasp"`` -- POSCAR format
            * ``"xyz"`` -- XYZ format
            * ``"fhi-aims"`` -- FHI-aims format
            * ``"netcdf/amber"`` -- Binary format for MD data following the `AMBER format convention <http://ambermd.org/netcdf/nctraj.pdf>`__
            * ``"vtk/trimesh"`` -- ParaView VTK format for exporting :py:class:`~ovito.data.SurfaceMesh` objects
            * ``"vtk/disloc"`` -- ParaView VTK format for exporting :py:class:`~ovito.data.DislocationNetwork` objects
            * ``"vtk/grid"`` -- ParaView VTK format for exporting :py:class:`~ovito.data.VoxelGrid` objects
            * ``"ca"`` -- :ovitoman:`Text-based format for storing dislocation lines <../../particles.modifiers.dislocation_analysis#particles.modifiers.dislocation_analysis.fileformat>`
            * ``"povray"`` -- POV-Ray scene format

        Depending on the selected output format, additional keyword arguments must be passed to :py:func:`!export_file`, which are documented below.

        **File columns**

        For the output formats *lammps/dump*, *xyz*, *imd* and *netcdf/amber*, you must specify the set of particle properties to export
        using the ``columns`` keyword parameter::

            export_file(pipeline, "output.xyz", "xyz", columns =
              ["Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z"]
            )

        You can export the :ref:`standard particle properties <particle-types-list>` and any user-defined properties present
        in the pipeline's output :py:class:`~ovito.data.DataCollection`.
        For vector properties, the component name must be appended to the base name as
        demonstrated above for the ``Position`` property.

        **Exporting several simulation frames**

        By default, only the current animation frame (frame 0 by default) is exported by the function.
        To export a different frame, pass the ``frame`` keyword parameter to the :py:func:`!export_file` function.
        Alternatively, you can export all frames of the current animation sequence at once by passing ``multiple_frames=True``. Refined
        control of the exported frame sequence is available through the keyword arguments ``start_frame``, ``end_frame``, and ``every_nth_frame``.

        The *lammps/dump* and *xyz* file formats can store multiple frames in a single output file. For other formats, or
        if you intentionally want to generate one file per frame, you must pass a wildcard filename to :py:func:`!export_file`.
        This filename must contain exactly one ``*`` character as in the following example, which will be replaced with the
        animation frame number::

            export_file(pipeline, "output.*.dump", "lammps/dump", multiple_frames=True)

        The above call is equivalent to the following ``for``-loop::

            for i in range(pipeline.source.num_frames):
                export_file(pipeline, "output.%i.dump" % i, "lammps/dump", frame=i)

        **Floating-point number precision**

        For text-based file formats, you can set the desired formatting precision for floating-point values using the
        ``precision`` keyword parameter. The default output precision is 10 digits; the maximum is 17.

        **LAMMPS atom style**

        When writing files in the *lammps/data* format, the LAMMPS atom style "atomic" is used by default. If you want to create
        a data file that uses a different atom style, specify it with the ``atom_style`` keyword parameter::

            export_file(pipeline, "output.data", "lammps/data", atom_style="bond")

        The following `LAMMPS atom styles <https://lammps.sandia.gov/doc/atom_style.html>`_ are currently supported by OVITO:
        ``angle``, ``atomic``, ``bond``, ``charge``, ``dipole``, ``full``, ``molecular``, ``sphere``.

        **VASP (POSCAR) format**

        When exporting to the *vasp* file format, OVITO will output atomic positions and velocities in Cartesian coordinates by default.
        You can request output in reduced cell coordinates instead by specifying the ``reduced`` keyword parameter::

            export_file(pipeline, "structure.poscar", "vasp", reduced=True)

        **Global attributes**

        The *txt/attr* file format allows you to export global quantities computed by the data pipeline to a text file.
        For example, to write out the number of FCC atoms identified by a :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier`
        as a function of simulation time, one would use the following::

            export_file(pipeline, "data.txt", "txt/attr",
                columns=["Timestep", "CommonNeighborAnalysis.counts.FCC"],
                multiple_frames=True)

        See the documentation of the individual modifiers to find out which global quantities they
        generate. You can also determine at runtime which :py:attr:`~ovito.data.DataCollection.attributes` are available
        in the output data collection of a :py:class:`~ovito.pipeline.Pipeline`::

            print(pipeline.compute().attributes)

    """

    # Determine the animation frame(s) to be exported.
    if 'frame' in params:
        frame = int(params['frame'])
        params['multiple_frames'] = True
        params['start_frame'] = frame
        params['end_frame'] = frame
        del params['frame']

    # Look up the exporter class for the selected format.
    if not format in export_file._formatTable:
        raise RuntimeError("Unknown output file format: %s" % format)

    # Create an instance of the exporter class.
    exporter = export_file._formatTable[format](params)

    # Pass function parameters to exporter object.
    exporter.output_filename = file

    # Detect wildcard filename.
    if '*' in file:
        exporter.wildcard_filename = file
        exporter.use_wildcard_filename = True

    # Exporting to a file is a long-running operation, which is not permitted during viewport rendering or pipeline evaluation.
    # In these situations, the following function call will raise an exception.
    ovito.scene.request_long_operation()

    # Pass data to be exported to the exporter:
    if isinstance(data, Pipeline):
        exporter.pipeline = data
    elif isinstance(data, PipelineObject):
        exporter.pipeline = Pipeline(source = data)
    elif isinstance(data, DataCollection):
        exporter.pipeline = Pipeline(source = StaticSource(data = data))
    elif isinstance(data, DataObject):
        data_collection = DataCollection()
        data_collection.objects.append(data)
        exporter.pipeline = Pipeline(source = StaticSource(data = data_collection))
        exporter.key = data.identifier
    elif data is None:
        exporter.select_default_exportable_data()
    else:
        raise TypeError("Invalid data. Cannot export this kind of Python object: {}".format(data))

    # Let the exporter do its job.
    exporter.do_export()

# This is the table of export formats used by the export_file() function
# to look up the right exporter class for a file format.
# Plugins can register their exporter class by inserting a new entry in this dictionary.
export_file._formatTable = {}
export_file._formatTable["txt/attr"] = AttributeFileExporter

# For backward compatibility with OVITO 2.9.0:
export_file._formatTable["txt"] = AttributeFileExporter
