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

#ifndef APPLICATIONS_COMPRESSIBLE_NAVIER_STOKES_TEST_CASES_TURBULENT_CHANNEL_H_
#define APPLICATIONS_COMPRESSIBLE_NAVIER_STOKES_TEST_CASES_TURBULENT_CHANNEL_H_

#include <exadg/postprocessor/statistics_manager.h>

namespace ExaDG
{
namespace CompNS
{
// set problem specific parameters like physical dimensions, etc.
double const DIMENSIONS_X1 = 2.0 * dealii::numbers::PI;
double const DIMENSIONS_X2 = 2.0;
double const DIMENSIONS_X3 = dealii::numbers::PI;

// set Re = u_tau * delta / nu, density=1, u_tau=1, delta=1 -> calculate kinematic and dynamic
// viscosities
double const Re            = 180.0;
double const RHO_0         = 1.0;
double const nu            = 1.0 / Re;
double const DYN_VISCOSITY = RHO_0 * nu;

// set R, gamma, Pr -> calculate c_p, lambda
double const R       = 287.0;
double const GAMMA   = 1.4;
double const C_P     = GAMMA / (GAMMA - 1.0) * R;
double const PRANDTL = 0.71; // Pr = mu * c_p / lambda
double const LAMBDA  = DYN_VISCOSITY * C_P / PRANDTL;

// set Ma number -> calculate speed of sound c_0, temperature T_0
double const MACH           = 0.1;
double const MAX_VELOCITY   = 18.3; // 18.3 for Re_tau = 180;
double const SPEED_OF_SOUND = MAX_VELOCITY / MACH;
double const T_0            = SPEED_OF_SOUND * SPEED_OF_SOUND / GAMMA / R;

// flow-through time based on mean centerline velocity
double const CHARACTERISTIC_TIME = DIMENSIONS_X1 / MAX_VELOCITY;

double const START_TIME = 0.0;
double const END_TIME   = 200.0 * CHARACTERISTIC_TIME;

double const SAMPLE_START_TIME = 100.0 * CHARACTERISTIC_TIME;
double const SAMPLE_END_TIME   = END_TIME;

// use a negative GRID_STRETCH_FAC to deactivate grid stretching
double const GRID_STRETCH_FAC = 1.8;

/*
 *  maps eta in [0,1] --> y in [-1,1]*length_y/2.0 (using a hyperbolic mesh stretching)
 */
double
grid_transform_y(double const & eta)
{
  double y = 0.0;

  if(GRID_STRETCH_FAC >= 0)
    y = DIMENSIONS_X2 / 2.0 * std::tanh(GRID_STRETCH_FAC * (2. * eta - 1.)) /
        std::tanh(GRID_STRETCH_FAC);
  else // use a negative GRID_STRETCH_FACTOR to deactivate grid stretching
    y = DIMENSIONS_X2 / 2.0 * (2. * eta - 1.);

  return y;
}

/*
 * inverse mapping:
 *
 *  maps y in [-1,1]*length_y/2.0 --> eta in [0,1]
 */
double
inverse_grid_transform_y(double const & y)
{
  double eta = 0.0;

  if(GRID_STRETCH_FAC >= 0)
    eta =
      (std::atanh(y * std::tanh(GRID_STRETCH_FAC) * 2.0 / DIMENSIONS_X2) / GRID_STRETCH_FAC + 1.0) /
      2.0;
  else // use a negative GRID_STRETCH_FACTOR to deactivate grid stretching
    eta = (2. * y / DIMENSIONS_X2 + 1.) / 2.0;

  return eta;
}

template<int dim>
class ManifoldTurbulentChannel : public dealii::ChartManifold<dim, dim, dim>
{
public:
  ManifoldTurbulentChannel(dealii::Tensor<1, dim> const & dimensions_in)
  {
    dimensions = dimensions_in;
  }

  /*
   *  push_forward operation that maps point xi in reference coordinates [0,1]^d to
   *  point x in physical coordinates
   */
  dealii::Point<dim>
  push_forward(dealii::Point<dim> const & xi) const final
  {
    dealii::Point<dim> x;

    x[0] = xi[0] * dimensions[0] - dimensions[0] / 2.0;
    x[1] = grid_transform_y(xi[1]);

    if(dim == 3)
      x[2] = xi[2] * dimensions[2] - dimensions[2] / 2.0;

    return x;
  }

