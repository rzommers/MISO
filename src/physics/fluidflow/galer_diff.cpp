// #include <fstream>
// #include <iostream>
// #include "default_options.hpp"
// #include "galer_diff.hpp"
// using namespace std;
// using namespace miso;
// using namespace apf;
// namespace mfem
// {
// GalerkinDifference::GalerkinDifference(const string &opt_file_name)
// {
// #ifndef MFEM_USE_PUMI
//    mfem_error(' mfem needs to be build with pumi to use GalerkinDifference ')
// #endif

//    nlohmann::json file_options;
//    /// solver options
//    nlohmann::json options;
//    ifstream options_file(opt_file_name);
//    options_file >> file_options;
//    options = default_options;
//    options.merge_patch(file_options);
//    //cout << setw(3) << options << endl;
//    PCU_Comm_Init();
// #ifdef MFEM_USE_SIMMETRIX
//    Sim_readLicenseFile(0);
//    gmi_sim_start();
//    gmi_register_sim();
// #endif
//    gmi_register_mesh();
//    // load pumi mesh
//    pumi_mesh = loadMdsMesh(options["model-file"].get<string>().c_str(),
//                            options["pumi-mesh"]["file"].get<string>().c_str());
//    dim = pumi_mesh->getDimension();
//    nEle = pumi_mesh->count(dim);
//    //cout << nEle << endl;
//    // verify pumi mesh
//    pumi_mesh->verify();
//    // Create the MFEM mesh object from the PUMI mesh.
//    mesh.reset(new MeshType(pumi_mesh, 1, 1));
//    // Mesh *mesh = new PumiMesh(pumi_mesh, 1, 1);
//    //cout << mesh->GetNE() << endl;
//    // write mesh
//    ofstream sol_ofs("tri32_mfem.vtk");
//    sol_ofs.precision(14);
//    mesh->PrintVTK(sol_ofs, 0);
//    apf::writeVtkFiles("pumi_mesh", pumi_mesh);
//    PCU_Comm_Free();

//    // TODO:
//    // 1. determine the size of cP. i.e. # of quadrature points and
//    barycenters.
//    // 2. call the mfem::FiniteElementSpace constructor.
//    // 3. make sure that the dofs' order is consistent with that in
//    bi/nonlinearforms
// } // class constructor ends

// void GalerkinDifference::BuildNeighbourMat(DenseMatrix &nmat1, DenseMatrix
// &nmat2)
// {
//    // create pumi iterator over elements
//    pMeshIter it = pumi_mesh->begin(pumi_mesh_getDim(pumi_mesh));
//    pMeshEnt e;
//    Vector3 x;
//    // vector<int> nv1;
//    //cout << "pumi mesh element centers " << endl;
//    cout << "element neighbours " << endl;
//    int degree = 2;
//    int req_n = ((degree + 1) * (degree + 2)) / 2;
//    int max_n = 0;
//    int min_n = req_n;
//    // iterate over mesh elements to get maximum number of neighbours for an
//    element.
//    // this provides the row size of neighbour matrices
//    while ((e = pumi_mesh->iterate(it)))
//    {
//       // create pumi mesh entity for neighbouring elements
//       Adjacent nels_e;
//       //get first neighbours (with shared edges)
//       getBridgeAdjacent(pumi_mesh, e, pumi_mesh_getDim(pumi_mesh) - 1,
//                         pumi_mesh_getDim(pumi_mesh), nels_e);
//       if (nels_e.size() > max_n)
//       {
//          max_n = nels_e.size();
//       }
//       if (nels_e.size() < min_n)
//       {
//          min_n = nels_e.size();
//       }
//    }
//    pumi_mesh->end(it); // end pumi iterations
//    cout << "max size " << endl;
//    cout << max_n << ", " << min_n << endl;
//    vector<int> nels;
//    //GetNeighbourSet(0, req_n, nels);
//    // set size of neighbour matrix
//    // To do: in 3D the # neighbours may be more than the required #
//    neighbours
//    // nmat1.SetSize((max_n + req_n-min_n), nEle);
// }

// // an overload function of previous one (more doable?)
// void GalerkinDiffereence::BuildNeighbourMat(const std::vector<int> elmt_id,
//                                             mfem::DenseMatrix &mat_cent,
//                                             mfem::DenseMatrix &mat_quad)
// {
//    // resize the DenseMatrices and clean the data
//    int num_el = elmt_id.size();
//    mat_cent.Clear(); mat_cent.SetSize(dim, num_el);

//    const FiniteElement *fe =
//    fec->FiniteElementForGeometry(Geometry::TRIANGLE); const int num_dofs =
//    fe->GetDof();
//    // vectors that hold coordinates of quadrature points
//    // used for duplication tests
//    vector<double> quad_data;
//    Vector quad_coord(dim); // used to store quadrature coordinate temperally
//    ElementTransformation *eltransf;
//    for (int j = 0; j < num_el; j++)
//    {
//       // Get and store the element center
//       mfem::Vector cent_coord(dim);
//       GetElementCenter(elmt_id[j], cent_coord);
//       for (int i = 0; i < dim; i++)
//       {
//          mat_cent(i,j) = cent_coord(i);
//       }

