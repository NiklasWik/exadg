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

// deal.II
#include <deal.II/base/exceptions.h>

// ExaDG
#include <exadg/incompressible_navier_stokes/user_interface/parameters.h>
#include <exadg/utilities/print_functions.h>

namespace ExaDG
{
namespace IncNS
{
// standard constructor that initializes parameters
Parameters::Parameters()
  : // MATHEMATICAL MODEL
    problem_type(ProblemType::Undefined),
    equation_type(EquationType::Undefined),
    formulation_viscous_term(FormulationViscousTerm::LaplaceFormulation),
    formulation_convective_term(FormulationConvectiveTerm::DivergenceFormulation),
    use_outflow_bc_convective_term(false),
    right_hand_side(false),
    boussinesq_term(false),
    boussinesq_dynamic_part_only(false),

    // ALE
    ale_formulation(false),
    mesh_movement_type(MeshMovementType::Function),
    neumann_with_variable_normal_vector(false),

    // PHYSICAL QUANTITIES
    start_time(0.),
    end_time(-1.),
    viscosity(-1.),
    density(1.),
    thermal_expansion_coefficient(1.0),
    reference_temperature(0.0),

    // TEMPORAL DISCRETIZATION
    solver_type(SolverType::Undefined),
    temporal_discretization(TemporalDiscretization::Undefined),
    treatment_of_convective_term(TreatmentOfConvectiveTerm::Undefined),
    calculation_of_time_step_size(TimeStepCalculation::Undefined),
    adaptive_time_stepping(false),
    adaptive_time_stepping_limiting_factor(1.2),
    time_step_size_max(std::numeric_limits<double>::max()),
    adaptive_time_stepping_cfl_type(CFLConditionType::VelocityNorm),
    max_velocity(-1.),
    cfl(-1.),
    cfl_exponent_fe_degree_velocity(2.0),
    c_eff(-1.),
    time_step_size(-1.),
    max_number_of_time_steps(std::numeric_limits<unsigned int>::max()),
    n_refine_time(0),
    order_time_integrator(1),
    start_with_low_order(true),

    // pseudo time-stepping
    convergence_criterion_steady_problem(ConvergenceCriterionSteadyProblem::Undefined),
    abs_tol_steady(1.e-20),
    rel_tol_steady(1.e-12),

    // output of solver information
    solver_info_data(SolverInfoData()),

    // restart
    restarted_simulation(false),
    restart_data(RestartData()),

    // SPATIAL DISCRETIZATION

    // grid
    grid(GridData()),

    // polynomial degrees
    degree_u(2),
    degree_p(DegreePressure::MixedOrder),

    // convective term
    upwind_factor(1.0),
    type_dirichlet_bc_convective(TypeDirichletBCs::Mirror),

    // viscous term
    IP_formulation_viscous(InteriorPenaltyFormulation::Undefined),
    penalty_term_div_formulation(PenaltyTermDivergenceFormulation::Symmetrized),
    IP_factor_viscous(1.),

    // gradient term
    gradp_integrated_by_parts(true),
    gradp_formulation(FormulationPressureGradientTerm::Weak),
    gradp_use_boundary_data(true),

    // divergence term
    divu_integrated_by_parts(true),
    divu_formulation(FormulationVelocityDivergenceTerm::Weak),
    divu_use_boundary_data(true),

    // special case: pure DBC's
    adjust_pressure_level(AdjustPressureLevel::ApplyZeroMeanValue),

    // div-div and continuity penalty terms
    use_divergence_penalty(true),
    divergence_penalty_factor(1.),
    use_continuity_penalty(true),
    continuity_penalty_factor(1.),
    apply_penalty_terms_in_postprocessing_step(true),
    continuity_penalty_components(ContinuityPenaltyComponents::Normal),
    continuity_penalty_use_boundary_data(false),
    type_penalty_parameter(TypePenaltyParameter::ConvectiveTerm),

    // TURBULENCE
    use_turbulence_model(false),
    turbulence_model_constant(1.0),
    turbulence_model(TurbulenceEddyViscosityModel::Undefined),

    // NUMERICAL PARAMETERS
    implement_block_diagonal_preconditioner_matrix_free(false),
    use_cell_based_face_loops(false),
    solver_data_block_diagonal(SolverData(1000, 1.e-12, 1.e-2, 1000)),
    quad_rule_linearization(QuadratureRuleLinearization::Overintegration32k),

    // PROJECTION METHODS

    // pressure Poisson equation
    IP_factor_pressure(1.),
    solver_pressure_poisson(SolverPressurePoisson::CG),
    solver_data_pressure_poisson(SolverData(1e4, 1.e-12, 1.e-6, 100)),
    preconditioner_pressure_poisson(PreconditionerPressurePoisson::Multigrid),
    multigrid_data_pressure_poisson(MultigridData()),
    update_preconditioner_pressure_poisson(false),
    update_preconditioner_pressure_poisson_every_time_steps(1),

