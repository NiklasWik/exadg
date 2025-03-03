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

#ifndef INCLUDE_EXADG_INCOMPRESSIBLE_NAVIER_STOKES_SPATIAL_DISCRETIZATION_SPATIAL_OPERATOR_BASE_H_
#define INCLUDE_EXADG_INCOMPRESSIBLE_NAVIER_STOKES_SPATIAL_DISCRETIZATION_SPATIAL_OPERATOR_BASE_H_

// deal.II
#include <deal.II/fe/fe_dgq.h>
#include <deal.II/fe/fe_system.h>
#include <deal.II/lac/la_parallel_block_vector.h>
#include <deal.II/lac/la_parallel_vector.h>


// ExaDG
#include <exadg/functions_and_boundary_conditions/interface_coupling.h>
#include <exadg/grid/grid.h>
#include <exadg/grid/grid_motion_interface.h>
#include <exadg/incompressible_navier_stokes/spatial_discretization/calculators/divergence_calculator.h>
#include <exadg/incompressible_navier_stokes/spatial_discretization/calculators/q_criterion_calculator.h>
#include <exadg/incompressible_navier_stokes/spatial_discretization/calculators/streamfunction_calculator_rhs_operator.h>
#include <exadg/incompressible_navier_stokes/spatial_discretization/calculators/velocity_magnitude_calculator.h>
#include <exadg/incompressible_navier_stokes/spatial_discretization/calculators/vorticity_calculator.h>
#include <exadg/incompressible_navier_stokes/spatial_discretization/operators/convective_operator.h>
#include <exadg/incompressible_navier_stokes/spatial_discretization/operators/divergence_operator.h>
#include <exadg/incompressible_navier_stokes/spatial_discretization/operators/gradient_operator.h>
#include <exadg/incompressible_navier_stokes/spatial_discretization/operators/momentum_operator.h>
#include <exadg/incompressible_navier_stokes/spatial_discretization/operators/projection_operator.h>
#include <exadg/incompressible_navier_stokes/spatial_discretization/operators/rhs_operator.h>
#include <exadg/incompressible_navier_stokes/spatial_discretization/operators/viscous_operator.h>
#include <exadg/incompressible_navier_stokes/spatial_discretization/turbulence_model.h>
#include <exadg/incompressible_navier_stokes/user_interface/boundary_descriptor.h>
#include <exadg/incompressible_navier_stokes/user_interface/field_functions.h>
#include <exadg/incompressible_navier_stokes/user_interface/parameters.h>
#include <exadg/matrix_free/matrix_free_data.h>
#include <exadg/operators/inverse_mass_operator.h>
#include <exadg/operators/mass_operator.h>
#include <exadg/poisson/preconditioners/multigrid_preconditioner.h>
#include <exadg/poisson/spatial_discretization/laplace_operator.h>
#include <exadg/solvers_and_preconditioners/preconditioners/preconditioner_base.h>
#include <exadg/time_integration/interpolate.h>

namespace ExaDG
{
namespace IncNS
{
template<int dim, typename Number>
class SpatialOperatorBase;

template<int dim, typename Number>
class SpatialOperatorBase : public dealii::Subscriptor
{
protected:
  typedef dealii::LinearAlgebra::distributed::Vector<Number>      VectorType;
  typedef dealii::LinearAlgebra::distributed::BlockVector<Number> BlockVectorType;

  typedef SpatialOperatorBase<dim, Number> This;

  typedef dealii::VectorizedArray<Number>                         scalar;
  typedef dealii::Tensor<1, dim, dealii::VectorizedArray<Number>> vector;
  typedef dealii::Tensor<2, dim, dealii::VectorizedArray<Number>> tensor;

  typedef std::pair<unsigned int, unsigned int> Range;

  typedef FaceIntegrator<dim, dim, Number> FaceIntegratorU;
  typedef FaceIntegrator<dim, 1, Number>   FaceIntegratorP;

