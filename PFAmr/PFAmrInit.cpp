#include <stdlib.h>
#include <time.h>
#include <AMReX_MultiFabUtil.H>
#include <PFAmr.H>

#include <PFAmrBC.H>

using namespace amrex;

void
PFAmr::InitData ()
{
    if (restart_chkfile.empty())
    {
	const Real time = 0.0;
	InitFromScratch(time);
	for (int lev = finest_level-1; lev >= 0; --lev)
	  {
	    amrex::average_down(*phi_new[0][lev+1], *phi_new[0][lev],
				geom[lev+1], geom[lev],
				0, phi_new[0][lev]->nComp(), refRatio(lev));
	  }

	if (plot_int > 0) {
	    WritePlotFile();
	}
    }
    else
    {
	InitFromCheckpoint();
    }
}


#if BL_SPACEDIM==2
#define INTVECT amrex::IntVect(i,j)
#elif BL_SPACEDIM==3
#define INTVECT amrex::IntVect(i,j,k)
#endif
void PFAmr::MakeNewLevelFromScratch (int lev, Real t, const BoxArray& ba,
				      const DistributionMapping& dm)
{
  const int nghost = 1;

  phi_new[0][lev].reset(new MultiFab(ba, dm, number_of_grains+2, nghost));
  phi_old[0][lev].reset(new MultiFab(ba, dm, number_of_grains+2, nghost));


  t_new[lev] = t;
  t_old[lev] = t - 1.e200;

  const Real* dx = geom[lev].CellSize();
  const Real* prob_lo = geom[lev].ProbLo();
  Real cur_time = t_new[lev];
  for (MFIter mfi(*phi_new[0][lev],true); mfi.isValid(); ++mfi)
    {
      const Box& box = mfi.tilebox();

      amrex::BaseFab<Real> &phi_box = (*phi_new[0][lev])[mfi];
      amrex::BaseFab<Real> &phi_box_old = (*phi_old[0][lev])[mfi];

      for (int i = box.loVect()[0]-nghost; i<=box.hiVect()[0]+nghost; i++) 
       	for (int j = box.loVect()[1]-nghost; j<=box.hiVect()[1]+nghost; j++)
#if BL_SPACEDIM==3
	  for (int k = box.loVect()[2]-nghost; k<=box.hiVect()[2]+nghost; k++)
#endif
	    {
	      amrex::Real x = geom[lev].ProbLo()[0] + ((amrex::Real)(i) + 0.5) * geom[lev].CellSize()[0];
	      amrex::Real y = geom[lev].ProbLo()[1] + ((amrex::Real)(j) + 0.5) * geom[lev].CellSize()[1];
#if BL_SPACEDIM==3
	      amrex::Real z = geom[lev].ProbLo()[2] + ((amrex::Real)(k) + 0.5) * geom[lev].CellSize()[2];
#endif
	      
	      // voronoi
	      amrex::Real len_x = (geom[0].ProbHi(0)-geom[0].ProbLo(0));
	      amrex::Real len_y = (geom[0].ProbHi(1)-geom[0].ProbLo(1));
#if BL_SPACEDIM==3
	      amrex::Real len_z = (geom[0].ProbHi(2)-geom[0].ProbLo(2));
#endif

	      amrex::Real min_distance = std::numeric_limits<amrex::Real>::infinity();
	      int min_grain_id = -1;
	      for (int n = 0; n<number_of_grains; n++)
		{
		  phi_box(INTVECT,n) = 0.;     // initialize
		  phi_box_old(INTVECT,n) = 0.; // good practice to initialize all new memory


		  
		  for (amrex::Real offset_x = -len_x; offset_x <= len_x; offset_x += len_x)
		    for (amrex::Real offset_y = -len_y; offset_y <= len_y; offset_y += len_y)
#if BL_SPACEDIM==3
		      for (amrex::Real offset_z = -len_z; offset_z <= len_z; offset_z += len_z)
#endif
			{
#if BL_SPACEDIM==2
			  amrex::Real d = sqrt((x-voronoi_x[n]-offset_x)*(x-voronoi_x[n]-offset_x) + (y-voronoi_y[n]-offset_y)*(y-voronoi_y[n]-offset_y));
#elif BL_SPACEDIM==3
			  amrex::Real d = sqrt((x-voronoi_x[n]-offset_x)*(x-voronoi_x[n]-offset_x) + (y-voronoi_y[n]-offset_y)*(y-voronoi_y[n]-offset_y) + (z-voronoi_z[n]-offset_z)*(z-voronoi_z[n]-offset_z));
#endif
			  if (d<min_distance )  {min_distance = d;  min_grain_id = n;}
			}

		  // amrex::Real d = sqrt((x-voronoi_x[n])*(x-voronoi_x[n])                  + (y-voronoi_y[n])*(y-voronoi_y[n]));
		  // amrex::Real d1 = sqrt((x-(voronoi_x[n]-len_x))*(x-(voronoi_x[n]-len_x)) + (y-voronoi_y[n])*(y-voronoi_y[n]));
		  // amrex::Real d2 = sqrt((x-(voronoi_x[n]+len_x))*(x-(voronoi_x[n]+len_x)) + (y-voronoi_y[n])*(y-voronoi_y[n]));
		  // amrex::Real d3 = sqrt((x-voronoi_x[n])*(x-voronoi_x[n])                 + (y-(voronoi_y[n]-len_y))*(y-(voronoi_y[n]-len_y)));
		  // amrex::Real d4 = sqrt((x-voronoi_x[n])*(x-voronoi_x[n])                 + (y-(voronoi_y[n]+len_y))*(y-(voronoi_y[n]+len_y)));
		  // amrex::Real d5 = sqrt((x-(voronoi_x[n]-len_x))*(x-(voronoi_x[n]-len_x)) + (y-(voronoi_y[n]-len_y))*(y-(voronoi_y[n]-len_y)));
		  // amrex::Real d6 = sqrt((x-(voronoi_x[n]-len_x))*(x-(voronoi_x[n]-len_x)) + (y-(voronoi_y[n]+len_y))*(y-(voronoi_y[n]+len_y)));
		  // amrex::Real d7 = sqrt((x-(voronoi_x[n]+len_x))*(x-(voronoi_x[n]+len_x)) + (y-(voronoi_y[n]-len_y))*(y-(voronoi_y[n]-len_y)));
		  // amrex::Real d8 = sqrt((x-(voronoi_x[n]+len_x))*(x-(voronoi_x[n]+len_x)) + (y-(voronoi_y[n]+len_y))*(y-(voronoi_y[n]+len_y)));

		  // if (d<min_distance )  {min_distance = d;  min_grain_id = n;}
		  // if (d1<min_distance ) {min_distance = d1; min_grain_id = n;}
		  // if (d2<min_distance ) {min_distance = d2; min_grain_id = n;}
		  // if (d3<min_distance ) {min_distance = d3; min_grain_id = n;}
		  // if (d4<min_distance ) {min_distance = d4; min_grain_id = n;}
		  // if (d5<min_distance ) {min_distance = d5; min_grain_id = n;}
		  // if (d6<min_distance ) {min_distance = d6; min_grain_id = n;}
		  // if (d7<min_distance ) {min_distance = d7; min_grain_id = n;}
		  // if (d8<min_distance ) {min_distance = d8; min_grain_id = n;}
		}
	      phi_box(INTVECT,min_grain_id) = 1.;
	    
	      phi_box(INTVECT,number_of_grains) = (amrex::Real)min_grain_id;
	      phi_box(INTVECT,number_of_grains+1) = 0;

	      phi_box_old(INTVECT,number_of_grains) = 0;   // Good practice to initialize
	      phi_box_old(INTVECT,number_of_grains+1) = 0; // all newly created data
	    }
    }

  PFAmrPhysBC physbc(geom[lev]);
  physbc.FillBoundary(*phi_new[0][lev],0,0,t);
  physbc.FillBoundary(*phi_old[0][lev],0,0,t);
}