    // projection step
    solver_projection(SolverProjection::CG),
    solver_data_projection(SolverData(1000, 1.e-12, 1.e-6, 100)),
    preconditioner_projection(PreconditionerProjection::InverseMassMatrix),
    multigrid_data_projection(MultigridData()),
    update_preconditioner_projection(false),
    update_preconditioner_projection_every_time_steps(1),
    preconditioner_block_diagonal_projection(Elementwise::Preconditioner::InverseMassMatrix),
    solver_data_block_diagonal_projection(SolverData(1000, 1.e-12, 1.e-2, 1000)),

    // HIGH-ORDER DUAL SPLITTING SCHEME

    // formulations
    order_extrapolation_pressure_nbc((order_time_integrator <= 2) ? order_time_integrator : 2),
    formulation_convective_term_bc(FormulationConvectiveTerm::ConvectiveFormulation),

    // convective step

    // viscous step
    solver_viscous(SolverViscous::CG),
    solver_data_viscous(SolverData(1e4, 1.e-12, 1.e-6, 100)),
    preconditioner_viscous(PreconditionerViscous::InverseMassMatrix),
    update_preconditioner_viscous(false),
    update_preconditioner_viscous_every_time_steps(1),
    multigrid_data_viscous(MultigridData()),

    // PRESSURE-CORRECTION SCHEME

    // momentum step
    newton_solver_data_momentum(Newton::SolverData(1e2, 1.e-12, 1.e-6)),
    solver_momentum(SolverMomentum::GMRES),
    solver_data_momentum(SolverData(1e4, 1.e-12, 1.e-6, 100)),
    preconditioner_momentum(MomentumPreconditioner::InverseMassMatrix),
    update_preconditioner_momentum(false),
    update_preconditioner_momentum_every_newton_iter(1),
    update_preconditioner_momentum_every_time_steps(1),
    multigrid_data_momentum(MultigridData()),
    multigrid_operator_type_momentum(MultigridOperatorType::Undefined),

    // formulations
    order_pressure_extrapolation(1),
    rotational_formulation(false),


    // COUPLED NAVIER-STOKES SOLVER

    // scaling of continuity equation
    use_scaling_continuity(false),
    scaling_factor_continuity(1.0),

    // nonlinear solver (Newton solver)
    newton_solver_data_coupled(Newton::SolverData(1e2, 1.e-12, 1.e-6)),

    // linear solver
    solver_coupled(SolverCoupled::GMRES),
    solver_data_coupled(SolverData(1e4, 1.e-12, 1.e-6, 100)),

    // preconditioning linear solver
    preconditioner_coupled(PreconditionerCoupled::BlockTriangular),
    update_preconditioner_coupled(false),
    update_preconditioner_coupled_every_newton_iter(1),
    update_preconditioner_coupled_every_time_steps(1),

    // preconditioner velocity/momentum block
    preconditioner_velocity_block(MomentumPreconditioner::InverseMassMatrix),
    multigrid_operator_type_velocity_block(MultigridOperatorType::Undefined),
    multigrid_data_velocity_block(MultigridData()),
    exact_inversion_of_velocity_block(false),
    solver_data_velocity_block(SolverData(1e4, 1.e-12, 1.e-6, 100)),

