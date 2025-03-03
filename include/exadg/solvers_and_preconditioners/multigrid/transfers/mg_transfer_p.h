/*  ______________________________________________________________________
 *
 *  ExaDG - High-Order Discontinuous Galerkin for the Exa-Scale
 *
 *  Copyright (C) 2021 by the ExaDG authors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *  ______________________________________________________________________
 */

#ifndef MG_TRANSFER_MF_P
#define MG_TRANSFER_MF_P

// deal.II
#include <deal.II/matrix_free/matrix_free.h>

// ExaDG
#include <exadg/solvers_and_preconditioners/multigrid/transfers/mg_transfer.h>

namespace ExaDG
{
template<int dim, typename Number, typename VectorType, int components = 1>
class MGTransferP : virtual public MGTransfer<VectorType>
{
public:
  typedef Number value_type;

  MGTransferP();

  MGTransferP(dealii::MatrixFree<dim, value_type> const * matrixfree_1,
              dealii::MatrixFree<dim, value_type> const * matrixfree_2,
              int                                         degree_1,
              int                                         degree_2,
              int                                         dof_handler_index = 0);

  void
  reinit(dealii::MatrixFree<dim, value_type> const * matrixfree_1,
         dealii::MatrixFree<dim, value_type> const * matrixfree_2,
         int                                         degree_1,
         int                                         degree_2,
         int                                         dof_handler_index = 0);

  virtual ~MGTransferP();

  virtual void
  interpolate(unsigned int const level, VectorType & dst, VectorType const & src) const;

  virtual void
  restrict_and_add(unsigned int const /*level*/, VectorType & dst, VectorType const & src) const;

  virtual void
  prolongate_and_add(unsigned int const /*level*/, VectorType & dst, VectorType const & src) const;

private:
  template<int fe_degree_1, int fe_degree_2>
  void
  do_interpolate(VectorType & dst, VectorType const & src) const;

  template<int fe_degree_1, int fe_degree_2>
  void
  do_restrict_and_add(VectorType & dst, VectorType const & src) const;

  template<int fe_degree_1, int fe_degree_2>
  void
  do_prolongate(VectorType & dst, VectorType const & src) const;

  dealii::MatrixFree<dim, value_type> const *            matrixfree_1;
  dealii::MatrixFree<dim, value_type> const *            matrixfree_2;
  dealii::AlignedVector<dealii::VectorizedArray<Number>> prolongation_matrix_1d;
  dealii::AlignedVector<dealii::VectorizedArray<Number>> interpolation_matrix_1d;

  unsigned int degree_1;
  unsigned int degree_2;
  unsigned int dof_handler_index;
  unsigned int quad_index;

  dealii::AlignedVector<dealii::VectorizedArray<Number>> weights;

  bool is_dg;
};

} // namespace ExaDG

#endif
