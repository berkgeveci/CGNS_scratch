#include "petscsf.h"

#include <petscdmplex.h>
#include <petsc/private/dmpleximpl.h> /*I   "petscdmplex.h"   I*/

#include <vtkUnstructuredGrid.h>
#include <vtkPoints.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <vtkUnstructuredGridAlgorithm.h>
#include <vtkObjectFactory.h>
#include <vtkNew.h>
#include <vtkInformationVector.h>


#if !defined(CGNS_ENUMT)
  #define CGNS_ENUMT(a) a
#endif
#if !defined(CGNS_ENUMV)
  #define CGNS_ENUMV(a) a
#endif
// Permute plex closure ordering to CGNS
static PetscErrorCode DMPlexCGNSGetPermutation_Internal(DMPolytopeType cell_type, PetscInt closure_size, const int **perm)
{
  // https://cgns.github.io/CGNS_docs_current/sids/conv.html#unstructgrid
  static const int bar_2[2]   = {0, 1};
  static const int bar_3[3]   = {1, 2, 0};
  static const int bar_4[4]   = {2, 3, 0, 1};
  static const int bar_5[5]   = {3, 4, 0, 1, 2};
  static const int tri_3[3]   = {0, 1, 2};
  static const int tri_6[6]   = {3, 4, 5, 0, 1, 2};
  static const int tri_10[10] = {7, 8, 9, 1, 2, 3, 4, 5, 6, 0};
  static const int quad_4[4]  = {0, 1, 2, 3};
  static const int quad_9[9]  = {
    5, 6, 7, 8, // vertices
    1, 2, 3, 4, // edges
    0,          // center
  };
  static const int quad_16[] = {
    12, 13, 14, 15,               // vertices
    4,  5,  6,  7,  8, 9, 10, 11, // edges
    0,  1,  3,  2,                // centers
  };
  static const int quad_25[] = {
    21, 22, 23, 24,                                 // vertices
    9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, // edges
    0,  1,  2,  5,  8,  7,  6,  3,  4,              // centers
  };
  static const int tetra_4[4]   = {0, 2, 1, 3};
  static const int tetra_10[10] = {6, 8, 7, 9, 2, 1, 0, 3, 5, 4};
  static const int tetra_20[20] = {
    16, 18, 17, 19,         // vertices
    9,  8,  7,  6,  5,  4,  // bottom edges
    10, 11, 14, 15, 13, 12, // side edges
    0,  2,  3,  1,          // faces
  };
  static const int hexa_8[8]   = {0, 3, 2, 1, 4, 5, 6, 7};
  static const int hexa_27[27] = {
    19, 22, 21, 20, 23, 24, 25, 26, // vertices
    10, 9,  8,  7,                  // bottom edges
    16, 15, 18, 17,                 // mid edges
    11, 12, 13, 14,                 // top edges
    1,  3,  5,  4,  6,  2,          // faces
    0,                              // center
  };
  static const int hexa_64[64] = {
    // debug with $PETSC_ARCH/tests/dm/impls/plex/tests/ex49 -dm_plex_simplex 0 -dm_plex_dim 3 -dm_plex_box_faces 1,1,1 -dm_coord_petscspace_degree 3
    56, 59, 58, 57, 60, 61, 62, 63, // vertices
    39, 38, 37, 36, 35, 34, 33, 32, // bottom edges
    51, 50, 48, 49, 52, 53, 55, 54, // mid edges; Paraview needs edge 21-22 swapped with 23-24
//    51, 50, 48, 49, 53, 52, 54, 55, // mid edges; Paraview needs edge 21-22 swapped with 23-24
    40, 41, 42, 43, 44, 45, 46, 47, // top edges
    8,  10, 11, 9,                  // z-minus face
    16, 17, 19, 18,                 // y-minus face
    24, 25, 27, 26,                 // x-plus face
    20, 21, 23, 22,                 // y-plus face
    30, 28, 29, 31,                 // x-minus face
    12, 13, 15, 14,                 // z-plus face
    0,  1,  3,  2,  4,  5,  7,  6,  // center
  };

  PetscFunctionBegin;
  *perm            = NULL;
  switch (cell_type) {
  case DM_POLYTOPE_SEGMENT:
    switch (closure_size) {
    case 2:
      *perm            = bar_2;
    case 3:
      *perm            = bar_3;
    case 4:
      *perm            = bar_4;
      break;
    case 5:
      *perm            = bar_5;
      break;
    default:
      SETERRQ(PETSC_COMM_SELF, PETSC_ERR_SUP, "Cell type %s with closure size %" PetscInt_FMT, DMPolytopeTypes[cell_type], closure_size);
    }
    break;
  case DM_POLYTOPE_TRIANGLE:
    switch (closure_size) {
    case 3:
      *perm            = tri_3;
      break;
    case 6:
      *perm            = tri_6;
      break;
    case 10:
      *perm            = tri_10;
      break;
    default:
      SETERRQ(PETSC_COMM_SELF, PETSC_ERR_SUP, "Cell type %s with closure size %" PetscInt_FMT, DMPolytopeTypes[cell_type], closure_size);
    }
    break;
  case DM_POLYTOPE_QUADRILATERAL:
    switch (closure_size) {
    case 4:
      *perm            = quad_4;
      break;
    case 9:
      *perm            = quad_9;
      break;
    case 16:
      *perm            = quad_16;
      break;
    case 25:
      *perm            = quad_25;
      break;
    default:
      SETERRQ(PETSC_COMM_SELF, PETSC_ERR_SUP, "Cell type %s with closure size %" PetscInt_FMT, DMPolytopeTypes[cell_type], closure_size);
    }
    break;
  case DM_POLYTOPE_TETRAHEDRON:
    switch (closure_size) {
    case 4:
      *perm            = tetra_4;
      break;
    case 10:
      *perm            = tetra_10;
      break;
    case 20:
      *perm            = tetra_20;
      break;
    default:
      SETERRQ(PETSC_COMM_SELF, PETSC_ERR_SUP, "Cell type %s with closure size %" PetscInt_FMT, DMPolytopeTypes[cell_type], closure_size);
    }
    break;
  case DM_POLYTOPE_HEXAHEDRON:
    switch (closure_size) {
    case 8:
      *perm            = hexa_8;
      break;
    case 27:
      *perm            = hexa_27;
      break;
    case 64:
      *perm            = hexa_64;
      break;
    default:
      SETERRQ(PETSC_COMM_SELF, PETSC_ERR_SUP, "Cell type %s with closure size %" PetscInt_FMT, DMPolytopeTypes[cell_type], closure_size);
    }
    break;
  default:
    SETERRQ(PETSC_COMM_SELF, PETSC_ERR_SUP, "Cell type %s with closure size %" PetscInt_FMT, DMPolytopeTypes[cell_type], closure_size);
  }
  PetscFunctionReturn(PETSC_SUCCESS);
}

class vtkPETScCGNSReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkPETScCGNSReader* New();
  vtkTypeMacro(vtkPETScCGNSReader, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetStdStringFromCharMacro(FileName);
  vtkGetCharFromStdStringMacro(FileName);

protected:

  std::string FileName;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkPETScCGNSReader();
  ~vtkPETScCGNSReader() override;

private:
  vtkPETScCGNSReader(const vtkPETScCGNSReader&) = delete;
  void operator=(const vtkPETScCGNSReader&) = delete;

  vtkDoubleArray* LoadSolution(DM* dm);
};

vtkStandardNewMacro(vtkPETScCGNSReader);

vtkPETScCGNSReader::vtkPETScCGNSReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

vtkPETScCGNSReader::~vtkPETScCGNSReader()
{}

void vtkPETScCGNSReader::PrintSelf(ostream& os, vtkIndent indent)
{}

vtkDoubleArray* vtkPETScCGNSReader::LoadSolution(DM* dm)
{
  MPI_Comm comm = PETSC_COMM_WORLD;

  Vec local_sln, V;
  PetscViewer viewer;
  PetscReal   time;
  PetscBool   set;

  // Set section component names, used when writing out CGNS files
  PetscSection section;
  DMGetLocalSection(*dm, &section);
  
  PetscSectionSetFieldName(section, 0, "");
  PetscSectionSetComponentName(section, 0, 0, "Pressure");
  PetscSectionSetComponentName(section, 0, 1, "VelocityX");
  PetscSectionSetComponentName(section, 0, 2, "VelocityY");
  PetscSectionSetComponentName(section, 0, 3, "VelocityZ");
  PetscSectionSetComponentName(section, 0, 4, "Temperature");
  
  // Load solution from CGNS file
  PetscViewerCGNSOpen(comm, this->FileName.c_str(), FILE_MODE_READ, &viewer);
  DMGetGlobalVector(*dm, &V);
  PetscViewerCGNSSetSolutionIndex(viewer, -1);
  PetscInt idx;
  // PetscViewerCGNSGetSolutionName(viewer, &name);
  PetscViewerCGNSGetSolutionTime(viewer, &time, &set);
  VecLoad(V, viewer);
  PetscViewerDestroy(&viewer);

  DMGetLocalVector(*dm, &local_sln);

  // Transfer data from global vector to local vector (with ghost points)
  DMGlobalToLocalBegin(*dm, V, INSERT_VALUES, local_sln);
  DMGlobalToLocalEnd(*dm, V, INSERT_VALUES, local_sln);
  
  PetscInt lsize;
  VecGetLocalSize(local_sln, &lsize);

  vtkNew<vtkDoubleArray> fields;
  fields->SetName("fields");
  fields->SetNumberOfComponents(5);
  fields->SetNumberOfTuples(lsize/5);
  PetscScalar* slnArray;
  VecGetArray(local_sln, &slnArray);
  memcpy(fields->GetPointer(0), slnArray, lsize*sizeof(double));
  VecRestoreArray(local_sln, &slnArray);

  fields->Register(this);
  return fields.GetPointer();
}

int vtkPETScCGNSReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  PetscOptionsSetValue(NULL, "-dm_plex_cgns_parallel", "1");
  PetscInitializeNoArguments();

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  // get the output
  vtkUnstructuredGrid* grid = vtkUnstructuredGrid::GetData(outInfo);

  MPI_Comm comm = PETSC_COMM_WORLD;
  int rank;
  MPI_Comm_rank(comm, &rank);

  DM          dm;
  Vec      coords_loc, local_sln, V;
  PetscInt coords_loc_size, coords_dim;
  PetscInt degree, dim;
  PetscViewer viewer;
  const char *name;
  PetscReal   time;
  PetscBool   set;

  DMPlexCreateFromFile(PETSC_COMM_WORLD, this->FileName.c_str(), "ex16_plex", PETSC_TRUE, &dm);
  PetscCall(DMSetUp(dm));
  PetscCall(DMSetFromOptions(dm));

  PetscFE        fe_natural;
  PetscDualSpace dual_space_natural;

  PetscCall(DMGetField(dm, 0, NULL, (PetscObject *)&fe_natural));
  PetscCall(PetscFEGetDualSpace(fe_natural, &dual_space_natural));
  PetscCall(PetscDualSpaceGetOrder(dual_space_natural, &degree));
  PetscCall(DMClearFields(dm));
  PetscCall(DMSetLocalSection(dm, NULL));

  PetscCall(DMGetDimension(dm, &dim));

  PetscInt          cStart, cEnd;
  PetscCall(DMPlexGetHeightStratum(dm, 0, &cStart, &cEnd));

  DMPolytopeType cell_type;

  PetscFE fe;
  PetscCall(DMPlexGetCellType(dm, cStart, &cell_type));
  PetscCall(PetscFECreateLagrangeByCell(PETSC_COMM_SELF, dim, 5, cell_type, degree, PETSC_DETERMINE, &fe));
  PetscCall(PetscObjectSetName((PetscObject)fe, "FE for VecLoad"));
  PetscCall(DMAddField(dm, NULL, (PetscObject)fe));
  PetscCall(DMCreateDS(dm));
  PetscCall(PetscFEDestroy(&fe));

  PetscCall(DMGetCoordinatesLocal(dm, &coords_loc));
  PetscCall(DMGetCoordinateDim(dm, &coords_dim));
  PetscCall(VecGetLocalSize(coords_loc, &coords_loc_size));

  auto num_local_nodes = coords_loc_size / coords_dim;
  
  vtkNew<vtkDoubleArray> pts_array;
  pts_array->SetNumberOfComponents(3);
  const double* pts_ptr;
  PetscCall(VecGetArrayRead(coords_loc, &pts_ptr));
  double* pts_copy = new double[coords_loc_size];
  memcpy(pts_copy, pts_ptr, coords_loc_size*sizeof(double));
  pts_array->SetArray(pts_copy, coords_loc_size, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
  VecRestoreArrayRead(coords_loc, &pts_ptr);
  vtkNew<vtkPoints> pts;
  pts->SetData(pts_array.GetPointer());
  grid->SetPoints(pts.GetPointer());

  PetscInt closure_dof, *closure_indices, elem_size;

  DM cdm;
  PetscCall(DMGetCoordinateDM(dm, &cdm));

  PetscCall(DMPlexGetClosureIndices(cdm, cdm->localSection, cdm->localSection, cStart, PETSC_FALSE, &closure_dof, &closure_indices, NULL, NULL));
  DMPlexRestoreClosureIndices(cdm, cdm->localSection, cdm->localSection, cStart, PETSC_FALSE, &closure_dof, &closure_indices, NULL, NULL);

  PetscInt cSize = closure_dof / coords_dim;
  PetscInt nCells = cEnd - cStart;

  vtkNew<vtkIdTypeArray> connectivity;
  connectivity->SetNumberOfTuples(cSize*nCells);

  vtkNew<vtkIdTypeArray> offsets;
  offsets->SetNumberOfTuples(nCells+1);
  for(PetscInt i=0; i<nCells; i++)
  {
    offsets->SetValue(i, i*cSize);
  }
  offsets->SetValue(nCells, nCells*cSize);

  vtkNew<vtkCellArray> cells;
  cells->SetData(offsets, connectivity);

  static const int HEXA_8_ToVTK[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
  static const int HEXA_27_ToVTK[27] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 16, 17, 18, 19, 12, 13, 14, 15, 24, 22, 21, 23, 20, 25, 26 };
  static const int HEXA_64_ToVTK[64] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 12, 15, 14, 24, 25, 26, 27, 29, 28, 31, 30, 16, 17, 18, 19, 22, 23, 20, 21, 49, 48, 50, 51, 40, 41, 43, 42, 36, 37, 39, 38, 45, 44, 46, 47, 32, 33, 35, 34, 52, 53, 55, 54, 56, 57, 59, 58, 60, 61, 63, 62 };

  const int* translator = nullptr;

  vtkNew<vtkUnsignedCharArray> cellTypes;
  cellTypes->SetNumberOfTuples(nCells);
  if (cSize == 8)
  {
    cellTypes->FillValue(VTK_HEXAHEDRON);
    translator = HEXA_8_ToVTK;
  }
  else if (cSize == 27)
  {
    cellTypes->FillValue(VTK_TRIQUADRATIC_HEXAHEDRON);
    translator = HEXA_27_ToVTK;
  }
  else if (cSize == 64)
  {
    cellTypes->FillValue(VTK_LAGRANGE_HEXAHEDRON);
    translator = HEXA_64_ToVTK;
  }
  else
  {
    std::cerr << "Cell type not supported." << std::endl;
    return 1;
  }
      
  grid->SetCells(cellTypes, cells);
  for(PetscInt cid=cStart, cid0=0; cid<cEnd; cid++, cid0++)
  {
    PetscCall(DMPlexGetClosureIndices(cdm, cdm->localSection, cdm->localSection, cid, PETSC_FALSE, &closure_dof, &closure_indices, NULL, NULL));
    const int *perm;
    PetscCall(DMPlexCGNSGetPermutation_Internal(cell_type, cSize, &perm));
    PetscInt* tmpids = new PetscInt[cSize];
    for(int i=0; i<cSize; i++)
    {
      tmpids[i] = closure_indices[perm[i]*coords_dim]/coords_dim;
    }
    for(int i=0; i<cSize; i++)
    {
      connectivity->SetValue(cid0*cSize + i, tmpids[translator[i]]);
    }
    DMPlexRestoreClosureIndices(cdm, cdm->localSection, cdm->localSection, 0, PETSC_FALSE, &closure_dof, &closure_indices, NULL, NULL);
  }

  vtkDoubleArray* fields = this->LoadSolution(&dm);
  grid->GetPointData()->AddArray(fields);
  fields->Delete();

  PetscCall(DMDestroy(&dm));

  return 1;
}

int main(int argc, char **argv)
{
  PetscInitialize(&argc, &argv, NULL, PETSC_NULLPTR);

  vtkNew<vtkPETScCGNSReader> reader;
  reader->SetFileName("test.cgns");
  reader->Update();

  MPI_Comm comm = PETSC_COMM_WORLD;
  int rank;
  MPI_Comm_rank(comm, &rank);

  if (rank ==0)
  {
    vtkNew<vtkXMLUnstructuredGridWriter> writer;
    writer->SetInputData(reader->GetOutput());
    writer->SetFileName("grid.vtu");
    writer->SetDataModeToAscii();
    writer->Write();
  }

  return 0;
}