//       // deal with quadrature points
//       eltransf = mesh->GetElementTransformation(elmt_id[j]);
//       for (int k = 0; k < num_dofs; k++)
//       {
//          eltransf->Transform(fe->GetNodes.IntPoint(k), quad_coord);
//          //check duplication
//          if ( !duplicated(quad_coord, quad_data) )
//          {
//             for (int di = 0; di < dim; di++)
//             {
//                quad_data.push_back(quad_coord(di));
//             }
//          }
//       }
//    }
//    // reset the quadrature point matrix
//    mat_quad.Clear();
//    int num_col = quad_data.size()/dim;
//    mat_quad.SetSize(dim, num_col);
//    for (int i = 0; i < num_col; i++)
//    {
//       for (int j = 0; j < dim; j++)
//       {
//          mat_quad(i,j) = quad_data[i*dim+j];
//       }
//    }
// }

// void GalerkinDifference::GetNeighbourSet(int id, int req_n, std::vector<int>
// &nels)
// {
//    /// this stores the elements for which we need neighbours
//    vector<pMeshEnt> el;
//    pMeshEnt e;
//    /// get pumi mesh entity (element) for the given id
//    e = getMdsEntity(pumi_mesh, dim, id);
//    /// first, need to find neighbour of the given element
//    el.push_back(e);
//    /// first entry in neighbour vector should be the element itself
//    nels.resize(0); // clean the queue vector
//    nels.push_back(id);
//    /// iterate for finding element neighbours.
//    /// it stops when the # of elements in patch are equal/greater
//    /// than the minimum required # of elements in patch.
//    while (nels.size() < req_n)
//    {
//       /// this stores the neighbour elements for which we need neighbours
//       vector<pMeshEnt> elm;
//       ///get neighbours (with shared edges)
//       for (int j = 0; j < el.size(); ++j)
//       {
//          /// vector for storing neighbours of el[j]
//          Adjacent nels_e1;
//          /// get neighbours
//          getBridgeAdjacent(pumi_mesh, el[j], pumi_mesh_getDim(pumi_mesh) - 1,
//                            pumi_mesh_getDim(pumi_mesh), nels_e1);
//          /// retrieve the id of neighbour elements
//          /// push in nels
//          for (int i = 0; i < nels_e1.size(); ++i)
//          {
//             int nid;
//             nid = getMdsIndex(pumi_mesh, nels_e1[i]);
//             /// check for element, push it if not there already
//             if (!(std::find(nels.begin(), nels.end(), nid) != nels.end()))
//             {
//                nels.push_back(nid);
//             }
//             /// push neighbour elements for next iteration
//             /// and use them if required
//             elm.push_back(nels_e1[i]);
//          }
//       }
//       /// resizing el to zero prevents finding neighbours of the same
//       elements el.resize(0);
//       /// insert the neighbour elements in 'el' and iterate to find their
//       neighbours if needed el.insert(end(el), begin(elm), end(elm));
//    }
// }

// void GalerkinDifference::GetElementCenter(int id, mfem::Vector &cent)
// {
//    cent.SetSize(mesh->Dimension());
//    int geom = mesh->GetElement(id)->GetGeometryType();
//    ElementTransformation *eltransf = mesh->GetElementTransformation(id);
//    eltransf->Transform(Geometries.GetCenter(geom), cent);
// }

// void GalerkinDifference::BuildGDProlongation() const
// {
//    // allocate the space for the prolongation matrix
//    // this step should be done in the constructor (probably)
//    // should it be GetTrueVSize() ? or GetVSize()?
//    // need a new method that directly construct a CSR format sparsematrix ？
//    cP = new mfem::SparseMatrix(GetVSize(), nEle);
//    // determine the minimum # of element in each patch
//    int nelmt;
//    switch(dim)
//    {
//       case 1: nelmt = degree + 1; break;
//       case 2: nelmt = (degree+1) * (degree+2) / 2; break;
//       case 3: throw MISOException("Not implemeneted yet.\n"); break;
//       default: throw MISOException("dim must be 1, 2 or 3.\n");
//    }
//    // loop over all the element:
//    // 1. build the patch for each element,
//    // 2. construct the local reconstruction operator
//    // 3. assemble local reconstruction operator

//    // vector that contains element id (resize to zero )
//    std::vector<int> elmt_id;
//    mfem::DenseMatrix cent_mat, quad_mat;

//    for (int i = 0; i < nEle; i++)
//    {
//       // 1. construct the patch the patch
//       // have more element than required to make it a underdetermined system
//       elmt_id.clear();
//       GetNeighbourSet(i, nelmt, elmt_id);

//       // 2. build the quadrature and barycenter coordinate matrices
//       BuildNeighbourMat(elmt_id, cent_mat, quad_mat);

//       // 3. buil the loacl reconstruction matrix
//       buildInterpolation(degree, cent_mat, quad_mat, local_mat);

//       // 4. assemble them back to prolongation matrix
//       AssmbleProlongationMatrix(elemt_id, local_mat);
//    }
// }

// bool GalerkinDifference::duplicated(const Vector quad, const vector<double>
// data)
// {
//    bool duplicated;
//    int data_size = data.size();
//    MFEM_ASSERT(data_size % dim == 0," Quadrature data size is wrong.\n");
//    for (int i = 0; i < data_size/dim; i++)
//    {
//       for (int di = 0; di < dim; di++)
//       {
//          if ( quad(di) != data[i*dim+di] ){ return false; }
//       }
//    }
//    // fall to pass the duplication test
//    return true;
// }

// void GalerkinDifference::AssembleProlongationMatrix(const vector<int> id,
//                                        const DenseMatrix local_mat) const
// {
//    // GetElementVDofs();
//    // cp->addsubmatrix();
// }

// } // namespace mfem
