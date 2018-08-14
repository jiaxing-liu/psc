
#pragma once

#include "../libpsc/vpic/VpicGridBase.h"
#include "../libpsc/vpic/VpicMaterial.h"

#include "Field3D.h"

#include "field_advance/field_advance.h"
#define IN_sfa
#include "field_advance/standard/sfa_private.h"

#include <mrc_common.h>
#include <cassert>

// ======================================================================
// VpicSfaParams

struct VpicSfaParams: sfa_params_t
{
};

// ======================================================================
// VpicFieldArrayBase

template<class G, class ML>
struct VpicFieldArrayBase : field_array_t {
  typedef G Grid;
  typedef ML MaterialList;
  typedef field_t Element;
  typedef VpicSfaParams SfaParams;
  typedef material_coefficient_t MaterialCoefficient;
  
  enum {
    EX  = 0,
    EY  = 1,
    EZ  = 2,
    CBX = 4,
    CBY = 5,
    CBZ = 6,
    N_COMP = sizeof(field_t) / sizeof(float),
  };

 public:
  // These operators can be used to access the field directly,
  // though the performance isn't great, so one you use Field3D
  // when performance is important
  float operator()(int m, int i, int j, int k) const
  {
    float *f_ = &f[0].ex;
    return f_[VOXEL(i,j,k, g->nx,g->ny,g->nz) * N_COMP + m];
  }
  
  float& operator()(int m, int i, int j, int k)
  {
    float *f_ = &f[0].ex;
    return f_[VOXEL(i,j,k, g->nx,g->ny,g->nz) * N_COMP + m];
  }

  Element  operator[](int idx) const { return f[idx]; }
  Element& operator[](int idx)       { return f[idx]; }

  Element* data() { return f; }
  
  Grid* grid() { return static_cast<Grid*>(g); }
  SfaParams& params() { return *static_cast<SfaParams*>(field_array_t::params); }
  
  // I'm keeping these for now, because I tink they're a nice interface,
  // but it doesn't scale well to other kinds of fields (as one can tell
  // from the macro use...)
#define MK_COMP_ACCESSOR(cbx)			\
  float cbx(int i, int j, int k) const		\
  {						\
    const int nx = g->nx, ny = g->ny;		\
    return f[VOXEL(i,j,k, nx,ny,nz)].cbx;	\
  }						\
						\
  float& cbx(int i, int j, int k)		\
  {						\
    const int nx = g->nx, ny = g->ny;		\
    return f[VOXEL(i,j,k, nx,ny,nz)].cbx;	\
  }

  MK_COMP_ACCESSOR(cbx)
  MK_COMP_ACCESSOR(cby)
  MK_COMP_ACCESSOR(cbz)
  MK_COMP_ACCESSOR(ex)
  MK_COMP_ACCESSOR(ey)
  MK_COMP_ACCESSOR(ez)
};


// ======================================================================
// MfieldsStateVpic

struct MfieldsStateVpic
{
  using real_t = float;
  using Grid = VpicGridBase;
  using MaterialList = VpicMaterialList;
  using FieldArray = VpicFieldArrayBase<Grid, MaterialList>;

  enum {
    EX = 0, EY = 1, EZ = 2, DIV_E_ERR = 3,
    BX = 4, BY = 5, BZ = 6, DIV_B_ERR = 7,
    TCAX = 8, TCAY = 9, TCAZ = 10, RHOB = 11,
    JFX = 12, JFY = 13, JFZ = 14, RHOF = 15,
    N_COMP = 20,
  };

  struct Patch
  {
    using Element = field_t;
    
    Element* data() { return fa->data(); }
    Grid* grid() { return fa->grid(); }
    field_t  operator[](int idx) const { return (*fa)[idx]; }
    field_t& operator[](int idx)       { return (*fa)[idx]; }
    
    FieldArray* fa;
  };
    
  using fields_t = fields3d<float, LayoutAOS>;

  MfieldsStateVpic(const Grid_t& grid, Grid* vgrid, const MaterialList& material_list, double damp = 0.)
    : grid_{grid}
  {
    patch_.fa = static_cast<FieldArray*>(::new_standard_field_array(vgrid, material_list, damp));
    assert(grid.n_patches() == 1);

    const int B = 1; // VPIC always uses one ghost cell (on c.c. grid)
    im_ = { vgrid->nx + 2*B, vgrid->ny + 2*B, vgrid->nz + 2*B };
    ib_ = { -B, -B, -B };
  }

  real_t* data() { return reinterpret_cast<real_t*>(patch_.data()); }
  fields_t operator[](int p) { return {ib_, im_, N_COMP, data()}; }
  Patch& getPatch(int p) { return patch_; }

  Grid* vgrid() { return patch_.grid(); }

  operator FieldArray*() { return patch_.fa; }

  const Grid_t& grid() const { return grid_; }
  int n_patches() const { return grid_.n_patches(); }
  int n_comps() const { return N_COMP; }
  Int3 ibn() const { return {1,1,1}; }
  
private:
  const Grid_t& grid_;
  Patch patch_;
  Int3 ib_, im_;
};

