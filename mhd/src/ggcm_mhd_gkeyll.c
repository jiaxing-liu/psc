/* 
 * utility functions relevant to an mrc_fld with gkeyll data
 */

#include <mrc_fld.h>
#include <ggcm_mhd_gkeyll.h>

// ----------------------------------------------------------------------
// mrc_fld_gkeyll_nr_moments

int
mrc_fld_gkeyll_nr_moments(struct mrc_fld *f)
{
  int nr_moments = 0;
  mrc_fld_get_param_int(f, "nr_moments", &nr_moments);
  return nr_moments;
}

// ----------------------------------------------------------------------
// mrc_fld_gkeyll_nr_moments

int
mrc_fld_gkeyll_nr_fluids(struct mrc_fld *f)
{
  int nr_fluids = 0;
  mrc_fld_get_param_int(f, "nr_fluids", &nr_fluids);
  return nr_fluids;
}

// ----------------------------------------------------------------------
// mrc_fld_gkeyll_mass_ratios
// the output mass_ratio is an array of length nr_flds

void
mrc_fld_gkeyll_mass_ratios(struct mrc_fld *f, float mass_ratios[])
{
  mrc_fld_get_param_float_array(f, "mass_ratios", mass_ratios);
}

// ----------------------------------------------------------------------
// mrc_fld_gkeyll_momentum_ratios
// the output momentum_ratio is an array of length nr_flds

void
mrc_fld_gkeyll_momentum_ratios(struct mrc_fld *f, float momentum_ratios[])
{
  mrc_fld_get_param_float_array(f, "momentum_ratios", momentum_ratios);
}

// ----------------------------------------------------------------------
// mrc_fld_gkeyll_pressure_ratios
// the output pressure_ratio is an array of length nr_flds

void
mrc_fld_gkeyll_pressure_ratios(struct mrc_fld *f, float pressure_ratios[])
{
  mrc_fld_get_param_float_array(f, "pressure_ratios", pressure_ratios);
}

// ----------------------------------------------------------------------
// mrc_fld_gkeyll_set_nr_moments

void
mrc_fld_gkeyll_set_nr_moments(struct mrc_fld *f, int nr_moments)
{
  // TODO: overwrite existing value
  mrc_fld_dict_add_int(f, "nr_moments", nr_moments);
}

// ----------------------------------------------------------------------
// mrc_fld_gkeyll_set_nr_moments

void
mrc_fld_gkeyll_set_nr_fluids(struct mrc_fld *f, int nr_fluids)
{
  mrc_fld_dict_add_int(f, "nr_fluids", nr_fluids);
}

// ----------------------------------------------------------------------
// mrc_fld_gkeyll_set_mass_ratios
//
// the input mass_ratio is an array of length nr_flds

void
mrc_fld_gkeyll_set_mass_ratios(struct mrc_fld *f, float mass_ratios[])
{
  mrc_fld_dict_add_float_array(f, "mass_ratios",
      mass_ratios, mrc_fld_gkeyll_nr_fluids(f));
}

// ----------------------------------------------------------------------
// mrc_fld_gkeyll_set_momentum_ratios
//
// the input momentum_ratio is an array of length nr_flds

void
mrc_fld_gkeyll_set_momentum_ratios(struct mrc_fld *f, float momentum_ratios[])
{
  mrc_fld_dict_add_float_array(f, "momentum_ratios",
      momentum_ratios, mrc_fld_gkeyll_nr_fluids(f));
}

// ----------------------------------------------------------------------
// mrc_fld_gkeyll_set_pressure_ratios
//
// the input pressure_ratio is an array of length nr_flds

void
mrc_fld_gkeyll_set_pressure_ratios(struct mrc_fld *f, float pressure_ratios[])
{
  mrc_fld_dict_add_float_array(f, "pressure_ratios",
      pressure_ratios, mrc_fld_gkeyll_nr_fluids(f));
}

// ----------------------------------------------------------------------
// mrc_fld_gkeyll_electron_index_two_fluids
//
// first index of the electron moments for a two-fluid case

int
mrc_fld_gkeyll_electron_index_two_fluids(struct mrc_fld *f, int m_beg)
{
  assert(mrc_fld_gkeyll_nr_fluids(f) == 2);
  return m_beg;
}

// ----------------------------------------------------------------------
// mrc_fld_gkeyll_ion_index_two_fluids
//
// first index of the electron moments for a two-fluid case

int
mrc_fld_gkeyll_ion_index_two_fluids(struct mrc_fld *f, int m_beg)
{
  assert(mrc_fld_gkeyll_nr_fluids(f) == 2);
  return m_beg + mrc_fld_gkeyll_nr_moments(f);
}

// ----------------------------------------------------------------------
// mrc_fld_gkeyll_species_index
//
// first index of a species

int
gkeyll_species_index(int m_beg, int species, int nr_moments)
{
  return m_beg + nr_moments * species;
}

int
mrc_fld_gkeyll_species_index(struct mrc_fld *f, int m_beg, int species)
{
  // species starts from 0 to nr_fluids-1
  assert(species < mrc_fld_gkeyll_nr_fluids(f));
  return m_beg + mrc_fld_gkeyll_nr_moments(f) * species;
}

// ----------------------------------------------------------------------
// mrc_fld_gkeyll_species_index_all
//
// first index of all species

void
gkeyll_species_index_all(int m_beg, int indices[], int nr_fluids, int nr_moments)
{
  for (int s = 0; s < nr_fluids; s++) {
    indices[s] = m_beg + nr_moments * s;
  }
}

void
mrc_fld_gkeyll_species_index_all(struct mrc_fld *f, int m_beg, int *indices)
{
  for ( int s = 0; s < mrc_fld_gkeyll_nr_fluids(f); s++) {
    indices[s] = mrc_fld_gkeyll_species_index(f, m_beg, s);
  }
}

// ----------------------------------------------------------------------
// mrc_fld_gkeyll_em_index
//
// first index of the EM fields

int
gkeyll_em_index(int m_beg, int nr_fluids, int nr_moments)
{
  return m_beg +nr_moments * nr_fluids;
}

int
mrc_fld_gkeyll_em_index(struct mrc_fld *f, int m_beg)
{
  return m_beg + mrc_fld_gkeyll_nr_moments(f) * mrc_fld_gkeyll_nr_fluids(f);
}

