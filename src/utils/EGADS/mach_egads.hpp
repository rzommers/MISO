#ifndef MACH_EGADS
#define MACH_EGADS

#include "mfem.hpp"
#include "utils.hpp"

#ifdef MFEM_USE_PUMI
#include "gmi_egads.h"

void getBoundaryNodeDisplacement(std::string oldmodel,
                                std::string newmodel, 
                                std::string tessname,
                                apf::Mesh2* mesh, 
                                mfem::Array<mfem::Vector>* disp_list);

#endif
#endif
