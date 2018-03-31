
#ifndef PSC_MARDER_PRIVATE_H
#define PSC_MARDER_PRIVATE_H

#include <psc_marder.h>
#include "fields3d.hxx"
#include "particles.hxx"

struct psc_marder {
  struct mrc_obj obj;
  // parameters
  int every_step; //< do Marder correction every so many steps
  double diffusion; //< diffusion coefficient for Marder correction
  int loop; //< execute this many relaxation steps in a loop
  bool dump; //< dump div_E, rho

  int clean_div_e_interval;
  int clean_div_b_interval;
  int sync_shared_interval;
  int num_div_e_round;
  int num_div_b_round;
};

struct psc_marder_ops {
  MRC_SUBCLASS_OPS(struct psc_marder);
};

#endif