  typedef typename Poisson::MultigridPreconditioner<dim, Number, 1> MultigridPoisson;

public:
  /*
   * Constructor.
   */
  SpatialOperatorBase(std::shared_ptr<Grid<dim> const>                  grid,
                      std::shared_ptr<GridMotionInterface<dim, Number>> grid_motion,
                      std::shared_ptr<BoundaryDescriptor<dim> const>    boundary_descriptor,
                      std::shared_ptr<FieldFunctions<dim> const>        field_functions,
                      Parameters const &                                parameters,
                      std::string const &                               field,
                      MPI_Comm const &                                  mpi_comm);

  /*
   * Destructor.
   */
  virtual ~SpatialOperatorBase(){};

  void
  fill_matrix_free_data(MatrixFreeData<dim, Number> & matrix_free_data) const;

  /*
   * Setup function. Initializes basic finite element components, matrix-free object, and basic
   * operators. This function does not perform the setup related to the solution of linear systems
   * of equations.
   */
  virtual void
  setup(std::shared_ptr<dealii::MatrixFree<dim, Number>> matrix_free,
        std::shared_ptr<MatrixFreeData<dim, Number>>     matrix_free_data,
        std::string const &                              dof_index_temperature = "");

  /*
   * This function initializes operators, preconditioners, and solvers related to the solution of
   * (non-)linear systems of equation required for implicit formulations. It has to be extended
   * by derived classes if necessary.
   */
  virtual void
  setup_solvers(double const & scaling_factor_mass, VectorType const & velocity);

  /*
   * Getters and setters.
   */
  dealii::MatrixFree<dim, Number> const &
  get_matrix_free() const;

  std::string
  get_dof_name_velocity() const;

  unsigned int
  get_dof_index_velocity() const;

  unsigned int
  get_dof_index_pressure() const;

  unsigned int
  get_quad_index_velocity_linear() const;

protected:
  unsigned int
  get_dof_index_velocity_scalar() const;

  unsigned int
  get_quad_index_pressure() const;

  unsigned int
  get_quad_index_velocity_nonlinear() const;

  unsigned int
  get_quad_index_velocity_gauss_lobatto() const;

  unsigned int
  get_quad_index_pressure_gauss_lobatto() const;

  unsigned int
  get_quad_index_velocity_linearized() const;

public:
  std::shared_ptr<dealii::Mapping<dim> const>
  get_mapping() const;

  dealii::FESystem<dim> const &
  get_fe_u() const;

  dealii::FE_DGQ<dim> const &
  get_fe_p() const;

  dealii::DoFHandler<dim> const &
  get_dof_handler_u() const;

  dealii::DoFHandler<dim> const &
  get_dof_handler_u_scalar() const;

  dealii::DoFHandler<dim> const &
  get_dof_handler_p() const;

  dealii::AffineConstraints<Number> const &
  get_constraint_p() const;

  dealii::types::global_dof_index
  get_number_of_dofs() const;

  double
  get_viscosity() const;

  dealii::VectorizedArray<Number>
  get_viscosity_boundary_face(unsigned int const face, unsigned int const q) const;

  // Multiphysics coupling via "Cached" boundary conditions
  std::shared_ptr<ContainerInterfaceData<dim, dim, Number>>
  get_container_interface_data();

  void
  set_velocity_ptr(VectorType const & velocity) const;

  /*
   * Initialization of vectors.
   */
  void
  initialize_vector_velocity(VectorType & src) const;

  void
  initialize_vector_velocity_scalar(VectorType & src) const;

  void
  initialize_vector_pressure(VectorType & src) const;

  void
  initialize_block_vector_velocity_pressure(BlockVectorType & src) const;

  /*
   * Prescribe initial conditions using a specified analytical/initial solution function.
   */
  void
  prescribe_initial_conditions(VectorType & velocity,
                               VectorType & pressure,
                               double const time) const;

  // FSI: coupling fluid -> structure
  // fills a DoF-vector (velocity) with values of traction on fluid-structure interface
  void
  interpolate_stress_bc(VectorType &       stress,
                        VectorType const & velocity,
                        VectorType const & pressure) const;

  /*
   * Time step calculation.
   */

  /*
   * Calculate time step size according to maximum efficiency criterion
   */
  double
  calculate_time_step_max_efficiency(unsigned int const order_time_integrator) const;

