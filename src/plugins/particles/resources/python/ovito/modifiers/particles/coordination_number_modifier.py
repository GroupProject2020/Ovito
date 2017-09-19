# Load system libraries
import numpy

# Load dependencies
import ovito
import ovito.modifiers

# Load the native code module
from ovito.plugins.Particles.Modifiers import CoordinationNumberModifier

# Implement the 'rdf' attribute of the CoordinationNumberModifier class.
def _CoordinationNumberModifier_rdf(self):
    """
    Returns a NumPy array containing the radial distribution function (RDF) computed by the modifier.    
    The returned array is two-dimensional and consists of the [*r*, *g(r)*] data points of the tabulated *g(r)* RDF function.
    
    Note that accessing this array is only possible after the modifier has computed its results. 
    Thus, you have to call :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` first to ensure that this information is up to date, see the example above.
    """
    return numpy.transpose((self.rdf_x,self.rdf_y))
CoordinationNumberModifier.rdf = property(_CoordinationNumberModifier_rdf)