  /*
   *  pull_back operation that maps point x in physical coordinates
   *  to point xi in reference coordinates [0,1]^d
   */
  dealii::Point<dim>
  pull_back(dealii::Point<dim> const & x) const final
  {
    dealii::Point<dim> xi;

    xi[0] = x[0] / dimensions[0] + 0.5;
    xi[1] = inverse_grid_transform_y(x[1]);

    if(dim == 3)
      xi[2] = x[2] / dimensions[2] + 0.5;

    return xi;
  }

  std::unique_ptr<dealii::Manifold<dim>>
  clone() const final
  {
    return std::make_unique<ManifoldTurbulentChannel<dim>>(dimensions);
  }

private:
  dealii::Tensor<1, dim> dimensions;
};

template<int dim>
class InitialSolution : public dealii::Function<dim>
{
public:
  InitialSolution(unsigned int const n_components = dim + 2, double const time = 0.)
    : dealii::Function<dim>(n_components, time)
  {
  }

  double
  value(dealii::Point<dim> const & p, unsigned int const component = 0) const
  {
    double const tol = 1.e-12;
    AssertThrow(std::abs(p[1]) < DIMENSIONS_X2 / 2.0 + tol,
                dealii::ExcMessage("Invalid geometry parameters."));

    double velocity1 =
      -MAX_VELOCITY * (pow(p[1], 6.0) - 1.0) *
      (1.0 + ((double)rand() / RAND_MAX - 1.0) * 0.5 - 2. / MAX_VELOCITY * std::sin(p[2] * 8.));
    double velocity3 = (pow(p[1], 6.0) - 1.0) * std::sin(p[0] * 8.) * 2.;

    // viscous time step limitations: consider a laminar test case with a large viscosity
    //    double velocity1 =  -MAX_VELOCITY*(pow(p[1],2.0)-1.0);
    //    double velocity3 = 0.0;

    double const u1  = velocity1;
    double const u2  = 0.0;
    double const u3  = velocity3;
    double const rho = RHO_0;
    double const T   = T_0;
    double const E   = R / (GAMMA - 1.0) * T /* e = c_v * T */
                     + 0.5 * (u1 * u1 + u2 * u2 + u3 * u3);

    double result = 0.0;

    if(component == 0)
      result = rho;
    else if(component == 1)
      result = rho * u1;
    else if(component == 2)
      result = rho * u2;
    else if(component == 3)
      result = 0.0;
    else if(component == 1 + dim)
      result = rho * E;

    return result;
  }
};

template<int dim>
struct MyPostProcessorData
{
  PostProcessorData<dim> pp_data;
  TurbulentChannelData   turb_ch_data;
};

template<int dim, typename Number>
class MyPostProcessor : public PostProcessor<dim, Number>
{
public:
  typedef PostProcessor<dim, Number> Base;

  typedef typename Base::VectorType VectorType;

  MyPostProcessor(MyPostProcessorData<dim> const & pp_data_turb_channel, MPI_Comm const & mpi_comm)
    : Base(pp_data_turb_channel.pp_data, mpi_comm), turb_ch_data(pp_data_turb_channel.turb_ch_data)
  {
  }

  void
  setup(Operator<dim, Number> const & pde_operator)
  {
    // call setup function of base class
    Base::setup(pde_operator);

    // perform setup of turbulent channel related things
    statistics_turb_ch.reset(
      new StatisticsManager<dim, Number>(pde_operator.get_dof_handler_vector(),
                                         pde_operator.get_mapping()));

    statistics_turb_ch->setup(&grid_transform_y, turb_ch_data);
  }

  void
  do_postprocessing(VectorType const & solution, double const time, int const time_step_number)
  {
    Base::do_postprocessing(solution, time, time_step_number);

    statistics_turb_ch->evaluate(this->velocity, time, time_step_number);
  }