  // global CFL criterion
  double
  calculate_time_step_cfl_global() const;

  // Calculate time step size according to local CFL criterion
  double
  calculate_time_step_cfl(VectorType const & velocity) const;

  // Calculate CFL numbers of cells
  void
  calculate_cfl_from_time_step(VectorType &       cfl,
                               VectorType const & velocity,
                               double const       time_step_size) const;

  /*
   * Calculates characteristic element length h
   */
  double
  calculate_characteristic_element_length() const;

  /*
   * For certain setups and types of boundary conditions, the pressure field is only defined up to
   * an additive constant which originates from the fact that only the derivative of the pressure
   * appears in the incompressible Navier-Stokes equations. Examples of such a scenario are problems
   * where the velocity is prescribed on the whole boundary or periodic boundaries.
   */

  // This function can be used to query whether the pressure level is undefined.
  bool
  is_pressure_level_undefined() const;

  // This function adjust the pressure level, where different options are available to fix the
  // pressure level. The method selected by this function depends on the specified parameter.
  void
  adjust_pressure_level_if_undefined(VectorType & pressure, double const & time) const;

  /*
   *  Boussinesq approximation
   */
  void
  set_temperature(VectorType const & temperature);

  /*
   * Computation of derived quantities which is needed for postprocessing but some of them are also
   * needed, e.g., for special splitting-type time integration schemes.
   */

  // vorticity
  void
  compute_vorticity(VectorType & dst, VectorType const & src) const;

  // divergence
  void
  compute_divergence(VectorType & dst, VectorType const & src) const;

  // velocity_magnitude
  void
  compute_velocity_magnitude(VectorType & dst, VectorType const & src) const;

  // vorticity_magnitude
  void
  compute_vorticity_magnitude(VectorType & dst, VectorType const & src) const;

  // streamfunction
  void
  compute_streamfunction(VectorType & dst, VectorType const & src) const;

  // Q criterion
  void
  compute_q_criterion(VectorType & dst, VectorType const & src) const;

  /*
   * Operators.
   */

  // mass operator
  void
  apply_mass_operator(VectorType & dst, VectorType const & src) const;

  void
  apply_mass_operator_add(VectorType & dst, VectorType const & src) const;

  // body force term
  void
  evaluate_add_body_force_term(VectorType & dst, double const time) const;

  // convective term
  void
  evaluate_convective_term(VectorType & dst, VectorType const & src, Number const time) const;

  // pressure gradient term
  void
  evaluate_pressure_gradient_term(VectorType &       dst,
                                  VectorType const & src,
                                  double const       time) const;

  // velocity divergence
  void
  evaluate_velocity_divergence_term(VectorType &       dst,
                                    VectorType const & src,
                                    double const       time) const;

  // inverse velocity mass operator
  void
  apply_inverse_mass_operator(VectorType & dst, VectorType const & src) const;

  /*
   *  Update turbulence model, i.e., calculate turbulent viscosity.
   */
  void
  update_turbulence_model(VectorType const & velocity);

  /*
   * Projection step.
   */
  void
  update_projection_operator(VectorType const & velocity, double const time_step_size) const;

  void
  rhs_add_projection_operator(VectorType & dst, double const time) const;

  unsigned int
  solve_projection(VectorType &       dst,
                   VectorType const & src,
                   bool const &       update_preconditioner) const;

  /*
   * Postprocessing.
   */
  double
  calculate_dissipation_convective_term(VectorType const & velocity, double const time) const;

  double
  calculate_dissipation_viscous_term(VectorType const & velocity) const;

  double
  calculate_dissipation_divergence_term(VectorType const & velocity) const;

  double
  calculate_dissipation_continuity_term(VectorType const & velocity) const;

  /*
   * Moves the grid for ALE-type problems.
   */
  void
  move_grid(double const & time) const;

  /*
   * Moves the grid and updates dependent data structures for ALE-type problems.
   */
  void
  move_grid_and_update_dependent_data_structures(double const & time);

  /*
   * Fills a dof-vector with grid coordinates for ALE-type problems.
   */
  void
  fill_grid_coordinates_vector(VectorType & vector) const;

