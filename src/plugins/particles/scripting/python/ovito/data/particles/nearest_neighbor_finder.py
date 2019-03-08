# Load dependencies
import ovito
from ovito.data import SimulationCell

# Load the native code module
import ovito.plugins.ParticlesPython

class NearestNeighborFinder(ovito.plugins.ParticlesPython.NearestNeighborFinder):
    """
    A utility class that finds the *N* nearest neighbors of a particle or around a spatial location.

    The constructor takes the (maximum) number of requested nearest neighbors, *N*, and a :py:class:`DataCollection <ovito.data.DataCollection>`
    containing the input particles and the cell geometry (including periodic boundary flags).
    *N* must be a positive integer not greater than 30 (which is the built-in maximum supported by this class).

    Once the :py:class:`!NearestNeighborFinder` has been constructed, you can call its :py:meth:`.find` method to
    iterate over the sorted list of nearest neighbors of a specific particle, for example:

    .. literalinclude:: ../example_snippets/nearest_neighbor_finder.py
       :lines: 1-23

    Furthermore, the class offers the :py:meth:`find_at` method, which lets you determine the *N* nearest particles around an
    arbitrary spatial location:

    .. literalinclude:: ../example_snippets/nearest_neighbor_finder.py
       :lines: 25-

    Note: In case you rather want to find all neighbor particles within a certain cutoff range of a particle,
    use the :py:class:`CutoffNeighborFinder` class instead.
    """

    def __init__(self, N, data_collection):
        """ This is the constructor. """
        super(self.__class__, self).__init__(N)
        if N<=0 or N>30:
            raise ValueError("The requested number of nearest neighbors is out of range.")
        if data_collection.particles is None or data_collection.particles.positions is None:
            raise KeyError("DataCollection does not contain any particles.")
        pos_property = data_collection.particles.positions
        self.particle_count = len(pos_property)
        if not self.prepare(pos_property, data_collection.cell):
            raise RuntimeError("Operation has been canceled by the user.")

    def find(self, index):
        """
        Returns an iterator that visits the *N* nearest neighbors of the given particle in order of ascending distance.

        :param int index: The index of the central particle whose neighbors should be iterated. Particle indices start at 0.
        :returns: A Python iterator that visits the *N* nearest neighbors of the central particle in order of ascending distance.
                  For each visited neighbor the iterator returns an object with the following attributes:

                      * **index**: The global index of the current neighbor particle.
                      * **distance**: The distance of the current neighbor from the central particle.
                      * **distance_squared**: The squared neighbor distance.
                      * **delta**: The three-dimensional vector connecting the central particle with the current neighbor (correctly taking into account periodic boundary conditions).

        The global index returned by the iterator can be used to look up properties of the neighbor as demonstrated in the first example code above.

        Note that several periodic images of the same particle may be visited if the periodic simulation cell is sufficiently small.
        Then the same particle index may appear more than once in the neighbor list. In fact, the central particle may be among its own neighbors in a sufficiently small periodic simulation cell.
        However, the computed neighbor vector (``delta``) will be unique for each image of a neighboring particle.

        The number of neighbors actually visited may be smaller than the requested number, *N*, if the
        system contains too few particles and has no periodic boundary conditions.

        Note that the :py:meth:`!find()` method will not find other particles located exactly at the same spatial position as the central particle for technical reasons.
        To find such particles too, which are positioned exactly on top of each other, make use of the :py:meth:`.find_at` method instead.
        """
        if index < 0 or index >= self.particle_count:
            raise IndexError("Particle index is out of range.")
        # Construct the C++ neighbor query.
        query = ovito.plugins.ParticlesPython.NearestNeighborFinder.Query(self)
        query.findNeighbors(int(index))
        # Iterate over neighbors.
        for i in range(query.count):
            yield query[i]

    def find_at(self, coords):
        """
        Returns an iterator that visits the *N* nearest particles around a spatial point given by *coords* in order of ascending distance.
        Unlike the :py:meth:`find` method, which queries the nearest neighbors of a physical particle, the :py:meth:`!find_at` method allows
        searching for neareby particles at arbitrary locations in space.

        :param coords: A (x,y,z) coordinate triplet specifying the spatial location where the *N* nearest particles should be queried.
        :returns: A Python iterator that visits the *N* nearest neighbors in order of ascending distance.
                  For each visited particle the iterator returns an object with the following attributes:

                      * **index**: The index of the current particle (starting at 0).
                      * **distance**: The distance of the current neighbor from the query location.
                      * **distance_squared**: The squared distance to the query location.
                      * **delta**: The three-dimensional vector from the query point to the current particle (correctly taking into account periodic boundary conditions).

        If there exists a particle that is exactly located at the query location given by *coords*, then it will be returned by this function.
        This is in contrast to the :py:meth:`find` function, which does not visit the central particle itself.

        The number of neighbors actually visited may be smaller than the requested number, *N*, if the
        system contains too few particles and has no periodic boundary conditions.
        """
        # Construct the C++ neighbor query.
        query = ovito.plugins.ParticlesPython.NearestNeighborFinder.Query(self)
        query.findNeighborsAtLocation(coords, True)
        # Iterate over neighbors.
        for i in range(query.count):
            yield query[i]