  TurbulentChannelData                            turb_ch_data;
  std::shared_ptr<StatisticsManager<dim, Number>> statistics_turb_ch;
};

template<int dim, typename Number>
class Application : public ApplicationBase<dim, Number>
{
public:
  Application(std::string input_file, MPI_Comm const & comm)
    : ApplicationBase<dim, Number>(input_file, comm)
  {
  }

private:
  void
  set_parameters() final
  {
    // MATHEMATICAL MODEL
    this->param.equation_type   = EquationType::NavierStokes;
    this->param.right_hand_side = true;

    // PHYSICAL QUANTITIES
    this->param.start_time            = START_TIME;
    this->param.end_time              = END_TIME;
    this->param.dynamic_viscosity     = DYN_VISCOSITY;
    this->param.reference_density     = RHO_0;
    this->param.heat_capacity_ratio   = GAMMA;
    this->param.thermal_conductivity  = LAMBDA;
    this->param.specific_gas_constant = R;
    this->param.max_temperature       = T_0;

    // TEMPORAL DISCRETIZATION
    this->param.temporal_discretization       = TemporalDiscretization::ExplRK3Stage7Reg2;
    this->param.order_time_integrator         = 3;
    this->param.stages                        = 7;
    this->param.calculation_of_time_step_size = TimeStepCalculation::CFLAndDiffusion;
    this->param.time_step_size                = 1.0e-3;
    this->param.max_velocity                  = MAX_VELOCITY;
    this->param.cfl_number                    = 1.5;
    this->param.diffusion_number              = 0.17;
    this->param.exponent_fe_degree_cfl        = 1.5;
    this->param.exponent_fe_degree_viscous    = 3.0;

    // output of solver information
    this->param.solver_info_data.interval_time = CHARACTERISTIC_TIME;

    // SPATIAL DISCRETIZATION
    this->param.grid.triangulation_type = TriangulationType::Distributed;
    this->param.grid.mapping_degree     = 1;
    this->param.n_q_points_convective   = QuadratureRule::Overintegration32k;
    this->param.n_q_points_viscous      = QuadratureRule::Overintegration32k;

    // viscous term
    this->param.IP_factor = 1.0;

    // NUMERICAL PARAMETERS
    this->param.use_combined_operator = true;
  }

  void
  create_grid() final
  {
    dealii::Tensor<1, dim> dimensions;
    dimensions[0] = DIMENSIONS_X1;
    dimensions[1] = DIMENSIONS_X2;
    if(dim == 3)
      dimensions[2] = DIMENSIONS_X3;

    dealii::GridGenerator::hyper_rectangle(*this->grid->triangulation,
                                           dealii::Point<dim>(-dimensions / 2.0),
                                           dealii::Point<dim>(dimensions / 2.0));

    // manifold
    unsigned int manifold_id = 1;
    for(auto cell : *this->grid->triangulation)
    {
      cell.set_all_manifold_ids(manifold_id);
    }

    // apply mesh stretching towards no-slip boundaries in y-direction
    static const ManifoldTurbulentChannel<dim> manifold(dimensions);
    this->grid->triangulation->set_manifold(manifold_id, manifold);

    // periodicity in x- and z-direction
    // add 10 to avoid conflicts with dirichlet boundary, which is 0
    this->grid->triangulation->begin()->face(0)->set_all_boundary_ids(0 + 10);
    this->grid->triangulation->begin()->face(1)->set_all_boundary_ids(1 + 10);
    // periodicity in z-direction
    if(dim == 3)
    {
      this->grid->triangulation->begin()->face(4)->set_all_boundary_ids(2 + 10);
      this->grid->triangulation->begin()->face(5)->set_all_boundary_ids(3 + 10);
    }

    dealii::GridTools::collect_periodic_faces(
      *this->grid->triangulation, 0 + 10, 1 + 10, 0, this->grid->periodic_faces);
    if(dim == 3)
      dealii::GridTools::collect_periodic_faces(
        *this->grid->triangulation, 2 + 10, 3 + 10, 2, this->grid->periodic_faces);

    this->grid->triangulation->add_periodicity(this->grid->periodic_faces);

    this->grid->triangulation->refine_global(this->param.grid.n_refine_global);
  }

