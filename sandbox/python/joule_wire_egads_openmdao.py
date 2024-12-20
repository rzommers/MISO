from miso import omEGADS, omMeshMove, omMISOState, Mesh, Vector
import openmdao.api as om

from miso import MISOSolver

mesh_options = {
    'mesh': {
        'file': 'wire.smb',
        'model-file': 'wire.egads'
    },
    'print-options': True,
    'space-dis': {
        'degree': 1,
        'basis-type': 'H1'
    },
    'time-dis': {
        'steady': True,
        'steady-abstol': 1e-12,
        'steady-restol': 1e-10,
        'ode-solver': 'PTC',
        't-final': 100,
        'dt': 1e12,
        'cfl': 1.0,
        'res-exp': 2.0
    },
    'nonlin-solver': {
        'type': 'newton',
        'printlevel': 3,
        'maxiter': 50,
        'reltol': 1e-10,
        'abstol': 1e-12
    },
    'lin-solver': {
        'type': 'hyprepcg',
        'printlevel': -1,
        'maxiter': 100,
        'abstol': 1e-14,
        'reltol': 1e-14
    },
    'lin-prec': {
        'type': 'hypreboomeramg',
        'printlevel': -1
    },
    'saveresults': False,
    'problem-opts': {
        'uniform-stiff': {
            'lambda': 1,
            'mu': 1
        }
    }
}

em_options = {
    "silent": False,
    "print-options": False,
    "mesh": {
        "file": "wire.smb",
        "model-file": "wire.egads"
    },
    "space-dis": {
        "basis-type": "nedelec",
        "degree": 2
    },
    "time-dis": {
        "steady": True,
        "steady-abstol": 1e-12,
        "steady-reltol": 1e-10,
        "ode-solver": "PTC",
        "t-final": 100,
        "dt": 1e12,
        "max-iter": 10
    },
    "lin-solver": {
        "type": "hypregmres",
        "printlevel": 0,
        "maxiter": 100,
        "abstol": 1e-14,
        "reltol": 1e-14
    },
    "lin-prec": {
        "type": "hypreams",
        "printlevel": 0
    },
    "nonlin-solver": {
        "type": "newton",
        "printlevel": 3,
        "maxiter": 50,
        "reltol": 1e-10,
        "abstol": 1e-12
    },
    "components": {
        "attr1": {
            "material": "copperwire",
            "attr": 1,
            "linear": True
        }
    },
    "problem-opts": {
        "fill-factor": 1.0,
        "current_density": 1.2732395447351627e7,
        "current": {
            "z": [1]
        }
    },
    "bcs": {
        "essential": [1, 2, 3, 4]
    },
    "outputs": {
        "co-energy": {}
    },
    "external-fields": {
        "mesh-coords": {}
    }
}

thermal_options = {
    "print-options": False,
    "mesh": {
        "file": "wire.smb",
        "model-file": "wire.egads"
    },
    "space-dis": {
        "basis-type": "H1",
        "degree": 6
    },
    "time-dis": {
        "steady": True,
        "steady-abstol": 1e-8,
        "steady-reltol": 1e-8,
        "ode-solver": "PTC",
        "dt": 1e12,
        "max-iter": 5
    },
    "lin-prec": {
        "type": "hypreboomeramg"
    },
    "lin-solver": {
        "reltol": 1e-14,
        "abstol": 0.0,
        "printlevel": 0,
        "maxiter": 500
    },
    "nonlin-solver": {
        "type": "newton",
        "printlevel": 3,
        "maxiter": 50,
        "reltol": 1e-10,
        "abstol": 1e-10
    },
    "components": {
        "attr1": {
            "material": "copperwire",
            "attr": 1,
            "linear": True
        }
    },
    "problem-opts": {
        "fill-factor": 1.0,
        "current_density": 1.2732395447351627e7,
        "frequency": 0,
        "current": {
            "z": [1]
        },
        "rho-agg": 10,
        "init-temp": 300
    },
    "bcs": {
        "essential": [1, 2]
    },
    "outflux-type": "test",
    "outputs": {
        "temp-agg": {}
    },
    "external-fields": {
        "mvp": {
            "basis-type": "nedelec",
            "degree": 2,
            "num-states": 1
        },
        "mesh-coords": {}
    }
}

if __name__ == "__main__":
    problem = om.Problem()
    model = problem.model
    ivc = om.IndepVarComp()

    ivc.add_output('radius', 0.006) # radius of wire

    model.add_subsystem('des_vars', ivc)
    model.add_subsystem('surf_mesh_move', omEGADS(csm_file='wire',
                                                  model_file='wire.egads',
                                                  mesh_file='wire.smb',
                                                  tess_file='wire.eto'))
    model.connect('des_vars.radius', 'surf_mesh_move.radius')
    
    meshMoveSolver = MISOSolver("MeshMovement", mesh_options, problem.comm)
    model.add_subsystem('vol_mesh_move', omMeshMove(solver=meshMoveSolver))
    model.connect('surf_mesh_move.surf_mesh_disp', 'vol_mesh_move.surf_mesh_disp')

    emSolver = MISOSolver("Magnetostatic", em_options, problem.comm)
    model.add_subsystem('em_solver', omMISOState(solver=emSolver, 
                                                 initial_condition=Vector([0.0, 0.0, 0.0])))
    model.connect('vol_mesh_move.vol_mesh_coords', 'em_solver.mesh-coords')

    thermalSolver = MISOSolver("Thermal", thermal_options, problem.comm)
    model.add_subsystem('thermal_solver', omMISOState(solver=thermalSolver,
                                                      initial_condition=300.0))
    model.connect('vol_mesh_move.vol_mesh_coords', 'thermal_solver.mesh-coords')
    model.connect('em_solver.state', 'thermal_solver.mvp')

    problem.setup()
    problem.run_model()
