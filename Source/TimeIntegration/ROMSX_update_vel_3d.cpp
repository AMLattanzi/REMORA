#include <ROMSX.H>
#include <Utils.H>

using namespace amrex;

//
// update_vel_3d -- called from prestep_uv_3d
// NOTE: "vel" here represents either u or v
//

void
ROMSX::update_vel_3d (const Box& vel_bx,
                      const int ioff, const int joff,
                      Array4<Real>  vel, Array4<Real> vel_old,
                      Array4<Real> rvel, Array4<Real> Hz,
                      Array4<Real>  Akv,
                      Array4<Real>   DC, Array4<Real> FC,
                      Array4<Real> sstr, Array4<Real> z_r,
                      Array4<Real>   pm, Array4<Real>  pn,
                      const int iic, const int ntfirst, const int nnew, int nstp, int nrhs, int N,
                      const Real lambda, const Real dt_lev)
{
    //
    //  Weighting coefficient for the newest (implicit) time step derivatives
    //  using either a Crack-Nicolson implicit scheme (lambda=0.5) or a
    //  backward implicit scheme (lambda=1.0).
    //

    amrex::Print() << "in update_vel_3d with box " << vel_bx << std::endl;

    Real oml_dt = dt_lev*(1.0-lambda);

    //  Except the commented out part means lambda is always 1.0
    amrex::ParallelFor(vel_bx,
    [=] AMREX_GPU_DEVICE (int i, int j, int k)
    {
        if(k+1<=N&&k>=0)
        {
            Real cff = 1.0 / ( z_r(i,j,k+1)+z_r(i-ioff,j-joff,k+1)
                              -z_r(i,j,k  )-z_r(i-ioff,j-joff,k  ));
            FC(i,j,k) = oml_dt * cff * (vel_old(i,j,k+1,nstp)-vel_old(i,j,k,nstp)) *
                                           (Akv(i,j,k)     +Akv(i-ioff,j-joff,k));
        }
        else
        {
            //  FC(i,j,-1) = 0.0;//dt_lev*bustr(i,j,0);
            //  FC(i,j, N) = 0.0;//dt_lev*sstr(i,j,0);
        }

        DC(i,j,k) = 0.25 * dt_lev * (pm(i,j,0)+pm(i-ioff,j-joff,0))
                                      * (pn(i,j,0)+pn(i-ioff,j-joff,0));
    });

    amrex::ParallelFor(vel_bx,
    [=] AMREX_GPU_DEVICE (int i, int j, int k)
    {
        Real cff3=dt_lev*(1.0-lambda);
        Real cff1 = 0.0, cff2 = 0.0, cff4;

        int indx=0; //nrhs-3

        if (iic==ntfirst)
        {
            //Hz still might need adjusting
            if (k+1<=N && k>=1)
            {
                cff1=vel_old(i,j,k,nstp)*0.5*(Hz(i,j,k)+Hz(i-ioff,j-joff,k));
                cff2=FC(i,j,k)-FC(i,j,k-1);
                vel(i,j,k,nnew)=cff1+cff2;
            }
            else if(k==0)
            {
                cff1=vel_old(i,j,k,nstp)*0.5*(Hz(i,j,k)+Hz(i-ioff,j-joff,k));
                cff2=FC(i,j,k);//-bustr(i,j,0);
                vel(i,j,k,nnew)=cff1+cff2;
            }
            else if(k==N)
            {
                cff1=vel_old(i,j,k,nstp)*0.5*(Hz(i,j,k)+Hz(i-ioff,j-joff,k));
                cff2=-FC(i,j,k-1)+dt_lev*sstr(i,j,0);
                vel(i,j,k,nnew)=cff1+cff2;
            }

        } else if(iic==ntfirst+1) {

            if (k<N && k>0) {
                cff1=vel_old(i,j,k,nstp)*0.5*(Hz(i,j,k)+Hz(i-ioff,j-joff,k));
                cff2=FC(i,j,k)-FC(i,j,k-1);
            }
            else if(k==0) {
                cff1=vel_old(i,j,k,nstp)*0.5*(Hz(i,j,k)+Hz(i-ioff,j-joff,k));
                cff2=FC(i,j,k);//-bustr(i,j,0);
            }
            else if(k==N) {
                cff1=vel_old(i,j,k,nstp)*0.5*(Hz(i,j,k)+Hz(i-ioff,j-joff,k));
                cff2=-FC(i,j,k-1)+dt_lev*sstr(i,j,0);
            }

            cff3=0.5*DC(i,j,k);
	    if(ioff==0&&joff==0)
            vel(i,j,k,nnew)=cff1 + cff2;
	    else {
            indx=nrhs ? 0 : 1;
            Real r_swap= rvel(i,j,k,indx);
            rvel(i,j,k,indx) = rvel(i,j,k,nrhs);
            rvel(i,j,k,nrhs) = r_swap;
            vel(i,j,k,nnew)=cff1- cff3*rvel(i,j,k,indx)+ cff2; }

        } else {

            cff1= 5.0/12.0;
            cff2=16.0/12.0;
            if (k==0) {
                cff3=vel_old(i,j,k,nstp)*0.5*(Hz(i,j,k)+Hz(i-ioff,j-joff,k));
                cff4=FC(i,j,k);//-bustr(i,j,0);

            } else if (k == N) {
                cff3=vel_old(i,j,k,nstp)*0.5*(Hz(i,j,k)+Hz(i-ioff,j-joff,k));
                cff4=-FC(i,j,k-1)+dt_lev*sstr(i,j,0);

            } else {
                cff3=vel_old(i,j,k,nstp)*0.5*(Hz(i,j,k)+Hz(i-ioff,j-joff,k));
                cff4=FC(i,j,k)-FC(i,j,k-1);
            }
	    if(ioff==0&&joff==0)
            vel(i,j,k,nnew)=cff3 + cff4;
	    else {
            indx=nrhs ? 0 : 1;
            Real r_swap= rvel(i,j,k,indx);
            rvel(i,j,k,indx) = rvel(i,j,k,nrhs);
            rvel(i,j,k,nrhs) = r_swap;

            vel(i,j,k,nnew)=cff3+
                DC(i,j,k)*(cff1*rvel(i,j,k,nrhs)-
                           cff2*rvel(i,j,k,indx))+
                cff4;
            rvel(i,j,k,nrhs) = 0.0; }
        }
    });
}
