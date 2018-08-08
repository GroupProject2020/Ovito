# Load dependencies
import ovito
from ovito.modifiers import ComputePropertyModifier


# Implement the ComputePropertyModifier.cutoff_radius property.
def ComputePropertyModifier_cutoff_radius(self):
    """ 
        The cutoff radius up to which neighboring particles are visited to compute :py:attr:`.neighbor_expressions`.
        This parameter is only used if :py:attr:`.operate_on` is set to ``'particles'`` and the :py:attr:`.neighbor_expressions`
        field has been set.

        :Default: 3.0
    """
    if self.operate_on != 'particles': return 3.0
    return self.delegate.cutoff_radius
def ComputePropertyModifier_set_cutoff_radius(self, radius):
    if self.operate_on != 'particles':
        raise AttributeError("Setting cutoff_radius is only permitted if operate_on was previously set to 'particles'.")
    self.delegate.cutoff_radius = radius
ComputePropertyModifier.cutoff_radius = property(ComputePropertyModifier_cutoff_radius, ComputePropertyModifier_set_cutoff_radius)


# Implement the ComputePropertyModifier.neighbor_expressions property.
def ComputePropertyModifier_neighbor_expressions(self):
    """ 
        The list of strings containing the math expressions for the per-neighbor terms, one for each vector component of the output particle property.
        If the output property is scalar, the list must comprise one string only.

        The neighbor expressions are only evaluated for each neighbor particle and the value is added to the output property of the central particle.
        Neighbor expressions are only evaluated if :py:attr:`.operate_on` is set to ``'particles'``.

        :Default: ``[""]``
    """
    if self.operate_on != 'particles': return []
    return self.delegate.neighbor_expressions
def ComputePropertyModifier_set_neighbor_expressions(self, expressions):
    if self.operate_on != 'particles':
        raise AttributeError("Setting neighbor_expressions is only permitted if operate_on was previously set to 'particles'.")
    self.delegate.neighbor_expressions = expressions
ComputePropertyModifier.neighbor_expressions = property(ComputePropertyModifier_neighbor_expressions, ComputePropertyModifier_set_neighbor_expressions)


# Implement the ComputePropertyModifier.neighbor_mode property.
# This is only here for backward compatibility with OVITO 2.9.0.
def ComputePropertyModifier_neighbor_mode(self):
    #    Boolean flag that enables the neighbor computation mode, where contributions from neighbor particles within the
    #    :py:attr:`.cutoff_radius` are taken into account. The :py:attr:`.neighbor_expressions` list specifies the 
    #    math expression(s) to be evaulated for every neighbor particle.
    if self.operate_on != 'particles': return False
    return True
def ComputePropertyModifier_set_neighbor_mode(self, flag):
    if self.operate_on != 'particles':
        raise AttributeError("Setting neighbor_mode is only permitted if operate_on was previously set to 'particles'.")
    if flag != True:
        raise ValueError("Setting neighbor_mode to False is not supported anymore. Please set the neighbor expression(s) to empty strings instead.")
ComputePropertyModifier.neighbor_mode = property(ComputePropertyModifier_neighbor_mode, ComputePropertyModifier_set_neighbor_mode)