  /*
   * Updates operators after grid has been moved.
   */
  virtual void
  update_after_grid_motion();

  /*
   * Sets the grid velocity.
   */
  void
  set_grid_velocity(VectorType velocity);

protected:
  /*
   * Projection step.
   */
  void
  setup_projection_solver();

  bool
  unsteady_problem_has_to_be_solved() const;

  /*
   * Grid
   */
  std::shared_ptr<Grid<dim> const> grid;

  /*
   * Grid motion for ALE formulations
   */
  std::shared_ptr<GridMotionInterface<dim, Number>> grid_motion;

  /*
   * User interface: Boundary conditions and field functions.
   */
  std::shared_ptr<BoundaryDescriptor<dim> const> boundary_descriptor;
  std::shared_ptr<FieldFunctions<dim> const>     field_functions;

  /*
   * List of parameters.
   */
  Parameters const & param;

  /*
   * A name describing the field being solved.
   */
  std::string const field;

  /*
   * In case of projection type incompressible Navier-Stokes solvers this variable is needed to
   * solve the pressure Poisson equation. However, this variable is also needed in case of a
   * coupled solution approach. In that case, it is used for the block preconditioner (or more
   * precisely for the Schur-complement preconditioner and the preconditioner used to approximately
   * invert the Laplace operator).
   *
   * While the functions specified in BoundaryDescriptorLaplace are relevant for projection-type
   * solvers (pressure Poisson equation has to be solved), the function specified in
   * BoundaryDescriptorLaplace are irrelevant for a coupled solution approach (since the pressure
   * Poisson operator is only needed for preconditioning, and hence, only the homogeneous part of
   * the operator has to be evaluated so that the boundary conditions are never applied).
   *
   */
  std::shared_ptr<Poisson::BoundaryDescriptor<0, dim>> boundary_descriptor_laplace;

  /*
   * Special case: pure Dirichlet boundary conditions.
   */
  dealii::Point<dim>              first_point;
  dealii::types::global_dof_index dof_index_first_point;

  /*
   * Element variable used to store the current physical time. This variable is needed for the
   * evaluation of certain integrals or weak forms.
   */
  double evaluation_time;

private:
  /*
   * Basic finite element ingredients.
   */
  std::shared_ptr<dealii::FESystem<dim>> fe_u;
  dealii::FE_DGQ<dim>                    fe_p;
  dealii::FE_DGQ<dim>                    fe_u_scalar;

  dealii::DoFHandler<dim> dof_handler_u;
  dealii::DoFHandler<dim> dof_handler_p;
  dealii::DoFHandler<dim> dof_handler_u_scalar;

  dealii::AffineConstraints<Number> constraint_u, constraint_p, constraint_u_scalar;

  std::string const dof_index_u        = "velocity";
  std::string const dof_index_p        = "pressure";
  std::string const dof_index_u_scalar = "velocity_scalar";

  std::string const quad_index_u               = "velocity";
  std::string const quad_index_p               = "pressure";
  std::string const quad_index_u_nonlinear     = "velocity_nonlinear";
  std::string const quad_index_u_gauss_lobatto = "velocity_gauss_lobatto";
  std::string const quad_index_p_gauss_lobatto = "pressure_gauss_lobatto";

  std::shared_ptr<MatrixFreeData<dim, Number>>     matrix_free_data;
  std::shared_ptr<dealii::MatrixFree<dim, Number>> matrix_free;

  bool pressure_level_is_undefined;

  /*
   * Interface coupling
   */
  std::shared_ptr<ContainerInterfaceData<dim, dim, Number>> interface_data_dirichlet_cached;

protected:
  /*
   * Operator kernels.
   */
  Operators::ConvectiveKernelData convective_kernel_data;
  Operators::ViscousKernelData    viscous_kernel_data;

  std::shared_ptr<Operators::ConvectiveKernel<dim, Number>> convective_kernel;
  std::shared_ptr<Operators::ViscousKernel<dim, Number>>    viscous_kernel;

