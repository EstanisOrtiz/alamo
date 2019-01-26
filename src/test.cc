#include <stdlib.h>

#include "Util/Util.H"

#include "Model/Solid/LinearElastic/Test.H"
#include "Model/Solid/LinearElastic/Isotropic.H"
#include "Model/Solid/LinearElastic/Cubic.H"
#include "Model/Solid/LinearElastic/Laplacian.H"
#include "Model/Solid/LinearElastic/Degradable/Isotropic.H"

#include "Test/Operator/Elastic.H"
#include "Test/Model/Interface/GrainBoundary/GrainBoundary.H"
#include "Model/Interface/GrainBoundary/Sin.H"
#include "Model/Interface/GrainBoundary/AbsSin.H"

#include "Operator/Elastic.H"

#include "Numeric/Interpolator/Test.H"
#include "Numeric/Interpolator/Linear.H"

int main (int argc, char* argv[])
{
	Util::Initialize(argc, argv);


	int failed = 0;

	{
		Test::Model::Interface::GrainBoundary::GrainBoundary<Model::Interface::GrainBoundary::Sin> test;
		Util::Test::Message(          "Model::Solid::Interface::GrainBoundary<Sin>");
		failed += Util::Test::Message("  ├ Sym",            test.Sym(1));
		failed += Util::Test::Message("  ├ DerivativeTest1",test.DerivativeTest1(1));
		failed += Util::Test::Message("  └ DerivativeTest2",test.DerivativeTest2(1));
	}

	{
		Test::Model::Interface::GrainBoundary::GrainBoundary<Model::Interface::GrainBoundary::AbsSin> test;
		Util::Test::Message(          "Model::Solid::Interface::GrainBoundary<AbsSin>");
		failed += Util::Test::Message("  ├ Sym",            test.Sym(1));
		failed += Util::Test::Message("  ├ DerivativeTest1",test.DerivativeTest1(1));
		failed += Util::Test::Message("  └ DerivativeTest2",test.DerivativeTest2(1));
	}

	{
		Model::Solid::LinearElastic::Test<Model::Solid::LinearElastic::Isotropic> test;
		Util::Test::Message(          "Model::Solid::LinearElastic<Isotropic>");
		failed += Util::Test::Message("  ├ Consistency",    test.Consistency(0));
		failed += Util::Test::Message("  ├ MinorSymmetry1", test.MinorSymmetry1(0));
		failed += Util::Test::Message("  ├ MinorSymmetry2", test.MinorSymmetry2(0));
		failed += Util::Test::Message("  └ MajorSymmetry",  test.MajorSymmetry(0));
	}

	{
		Model::Solid::LinearElastic::Test<Model::Solid::LinearElastic::Cubic> test;
		Util::Test::Message(          "Model::Solid::LinearElastic<Cubic>");
		failed += Util::Test::Message("  ├ Consistency",    test.Consistency(2));
		failed += Util::Test::Message("  ├ MinorSymmetry1", test.MinorSymmetry1(2));
		failed += Util::Test::Message("  ├ MinorSymmetry2", test.MinorSymmetry2(2));
		failed += Util::Test::Message("  └ MajorSymmetry",  test.MajorSymmetry(2));
	}

	{
		Model::Solid::LinearElastic::Test<Model::Solid::LinearElastic::Degradable::Isotropic> test;
		Util::Test::Message(          "Model::Solid::LinearElastic<Degradable::Isotropic>");
		failed += Util::Test::Message("  ├ Consistency",    test.Consistency(2));
		failed += Util::Test::Message("  ├ MinorSymmetry1", test.MinorSymmetry1(2));
		failed += Util::Test::Message("  ├ MinorSymmetry2", test.MinorSymmetry2(2));
		failed += Util::Test::Message("  └ MajorSymmetry",  test.MajorSymmetry(2));
	}

	{
		Model::Solid::LinearElastic::Test<Model::Solid::LinearElastic::Laplacian> test;
		Util::Test::Message(          "Model::Solid::LinearElastic<Laplacian>");
		failed += Util::Test::Message("  └ Consistency",    test.Consistency(2));
	}

	{
		Numeric::Interpolator::Test<Numeric::Interpolator::Linear<Set::Scalar> > test;
		Util::Test::Message(          "Numeric::Interpolator<Linear>");
		failed += Util::Test::Message("  └ Match",test.Match(0));
	}

	{
		Test::Operator::Elastic test;
		test.Define(16,1);
		Util::Test::Message(          "Elastic Operator Trig Test 16x16, 1 level");
		failed += Util::Test::Message("  └ Component 0, period=1",test.TrigTest(0,0,1));

		test.Define(16,2);
		Util::Test::Message(          "Elastic Operator Trig Test 16x16, 2 levels");
		failed += Util::Test::Message("  ├ Reflux test",          test.RefluxTest(0));
		failed += Util::Test::Message("  └ Component 0, period=1",test.TrigTest(0,0,1));
	}

	Util::Message(INFO,failed," tests failed");

	Util::Finalize();
	return failed;
}
