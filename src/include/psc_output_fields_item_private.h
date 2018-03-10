
#ifndef PSC_OUTPUT_FIELDS_ITEM_PRIVATE_H
#define PSC_OUTPUT_FIELDS_ITEM_PRIVATE_H

#include <psc_output_fields_item.h>

struct psc_output_fields_item {
  struct mrc_obj obj;
  struct psc_bnd *bnd;
};

enum {
  POFI_ADD_GHOSTS = 1, // this item needs to have ghost points added to interior points
  POFI_BY_KIND    = 2, // this item needs to be replicated by kind
};

#define POFI_MAX_COMPS (16)

using fld_names_t = std::array<const char*, POFI_MAX_COMPS>;

struct psc_output_fields_item_ops {
  MRC_SUBCLASS_OPS(struct psc_output_fields_item);
};

#endif
