#ifndef OPERATOR_CELLELASTIC_POLYCRYSTAL_ISOTROPIC_H_
#define OPERATOR_CELLELASTIC_POLYCRYSTAL_ISOTROPIC_H_

#include <AMReX_MLCellLinOp.H>
#include <AMReX_Array.H>
#include <limits>

#include "Operator/CellElastic/PolyCrystal/PolyCrystal.H"

using namespace amrex;

namespace Operator
{
namespace CellElastic
{
namespace PolyCrystal
{
class Isotropic : public PolyCrystalModel
{
public:

  /// \brief Base constructor
  Isotropic ();
  /// \brief Destructor (does nothing)
  virtual ~Isotropic () {};

protected:

  amrex::Real C(const int i, const int j, const int k, const int l) const;

  amrex::Real mu1, mu2;     ///< First Lame parameter
  amrex::Real lambda1, lambda2; ///< Second Lame parameter



};
}
}
}
#endif
