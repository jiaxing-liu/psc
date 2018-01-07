
#include "psc_push_particles_private.h"

#include "psc_particles_as_double.h"
#include "psc_fields_as_c.h"
#include "1vb/psc_push_particles_1vb.h"

// ======================================================================
// psc_push_particles: subclass "1vbec_double"

struct psc_push_particles_ops psc_push_particles_1vbec_double_ops = {
  .name                  = "1vbec_double",
  .push_mprts_xyz        = psc_push_particles_1vbec_double_push_mprts_xyz,
  .push_mprts_yz         = psc_push_particles_1vbec_double_push_mprts_yz,
  .push_mprts_1          = psc_push_particles_1vbec_double_push_mprts_1,
  //  .stagger_mprts_1       = psc_push_particles_1vbec_double_stagger_mprts_1,
  .particles_type        = PARTICLE_TYPE,
  .fields_type           = FIELDS_TYPE,
};

