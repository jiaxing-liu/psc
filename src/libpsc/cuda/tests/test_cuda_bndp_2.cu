
#include "cuda_mparticles.cuh"
#include "cuda_bndp.h"
#include "cuda_test.hxx"

#include <mrc_profile.h>

#include "gtest/gtest.h"

struct prof_globals prof_globals; // FIXME

int
prof_register(const char *name, float simd, int flops, int bytes)
{
  return 0;
}

using CudaMparticles = cuda_mparticles<BS444>;

// ======================================================================
// CudaMparticlesBndTest

struct CudaMparticlesBndTest : TestBase<CudaMparticles>, ::testing::Test
{
  using Double3 = Vec3<double>;
  
  std::unique_ptr<Grid_t> grid;
  std::unique_ptr<CudaMparticles> cmprts_;
  std::unique_ptr<cuda_bndp<CudaMparticles, dim_xyz>> cbndp;

  void SetUp()
  {
    auto domain = Grid_t::Domain{{32, 32, 32}, {320., 320., 320.}, {0., 0., 0.},
				 {2, 2, 2}};
    auto bc = GridBc{};
    auto kinds = Grid_t::Kinds{Grid_t::Kind{1., 1., "k0"},
			       Grid_t::Kind{1., 1., "k1"},
			       Grid_t::Kind{1., 1., "k2"},
			       Grid_t::Kind{1., 1., "k3"}};
    auto norm = Grid_t::Normalization{};
    double dt = .1;
    grid.reset(new Grid_t(domain, bc, kinds, norm, dt));

    cmprts_.reset(new CudaMparticles{*grid});
    auto& cmprts = *cmprts_;

    for (int p = 0; p < cmprts.n_patches; ++p) {
      auto injector = cmprts[p].injector();
      if (p == 0) { // patch 0
	injector(particle_inject{{ .5,  35., 5.}, {}, 0., 0});
	injector(particle_inject{{ .5, 155., 5.}, {}, 0., 1});
      } else if (p == 1) { // patch 1
	injector(particle_inject{{ 160.5,  35., 5.}, {}, 0., 2});
	injector(particle_inject{{ 160.5, 155., 5.}, {}, 0., 3});
      }
    }

    // move every particle one full cell to the right (+y, that is)
    // (position doesn't actually matter since we'll only look at bidx)
    for (int n = 0; n < cmprts.n_prts; n++) {
      auto prt = cmprts.storage.load(n);
      prt.x()[1] += 10.;
      cmprts.storage.store(prt, n);
    }
    auto& d_bidx = cmprts.by_block_.d_idx;
    d_bidx[0] = 4;
    d_bidx[1] = cmprts.n_blocks + 0; // oob p0
    d_bidx[2] = 68;
    d_bidx[3] = cmprts.n_blocks + 1; // oob p1
    
#if 0
    cmprts.dump();
#endif

    cbndp.reset(new cuda_bndp<CudaMparticles, dim_xyz>(*grid));
  }
};


// ----------------------------------------------------------------------
// BndPrep
//
// tests cuda_bndp::prep()

TEST_F(CudaMparticlesBndTest, BndPrep)
{
  cbndp->prep(cmprts_.get());

  // particles 0 and 2 remain in their patch,
  // particles 1 and 3 leave their patch and need special handling
  EXPECT_EQ(cbndp->bpatch[0].buf.size(), 1);
  EXPECT_EQ(cbndp->bpatch[1].buf.size(), 1);
  EXPECT_EQ(cbndp->bpatch[0].buf[0].kind(), 1);
  EXPECT_EQ(cbndp->bpatch[1].buf[0].kind(), 3);
}

// ----------------------------------------------------------------------
// BndPrepDetail
//
// tests the pieces that go into cuda_bndp::prep()

struct is_inside
{
  is_inside(int n_blocks) : n_blocks_(n_blocks) {}
  
  __host__ __device__
  bool operator()(thrust::tuple<uint, float4, float4> tup)
  {
    uint bidx = thrust::get<0>(tup);
    return bidx < n_blocks_;
  }
  
  int n_blocks_;
};

