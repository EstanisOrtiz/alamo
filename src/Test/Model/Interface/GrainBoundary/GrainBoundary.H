#include "Util/Util.H"

namespace Test
{
namespace Model
{
namespace Interface
{
namespace GrainBoundary
{
template<class T>
class GrainBoundary
{
public:
	GrainBoundary() {};
	~GrainBoundary() {};

	
	int Sym(int verbose)
	{
		T model;
		for (int i = 0; i < 10; i++)
		{
			model.Randomize();
			for (Set::Scalar theta = 0; theta < 1.0*Set::Constant::Pi; theta += 0.001)
			{
				if (fabs(model.DW(theta) - model.DW(theta + Set::Constant::Pi)) > tolerance)
				{
					if (verbose > 0)
					{
						Util::Message(INFO, "Model does not obey symmetry");
						Util::Message(INFO, "       W(theta)       ",model.DW(theta));
						Util::Message(INFO, "       W(Theta+Pi)    ",model.DW(theta+Set::Constant::Pi));
					}
					return 1;
				}
			}
		}
		return 0;

	}
	int DerivativeTest1(int verbose)
	{
		int failed = 0;
		T model;
		for (int i = 0; i < 10; i++)
		{
			model.Randomize();
			for (Set::Scalar theta = 0; theta < 2.0*Set::Constant::Pi; theta += 0.001)
			{
				Set::Scalar numerical_DW = (model.W(theta + 0.5*small) - model.W(theta - 0.5*small))/small;
				if (fabs(numerical_DW-model.DW(theta))/(model.DW(theta)) > tolerance)
				{
					if (verbose > 0)
					{
						Util::Message(INFO, "DW Exact does not match DW Numerical");
						Util::Message(INFO, "       Theta:         ",theta);
						Util::Message(INFO, "       DW Exact:      ",model.DW(theta));
						Util::Message(INFO, "       DW Numerical:  ",numerical_DW);
						Util::Message(INFO, "       Difference:    ",numerical_DW-model.DW(theta));
						Util::Message(INFO, "       % Difference:  ",100*(numerical_DW-model.DW(theta))/model.DW(theta),"%");
					}
					return 1;
				}
			}
		}
		return failed;
	}
	int DerivativeTest2(int verbose)
	{
		T model;
		for (int i = 0; i < 10; i++)
		{
			model.Randomize();
			for (Set::Scalar theta = 0; theta < 2.0*Set::Constant::Pi; theta += 0.001)
			{
				Set::Scalar numerical_DDW = (model.DW(theta + 0.5*small) - model.DW(theta - 0.5*small))/small;
				if (fabs(numerical_DDW-model.DDW(theta))/(model.DDW(theta)) > tolerance)
				{
					if (verbose > 0)
					{
						Util::Message(INFO,"DDW Exact does not match DDW Numerical");
						Util::Message(INFO,"       Theta:         " , theta);
						Util::Message(INFO,"       DDW Exact:     " , model.DDW(theta));
						Util::Message(INFO,"       DDW Numerical: " , numerical_DDW);
						Util::Message(INFO,"       Difference:    " , fabs(numerical_DDW-model.DDW(theta)));
						Util::Message(INFO,"       % Difference:  " , 100*fabs(numerical_DDW-model.DDW(theta)/model.DDW(theta)),"%");
					}
					return 1;
				}
			}
		}
		return 0;
	}
private:
	static constexpr Set::Scalar small = 1E-12;
	static constexpr Set::Scalar tolerance = 1E-6;

};
}
}
}
}
