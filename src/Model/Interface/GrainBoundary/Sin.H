#ifndef MODEL_INTERFACE_GRAINBOUNDARY_SIN_H
#define MODEL_INTERFACE_GRAINBOUNDARY_SIN_H

#include <iostream>
#include <fstream>

#include "AMReX.H"
#include "GrainBoundary.H"
#include "Util/Util.H"

namespace Model
{
namespace Interface
{
namespace GrainBoundary
{
class Sin : public GrainBoundary
{
public:
	Sin() {}
	Sin(amrex::Real _theta0, amrex::Real _sigma0, amrex::Real _sigma1)
	{
		Define(_theta0,_sigma0,_sigma1);
	};
	void Define(amrex::Real _theta0, amrex::Real _sigma0, amrex::Real _sigma1)
	{
		theta0 = _theta0;
		sigma0 = _sigma0;
		sigma1 = _sigma1;
	};
	void Randomize()
	{
		theta0 = 2*Set::Constant::Pi*Util::Random();
		sigma0 = Util::Random();
		sigma1 = Util::Random();
	};
	amrex::Real W(amrex::Real theta)
	{
		return sin(theta);
		//return sigma0 + 0.5*sigma1*(1.0 - cos(4.0*(theta-theta0)));
	};
	amrex::Real DW(amrex::Real theta)
	{
		return cos(theta);
		//return 2.0*sigma1*sin(4.0*(theta-theta0));
	};
	amrex::Real DDW(amrex::Real theta)
	{
		return -sin(theta);
		//return 8.0*sigma1*cos(4.0*(theta-theta0));
	};

private:
	amrex::Real theta0, sigma0, sigma1;
};
}
}
}
#endif