TEST_F(CudaMparticlesBndTest, BndPrepDetail)
{
  auto& cmprts = *this->cmprts_;

  auto& d_bidx = cmprts.by_block_.d_idx;
#if 0
  for (int n = 0; n < cmprts.n_prts; n++) {
    auto prt = cmprts.storage.load(n);
    printf("n %d: %g:%g:%g kind %d bidx %d\n", n, prt.x()[0], prt.x()[1], prt.x()[2], prt.kind(),
	   int(d_bidx[n]));
  }
#endif

  auto begin = thrust::make_zip_iterator(thrust::make_tuple(d_bidx.begin(), cmprts.storage.xi4.begin(), cmprts.storage.pxi4.begin()));
  auto end = thrust::make_zip_iterator(thrust::make_tuple(d_bidx.end(), cmprts.storage.xi4.end(), cmprts.storage.pxi4.end()));
  auto oob = thrust::stable_partition(begin, end, is_inside(cmprts.n_blocks));

#if 0
  for (int n = 0; n < cmprts.n_prts; n++) {
    auto prt = cmprts.storage.load(n);
    printf("n %d: %g:%g:%g kind %d bidx %d\n", n, prt.x()[0], prt.x()[1], prt.x()[2], prt.kind(),
	   int(d_bidx[n]));
  }
#endif

  EXPECT_EQ(oob, begin + 2);
  EXPECT_EQ(cmprts.storage.load(0).kind(), 0);
  EXPECT_EQ(cmprts.storage.load(1).kind(), 2);
  EXPECT_EQ(cmprts.storage.load(2).kind(), 1);
  EXPECT_EQ(cmprts.storage.load(3).kind(), 3);

  cbndp->n_prts_send = end - oob;

  EXPECT_EQ(cmprts.n_prts, 4);
  EXPECT_EQ(cbndp->n_prts_send, 2);

  cmprts.n_prts -= cbndp->n_prts_send;

  // particles 1, 3, which need to be exchanged, should now be at the
  // end of the regular array
  EXPECT_EQ(cmprts.storage.load(cmprts.n_prts  ).kind(), 1);
  EXPECT_EQ(cmprts.storage.load(cmprts.n_prts+1).kind(), 3);

  // test copy_from_dev_and_convert
  cbndp->copy_from_dev_and_convert(&cmprts, cbndp->n_prts_send);

#if 0
  for (int p = 0; p < cmprts.n_patches; p++) {
    printf("from_dev: p %d\n", p);
    for (auto& prt : cbndp->bpatch[p].buf) {
      printf("  prt xyz %g %g %g kind %d\n", prt.xi, prt.yi, prt.zi, prt.kind_);
    }
  }
#endif

  EXPECT_EQ(cbndp->bpatch[0].buf.size(), 1);
  EXPECT_EQ(cbndp->bpatch[1].buf.size(), 1);
  EXPECT_EQ(cbndp->bpatch[0].buf[0].kind(), 1);
  EXPECT_EQ(cbndp->bpatch[1].buf[0].kind(), 3);
}

// ----------------------------------------------------------------------
// BndPost
//
// tests cuda_bndp::post()

TEST_F(CudaMparticlesBndTest, BndPost)
{
  auto& cmprts = *this->cmprts_;
  // BndPost expects the work done by bnd_prep()
  cbndp->prep(&cmprts);

  // particles 0 and 2 remain in their patch,
  // particles 1 and 3 leave their patch and need special handling
  EXPECT_EQ(cbndp->bpatch[0].buf.size(), 1);
  EXPECT_EQ(cbndp->bpatch[1].buf.size(), 1);
  EXPECT_EQ(cbndp->bpatch[0].buf[0].kind(), 1);
  EXPECT_EQ(cbndp->bpatch[1].buf[0].kind(), 3);

  // Mock what the actual boundary exchange does, ie., move
  // particles to their new patch and adjust the relative position.
  // This assumes periodic b.c.
  auto prt1 = cbndp->bpatch[0].buf[0];
  auto prt3 = cbndp->bpatch[1].buf[0];
  prt1.x()[1] -= 40.;
  prt3.x()[1] -= 40.;
  cbndp->bpatch[0].buf[0] = prt3;
  cbndp->bpatch[1].buf[0] = prt1;
  
  cbndp->post(&cmprts);

  // bnd_post doesn't do the actual final reordering
  EXPECT_TRUE(cmprts.need_reorder);
  cmprts.reorder();
  EXPECT_TRUE(cmprts.check_ordered());

#if 0
  cmprts.dump();
#endif
}

// ----------------------------------------------------------------------
// BndPostDetail
//
// tests the pieces that go into cuda_bndp::post()

