# -*- coding: UTF-8 -*-

# Load system libraries
import numpy

# Load dependencies
import ovito
import ovito.modifiers

# Load the native code module
from ovito.plugins.CorrelationFunctionPlugin import CorrelationFunctionModifier

# Implementation of the CorrelationFunctionModifier.get_real_space_function() function.
def _CorrelationFunctionModifier_get_real_space_function(self):
    """
    Returns the computed real-space correlation function as a NumPy array.
    The array is two-dimensional and consists of [*r*, *C(r)*] value pairs, where *r* denotes the distance and *C(r)* the unnormalized function value at that distance.
    
    Calling this getter function is only permitted after the modifier has computed its results as part of a data pipeline evaluation. 
    Thus, you should typically call :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` first to ensure that the modifier has calculated its results.
    """
    return numpy.transpose((self._realspace_x, self._realspace_correlation))
CorrelationFunctionModifier.get_real_space_function = _CorrelationFunctionModifier_get_real_space_function

# Implementation of the CorrelationFunctionModifier.get_rdf() function.
def _CorrelationFunctionModifier_get_rdf(self):
    """
    Returns the computed radial distribution function (RDF) as a NumPy array.
    The array is two-dimensional and consists of [*r*, *g(r)*] value pairs, where *r* denotes the distance and *g(r)* the RDF value at that distance.
    
    Calling this getter function is only permitted after the modifier has computed its results as part of a data pipeline evaluation. 
    Thus, you should typically call :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` first to ensure that the modifier has calculated its results.
    """
    return numpy.transpose((self._realspace_x, self._realspace_rdf))
CorrelationFunctionModifier.get_rdf = _CorrelationFunctionModifier_get_rdf

# Implementation of the CorrelationFunctionModifier.get_reciprocal_space_function() function.
def _CorrelationFunctionModifier_get_reciprocal_space_function(self):
    """
    Returns the computed reciprocal-space correlation function as a NumPy array.
    The array is two-dimensional and consists of [*q*, *C(q)*] value pairs, where *q* denotes the wave vector (including a factor of 2π) and *C(q)* the unnormalized function value at *q*.
    
    Calling this getter function is only permitted after the modifier has computed its results as part of a data pipeline evaluation. 
    Thus, you should typically call :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` first to ensure that the modifier has calculated its results.
    """
    return numpy.transpose((self._reciprocspace_x, self._reciprocspace_correlation))
CorrelationFunctionModifier.get_reciprocal_space_function = _CorrelationFunctionModifier_get_reciprocal_space_function
