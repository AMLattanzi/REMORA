#include "AMReX_PhysBCFunct.H"
#include <REMORA_PhysBCFunct.H>

using namespace amrex;

//
// dest_arr is the Array4 to be filled
// time is the time at which the data should be filled
// bccomp is the index into both domain_bcs_type_bcr and bc_extdir_vals
//     so this follows the BCVars enum
//
void REMORAPhysBCFunct::impose_xvel_bcs (const Array4<Real>& dest_arr, const Box& bx, const Box& domain,
                                        const GpuArray<Real,AMREX_SPACEDIM> /*dxInv*/, const Array4<const Real>& msku,
                                        Real /*time*/, int bccomp)
{
    BL_PROFILE_VAR("impose_xvel_bcs()",impose_xvel_bcs);
    const auto& dom_lo = amrex::lbound(domain);
    const auto& dom_hi = amrex::ubound(domain);

    // Based on BCRec for the domain, we need to make BCRec for this Box
    // bccomp is used as starting index for m_domain_bcs_type
    //      0 is used as starting index for bcrs
    int ncomp = 1;
    Vector<BCRec> bcrs(ncomp);
    amrex::setBC(bx, domain, bccomp, 0, ncomp, m_domain_bcs_type, bcrs);

    // xlo: ori = 0
    // ylo: ori = 1
    // zlo: ori = 2
    // xhi: ori = 3
    // yhi: ori = 4
    // zhi: ori = 5

    amrex::Gpu::DeviceVector<BCRec> bcrs_d(ncomp);
#ifdef AMREX_USE_GPU
    Gpu::htod_memcpy_async(bcrs_d.data(), bcrs.data(), sizeof(BCRec)*ncomp);
#else
    std::memcpy(bcrs_d.data(), bcrs.data(), sizeof(BCRec)*ncomp);
#endif
    const amrex::BCRec* bc_ptr = bcrs_d.data();

    GpuArray<GpuArray<Real, AMREX_SPACEDIM*2>,AMREX_SPACEDIM+NCONS> l_bc_extdir_vals_d;

    for (int i = 0; i < ncomp; i++)
        for (int ori = 0; ori < 2*AMREX_SPACEDIM; ori++)
            l_bc_extdir_vals_d[i][ori] = m_bc_extdir_vals[bccomp+i][ori];

    GeometryData const& geomdata = m_geom.data();
    bool is_periodic_in_x = geomdata.isPeriodic(0);
    bool is_periodic_in_y = geomdata.isPeriodic(1);

    // First do all ext_dir bcs
    if (!is_periodic_in_x or bccomp==BCVars::foextrap_bc)
    {
        Box bx_xlo(bx);  bx_xlo.setBig  (0,dom_lo.x-1);
        Box bx_xhi(bx);  bx_xhi.setSmall(0,dom_hi.x+2);
        Box bx_xlo_face(bx); bx_xlo_face.setSmall(0,dom_lo.x  ); bx_xlo_face.setBig(0,dom_lo.x  );
        Box bx_xhi_face(bx); bx_xhi_face.setSmall(0,dom_hi.x+1); bx_xhi_face.setBig(0,dom_hi.x+1);
        ParallelFor(
            bx_xlo, ncomp, [=] AMREX_GPU_DEVICE (int i, int j, int k, int n) {
                int iflip = dom_lo.x - i;
                if (bc_ptr[n].lo(0) == REMORABCType::ext_dir) {
                    dest_arr(i,j,k) = l_bc_extdir_vals_d[n][0]*msku(i,j,0);
                } else if (bc_ptr[n].lo(0) == REMORABCType::foextrap) {
                    dest_arr(i,j,k) =  dest_arr(dom_lo.x,j,k);
                } else if (bc_ptr[n].lo(0) == REMORABCType::reflect_even) {
                    dest_arr(i,j,k) =  dest_arr(iflip,j,k);
                } else if (bc_ptr[n].lo(0) == REMORABCType::reflect_odd) {
                    dest_arr(i,j,k) = -dest_arr(iflip,j,k);
                }
            },
            // We only set the values on the domain faces themselves if EXT_DIR
            bx_xlo_face, ncomp, [=] AMREX_GPU_DEVICE (int i, int j, int k, int n) {
                if (bc_ptr[n].lo(0) == REMORABCType::ext_dir)
                    dest_arr(i,j,k) = l_bc_extdir_vals_d[n][0]*msku(i,j,0);
            }
        );
        ParallelFor(
            bx_xhi, ncomp, [=] AMREX_GPU_DEVICE (int i, int j, int k, int n) {
                int iflip =  2*(dom_hi.x + 1) - i;
                if (bc_ptr[n].hi(0) == REMORABCType::ext_dir) {
                    dest_arr(i,j,k) = l_bc_extdir_vals_d[n][3]*msku(i,j,0);
                } else if (bc_ptr[n].hi(0) == REMORABCType::foextrap) {
                    dest_arr(i,j,k) =  dest_arr(dom_hi.x+1,j,k);
                } else if (bc_ptr[n].hi(0) == REMORABCType::reflect_even) {
                    dest_arr(i,j,k) =  dest_arr(iflip,j,k);
                } else if (bc_ptr[n].hi(0) == REMORABCType::reflect_odd) {
                    dest_arr(i,j,k) = -dest_arr(iflip,j,k);
                }
            },
            // We only set the values on the domain faces themselves if EXT_DIR
            bx_xhi_face, ncomp, [=] AMREX_GPU_DEVICE (int i, int j, int k, int n) {
                if (bc_ptr[n].lo(3) == REMORABCType::ext_dir)
                    dest_arr(i,j,k) = l_bc_extdir_vals_d[n][3]*msku(i,j,0);
            }
        );
    } // not periodic in x

    if (!is_periodic_in_y or bccomp==BCVars::foextrap_bc)
    {
        // Populate ghost cells on lo-y and hi-y domain boundaries
        Box bx_ylo(bx);  bx_ylo.setBig  (1,dom_lo.y-1);
        Box bx_yhi(bx);  bx_yhi.setSmall(1,dom_hi.y+1);
        ParallelFor(
            bx_ylo, ncomp, [=] AMREX_GPU_DEVICE (int i, int j, int k, int n) {
                int jflip = dom_lo.y - 1 - j;
                if (bc_ptr[n].lo(1) == REMORABCType::ext_dir) {
                    dest_arr(i,j,k) = l_bc_extdir_vals_d[n][1]*msku(i,j,0);
                } else if (bc_ptr[n].lo(1) == REMORABCType::foextrap) {
                    dest_arr(i,j,k) =  dest_arr(i,dom_lo.y,k);
                } else if (bc_ptr[n].lo(1) == REMORABCType::reflect_even) {
                    dest_arr(i,j,k) =  dest_arr(i,jflip,k);
                } else if (bc_ptr[n].lo(1) == REMORABCType::reflect_odd) {
                    dest_arr(i,j,k) = -dest_arr(i,jflip,k);
                }
            },
            bx_yhi, ncomp, [=] AMREX_GPU_DEVICE (int i, int j, int k, int n) {
                int jflip =  2*dom_hi.y + 1 - j;
                if (bc_ptr[n].hi(1) == REMORABCType::ext_dir) {
                    dest_arr(i,j,k) = l_bc_extdir_vals_d[n][4]*msku(i,j,0);
                } else if (bc_ptr[n].hi(1) == REMORABCType::foextrap) {
                    dest_arr(i,j,k) =  dest_arr(i,dom_hi.y,k);
                } else if (bc_ptr[n].hi(1) == REMORABCType::reflect_even) {
                    dest_arr(i,j,k) =  dest_arr(i,jflip,k);
                } else if (bc_ptr[n].hi(1) == REMORABCType::reflect_odd) {
                    dest_arr(i,j,k) = -dest_arr(i,jflip,k);
                }
            }
        );
    } // not periodic in y

    {
        // Populate ghost cells on lo-z and hi-z domain boundaries
        Box bx_zlo(bx);  bx_zlo.setBig  (2,dom_lo.z-1);
        Box bx_zhi(bx);  bx_zhi.setSmall(2,dom_hi.z+1);
        ParallelFor(
            bx_zlo, ncomp, [=] AMREX_GPU_DEVICE (int i, int j, int k, int n) {
                int kflip = dom_lo.z - 1 - k;
                if (bc_ptr[n].lo(2) == REMORABCType::ext_dir) {
                    dest_arr(i,j,k) = l_bc_extdir_vals_d[n][2]*msku(i,j,0);
                } else if (bc_ptr[n].lo(2) == REMORABCType::foextrap) {
                    dest_arr(i,j,k) =  dest_arr(i,j,dom_lo.z);
                } else if (bc_ptr[n].lo(2) == REMORABCType::reflect_even) {
                    dest_arr(i,j,k) =  dest_arr(i,j,kflip);
                } else if (bc_ptr[n].lo(2) == REMORABCType::reflect_odd) {
                    dest_arr(i,j,k) = -dest_arr(i,j,kflip);
                }
            },
            bx_zhi, ncomp, [=] AMREX_GPU_DEVICE (int i, int j, int k, int n) {
                int kflip =  2*dom_hi.z + 1 - k;
                if (bc_ptr[n].hi(2) == REMORABCType::ext_dir) {
                    dest_arr(i,j,k) = l_bc_extdir_vals_d[n][5]*msku(i,j,0);
                } else if (bc_ptr[n].hi(2) == REMORABCType::foextrap) {
                    dest_arr(i,j,k) =  dest_arr(i,j,dom_hi.z);
                } else if (bc_ptr[n].hi(2) == REMORABCType::reflect_even) {
                    dest_arr(i,j,k) =  dest_arr(i,j,kflip);
                } else if (bc_ptr[n].hi(2) == REMORABCType::reflect_odd) {
                    dest_arr(i,j,k) = -dest_arr(i,j,kflip);
                }
            }
        );
    } // z


    Gpu::streamSynchronize();
}