TEST_F(CudaMparticlesBndTest, BndPostDetail)
{
  auto& cmprts = *this->cmprts_;
  // BndPost expects the work done by bnd_prep()
  cbndp->prep(&cmprts);

  // particles 0 and 2 remain in their patch,
  // particles 1 and 3 leave their patch and need special handling
  EXPECT_EQ(cbndp->bpatch[0].buf.size(), 1);
  EXPECT_EQ(cbndp->bpatch[1].buf.size(), 1);
  EXPECT_EQ(cbndp->bpatch[0].buf[0].kind(), 1);
  EXPECT_EQ(cbndp->bpatch[1].buf[0].kind(), 3);

  // Mock what the actual boundary exchange does, ie., move
  // particles to their new patch and adjust the relative position.
  // This assumes periodic b.c.
  auto prt1 = cbndp->bpatch[0].buf[0];
  auto prt3 = cbndp->bpatch[1].buf[0];
  prt1.x()[1] -= 160.;
  prt3.x()[1] -= 160.;
  cbndp->bpatch[0].buf[0] = prt3;
  cbndp->bpatch[1].buf[0] = prt1;

  // === test convert_and_copy_to_dev()
  uint n_prts_recv = cbndp->convert_and_copy_to_dev(&cmprts);
  cmprts.n_prts += n_prts_recv;

  // n_recv should be set for each patch, and its total
  EXPECT_EQ(cbndp->bpatch[0].n_recv, 1);
  EXPECT_EQ(cbndp->bpatch[1].n_recv, 1);
  EXPECT_EQ(n_prts_recv, 2);

  // the received particle have been appended to the two remaining ones
  EXPECT_EQ(cmprts.n_prts, 4);

  // and the particle have been appended after the old end of the particle list
  int n_prts_old = cmprts.n_prts - n_prts_recv;
  EXPECT_EQ(cmprts.storage.load(n_prts_old  ).kind(), 3);
  EXPECT_EQ(cmprts.storage.load(n_prts_old+1).kind(), 1);

  // block indices have been calculated
  auto& d_bidx = cmprts.by_block_.d_idx;
  EXPECT_EQ(d_bidx[n_prts_old  ], 0);  // 0th block in 0th patch
  EXPECT_EQ(d_bidx[n_prts_old+1], 64); // 0th block in 1st patch

  cmprts.resize(cmprts.n_prts);
  thrust::sequence(cmprts.by_block_.d_id.begin(), cmprts.by_block_.d_id.end());
  thrust::stable_sort_by_key(d_bidx.begin(), d_bidx.end(), cmprts.by_block_.d_id.begin());

  EXPECT_EQ(cmprts.n_prts, 4);
  auto& d_id = cmprts.by_block_.d_id;
  EXPECT_EQ(d_id[0], 2);
  EXPECT_EQ(d_id[1], 0);
  EXPECT_EQ(d_id[2], 3);
  EXPECT_EQ(d_id[3], 1);

  // find offsets
  thrust::counting_iterator<uint> search_begin(0);
  thrust::upper_bound(d_bidx.begin(), d_bidx.end(),
		      search_begin, search_begin + cmprts.n_blocks,
		      cmprts.by_block_.d_off.begin() + 1);
  // d_off[0] was set to zero during d_off initialization
  auto& d_off = cmprts.by_block_.d_off;
  for (int b = 0; b <= cmprts.n_blocks; b++) {
    //if (b < cmprts.n_blocks) printf("b %d: off [%d:%d[\n", b, int(d_off[b]), int(d_off[b+1]));
    if (b < 1) {
      EXPECT_EQ(d_off[b], 0) << "where b = " << b;
    } else if (b < 5) {
      EXPECT_EQ(d_off[b], 1) << "where b = " << b;
    } else if (b < 65) {
      EXPECT_EQ(d_off[b], 2) << "where b = " << b;
    } else if (b < 69) {
      EXPECT_EQ(d_off[b], 3) << "where b = " << b;
    } else {
      EXPECT_EQ(d_off[b], 4) << "where b = " << b;
    }
  }

  cmprts.need_reorder = true;

  // bnd_post doesn't do the actually final reordering, but
  // let's do it here for a final check
  cmprts.reorder();
  EXPECT_TRUE(cmprts.check_ordered());

#if 0
  cmprts.dump();
#endif
}

// ======================================================================
// main

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  ::testing::InitGoogleTest(&argc, argv);
  int rc = RUN_ALL_TESTS();

  MPI_Finalize();
  return rc;
}
