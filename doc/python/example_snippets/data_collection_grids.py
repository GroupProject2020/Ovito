from ovito.io import import_file

pipeline = import_file("input/CHGCAR.nospin.gz")
data = pipeline.compute()

################# Code snippet begin #####################
print(data.grids)
################## Code snippet end ######################


################# Code snippet begin #####################
charge_density_grid = data.grids['imported']
print(charge_density_grid.shape)
################## Code snippet end ######################