  void
  set_boundary_descriptor() final
  {
    typedef typename std::pair<dealii::types::boundary_id, std::shared_ptr<dealii::Function<dim>>>
                                                                                   pair;
    typedef typename std::pair<dealii::types::boundary_id, EnergyBoundaryVariable> pair_variable;

    // For Neumann boundaries, no value is prescribed (only first derivative of density occurs in
    // equations). Hence the specified function is irrelevant (i.e., it is not used).
    this->boundary_descriptor->density.neumann_bc.insert(
      pair(0, new dealii::Functions::ZeroFunction<dim>(1)));
    this->boundary_descriptor->velocity.dirichlet_bc.insert(
      pair(0, new dealii::Functions::ZeroFunction<dim>(dim)));
    this->boundary_descriptor->pressure.neumann_bc.insert(
      pair(0, new dealii::Functions::ZeroFunction<dim>(1)));

    // energy: prescribe temperature
    this->boundary_descriptor->energy.boundary_variable.insert(
      pair_variable(0, EnergyBoundaryVariable::Temperature));
    this->boundary_descriptor->energy.dirichlet_bc.insert(
      pair(0, new dealii::Functions::ConstantFunction<dim>(T_0, 1)));
  }

  void
  set_field_functions() final
  {
    this->field_functions->initial_solution.reset(new InitialSolution<dim>());
    this->field_functions->right_hand_side_density.reset(
      new dealii::Functions::ZeroFunction<dim>(1));
    std::vector<double> forcing = std::vector<double>(dim, 0.0);
    forcing[0]                  = RHO_0; // constant forcing in x_1-direction
    this->field_functions->right_hand_side_velocity.reset(
      new dealii::Functions::ConstantFunction<dim>(forcing));
    this->field_functions->right_hand_side_energy.reset(
      new dealii::Functions::ZeroFunction<dim>(1));
  }

  std::shared_ptr<PostProcessorBase<dim, Number>>
  create_postprocessor() final
  {
    PostProcessorData<dim> pp_data;
    pp_data.output_data.write_output = this->output_parameters.write;
    pp_data.output_data.directory    = this->output_parameters.directory + "vtu/";
    pp_data.output_data.filename     = this->output_parameters.filename;
    pp_data.calculate_velocity = true; // activate this for kinetic energy calculations (see below)
    pp_data.output_data.write_pressure     = true;
    pp_data.output_data.write_velocity     = true;
    pp_data.output_data.write_temperature  = true;
    pp_data.output_data.write_vorticity    = false;
    pp_data.output_data.write_divergence   = false;
    pp_data.output_data.start_time         = START_TIME;
    pp_data.output_data.interval_time      = 1.0;
    pp_data.output_data.degree             = this->param.degree;
    pp_data.output_data.write_higher_order = false;

    MyPostProcessorData<dim> pp_data_turb_ch;
    pp_data_turb_ch.pp_data = pp_data;

    // turbulent channel statistics
    pp_data_turb_ch.turb_ch_data.calculate              = true;
    pp_data_turb_ch.turb_ch_data.cells_are_stretched    = true;
    pp_data_turb_ch.turb_ch_data.sample_start_time      = SAMPLE_START_TIME;
    pp_data_turb_ch.turb_ch_data.sample_end_time        = SAMPLE_END_TIME;
    pp_data_turb_ch.turb_ch_data.sample_every_timesteps = 10;
    pp_data_turb_ch.turb_ch_data.viscosity              = DYN_VISCOSITY;
    pp_data_turb_ch.turb_ch_data.density                = RHO_0;
    pp_data_turb_ch.turb_ch_data.directory              = this->output_parameters.directory;
    pp_data_turb_ch.turb_ch_data.filename               = this->output_parameters.filename;

    std::shared_ptr<PostProcessorBase<dim, Number>> pp;
    pp.reset(new MyPostProcessor<dim, Number>(pp_data_turb_ch, this->mpi_comm));

    return pp;
  }
};

} // namespace CompNS

} // namespace ExaDG

#include <exadg/compressible_navier_stokes/user_interface/implement_get_application.h>

#endif /* APPLICATIONS_COMPRESSIBLE_NAVIER_STOKES_TEST_CASES_TURBULENT_CHANNEL_H_ */