    // preconditioner pressure/Schur-complement block
    preconditioner_pressure_block(SchurComplementPreconditioner::PressureConvectionDiffusion),
    multigrid_data_pressure_block(MultigridData()),
    exact_inversion_of_laplace_operator(false),
    solver_data_pressure_block(SolverData(1e4, 1.e-12, 1.e-6, 100))
{
}

void
Parameters::check(dealii::ConditionalOStream const & pcout) const
{
  // MATHEMATICAL MODEL
  AssertThrow(problem_type != ProblemType::Undefined,
              dealii::ExcMessage("parameter must be defined"));
  AssertThrow(equation_type != EquationType::Undefined,
              dealii::ExcMessage("parameter must be defined"));

  if(equation_type == EquationType::Euler)
  {
    AssertThrow(std::abs(viscosity) < 1.e-15,
                dealii::ExcMessage(
                  "Make sure that the viscosity is zero when solving the Euler equations."));
  }

  AssertThrow(formulation_viscous_term != FormulationViscousTerm::Undefined,
              dealii::ExcMessage("parameter must be defined"));
  AssertThrow(formulation_convective_term != FormulationConvectiveTerm::Undefined,
              dealii::ExcMessage("parameter must be defined"));

  // ALE
  if(ale_formulation)
  {
    AssertThrow(
      formulation_convective_term == FormulationConvectiveTerm::ConvectiveFormulation,
      dealii::ExcMessage(
        "Convective formulation of convective operator has to be used for ALE formulation."));

    AssertThrow(
      problem_type == ProblemType::Unsteady && solver_type == SolverType::Unsteady,
      dealii::ExcMessage(
        "Both problem type and solver type have to be Unsteady when using ALE formulation."));

    AssertThrow(
      convective_problem() == true,
      dealii::ExcMessage(
        "ALE formulation only implemented for equations that include the convective operator, "
        "e.g., ALE is currently not available for the Stokes equations."));
  }

  // PHYSICAL QUANTITIES
  AssertThrow(end_time > start_time, dealii::ExcMessage("parameter end_time must be defined"));
  AssertThrow(viscosity >= 0.0, dealii::ExcMessage("parameter must be defined"));

  // TEMPORAL DISCRETIZATION
  AssertThrow(solver_type != SolverType::Undefined,
              dealii::ExcMessage("parameter must be defined"));
  AssertThrow(temporal_discretization != TemporalDiscretization::Undefined,
              dealii::ExcMessage("parameter must be defined"));

  if(convective_problem())
  {
    AssertThrow(treatment_of_convective_term != TreatmentOfConvectiveTerm::Undefined,
                dealii::ExcMessage("parameter must be defined"));
  }

  AssertThrow(calculation_of_time_step_size != TimeStepCalculation::Undefined,
              dealii::ExcMessage("parameter must be defined"));

  if(calculation_of_time_step_size == TimeStepCalculation::CFL)
  {
    AssertThrow(cfl > 0., dealii::ExcMessage("parameter must be defined"));
    AssertThrow(max_velocity > 0., dealii::ExcMessage("parameter must be defined"));
  }

  if(calculation_of_time_step_size == TimeStepCalculation::UserSpecified)
    AssertThrow(time_step_size > 0., dealii::ExcMessage("parameter must be defined"));

  if(calculation_of_time_step_size == TimeStepCalculation::MaxEfficiency)
    AssertThrow(c_eff > 0., dealii::ExcMessage("parameter must be defined"));

  if(adaptive_time_stepping)
  {
    AssertThrow(calculation_of_time_step_size == TimeStepCalculation::CFL,
                dealii::ExcMessage(
                  "Adaptive time stepping is only implemented for TimeStepCalculation::CFL."));
  }

  if(problem_type == ProblemType::Unsteady)
  {
    AssertThrow(solver_type == SolverType::Unsteady,
                dealii::ExcMessage(
                  "An unsteady solver has to be used to solve unsteady problems."));
  }

  if(solver_type == SolverType::Steady)
  {
    if(convective_problem())
    {
      AssertThrow(treatment_of_convective_term == TreatmentOfConvectiveTerm::Implicit,
                  dealii::ExcMessage(
                    "Convective term has to be formulated implicitly when using a steady solver."));
    }
  }

  // SPATIAL DISCRETIZATION

  grid.check();

  // For the coupled solution approach, degree_p = 0 is allowed in principle.
  // For projection-type methods, degree_p > 0 has to be fulfilled (the SIPG discretization
  // of the pressure Poisson equation would be inconsistent for degree_p = 0).
  if(temporal_discretization != TemporalDiscretization::BDFCoupledSolution)
    AssertThrow(
      get_degree_p(degree_u) > 0,
      dealii::ExcMessage(
        "Polynomial degree of pressure has to be larger than zero for projection-type methods."));

  AssertThrow(IP_formulation_viscous != InteriorPenaltyFormulation::Undefined,
              dealii::ExcMessage("parameter must be defined"));

  if(formulation_viscous_term == FormulationViscousTerm::DivergenceFormulation)
  {
    AssertThrow(penalty_term_div_formulation != PenaltyTermDivergenceFormulation::Undefined,
                dealii::ExcMessage("parameter must be defined"));
  }

  if(equation_type == EquationType::NavierStokes)
  {
    AssertThrow(upwind_factor >= 0.0, dealii::ExcMessage("Upwind factor must not be negative."));
  }

  if(use_continuity_penalty == true)
  {
    AssertThrow(continuity_penalty_components != ContinuityPenaltyComponents::Undefined,
                dealii::ExcMessage("Parameter must be defined"));

    if(continuity_penalty_use_boundary_data == true)
    {
      if(temporal_discretization == TemporalDiscretization::BDFDualSplittingScheme)
      {
        AssertThrow(
          apply_penalty_terms_in_postprocessing_step == true,
          dealii::ExcMessage(
            "Penalty terms have to be applied in postprocessing step if boundary data is used. "
            "Otherwise, the boundary condition will be inconsistent and temporal accuracy is limited to low order."));
      }
    }
  }

  if(use_divergence_penalty == true || use_continuity_penalty == true)
  {
    AssertThrow(type_penalty_parameter != TypePenaltyParameter::Undefined,
                dealii::ExcMessage("Parameter must be defined"));
  }

  if(solver_type == SolverType::Steady)
  {
    if(use_divergence_penalty == true || use_continuity_penalty == true)
    {
      AssertThrow(apply_penalty_terms_in_postprocessing_step == false,
                  dealii::ExcMessage(
                    "Use apply_penalty_terms_in_postprocessing_step = false, "
                    "otherwise the penalty terms will be ignored by the steady solver."));
    }
  }

  // HIGH-ORDER DUAL SPLITTING SCHEME
  if(temporal_discretization == TemporalDiscretization::BDFDualSplittingScheme)
  {
    AssertThrow(order_extrapolation_pressure_nbc <= order_time_integrator,
                dealii::ExcMessage("Invalid parameter order_extrapolation_pressure_nbc!"));

    if(order_extrapolation_pressure_nbc > 2)
    {
      pcout
        << "WARNING:" << std::endl
        << "Order of extrapolation of viscous and convective terms in pressure Neumann boundary"
        << std::endl
        << "condition is larger than 2 which leads to a scheme that is only conditionally stable."
        << std::endl;
    }

    AssertThrow(formulation_convective_term_bc ==
                    FormulationConvectiveTerm::DivergenceFormulation ||
                  formulation_convective_term_bc ==
                    FormulationConvectiveTerm::ConvectiveFormulation,
                dealii::ExcMessage("Not implemented."));

    AssertThrow(treatment_of_convective_term != TreatmentOfConvectiveTerm::Implicit,
                dealii::ExcMessage("An implicit treatment of the convective term is not possible "
                                   "in combination with the dual splitting scheme."));
  }

  // PRESSURE-CORRECTION SCHEME
  if(temporal_discretization == TemporalDiscretization::BDFPressureCorrection)
  {
    AssertThrow(order_pressure_extrapolation <= order_time_integrator,
                dealii::ExcMessage("Invalid parameter order_pressure_extrapolation!"));

    if(preconditioner_momentum == MomentumPreconditioner::Multigrid)
    {
      AssertThrow(multigrid_operator_type_momentum != MultigridOperatorType::Undefined,
                  dealii::ExcMessage("Parameter must be defined"));

      if(treatment_of_convective_term == TreatmentOfConvectiveTerm::Explicit)
      {
        AssertThrow(
          multigrid_operator_type_momentum != MultigridOperatorType::ReactionConvectionDiffusion,
          dealii::ExcMessage("Invalid parameter. Convective term is treated explicitly."));
      }
    }
  }

  // COUPLED NAVIER-STOKES SOLVER
  if(temporal_discretization == TemporalDiscretization::BDFCoupledSolution)
  {
    if(use_scaling_continuity == true)
      AssertThrow(scaling_factor_continuity > 0.0, dealii::ExcMessage("Invalid parameter"));

    if(preconditioner_velocity_block == MomentumPreconditioner::Multigrid)
    {
      AssertThrow(multigrid_operator_type_velocity_block != MultigridOperatorType::Undefined,
                  dealii::ExcMessage("Parameter must be defined"));

      if(equation_type == EquationType::Stokes)
      {
        AssertThrow(multigrid_operator_type_velocity_block !=
                      MultigridOperatorType::ReactionConvectionDiffusion,
                    dealii::ExcMessage(
                      "Invalid parameter (the specified equation type is Stokes)."));
      }

      if(treatment_of_convective_term == TreatmentOfConvectiveTerm::Explicit)
      {
        AssertThrow(multigrid_operator_type_velocity_block !=
                      MultigridOperatorType::ReactionConvectionDiffusion,
                    dealii::ExcMessage(
                      "Invalid parameter. Convective term is treated explicitly."));
      }
    }
  }

  // NUMERICAL PARAMETERS
  if(implement_block_diagonal_preconditioner_matrix_free)
  {
    AssertThrow(
      use_cell_based_face_loops == true,
      dealii::ExcMessage(
        "Cell based face loops have to be used for matrix-free implementation of block diagonal preconditioner."));
  }


  // TURBULENCE
  if(use_turbulence_model)
  {
    AssertThrow(turbulence_model != TurbulenceEddyViscosityModel::Undefined,
                dealii::ExcMessage("parameter must be defined"));
    AssertThrow(turbulence_model_constant > 0,
                dealii::ExcMessage("parameter must be greater than zero"));
  }
}

bool
Parameters::convective_problem() const
{
  return (equation_type == EquationType::NavierStokes || equation_type == EquationType::Euler);
}

bool
Parameters::viscous_problem() const
{
  return (equation_type == EquationType::Stokes || equation_type == EquationType::NavierStokes ||
          use_turbulence_model == true);
}

bool
Parameters::nonlinear_problem_has_to_be_solved() const
{
  return convective_problem() &&
         (solver_type == SolverType::Steady ||
          (solver_type == SolverType::Unsteady &&
           treatment_of_convective_term == TreatmentOfConvectiveTerm::Implicit));
}

bool
Parameters::linear_problem_has_to_be_solved() const
{
  return equation_type == EquationType::Stokes ||
         treatment_of_convective_term == TreatmentOfConvectiveTerm::Explicit;
}

unsigned int
Parameters::get_degree_p(unsigned int const degree_u) const
{
  unsigned int k = 1;

  if(degree_p == DegreePressure::MixedOrder)
  {
    AssertThrow(degree_u > 0,
                dealii::ExcMessage("The polynomial degree of the velocity shape functions"
                                   " has to be larger than zero for a mixed-order formulation."));

    k = degree_u - 1;
  }
  else if(degree_p == DegreePressure::EqualOrder)
  {
    k = degree_u;
  }
  else
  {
    AssertThrow(false, dealii::ExcMessage("Not implemented."));
  }

  return k;
}

void
Parameters::print(dealii::ConditionalOStream const & pcout, std::string const & name) const
{
  pcout << std::endl << name << std::endl;

  // MATHEMATICAL MODEL
  print_parameters_mathematical_model(pcout);

  // PHYSICAL QUANTITIES
  print_parameters_physical_quantities(pcout);

  // TEMPORAL DISCRETIZATION
  if(solver_type == SolverType::Unsteady)
    print_parameters_temporal_discretization(pcout);

  // SPATIAL DISCRETIZATION
  print_parameters_spatial_discretization(pcout);

  // TURBULENCE
  print_parameters_turbulence(pcout);

  // NUMERICAL PARAMTERS
  print_parameters_numerical_parameters(pcout);

  // HIGH-ORDER DUAL SPLITTING SCHEME
  if(temporal_discretization == TemporalDiscretization::BDFDualSplittingScheme)
    print_parameters_dual_splitting(pcout);

  // PRESSURE-CORRECTION  SCHEME
  if(temporal_discretization == TemporalDiscretization::BDFPressureCorrection)
    print_parameters_pressure_correction(pcout);

  // COUPLED NAVIER-STOKES SOLVER
  if(solver_type == SolverType::Steady ||
     (solver_type == SolverType::Unsteady &&
      temporal_discretization == TemporalDiscretization::BDFCoupledSolution))
    print_parameters_coupled_solver(pcout);
}

void
Parameters::print_parameters_mathematical_model(dealii::ConditionalOStream const & pcout) const
{
  pcout << std::endl << "Mathematical model:" << std::endl;

  print_parameter(pcout, "Problem type", enum_to_string(problem_type));
  print_parameter(pcout, "Equation type", enum_to_string(equation_type));

  if(this->viscous_problem())
  {
    print_parameter(pcout, "Formulation of viscous term", enum_to_string(formulation_viscous_term));
  }

  if(this->convective_problem())
  {
    print_parameter(pcout,
                    "Formulation of convective term",
                    enum_to_string(formulation_convective_term));
    print_parameter(pcout, "Outflow BC for convective term", use_outflow_bc_convective_term);
  }

  print_parameter(pcout, "Right-hand side", right_hand_side);
  print_parameter(pcout, "Boussinesq term", boussinesq_term);
  print_parameter(pcout, "Boussinesq - dynamic part only", boussinesq_dynamic_part_only);

  print_parameter(pcout, "Use ALE formulation", ale_formulation);
  if(ale_formulation)
  {
    print_parameter(pcout, "Mesh movement type", enum_to_string(mesh_movement_type));
    print_parameter(pcout, "NBC with variable normal vector", neumann_with_variable_normal_vector);
  }
}


void
Parameters::print_parameters_physical_quantities(dealii::ConditionalOStream const & pcout) const
{
  pcout << std::endl << "Physical quantities:" << std::endl;

  // start and end time
  if(solver_type == SolverType::Unsteady)
  {
    print_parameter(pcout, "Start time", start_time);
    print_parameter(pcout, "End time", end_time);
  }

  // viscosity
  print_parameter(pcout, "Viscosity", viscosity);

  // density
  print_parameter(pcout, "Density", density);

  if(boussinesq_term)
  {
    print_parameter(pcout, "Thermal expansion coefficient", thermal_expansion_coefficient);
    print_parameter(pcout, "Reference temperature", reference_temperature);
  }
}

void
Parameters::print_parameters_temporal_discretization(dealii::ConditionalOStream const & pcout) const
{
  pcout << std::endl << "Temporal discretization:" << std::endl;

  print_parameter(pcout, "Temporal discretization method", enum_to_string(temporal_discretization));
  print_parameter(pcout,
                  "Treatment of convective term",
                  enum_to_string(treatment_of_convective_term));

  print_parameter(pcout,
                  "Calculation of time step size",
                  enum_to_string(calculation_of_time_step_size));

  print_parameter(pcout, "Adaptive time stepping", adaptive_time_stepping);

  if(adaptive_time_stepping)
  {
    print_parameter(pcout,
                    "Adaptive time stepping limiting factor",
                    adaptive_time_stepping_limiting_factor);

    print_parameter(pcout, "Maximum allowable time step size", time_step_size_max);

    print_parameter(pcout,
                    "Type of CFL condition",
                    enum_to_string(adaptive_time_stepping_cfl_type));
  }


  // here we do not print quantities such as max_velocity, cfl, time_step_size
  // because this is done by the time integration scheme (or the functions that
  // calculate the time step size)

  print_parameter(pcout, "Maximum number of time steps", max_number_of_time_steps);
  print_parameter(pcout, "Temporal refinements", n_refine_time);
  print_parameter(pcout, "Order of time integration scheme", order_time_integrator);
  print_parameter(pcout, "Start with low order method", start_with_low_order);

  if(problem_type == ProblemType::Steady)
  {
    print_parameter(pcout,
                    "Convergence criterion steady problems",
                    enum_to_string(convergence_criterion_steady_problem));

    print_parameter(pcout, "Absolute tolerance", abs_tol_steady);
    print_parameter(pcout, "Relative tolerance", rel_tol_steady);
  }

  // output of solver information
  solver_info_data.print(pcout);

  // restart
  print_parameter(pcout, "Restarted simulation", restarted_simulation);
  restart_data.print(pcout);
}

void
Parameters::print_parameters_spatial_discretization(dealii::ConditionalOStream const & pcout) const
{
  pcout << std::endl << "Spatial discretization:" << std::endl;

  grid.print(pcout);

  print_parameter(pcout, "Polynomial degree velocity", degree_u);
  print_parameter(pcout, "Polynomial degree pressure", enum_to_string(degree_p));

  if(this->convective_problem())
  {
    print_parameter(pcout, "Convective term - Upwind factor", upwind_factor);
    print_parameter(pcout,
                    "Convective term - Type of Dirichlet BC's",
                    enum_to_string(type_dirichlet_bc_convective));
  }

  if(this->viscous_problem())
  {
    print_parameter(pcout, "Viscous term - IP formulation", enum_to_string(IP_formulation_viscous));
    print_parameter(pcout, "Viscous term - IP factor", IP_factor_viscous);

    if(formulation_viscous_term == FormulationViscousTerm::DivergenceFormulation)
    {
      print_parameter(pcout,
                      "Penalty term formulation viscous term",
                      enum_to_string(penalty_term_div_formulation));
    }
  }

  // pressure gradient term
  print_parameter(pcout, "Grad(p) - integration by parts", gradp_integrated_by_parts);
  if(gradp_integrated_by_parts)
  {
    print_parameter(pcout, "Grad(p) - formulation", enum_to_string(gradp_formulation));
    print_parameter(pcout, "Grad(p) - use boundary data", gradp_use_boundary_data);
  }

  // divergence term
  print_parameter(pcout, "Div(u) . integration by parts", divu_integrated_by_parts);
  if(divu_integrated_by_parts)
  {
    print_parameter(pcout, "Div(u) - formulation", enum_to_string(divu_formulation));
    print_parameter(pcout, "Div(u) - use boundary data", divu_use_boundary_data);
  }

  print_parameter(pcout,
                  "Adjust pressure level (if undefined)",
                  enum_to_string(adjust_pressure_level));

  print_parameter(pcout, "Use divergence penalty term", use_divergence_penalty);

  if(use_divergence_penalty == true)
  {
    print_parameter(pcout, "Penalty factor divergence", divergence_penalty_factor);
  }

  print_parameter(pcout, "Use continuity penalty term", use_continuity_penalty);

  if(temporal_discretization == TemporalDiscretization::BDFCoupledSolution ||
     temporal_discretization == TemporalDiscretization::BDFDualSplittingScheme)
  {
    if(use_divergence_penalty == true || use_continuity_penalty == true)
    {
      print_parameter(pcout,
                      "Apply penalty terms in postprocessing step",
                      apply_penalty_terms_in_postprocessing_step);
    }
  }

  if(use_continuity_penalty == true)
  {
    print_parameter(pcout, "Use boundary data", continuity_penalty_use_boundary_data);
    print_parameter(pcout, "Penalty factor continuity", continuity_penalty_factor);

    print_parameter(pcout,
                    "Continuity penalty term components",
                    enum_to_string(continuity_penalty_components));
  }

  if(use_divergence_penalty == true || use_continuity_penalty == true)
  {
    print_parameter(pcout, "Type of penalty parameter", enum_to_string(type_penalty_parameter));
  }
}

void
Parameters::print_parameters_turbulence(dealii::ConditionalOStream const & pcout) const
{
  pcout << std::endl << "Turbulence:" << std::endl;

  print_parameter(pcout, "Use turbulence model", use_turbulence_model);

  if(use_turbulence_model == true)
  {
    print_parameter(pcout, "Turbulence model", enum_to_string(turbulence_model));
    print_parameter(pcout, "Turbulence model constant", turbulence_model_constant);
  }
}

void
Parameters::print_parameters_numerical_parameters(dealii::ConditionalOStream const & pcout) const
{
  pcout << std::endl << "Numerical parameters:" << std::endl;

  print_parameter(pcout,
                  "Block Jacobi matrix-free",
                  implement_block_diagonal_preconditioner_matrix_free);

  print_parameter(pcout, "Use cell-based face loops", use_cell_based_face_loops);

  if(implement_block_diagonal_preconditioner_matrix_free)
  {
    solver_data_block_diagonal.print(pcout);
  }

  print_parameter(pcout, "Quadrature rule linearization", enum_to_string(quad_rule_linearization));
}

void
Parameters::print_parameters_pressure_poisson(dealii::ConditionalOStream const & pcout) const
{
  // pressure Poisson equation
  pcout << std::endl << "  Pressure Poisson equation (PPE):" << std::endl;

  print_parameter(pcout, "interior penalty factor", IP_factor_pressure);

  print_parameter(pcout, "Solver", enum_to_string(solver_pressure_poisson));

  solver_data_pressure_poisson.print(pcout);

  print_parameter(pcout, "Preconditioner", enum_to_string(preconditioner_pressure_poisson));

  print_parameter(pcout,
                  "Update preconditioner pressure step",
                  update_preconditioner_pressure_poisson);

  if(update_preconditioner_pressure_poisson)
  {
    print_parameter(pcout,
                    "Update preconditioner every time steps",
                    update_preconditioner_pressure_poisson_every_time_steps);
  }

  if(preconditioner_pressure_poisson == PreconditionerPressurePoisson::Multigrid)
  {
    multigrid_data_pressure_poisson.print(pcout);
  }
}

void
Parameters::print_parameters_projection_step(dealii::ConditionalOStream const & pcout) const
{
  if(use_divergence_penalty == true)
  {
    print_parameter(pcout, "Solver projection step", enum_to_string(solver_projection));

    solver_data_projection.print(pcout);

    if(use_divergence_penalty == true && use_continuity_penalty == true)
    {
      print_parameter(pcout,
                      "Preconditioner projection step",
                      enum_to_string(preconditioner_projection));

      print_parameter(pcout,
                      "Update preconditioner projection step",
                      update_preconditioner_projection);

      if(update_preconditioner_projection)
      {
        print_parameter(pcout,
                        "Update preconditioner every time steps",
                        update_preconditioner_projection_every_time_steps);
      }

      if(preconditioner_projection == PreconditionerProjection::BlockJacobi &&
         implement_block_diagonal_preconditioner_matrix_free)
      {
        print_parameter(pcout,
                        "Preconditioner block diagonal",
                        enum_to_string(preconditioner_block_diagonal_projection));

        solver_data_block_diagonal_projection.print(pcout);
      }

      if(preconditioner_projection == PreconditionerProjection::Multigrid)
      {
        multigrid_data_projection.print(pcout);
      }
    }
  }
}

void
Parameters::print_parameters_dual_splitting(dealii::ConditionalOStream const & pcout) const
{
  pcout << std::endl << "High-order dual splitting scheme:" << std::endl;

  // formulations
  print_parameter(pcout, "Order of extrapolation pressure NBC", order_extrapolation_pressure_nbc);

  if(this->convective_problem())
  {
    print_parameter(pcout,
                    "Formulation convective term in BC",
                    enum_to_string(formulation_convective_term_bc));
  }

  // projection method
  print_parameters_pressure_poisson(pcout);

  // projection step
  pcout << std::endl << "  Projection step:" << std::endl;
  print_parameters_projection_step(pcout);

  // Viscous step
  if(this->viscous_problem())
  {
    pcout << std::endl << "  Viscous step:" << std::endl;

    print_parameter(pcout, "Solver viscous step", enum_to_string(solver_viscous));

    solver_data_viscous.print(pcout);

    print_parameter(pcout, "Preconditioner viscous step", enum_to_string(preconditioner_viscous));

    print_parameter(pcout, "Update preconditioner viscous", update_preconditioner_viscous);

    if(update_preconditioner_viscous)
    {
      print_parameter(pcout,
                      "Update preconditioner every time steps",
                      update_preconditioner_viscous_every_time_steps);
    }

    if(preconditioner_viscous == PreconditionerViscous::Multigrid)
    {
      multigrid_data_viscous.print(pcout);
    }
  }
}

void
Parameters::print_parameters_pressure_correction(dealii::ConditionalOStream const & pcout) const
{
  pcout << std::endl << "Pressure-correction scheme:" << std::endl;

  // formulations of pressure-correction scheme
  pcout << std::endl << "  Formulation of pressure-correction scheme:" << std::endl;
  print_parameter(pcout, "Order of pressure extrapolation", order_pressure_extrapolation);
  print_parameter(pcout, "Rotational formulation", rotational_formulation);

  // Momentum step
  pcout << std::endl << "  Momentum step:" << std::endl;

  // Newton solver
  if(equation_type == EquationType::NavierStokes &&
     treatment_of_convective_term == TreatmentOfConvectiveTerm::Implicit)
  {
    pcout << "  Newton solver:" << std::endl;

    newton_solver_data_momentum.print(pcout);

    pcout << std::endl;
  }

  // Solver linear(ized) problem
  pcout << "  Linear solver:" << std::endl;

  print_parameter(pcout, "Solver", enum_to_string(solver_momentum));

  solver_data_momentum.print(pcout);

  print_parameter(pcout, "Preconditioner", enum_to_string(preconditioner_momentum));

  print_parameter(pcout, "Update of preconditioner", update_preconditioner_momentum);

  if(update_preconditioner_momentum == true)
  {
    // if a nonlinear problem has to be solved
    if(equation_type == EquationType::NavierStokes &&
       treatment_of_convective_term == TreatmentOfConvectiveTerm::Implicit)
    {
      print_parameter(pcout,
                      "Update every Newton iterations",
                      update_preconditioner_momentum_every_newton_iter);
    }

    print_parameter(pcout,
                    "Update every time steps",
                    update_preconditioner_momentum_every_time_steps);
  }

  if(preconditioner_momentum == MomentumPreconditioner::Multigrid)
  {
    print_parameter(pcout,
                    "Multigrid operator type",
                    enum_to_string(multigrid_operator_type_momentum));

    multigrid_data_momentum.print(pcout);
  }

  // projection method
  print_parameters_pressure_poisson(pcout);

  // projection step
  pcout << std::endl << "  Projection step:" << std::endl;
  print_parameters_projection_step(pcout);
}


void
Parameters::print_parameters_coupled_solver(dealii::ConditionalOStream const & pcout) const
{
  pcout << std::endl << "Coupled Navier-Stokes solver:" << std::endl;

  print_parameter(pcout, "Use scaling of continuity equation", use_scaling_continuity);
  if(use_scaling_continuity == true)
    print_parameter(pcout, "Scaling factor continuity equation", scaling_factor_continuity);

  pcout << std::endl;

  // Newton solver

  // if a nonlinear problem has to be solved
  if(equation_type == EquationType::NavierStokes &&
     treatment_of_convective_term == TreatmentOfConvectiveTerm::Implicit)
  {
    pcout << "Newton solver:" << std::endl;

    newton_solver_data_coupled.print(pcout);

    pcout << std::endl;
  }

  // Solver linearized problem
  pcout << "Linear solver:" << std::endl;

  print_parameter(pcout, "Solver", enum_to_string(solver_coupled));

  solver_data_coupled.print(pcout);

  print_parameter(pcout, "Preconditioner", enum_to_string(preconditioner_coupled));

  print_parameter(pcout, "Update preconditioner", update_preconditioner_coupled);

  if(update_preconditioner_coupled == true)
  {
    // if a nonlinear problem has to be solved
    if(equation_type == EquationType::NavierStokes &&
       treatment_of_convective_term == TreatmentOfConvectiveTerm::Implicit)
    {
      print_parameter(pcout,
                      "Update every Newton iterations",
                      update_preconditioner_coupled_every_newton_iter);
    }

    print_parameter(pcout,
                    "Update every time steps",
                    update_preconditioner_coupled_every_time_steps);
  }

  pcout << std::endl << "  Velocity/momentum block:" << std::endl;

  print_parameter(pcout, "Preconditioner", enum_to_string(preconditioner_velocity_block));

  if(preconditioner_velocity_block == MomentumPreconditioner::Multigrid)
  {
    print_parameter(pcout,
                    "Multigrid operator type",
                    enum_to_string(multigrid_operator_type_velocity_block));

    multigrid_data_velocity_block.print(pcout);

    print_parameter(pcout, "Exact inversion of velocity block", exact_inversion_of_velocity_block);

    if(exact_inversion_of_velocity_block == true)
    {
      solver_data_velocity_block.print(pcout);
    }
  }

  pcout << std::endl << "  Pressure/Schur-complement block:" << std::endl;

  print_parameter(pcout, "Preconditioner", enum_to_string(preconditioner_pressure_block));

  if(preconditioner_pressure_block == SchurComplementPreconditioner::LaplaceOperator ||
     preconditioner_pressure_block == SchurComplementPreconditioner::CahouetChabard ||
     preconditioner_pressure_block == SchurComplementPreconditioner::PressureConvectionDiffusion)
  {
    multigrid_data_pressure_block.print(pcout);

    print_parameter(pcout,
                    "Exact inversion of Laplace operator",
                    exact_inversion_of_laplace_operator);

    if(exact_inversion_of_laplace_operator)
    {
      solver_data_pressure_block.print(pcout);
    }
  }

  // projection_step
  if(use_divergence_penalty == true || use_continuity_penalty == true)
  {
    if(apply_penalty_terms_in_postprocessing_step == true)
    {
      pcout << std::endl << "Postprocessing of velocity (penalty terms):" << std::endl;
      print_parameters_projection_step(pcout);
    }
  }
}

} // namespace IncNS
} // namespace ExaDG
