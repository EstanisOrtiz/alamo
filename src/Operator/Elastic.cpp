#include "Model/Solid/LinearElastic/Isotropic.H"
#include "Model/Solid/LinearElastic/Cubic.H"
#include "Model/Solid/LinearElastic/Laplacian.H"
#include "Model/Solid/LinearElastic/Degradable/Isotropic.H"
#include "Model/Solid/Viscoelastic/Isotropic.H"
#include "Elastic.H"


namespace Operator
{
template<class T>
Elastic<T>::Elastic (const Vector<Geometry>& a_geom,
		     const Vector<BoxArray>& a_grids,
		     const Vector<DistributionMapping>& a_dmap,
		     const LPInfo& a_info)
{
	BL_PROFILE("Operator::Elastic::Elastic()");

	define(a_geom, a_grids, a_dmap, a_info);
}

template<class T>
Elastic<T>::~Elastic ()
{}

template<class T>
void
Elastic<T>::define (const Vector<Geometry>& a_geom,
		    const Vector<BoxArray>& a_grids,
		    const Vector<DistributionMapping>& a_dmap,
		    const LPInfo& a_info,
		    const Vector<FabFactory<FArrayBox> const*>& a_factory)
{
	BL_PROFILE("Operator::Elastic::define()");

	Operator::define(a_geom,a_grids,a_dmap,a_info,a_factory);

	int model_nghost = 2;

	model.resize(m_num_amr_levels);
	for (int amrlev = 0; amrlev < m_num_amr_levels; ++amrlev)
	{
		model[amrlev].resize(m_num_mg_levels[amrlev]);
		for (int mglev = 0; mglev < m_num_mg_levels[amrlev]; ++mglev)
		{
			model[amrlev][mglev].reset(new MultiTab(amrex::convert(m_grids[amrlev][mglev],
									       amrex::IntVect::TheNodeVector()),
								m_dmap[amrlev][mglev], 1, model_nghost));
		}
	}
}

template <class T>
void
Elastic<T>::SetModel (int amrlev, const amrex::FabArray<amrex::BaseFab<T> >& a_model)
{
	BL_PROFILE("Operator::Elastic::SetModel()");

	int nghost = model[amrlev][0]->nGrow();
	for (MFIter mfi(a_model, false); mfi.isValid(); ++mfi)
	{
		const Box& bx = mfi.validbox();
		amrex::BaseFab<T> &modelfab = (*(model[amrlev][0]))[mfi];
		const amrex::BaseFab<T> &a_modelfab = a_model[mfi];

		AMREX_D_TERM(for (int m1 = bx.loVect()[0]-nghost; m1<=bx.hiVect()[0]+nghost; m1++),
			     for (int m2 = bx.loVect()[1]-nghost; m2<=bx.hiVect()[1]+nghost; m2++),
			     for (int m3 = bx.loVect()[2]-nghost; m3<=bx.hiVect()[2]+nghost; m3++))
		{
			amrex::IntVect m(AMREX_D_DECL(m1,m2,m3));
			modelfab(m) = a_modelfab(m);
		}
	}
}

template<class T>
void
Elastic<T>::Fapply (int amrlev, int mglev, MultiFab& f, const MultiFab& u) const
{
	BL_PROFILE("Operator::Elastic::Fapply()");

	amrex::Box domain(m_geom[amrlev][mglev].Domain());
	const Real* DX = m_geom[amrlev][mglev].CellSize();

	int nghost = 1; //(f.nGrow() && u.nGrow()) ? 1 : 0;
	//Util::Message(INFO,"amrlev = ", amrlev, "nghost = ", nghost);

	for (MFIter mfi(f, false); mfi.isValid(); ++mfi)
	{
		const Box& bx = mfi.validbox();
		amrex::BaseFab<T> &C = (*(model[amrlev][mglev]))[mfi];
		const amrex::FArrayBox &ufab    = u[mfi];
		amrex::FArrayBox       &ffab    = f[mfi];

		AMREX_D_TERM(for (int m1 = bx.loVect()[0] - nghost; m1<=bx.hiVect()[0] + nghost; m1++),
			     for (int m2 = bx.loVect()[1] - nghost; m2<=bx.hiVect()[1] + nghost; m2++),
			     for (int m3 = bx.loVect()[2] - nghost; m3<=bx.hiVect()[2] + nghost; m3++))
		{
			amrex::IntVect m(AMREX_D_DECL(m1,m2,m3));

			
			// Util::Message(INFO,"amrlev=",amrlev," m=",m," nghost = ", nghost);
			// Util::Message(INFO,"domain =",domain.loVect()[0]," ",domain.loVect()[1]);
			// Util::Message(INFO,"domain =",domain.hiVect()[0]," ",domain.hiVect()[1]);

			if (m[0] < domain.loVect()[0]) continue;
			if (m[1] < domain.loVect()[1]) continue;
			if (m[0] > domain.hiVect()[0]+1) continue;
			if (m[1] > domain.hiVect()[1]+1) continue;


			Set::Vector f = Set::Vector::Zero();
			

			bool    AMREX_D_DECL(xmin = (m[0] == domain.loVect()[0]),
					     ymin = (m[1] == domain.loVect()[1]),
					     zmin = (m[2] == domain.loVect()[2])),
				AMREX_D_DECL(xmax = (m[0] == domain.hiVect()[0]+1),
					     ymax = (m[1] == domain.hiVect()[1]+1),
					     zmax = (m[2] == domain.hiVect()[2]+1));

			// The displacement gradient tensor
			Set::Matrix gradu; // gradu(i,j) = u_{i,j)

			// Fill gradu and gradgradu
			for (int i = 0; i < AMREX_SPACEDIM; i++)
			{
				AMREX_D_TERM(gradu(i,0) = ((!xmax ? ufab(m+dx,i) : ufab(m,i)) - (!xmin ? ufab(m-dx,i) : ufab(m,i)))/((xmin || xmax ? 1.0 : 2.0)*DX[0]);,
					     gradu(i,1) = ((!ymax ? ufab(m+dy,i) : ufab(m,i)) - (!ymin ? ufab(m-dy,i) : ufab(m,i)))/((ymin || ymax ? 1.0 : 2.0)*DX[1]);,
					     gradu(i,2) = ((!zmax ? ufab(m+dz,i) : ufab(m,i)) - (!zmin ? ufab(m-dz,i) : ufab(m,i)))/((zmin || zmax ? 1.0 : 2.0)*DX[2]););
			}

			// Stress tensor computed using the model fab
			Set::Matrix sig = C(m)(gradu);

			//
			// Boundary conditions
			//
			// BCs are implemented as boundary operators.
			//
			// ┌                      ┐ ┌                     ┐   ┌              ┐
			// │              |       │ │ interior		  │   │ body	     │
			// │  Div C Grad  │       │ │ displacements	  │   │ forces	     │
			// │              │       │ │			  │ = │		     │
			// │ ─────────────┼────── │ │ ──────────────────  │   │ ──────────── │
			// │              │ Bndry │ │ bndry displacements │   │ bndry values │
			// └                      ┘ └			  ┘   └		     ┘
			//
			// For displacement:
			//   (Bndry)(u) = u
			// For traction:
			//   (Bndry)(u) = C Grad (u) n   (n is surface normal)
			//
			// The displacement values or traction values are set as the boundary
			// values of the rhs fab.
			//
			if (AMREX_D_TERM(xmax || xmin, || ymax || ymin, || zmax || zmin))
			{
				for (int i = 0; i < AMREX_SPACEDIM; i++) // iterate over DIMENSIONS
				{
					for (int j = 0; j < AMREX_SPACEDIM; j++) // iterate over FACES
					{
						if (m[j] == domain.loVect()[j])
						{
							if (m_bc_lo[j][i] == BC::Displacement)
								f(i) = ufab(m,i);
							else if (m_bc_lo[j][i] == BC::Traction) 
								f(i) += -sig(i,j);
							else Util::Abort(INFO, "Invalid BC");
						}
						if (m[j] == domain.hiVect()[j] + 1)
						{
							if (m_bc_hi[j][i] == BC::Displacement)
								f(i) = ufab(m,i);
							else if (m_bc_hi[j][i] == BC::Traction) 
								f(i) += +sig(i,j);
							else Util::Abort(INFO, "Invalid BC");

						}
					}
				}
			}
			else
			{
				// The gradient of the displacement gradient tensor
				std::array<Set::Matrix,AMREX_SPACEDIM> gradgradu; // gradgradu[k](l,j) = u_{k,lj}

				// Fill gradu and gradgradu
				for (int i = 0; i < AMREX_SPACEDIM; i++)
				{
			
					AMREX_D_TERM(gradgradu[i](0,0) = (ufab(m+dx,i) - 2.0*ufab(m,i) + ufab(m-dx,i))/DX[0]/DX[0];
						     ,// 2D
						     gradgradu[i](0,1) = (ufab(m+dx+dy,i) + ufab(m-dx-dy,i) - ufab(m+dx-dy,i) - ufab(m-dx+dy,i))/(2.0*DX[0])/(2.0*DX[1]);
						     gradgradu[i](1,0) = gradgradu[i](0,1);
						     gradgradu[i](1,1) = (ufab(m+dy,i) - 2.0*ufab(m,i) + ufab(m-dy,i))/DX[1]/DX[1];
						     ,// 3D
						     gradgradu[i](0,2) = (ufab(m+dx+dz,i) + ufab(m-dx-dz,i) - ufab(m+dx-dz,i) - ufab(m-dx+dz,i))/(2.0*DX[0])/(2.0*DX[2]);
						     gradgradu[i](1,2) = (ufab(m+dy+dz,i) + ufab(m-dy-dz,i) - ufab(m+dy-dz,i) - ufab(m-dy+dz,i))/(2.0*DX[1])/(2.0*DX[2]);
						     gradgradu[i](2,0) = gradgradu[i](0,2);
						     gradgradu[i](2,1) = gradgradu[i](1,2);
						     gradgradu[i](2,2) = (ufab(m+dz,i) - 2.0*ufab(m,i) + ufab(m-dz,i))/DX[2]/DX[2];);
				}
	
				//
				// Operator
				//
				// The return value is
				//    f = C(grad grad u) + grad(C)*grad(u)
				// In index notation
				//    f_i = C_{ijkl,j} u_{k,l}  +  C_{ijkl}u_{k,lj}
				//

				f =     C(m)(gradgradu) + 
					 AMREX_D_TERM(( ( C(m+dx) - C(m-dx))/2.0/DX[0])(gradu).col(0),
					  	     + ((C(m+dy) - C(m-dy))/2.0/DX[1])(gradu).col(1),
					  	     + ((C(m+dz) - C(m-dz))/2.0/DX[2])(gradu).col(2));

			}

			AMREX_D_TERM(ffab(m,0) = f[0];, ffab(m,1) = f[1];, ffab(m,2) = f[2];);
		}
	}
}



template<class T>
void
Elastic<T>::Diagonal (int amrlev, int mglev, MultiFab& diag)
{
	BL_PROFILE("Operator::Elastic::Diagonal()");

	amrex::Box domain(m_geom[amrlev][mglev].Domain());
	const Real* DX = m_geom[amrlev][mglev].CellSize();
	
	for (MFIter mfi(diag, false); mfi.isValid(); ++mfi)
	{
		const Box& bx = mfi.validbox();
		amrex::BaseFab<T> &C = (*(model[amrlev][mglev]))[mfi];
		amrex::FArrayBox       &diagfab    = diag[mfi];

		AMREX_D_TERM(for (int m1 = bx.loVect()[0] - 1; m1<=bx.hiVect()[0] + 1; m1++),
		 	     for (int m2 = bx.loVect()[1] - 1; m2<=bx.hiVect()[1] + 1; m2++),
		 	     for (int m3 = bx.loVect()[2] - 1; m3<=bx.hiVect()[2] + 1; m3++))
		{
			amrex::IntVect m(AMREX_D_DECL(m1,m2,m3));

			if (m[0] < domain.loVect()[0]) continue;
			if (m[1] < domain.loVect()[1]) continue;
			if (m[0] > domain.hiVect()[0]+1) continue;
			if (m[1] > domain.hiVect()[1]+1) continue;

			bool    AMREX_D_DECL(xmin = (m1 == domain.loVect()[0]),
					     ymin = (m2 == domain.loVect()[1]),
					     zmin = (m3 == domain.loVect()[2])),
				AMREX_D_DECL(xmax = (m1 == domain.hiVect()[0] + 1),
					     ymax = (m2 == domain.hiVect()[1] + 1),
					     zmax = (m3 == domain.hiVect()[2] + 1));

			Set::Matrix gradu; // gradu(i,j) = u_{i,j)
			std::array<Set::Matrix,AMREX_SPACEDIM> gradgradu; // gradgradu[k](l,j) = u_{k,lj}

			for (int i = 0; i < AMREX_SPACEDIM; i++)
			{
				diagfab(m,i) = 0.0;
				for (int k = 0; k < AMREX_SPACEDIM; k++)
				{
					AMREX_D_TERM(gradu(k,0) = ((!xmax ? 0.0 : (i==k ? 1.0 : 0.0)) - (!xmin ? 0.0 : (i==k ? 1.0 : 0.0)))/((xmin || xmax ? 1.0 : 2.0)*DX[0]);,
						     gradu(k,1) = ((!ymax ? 0.0 : (i==k ? 1.0 : 0.0)) - (!ymin ? 0.0 : (i==k ? 1.0 : 0.0)))/((ymin || ymax ? 1.0 : 2.0)*DX[1]);,
						     gradu(k,2) = ((!zmax ? 0.0 : (i==k ? 1.0 : 0.0)) - (!zmin ? 0.0 : (i==k ? 1.0 : 0.0)))/((zmin || zmax ? 1.0 : 2.0)*DX[2]););
			
					AMREX_D_TERM(gradgradu[k](0,0) = (i==k ? -2.0 : 0.0)/DX[0]/DX[0];
						     ,// 2D
						     gradgradu[k](0,1) = 0.0;
						     gradgradu[k](1,0) = 0.0;
						     gradgradu[k](1,1) = (i==k ? -2.0 : 0.0)/DX[1]/DX[1];
						     ,// 3D
						     gradgradu[k](0,2) = 0.0;
						     gradgradu[k](1,2) = 0.0;
						     gradgradu[k](2,0) = 0.0;
						     gradgradu[k](2,1) = 0.0;
						     gradgradu[k](2,2) = (i==k ? -2.0 : 0.0)/DX[2]/DX[2]);
				}

				Set::Matrix sig = C(m)(gradu);

				if (AMREX_D_TERM(xmax || xmin, || ymax || ymin, || zmax || zmin)) 
				{
					for (int j = 0; j < AMREX_SPACEDIM; j++) // iterate over FACES
					{
						if (m[j] == domain.loVect()[j])
						{
							if (m_bc_lo[j][i] == BC::Displacement)
								diagfab(m,i) += 1.0;
							else if (m_bc_lo[j][i] == BC::Traction) 
								diagfab(m,i) -= sig(i,j);
							else Util::Abort(INFO, "Invalid BC");
						}
						if (m[j] == domain.hiVect()[j] + 1)
						{
							if (m_bc_hi[j][i] == BC::Displacement)
								diagfab(m,i) += 1.0;
							else if (m_bc_hi[j][i] == BC::Traction) 
								diagfab(m,i) += sig(i,j);
							else Util::Abort(INFO, "Invalid BC");
						}
					}
				}
				else
				{
					Set::Vector f =
						C(m)(gradgradu)  + 
						AMREX_D_TERM(((C(m+dx) - C(m-dx))/2.0/DX[0])(gradu).col(0),
						   	     + ((C(m+dy) - C(m-dy))/2.0/DX[1])(gradu).col(1),
						     	     + ((C(m+dz) - C(m-dz))/2.0/DX[2])(gradu).col(2));
					diagfab(m,i) += f(i);
				}
			}
		}
	}
}


template<class T>
void
Elastic<T>::Error0x (int amrlev, int mglev, MultiFab& R0x, const MultiFab& x) const
{
	BL_PROFILE("Operator::Elastic::Error0x()");
	Util::Message(INFO);

	int ncomp = x.nComp();//getNComp();
	int nghost = x.nGrow();

	if (!m_diagonal_computed)
		Util::Abort(INFO,"Operator::Diagonal() must be called before using normalize");

	amrex::MultiFab D0x(x.boxArray(), x.DistributionMap(), ncomp, nghost);
	amrex::MultiFab AD0x(x.boxArray(), x.DistributionMap(), ncomp, nghost);

	amrex::MultiFab::Copy(D0x,x,0,0,ncomp,nghost); // D0x = x
	amrex::MultiFab::Divide(D0x,*m_diag[amrlev][mglev],0,0,ncomp,0); // D0x = x/diag
	amrex::MultiFab::Copy(AD0x,D0x,0,0,ncomp,nghost); // AD0x = D0x

	Fapply(amrlev,mglev,AD0x,D0x);	// AD0x = A * D0 * x
	
	amrex::MultiFab::Copy(R0x,x,0,0,ncomp,nghost); // R0x = x
	amrex::MultiFab::Subtract(R0x,AD0x,0,0,ncomp,nghost); // R0x = x - AD0x
}


template<class T>
void
Elastic<T>::FFlux (int /*amrlev*/, const MFIter& /*mfi*/,
		const std::array<FArrayBox*,AMREX_SPACEDIM>& sigmafab,
		const FArrayBox& /*ufab*/, const int /*face_only*/) const
{
	BL_PROFILE("Operator::Elastic::FFlux()");
	Util::Message(INFO);
	amrex::BaseFab<amrex::Real> AMREX_D_DECL( &fxfab = *sigmafab[0],
	 					  &fyfab = *sigmafab[1],
	 					  &fzfab = *sigmafab[2] ) ;
	AMREX_D_TERM(fxfab.setVal(0.0);,
	 	     fyfab.setVal(0.0);,
	 	     fzfab.setVal(0.0););

}

template<class T>
void
Elastic<T>::Strain  (int amrlev,
		    amrex::MultiFab& eps,
		    const amrex::MultiFab& u,
		    bool voigt) const
{
	BL_PROFILE("Operator::Elastic::Strain()");
	Util::Message(INFO);

	if (voigt)
		AMREX_ASSERT(eps.nComp() == (AMREX_SPACEDIM*(AMREX_SPACEDIM-1)/2));
	else
		AMREX_ASSERT(eps.nComp() == AMREX_SPACEDIM*AMREX_SPACEDIM);
	
	const amrex::Real* DX = m_geom[amrlev][0].CellSize();
	
	for (MFIter mfi(u, true); mfi.isValid(); ++mfi)
	{
		const Box& bx = mfi.tilebox();
		amrex::FArrayBox  &epsfab   = eps[mfi];
		const amrex::FArrayBox  &ufab = u[mfi];
		
		AMREX_D_TERM(for (int m1 = bx.loVect()[0]; m1<=bx.hiVect()[0]; m1++),
			     for (int m2 = bx.loVect()[1]; m2<=bx.hiVect()[1]; m2++),
			     for (int m3 = bx.loVect()[2]; m3<=bx.hiVect()[2]; m3++))
		{
			amrex::IntVect m(AMREX_D_DECL(m1,m2,m3));
			bool	AMREX_D_DECL(xmin = (m1 == bx.loVect()[0]),
					     ymin = (m2 == bx.loVect()[1]),
					     zmin = (m3 == bx.loVect()[2])),
				AMREX_D_DECL(xmax = (m1 == bx.hiVect()[0]),
					     ymax = (m2 == bx.hiVect()[1]),
					     zmax = (m3 == bx.hiVect()[2]));

			Set::Matrix gradu;
			AMREX_D_TERM(gradu(0,0) = ((!xmax ? ufab(m+dx,0) : ufab(m,0)) - (!xmin ? ufab(m-dx,0) : ufab(m,0)))/((xmin || xmax ? 1.0 : 2.0)*DX[0]);
				     ,
				     gradu(0,1) = ((!ymax ? ufab(m+dy,0) : ufab(m,0)) - (!ymin ? ufab(m-dy,0) : ufab(m,0)))/((ymin || ymax ? 1.0 : 2.0)*DX[1]);
				     gradu(1,0) = ((!xmax ? ufab(m+dx,1) : ufab(m,1)) - (!xmin ? ufab(m-dx,1) : ufab(m,1)))/((xmin || xmax ? 1.0 : 2.0)*DX[0]);
				     gradu(1,1) = ((!ymax ? ufab(m+dy,1) : ufab(m,1)) - (!ymin ? ufab(m-dy,1) : ufab(m,1)))/((ymin || ymax ? 1.0 : 2.0)*DX[1]);
				     ,
				     gradu(0,2) = ((!zmax ? ufab(m+dz,0) : ufab(m,0)) - (!zmin ? ufab(m-dz,0) : ufab(m,0)))/((zmin || zmax ? 1.0 : 2.0)*DX[2]);
				     gradu(1,2) = ((!zmax ? ufab(m+dz,1) : ufab(m,1)) - (!zmin ? ufab(m-dz,1) : ufab(m,1)))/((zmin || zmax ? 1.0 : 2.0)*DX[2]);
				     gradu(2,0) = ((!xmax ? ufab(m+dx,2) : ufab(m,2)) - (!xmin ? ufab(m-dx,2) : ufab(m,2)))/((xmin || xmax ? 1.0 : 2.0)*DX[0]);
				     gradu(2,1) = ((!ymax ? ufab(m+dy,2) : ufab(m,2)) - (!ymin ? ufab(m-dy,2) : ufab(m,2)))/((ymin || ymax ? 1.0 : 2.0)*DX[1]);
				     gradu(2,2) = ((!zmax ? ufab(m+dz,2) : ufab(m,2)) - (!zmin ? ufab(m-dz,2) : ufab(m,2)))/((zmin || zmax ? 1.0 : 2.0)*DX[2]););
			
			Set::Matrix strain = 0.5 * (gradu + gradu.transpose());
			
			if (voigt)
			{
#if AMREX_SPACEDIM == 2
				epsfab(m,0) = strain(0,0); epsfab(m,1) = strain(1,1); epsfab(m,2) = strain(0,1); 
#elif AMREX_SPACEDIM == 3
				epsfab(m,0) = strain(0,0); epsfab(m,1) = strain(1,1); epsfab(m,2) = strain(2,2); 
				epsfab(m,3) = strain(1,2); epsfab(m,4) = strain(2,0); epsfab(m,5) = strain(0,1); 
#endif
			}
			else
			{
#if   AMREX_SPACEDIM == 2
				epsfab(m,0) = strain(0,0); epsfab(m,1) = strain(0,1); 
				epsfab(m,2) = strain(1,0); epsfab(m,3) = strain(1,1); 
#elif AMREX_SPACEDIM == 3
				epsfab(m,0) = strain(0,0); epsfab(m,1) = strain(0,1); epsfab(m,2) = strain(0,2); 
				epsfab(m,3) = strain(1,0); epsfab(m,4) = strain(1,1); epsfab(m,5) = strain(1,2); 
				epsfab(m,6) = strain(2,0); epsfab(m,7) = strain(2,1); epsfab(m,8) = strain(2,2); 
#endif
			}
		}
	}
}


template<class T>
void
Elastic<T>::Stress (int amrlev,
		    amrex::MultiFab& sigma,
		    const amrex::MultiFab& u,
		    bool voigt) const
{
	BL_PROFILE("Operator::Elastic::Stress()");
	Util::Message(INFO);
	if (voigt)
		AMREX_ASSERT(sigma.nComp() == (AMREX_SPACEDIM*(AMREX_SPACEDIM-1)/2));
	else
		AMREX_ASSERT(sigma.nComp() == AMREX_SPACEDIM*AMREX_SPACEDIM);

	const amrex::Real* DX = m_geom[amrlev][0].CellSize();

	Util::Message(INFO,"u grow = ", u.nGrow());

	for (MFIter mfi(u, true); mfi.isValid(); ++mfi)
	{
		const Box& bx = mfi.tilebox();
		amrex::BaseFab<T> &C          = (*(model[amrlev][0]))[mfi];
		amrex::FArrayBox  &sigmafab   = sigma[mfi];
		const amrex::FArrayBox  &ufab = u[mfi];

		AMREX_D_TERM(for (int m1 = bx.loVect()[0]; m1<=bx.hiVect()[0]; m1++),
			     for (int m2 = bx.loVect()[1]; m2<=bx.hiVect()[1]; m2++),
			     for (int m3 = bx.loVect()[2]; m3<=bx.hiVect()[2]; m3++))
		{
			amrex::IntVect m(AMREX_D_DECL(m1,m2,m3));
			//Util::Message(INFO,"m=",m," & box = (",bx.loVect()[0],",",bx.loVect()[1],",",bx.loVect()[2],")(",bx.hiVect()[0],",",bx.hiVect()[1],",",bx.hiVect()[2],")");

			bool    AMREX_D_DECL(xmin = (m1 == bx.loVect()[0]),
					     ymin = (m2 == bx.loVect()[1]),
					     zmin = (m3 == bx.loVect()[2])),
				AMREX_D_DECL(xmax = (m1 == bx.hiVect()[0]),
					     ymax = (m2 == bx.hiVect()[1]),
					     zmax = (m3 == bx.hiVect()[2]));

			Set::Matrix gradu;

			AMREX_D_TERM(gradu(0,0) = ((!xmax ? ufab(m+dx,0) : ufab(m,0)) - (!xmin ? ufab(m-dx,0) : ufab(m,0)))/((xmin || xmax ? 1.0 : 2.0)*DX[0]);
				     ,
				     gradu(0,1) = ((!ymax ? ufab(m+dy,0) : ufab(m,0)) - (!ymin ? ufab(m-dy,0) : ufab(m,0)))/((ymin || ymax ? 1.0 : 2.0)*DX[1]);
				     gradu(1,0) = ((!xmax ? ufab(m+dx,1) : ufab(m,1)) - (!xmin ? ufab(m-dx,1) : ufab(m,1)))/((xmin || xmax ? 1.0 : 2.0)*DX[0]);
				     gradu(1,1) = ((!ymax ? ufab(m+dy,1) : ufab(m,1)) - (!ymin ? ufab(m-dy,1) : ufab(m,1)))/((ymin || ymax ? 1.0 : 2.0)*DX[1]);
				     ,
				     gradu(0,2) = ((!zmax ? ufab(m+dz,0) : ufab(m,0)) - (!zmin ? ufab(m-dz,0) : ufab(m,0)))/((zmin || zmax ? 1.0 : 2.0)*DX[2]);
				     gradu(1,2) = ((!zmax ? ufab(m+dz,1) : ufab(m,1)) - (!zmin ? ufab(m-dz,1) : ufab(m,1)))/((zmin || zmax ? 1.0 : 2.0)*DX[2]);
				     gradu(2,0) = ((!xmax ? ufab(m+dx,2) : ufab(m,2)) - (!xmin ? ufab(m-dx,2) : ufab(m,2)))/((xmin || xmax ? 1.0 : 2.0)*DX[0]);
				     gradu(2,1) = ((!ymax ? ufab(m+dy,2) : ufab(m,2)) - (!ymin ? ufab(m-dy,2) : ufab(m,2)))/((ymin || ymax ? 1.0 : 2.0)*DX[1]);
				     gradu(2,2) = ((!zmax ? ufab(m+dz,2) : ufab(m,2)) - (!zmin ? ufab(m-dz,2) : ufab(m,2)))/((zmin || zmax ? 1.0 : 2.0)*DX[2]););
			
			Set::Matrix eps = 0.5 * (gradu + gradu.transpose());

			Set::Matrix sig = C(m)(eps);

			if (voigt)
			{
#if AMREX_SPACEDIM == 2
				sigmafab(m,0) = sig(0,0); sigmafab(m,1) = sig(1,1); sigmafab(m,2) = sig(0,1); 
#elif AMREX_SPACEDIM == 3
				sigmafab(m,0) = sig(0,0); sigmafab(m,1) = sig(1,1); sigmafab(m,2) = sig(2,2); 
				sigmafab(m,3) = sig(1,2); sigmafab(m,4) = sig(2,0); sigmafab(m,5) = sig(0,1); 
#endif
			}
			else
			{
#if   AMREX_SPACEDIM == 2
				sigmafab(m,0) = sig(0,0); sigmafab(m,1) = sig(0,1); 
				sigmafab(m,2) = sig(1,0); sigmafab(m,3) = sig(1,1); 
#elif AMREX_SPACEDIM == 3
				sigmafab(m,0) = sig(0,0); sigmafab(m,1) = sig(0,1); sigmafab(m,2) = sig(0,2); 
				sigmafab(m,3) = sig(1,0); sigmafab(m,4) = sig(1,1); sigmafab(m,5) = sig(1,2); 
				sigmafab(m,6) = sig(2,0); sigmafab(m,7) = sig(2,1); sigmafab(m,8) = sig(2,2); 
#endif
			}
			}
	}
}


template<class T>
void
Elastic<T>::Energy (int amrlev,
		    amrex::MultiFab& energy,
		    const amrex::MultiFab& u) const
{
	BL_PROFILE("Operator::Elastic::Energy()");
	Util::Message(INFO);
	AMREX_ASSERT(energy.nComp() == 1);
	AMREX_ASSERT(u.nComp() == AMREX_SPACEDIM);

	const amrex::Real* DX = m_geom[amrlev][0].CellSize();

	for (MFIter mfi(u, true); mfi.isValid(); ++mfi)
	{
		const Box& bx = mfi.tilebox();
		amrex::BaseFab<T> &C          = (*(model[amrlev][0]))[mfi];
		amrex::FArrayBox  &energyfab  = energy[mfi];
		const amrex::FArrayBox  &ufab = u[mfi];

		AMREX_D_TERM(for (int m1 = bx.loVect()[0]; m1<=bx.hiVect()[0]; m1++),
			     for (int m2 = bx.loVect()[1]; m2<=bx.hiVect()[1]; m2++),
			     for (int m3 = bx.loVect()[2]; m3<=bx.hiVect()[2]; m3++))
		{
			amrex::IntVect m(AMREX_D_DECL(m1,m2,m3));
			bool    AMREX_D_DECL(xmin = (m1 == bx.loVect()[0]),
					     ymin = (m2 == bx.loVect()[1]),
					     zmin = (m3 == bx.loVect()[2])),
				AMREX_D_DECL(xmax = (m1 == bx.hiVect()[0]),
					     ymax = (m2 == bx.hiVect()[1]),
					     zmax = (m3 == bx.hiVect()[2]));
			

			Set::Matrix gradu;

			AMREX_D_TERM(gradu(0,0) = ((!xmax ? ufab(m+dx,0) : ufab(m,0)) - (!xmin ? ufab(m-dx,0) : ufab(m,0)))/((xmin || xmax ? 1.0 : 2.0)*DX[0]);
				     ,
				     gradu(0,1) = ((!ymax ? ufab(m+dy,0) : ufab(m,0)) - (!ymin ? ufab(m-dy,0) : ufab(m,0)))/((ymin || ymax ? 1.0 : 2.0)*DX[1]);
				     gradu(1,0) = ((!xmax ? ufab(m+dx,1) : ufab(m,1)) - (!xmin ? ufab(m-dx,1) : ufab(m,1)))/((xmin || xmax ? 1.0 : 2.0)*DX[0]);
				     gradu(1,1) = ((!ymax ? ufab(m+dy,1) : ufab(m,1)) - (!ymin ? ufab(m-dy,1) : ufab(m,1)))/((ymin || ymax ? 1.0 : 2.0)*DX[1]);
				     ,
				     gradu(0,2) = ((!zmax ? ufab(m+dz,0) : ufab(m,0)) - (!zmin ? ufab(m-dz,0) : ufab(m,0)))/((zmin || zmax ? 1.0 : 2.0)*DX[2]);
				     gradu(1,2) = ((!zmax ? ufab(m+dz,1) : ufab(m,1)) - (!zmin ? ufab(m-dz,1) : ufab(m,1)))/((zmin || zmax ? 1.0 : 2.0)*DX[2]);
				     gradu(2,0) = ((!xmax ? ufab(m+dx,2) : ufab(m,2)) - (!xmin ? ufab(m-dx,2) : ufab(m,2)))/((xmin || xmax ? 1.0 : 2.0)*DX[0]);
				     gradu(2,1) = ((!ymax ? ufab(m+dy,2) : ufab(m,2)) - (!ymin ? ufab(m-dy,2) : ufab(m,2)))/((ymin || ymax ? 1.0 : 2.0)*DX[1]);
				     gradu(2,2) = ((!zmax ? ufab(m+dz,2) : ufab(m,2)) - (!zmin ? ufab(m-dz,2) : ufab(m,2)))/((zmin || zmax ? 1.0 : 2.0)*DX[2]););
			
			Set::Matrix eps = 0.5 * (gradu + gradu.transpose());
			Set::Matrix sig = C(m)(eps);

			energyfab(m) = (eps.transpose() * sig).trace();

		}
	}
}



template<class T>
void
Elastic<T>::averageDownCoeffs ()
{
	BL_PROFILE("Elastic::averageDownCoeffs()");
	
	// for (int amrlev = 0; amrlev < m_num_amr_levels; ++amrlev)
	// {
	// 	for (int mglev = 0; mglev < m_num_mg_levels[amrlev]; ++mglev)
	// 	{
	// 		///\todo replace number of ghost cells with 0
	// 		///\todo I think we can erase this section.
	// 		model[amrlev][mglev].reset(new amrex::FabArray<amrex::BaseFab<T> >(m_grids[amrlev][mglev], m_dmap[amrlev][mglev], 1, 1));
	// 	}
	// }

	for (int amrlev = m_num_amr_levels-1; amrlev > 0; --amrlev)
	{
		averageDownCoeffsSameAmrLevel(amrlev);
	 	averageDownCoeffsToCoarseAmrLevel(amrlev);
	}

	averageDownCoeffsSameAmrLevel(0);
	for (int amrlev = 0; amrlev < m_num_amr_levels; ++amrlev)
	{
	 	for (int mglev = 0; mglev < m_num_mg_levels[amrlev]; ++mglev)
	 	{
	 		if (model[amrlev][mglev]) {
	 			FillBoundaryCoeff(*model[amrlev][mglev], m_geom[amrlev][mglev]);
	 		}
	 	}
	}
}

template<class T>
void
Elastic<T>::averageDownCoeffsToCoarseAmrLevel (int flev) // this is where the problem is happening
{
	BL_PROFILE("Operator::Elastic::averageDownCoeffsToCoarseAmrLevel()");

	//const int mglev = 0;

	// const int idim = 0;  // other dimensions are just aliases

	// // amrex::average_down(*m_sigma[flev][mglev][idim], *m_sigma[flev-1][mglev][idim], 0, 1,
	// // 		    m_amr_ref_ratio[flev-1]);

	int ncomp = AMREX_SPACEDIM;
	MultiTab finemt;
	MultiTab crsemt;
	
	

	amrex::BoxArray fineBACoarsened = finemt.boxArray(); fineBACoarsened.coarsen(m_amr_ref_ratio[flev-1]);


	//MultiFab crse_S_fine(crse_S_fine_BA, S_fine.DistributionMap(), ncomp, 0, MFInfo(), FArrayBoxFactory());
        if ((fineBACoarsened == crsemt.boxArray()) &&
	    (finemt.DistributionMap() == crsemt.DistributionMap()))
	{
		Util::Message(INFO); // this never seems to happen
	}
	else
	{
		//Util::Abort(INFO, "difference in box arrays");

		MultiTab finemtcoarsened(fineBACoarsened, finemt.DistributionMap(), ncomp, 0);

// #ifdef _OPENMP
// #pragma omp parallel
// #endif
		for (MFIter mfi(finemtcoarsened,true); mfi.isValid(); ++mfi)
		{
			//  NOTE: The tilebox is defined at the coarse level.
			const Box& bx = mfi.tilebox();
                
			const TArrayBox &fine = finemtcoarsened[mfi];
			TArrayBox &crse = crsemt[mfi];

			//  NOTE: We copy from component scomp of the fine fab into component 0 of the crse fab
			//        because the crse fab is a temporary which was made starting at comp 0, it is
			//        not part of the actual crse multifab which came in.


			AMREX_D_TERM(for (int m1 = bx.loVect()[0]-1; m1<=bx.hiVect()[0]+1; m1++),
				     for (int m2 = bx.loVect()[1]-1; m2<=bx.hiVect()[1]+1; m2++),
				     for (int m3 = bx.loVect()[2]-1; m3<=bx.hiVect()[2]+1; m3++))
			{
				amrex::IntVect m_crse(AMREX_D_DECL(m1,m2,m3));
				amrex::IntVect m_fine(AMREX_D_DECL(m1*2,m2*2,m3*2));

				crse(m_crse) = fine(m_fine);
			}
		}
            
		crsemt.copy(finemtcoarsened,0,0,ncomp);

	}
}

template<class T>
void
Elastic<T>::averageDownCoeffsSameAmrLevel (int amrlev)
{
	BL_PROFILE("Elastic::averageDownCoeffsSameAmrLevel()");

// 	if (m_coarsening_strategy != CoarseningStrategy::Sigma) return;

// 	const int nsigma = (m_use_harmonic_average) ? AMREX_SPACEDIM : 1;

 	for (int mglev = 1; mglev < m_num_mg_levels[amrlev]; ++mglev)
 	{
		MultiTab& crse = *model[amrlev][mglev];
		MultiTab& fine = *model[amrlev][mglev-1];
		
		bool isMFIterSafe  = (crse.DistributionMap() == fine.DistributionMap()) && BoxArray::SameRefs(crse.boxArray(),fine.boxArray());
		bool need_parallel_copy = !isMFIterSafe;

		MultiTab cfine;
		if (need_parallel_copy) {
			const BoxArray& ba = amrex::coarsen(fine.boxArray(), 2);
			cfine.define(ba, fine.DistributionMap(), 1, 1);
		}

		MultiTab* pcrse = (need_parallel_copy) ? &cfine : &crse;

		for (MFIter mfi(*pcrse, true); mfi.isValid(); ++mfi)
 			{
				const Box& bx = mfi.tilebox();

				TArrayBox &crsetab = (*pcrse)[mfi];
				TArrayBox &finetab = fine[mfi];

				AMREX_D_TERM(for (int m1 = bx.loVect()[0]-1; m1<=bx.hiVect()[0]+1; m1++),
					     for (int m2 = bx.loVect()[1]-1; m2<=bx.hiVect()[1]+1; m2++),
					     for (int m3 = bx.loVect()[2]-1; m3<=bx.hiVect()[2]+1; m3++))
				{

					amrex::IntVect m_crse(AMREX_D_DECL(m1,m2,m3));
					amrex::IntVect m_fine(AMREX_D_DECL(m1*2,m2*2,m3*2));


					if (m1 == bx.loVect()[0] - 1) ++m_fine[0];
					if (m2 == bx.loVect()[1] - 1) ++m_fine[1];
					if (m1 == bx.hiVect()[0] + 1) --m_fine[0];
					if (m2 == bx.hiVect()[1] + 1) --m_fine[1];
					
#if AMREX_SPACEDIM == 2
					Set::Scalar total = 0.0;
					crsetab(m_crse) = finetab(m_fine)*4.0; 
					total += 4.0;
					if (m1 > bx.loVect()[0]-1 && m1 < bx.hiVect()[0]+1)
					{
						crsetab(m_crse) += finetab(m_fine-dx)*2.0 + finetab(m_fine+dx)*2.0;
						total += 4.0;
					}	
					if (m2 > bx.loVect()[1]-1 && m2 < bx.hiVect()[1]+1)
					{
						crsetab(m_crse) += finetab(m_fine-dy)*2.0 + finetab(m_fine+dy)*2.0;
						total += 4.0;
					}	
					if (m1 > bx.loVect()[0]-1 && m1 < bx.hiVect()[0]+1 &&
					    m2 > bx.loVect()[1]-1 && m2 < bx.hiVect()[1]+1 )
					{
						crsetab(m_crse) +=
							finetab(m_fine-dx-dy) + finetab(m_fine-dx+dy) +
							finetab(m_fine+dx-dy) + finetab(m_fine+dx+dy);
						total += 4.0;
					}	
					crsetab(m_crse) = crsetab(m_crse) / total;
#elif AMREX_SPACEDIM == 3
					crsetab(m_crse) = finetab(m_fine);
					// corner
					if ((m1 == bx.loVect()[0]-1 || m1 == bx.hiVect()[0]+1) &&
					    (m2 == bx.loVect()[1]-1 || m2 == bx.hiVect()[1]+1) &&
					    (m3 == bx.loVect()[2]-1 || m3 == bx.hiVect()[2]+1))
					{
						crsetab(m_crse) = finetab(m_fine);
					}
					else if ((m2 == bx.loVect()[1]-1 || m2 == bx.hiVect()[1]+1) && // x edge
						 (m3 == bx.loVect()[2]-1 || m3 == bx.hiVect()[2]+1))
					{
						crsetab(m_crse) = finetab(m_fine-dx)*0.25 + finetab(m_fine)*0.5 + finetab(m_fine+dx)*0.25;
					}
					else if ((m1 == bx.loVect()[0]-1 || m1 == bx.hiVect()[0]+1) && // y edge
						 (m3 == bx.loVect()[2]-1 || m3 == bx.hiVect()[2]+1))
					{
						crsetab(m_crse) = finetab(m_fine-dy)*0.25 + finetab(m_fine)*0.5 + finetab(m_fine+dy)*0.25;
					}
					else if ((m1 == bx.loVect()[0]-1 || m1 == bx.hiVect()[0]+1) && // z edge
						 (m2 == bx.loVect()[1]-1 || m2 == bx.hiVect()[1]+1))
					{
						crsetab(m_crse) = finetab(m_fine-dz)*0.25 + finetab(m_fine)*0.5 + finetab(m_fine+dz)*0.25;
					}
					else if ((m1 == bx.loVect()[0]-1 || m1 == bx.hiVect()[0]+1)) // x face
					{
						crsetab(m_crse) =
							(finetab(m_fine+dy+dz) + finetab(m_fine+dy-dz) + finetab(m_fine-dy+dz) + finetab(m_fine-dy-dz)) / 16. +
							(finetab(m_fine+dy) + finetab(m_fine-dy) + finetab(m_fine+dz) + finetab(m_fine-dz)) / 8. +
							(finetab(m_fine)) / 4.;							
					}
					else if ((m2 == bx.loVect()[1]-1 || m2 == bx.hiVect()[1]+1)) // y face
					{
						crsetab(m_crse) =
							(finetab(m_fine+dz+dx) + finetab(m_fine+dz-dx) + finetab(m_fine-dz+dx) + finetab(m_fine-dz-dx)) / 16. +
							(finetab(m_fine+dz) + finetab(m_fine-dz) + finetab(m_fine+dx) + finetab(m_fine-dx)) / 8. +
							(finetab(m_fine)) / 4.;
					}
					else if ((m3 == bx.loVect()[2]-1 || m3 == bx.hiVect()[2]+1)) // z face
					{
						crsetab(m_crse) =
							(finetab(m_fine+dx+dy) + finetab(m_fine+dx-dy) + finetab(m_fine-dx+dy) + finetab(m_fine-dx-dy)) / 16. +
							(finetab(m_fine+dx) + finetab(m_fine-dx) + finetab(m_fine+dy) + finetab(m_fine-dy)) / 8. +
							(finetab(m_fine)) / 4.;
					}
					else
					{
						crsetab(m_crse) =
							(finetab(m_fine-dx-dy-dz) + finetab(m_fine-dx-dy+dz) + finetab(m_fine-dx+dy-dz) + finetab(m_fine-dx+dy+dz) +
							 finetab(m_fine+dx-dy-dz) + finetab(m_fine+dx-dy+dz) + finetab(m_fine+dx+dy-dz) + finetab(m_fine+dx+dy+dz)) / 64.0
							+
							(finetab(m_fine-dy-dz) + finetab(m_fine-dy+dz) + finetab(m_fine+dy-dz) + finetab(m_fine+dy+dz) +
							 finetab(m_fine-dz-dx) + finetab(m_fine-dz+dx) + finetab(m_fine+dz-dx) + finetab(m_fine+dz+dx) +
							 finetab(m_fine-dx-dy) + finetab(m_fine-dx+dy) + finetab(m_fine+dx-dy) + finetab(m_fine+dx+dy)) / 32.0
							+
							(finetab(m_fine-dx) + finetab(m_fine-dy) + finetab(m_fine-dz) +
							 finetab(m_fine+dx) + finetab(m_fine+dy) + finetab(m_fine+dz)) / 16.0
							+
							finetab(m_fine) / 8.0;
					}
#endif
					

				}
 			}

		if (need_parallel_copy) {
			crse.ParallelCopy(cfine);
		}
 	}
}

template<class T>
void
Elastic<T>::FillBoundaryCoeff (amrex::FabArray<amrex::BaseFab<T> >& sigma, const Geometry& geom)
{
	BL_PROFILE("Elastic::FillBoundaryCoeff()");

	sigma.FillBoundary(geom.periodicity());

	//const Box& domain = geom.Domain();

#ifdef _OPENMP
#pragma omp parallel
#endif
	// for (MFIter mfi(sigma, MFItInfo().SetDynamic(true)); mfi.isValid(); ++mfi)
	// {
	// 	if (!domain.contains(mfi.fabbox()))
	// 	{
			

	// 	}
	// }
	///////Util::Warning(INFO, "FillBoundaryCoeff not fully implemented");
}



template class Elastic<Model::Solid::LinearElastic::Isotropic>;
template class Elastic<Model::Solid::LinearElastic::Cubic>;
template class Elastic<Model::Solid::LinearElastic::Laplacian>;
template class Elastic<Model::Solid::LinearElastic::Degradable::Isotropic>;
template class Elastic<Model::Solid::Viscoelastic::Isotropic>;
}

