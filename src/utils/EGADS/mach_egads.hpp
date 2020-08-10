#ifndef MACH_EGADS
#define MACH_EGADS

/// try to put includes in the .cpp file unless theyre explicitly needed for
/// the function prototypes
#include "mfem.hpp"
#include "utils.hpp"

#ifdef MFEM_USE_PUMI
#include "gmi_egads.h"
#include <PCU.h>
#include <apfMDS.h>

void getBoundaryNodeDisplacement(std::string oldmodel,
                                 std::string newmodel, 
                                 std::string tessname,
                                 apf::Mesh2* mesh, 
                                 mfem::Array<mfem::Vector>* disp_list);

apf::Mesh2* getNewMesh(std::string newmodel,
                       std::string newmesh,
                       mfem::Mesh* mfemmesh,
                       apf::Mesh2* oldmesh);

#endif
#endif
