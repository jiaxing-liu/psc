
#include "psc_bnd_fields_private.h"

#include "psc.h"
#include "psc_glue.h"
#include <mrc_profile.h>

// ----------------------------------------------------------------------
// psc_bnd_fields_fortran_fill_ghosts_b_H

static void
psc_bnd_fields_fortran_fill_ghosts_b_H(struct psc_bnd_fields *bnd,
				       struct psc_mfields *mflds_base)
{
  struct psc_mfields *mflds = psc_mfields_get_as(mflds_base, "fortran", JXI, HZ + 1);
  
  static int pr;
  if (!pr) {
    pr = prof_register("fort_fill_H_b", 1., 0, 0);
  }
  prof_start(pr);
  for (int p = 0; p < mflds->nr_patches; p++) {
    PIC_fill_ghosts_h_b(ppsc, p, psc_mfields_get_patch(mflds, p));
  }
  prof_stop(pr);
  
  psc_mfields_put_as(mflds, mflds_base, HX, HZ + 1);
}

// ======================================================================
// psc_bnd_fields: subclass "fortran"

struct psc_bnd_fields_ops psc_bnd_fields_fortran_ops = {
  .name                  = "fortran",
  .fill_ghosts_b_H       = psc_bnd_fields_fortran_fill_ghosts_b_H,
};