  std::shared_ptr<Operators::DivergencePenaltyKernel<dim, Number>> div_penalty_kernel;
  std::shared_ptr<Operators::ContinuityPenaltyKernel<dim, Number>> conti_penalty_kernel;

  /*
   * Basic operators.
   */
  MassOperator<dim, dim, Number>  mass_operator;
  ConvectiveOperator<dim, Number> convective_operator;
  ViscousOperator<dim, Number>    viscous_operator;
  RHSOperator<dim, Number>        rhs_operator;
  GradientOperator<dim, Number>   gradient_operator;
  DivergenceOperator<dim, Number> divergence_operator;

  DivergencePenaltyOperator<dim, Number> div_penalty_operator;
  ContinuityPenaltyOperator<dim, Number> conti_penalty_operator;

  /*
   * Linear(ized) momentum operator.
   */
  mutable MomentumOperator<dim, Number> momentum_operator;

  /*
   * Inverse mass operator.
   */
  InverseMassOperator<dim, dim, Number> inverse_mass_velocity;
  InverseMassOperator<dim, 1, Number>   inverse_mass_velocity_scalar;

  /*
   * Projection operator.
   */
  typedef ProjectionOperator<dim, Number> ProjOperator;
  std::shared_ptr<ProjOperator>           projection_operator;

  /*
   * Projection solver.
   */

  // elementwise solver/preconditioner
  typedef Elementwise::OperatorBase<dim, Number, ProjOperator> ELEMENTWISE_PROJ_OPERATOR;
  std::shared_ptr<ELEMENTWISE_PROJ_OPERATOR>                   elementwise_projection_operator;

  typedef Elementwise::PreconditionerBase<dealii::VectorizedArray<Number>>
                                              ELEMENTWISE_PRECONDITIONER;
  std::shared_ptr<ELEMENTWISE_PRECONDITIONER> elementwise_preconditioner_projection;

  // projection solver
  std::shared_ptr<Krylov::SolverBase<VectorType>> projection_solver;
  std::shared_ptr<PreconditionerBase<Number>>     preconditioner_projection;

  /*
   * Calculators used to obtain derived quantities.
   */
  VorticityCalculator<dim, Number>         vorticity_calculator;
  DivergenceCalculator<dim, Number>        divergence_calculator;
  VelocityMagnitudeCalculator<dim, Number> velocity_magnitude_calculator;
  QCriterionCalculator<dim, Number>        q_criterion_calculator;

  MPI_Comm const mpi_comm;

  dealii::ConditionalOStream pcout;

private:
  // Minimum element length h_min required for global CFL condition.
  double
  calculate_minimum_element_length() const;

  /*
   * Initialization functions called during setup phase.
   */
  void
  initialize_boundary_descriptor_laplace();

  void
  distribute_dofs();

  void
  initialize_operators(std::string const & dof_index_temperature);

  void
  initialize_turbulence_model();

  void
  initialize_calculators_for_derived_quantities();

  void
  initialization_pure_dirichlet_bc();

  void
  cell_loop_empty(dealii::MatrixFree<dim, Number> const &,
                  VectorType &,
                  VectorType const &,
                  Range const &) const
  {
  }

  void
  face_loop_empty(dealii::MatrixFree<dim, Number> const &,
                  VectorType &,
                  VectorType const &,
                  Range const &) const
  {
  }

  void
  local_interpolate_stress_bc_boundary_face(dealii::MatrixFree<dim, Number> const & matrix_free,
                                            VectorType &                            dst,
                                            VectorType const &                      src,
                                            Range const & face_range) const;

  // Interpolation of stress requires velocity and pressure, but the MatrixFree interface
  // only provides one argument, so we store pointers to have access to both velocity and
  // pressure.
  mutable VectorType const * velocity_ptr;
  mutable VectorType const * pressure_ptr;

  /*
   * LES turbulence modeling.
   */
  TurbulenceModel<dim, Number> turbulence_model;
};

} // namespace IncNS
} // namespace ExaDG

#endif /* INCLUDE_EXADG_INCOMPRESSIBLE_NAVIER_STOKES_SPATIAL_DISCRETIZATION_SPATIAL_OPERATOR_BASE_H_ \
        */
