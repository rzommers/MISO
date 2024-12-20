import tempfile
import os

import numpy as np
import openmdao.api as om
import pyCAPS

from .pyMISO import Mesh, Vector
from .pyMISO import MeshMovement

class omEGADS(om.ExplicitComponent):
    """
    Uses the pyCAPS interface to construct a model of a bolt
    """

    def initialize(self):
        self.options.declare('csm_file', types=str)
        self.options.declare('tess_file', types=str)
        self.options.declare('model_file', types=str)
        self.options.declare('mesh_file', types=str)

    def setup(self):
        self.caps_problem = pyCAPS.capsProblem()

        geometryScript = self.options['csm_file'] + '.csm'
        self.geom = self.caps_problem.loadCAPS(geometryScript)

        self.design_pmtrs = self.geom.getGeometryVal()
        for key in self.design_pmtrs:
            if self.design_pmtrs[key].is_integer():
                continue
            self.add_input(key, self.design_pmtrs[key])

        model_file = self.options['model_file']
        mesh_file = self.options['mesh_file']

        self.mesh = Mesh(model_file=model_file, mesh_file=mesh_file)
        local_mesh_size = self.mesh.getMeshSize()
        self.add_output('surf_mesh_disp', shape=local_mesh_size)
    
    def compute(self, inputs, outputs):
        """
        Build the geometry model 
        """
        for key in self.design_pmtrs:
            if key in inputs:
                print("setting geom val: ", key, "to: ", inputs[key])
                self.geom.setGeometryVal(key, inputs[key][0])

        tmp_dir = tempfile.gettempdir()
        tmp_model = os.path.join(tmp_dir, "tmp_model.egads")

        self.geom.saveGeometry(tmp_model)
        
        old_model = self.options['model_file']
        tess_file = self.options['tess_file']
        surf_disp = outputs['surf_mesh_disp']
        surf_disp.fill(0)

        MeshMovement.mapSurfaceMesh(old_model, tmp_model, tess_file, surf_disp)
        surf_disp2 = Vector(surf_disp)
        # print("surf nodes")
        # print(surf_disp2)

        mesh_coords = Vector(self.mesh.getMeshSize())
        self.mesh.getNodes(mesh_coords)
        # print("mesh coords")
        # print(mesh_coords)

        self.mesh.setNodes(surf_disp2)
        self.mesh.Print("testegads")
        self.mesh.PrintVTU("testegads")

        mesh_coords2 = Vector(self.mesh.getMeshSize())
        self.mesh.getNodes(mesh_coords2)
        # print("mesh coords2")
        # print(mesh_coords2)


        # np_mesh_coords = np.array(mesh_coords, copy=False)
        # diff = surf_disp - np_mesh_coords
        # print(diff)


        