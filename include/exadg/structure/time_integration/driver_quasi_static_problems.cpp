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

#include <exadg/structure/postprocessor/postprocessor_base.h>
#include <exadg/structure/spatial_discretization/interface.h>
#include <exadg/structure/time_integration/driver_quasi_static_problems.h>
#include <exadg/structure/user_interface/parameters.h>
#include <exadg/utilities/print_solver_results.h>

namespace ExaDG
{
namespace Structure
{
template<int dim, typename Number>
DriverQuasiStatic<dim, Number>::DriverQuasiStatic(
  std::shared_ptr<Interface::Operator<Number>> operator_,
  std::shared_ptr<PostProcessorBase<Number>>   postprocessor_,
  Parameters const &                           param_,
  MPI_Comm const &                             mpi_comm_,
  bool const                                   is_test_)
  : pde_operator(operator_),
    postprocessor(postprocessor_),
    param(param_),
    mpi_comm(mpi_comm_),
    is_test(is_test_),
    pcout(std::cout, dealii::Utilities::MPI::this_mpi_process(mpi_comm_) == 0),
    step_number(1),
    timer_tree(new TimerTree()),
    iterations({0, {0, 0}})
{
}

template<int dim, typename Number>
void
DriverQuasiStatic<dim, Number>::setup()
{
  AssertThrow(
    param.large_deformation,
    dealii::ExcMessage(
      "DriverQuasiStatic makes only sense for nonlinear problems. For linear problems, use DriverSteady instead."));

  // initialize global solution vectors (allocation)
  initialize_vectors();

  // initialize solution by interpolation of initial data
  initialize_solution();
}

template<int dim, typename Number>
void
DriverQuasiStatic<dim, Number>::solve()
{
  dealii::Timer timer;
  timer.restart();

  postprocessing();

  do_solve();

  postprocessing();

  timer_tree->insert({"DriverQuasiStatic"}, timer.wall_time());
}

template<int dim, typename Number>
void
DriverQuasiStatic<dim, Number>::print_iterations() const
{
  std::vector<std::string> names;
  std::vector<double>      iterations_avg;

  if(param.large_deformation)
  {
    names = {"Nonlinear iterations",
             "Linear iterations (accumulated)",
             "Linear iterations (per nonlinear it.)"};

    iterations_avg.resize(3);
    iterations_avg[0] =
      (double)std::get<0>(iterations.second) / std::max(1., (double)iterations.first);
    iterations_avg[1] =
      (double)std::get<1>(iterations.second) / std::max(1., (double)iterations.first);
    if(iterations_avg[0] > std::numeric_limits<double>::min())
      iterations_avg[2] = iterations_avg[1] / iterations_avg[0];
    else
      iterations_avg[2] = iterations_avg[1];
  }
  else // linear
  {
    AssertThrow(false, dealii::ExcMessage("Not implemented."));
  }

  print_list_of_iterations(pcout, names, iterations_avg);
}

template<int dim, typename Number>
std::shared_ptr<TimerTree>
DriverQuasiStatic<dim, Number>::get_timings() const
{
  return timer_tree;
}

template<int dim, typename Number>
void
DriverQuasiStatic<dim, Number>::do_solve()
{
  dealii::Timer timer;
  timer.restart();

  pcout << std::endl << "Solving quasi-static problem ..." << std::endl << std::flush;

  // perform time loop
  double       load_factor    = 0.0;
  double       load_increment = param.load_increment;
  double const eps            = 1.e-10;
  while(load_factor < 1.0 - eps)
  {
    std::tuple<unsigned int, unsigned int> iter;

    // compute displacement for new load factor
    if(param.adjust_load_increment)
    {
      // reduce load increment in factors of 2 until the current
      // step can be solved successfully
      bool         success        = false;
      unsigned int re_try_counter = 0;
      while(!success && re_try_counter < 10)
      {
        try
        {
          iter    = solve_step(load_factor + load_increment);
          success = true;
          ++re_try_counter;
        }
        catch(...)
        {
          load_increment *= 0.5;
          pcout << std::endl
                << "Could not solve non-linear problem. Reduce load factor to "
                << load_factor + load_increment << std::flush;
        }
      }
    }
    else
    {
      iter = solve_step(load_factor + load_increment);
    }

    iterations.first += 1;
    std::get<0>(iterations.second) += std::get<0>(iter);
    std::get<1>(iterations.second) += std::get<1>(iter);

    // increment load factor
    load_factor += load_increment;
    ++step_number;

    // adjust increment for next load step
    if(param.adjust_load_increment)
    {
      if(std::get<0>(iter) > 0)
        load_increment *=
          std::pow((double)param.desired_newton_iterations / (double)std::get<0>(iter), 0.5);
    }

    // make sure to hit maximum load exactly
    if(load_factor + load_increment >= 1.0)
      load_increment = 1.0 - load_factor;
  }

  pcout << std::endl << "... done!" << std::endl;

  timer_tree->insert({"DriverQuasiStatic", "Solve"}, timer.wall_time());
}

template<int dim, typename Number>
void
DriverQuasiStatic<dim, Number>::initialize_vectors()
{
  // solution
  pde_operator->initialize_dof_vector(solution);

  // rhs_vector
  pde_operator->initialize_dof_vector(rhs_vector);
}

template<int dim, typename Number>
void
DriverQuasiStatic<dim, Number>::initialize_solution()
{
  pde_operator->prescribe_initial_displacement(solution, 0.0 /* time */);
}

template<int dim, typename Number>
void
DriverQuasiStatic<dim, Number>::output_solver_info_header(double const load_factor)
{
  pcout << std::endl
        << print_horizontal_line() << std::endl
        << std::endl
        << " Solve non-linear problem for load factor = " << std::scientific << std::setprecision(4)
        << load_factor << std::endl
        << print_horizontal_line() << std::endl;
}

template<int dim, typename Number>
std::tuple<unsigned int, unsigned int>
DriverQuasiStatic<dim, Number>::solve_step(double const load_factor)
{
  dealii::Timer timer;
  timer.restart();

  output_solver_info_header(load_factor);

  bool const update_preconditioner =
    this->param.update_preconditioner &&
    ((this->step_number - 1) % this->param.update_preconditioner_every_time_steps == 0);

  VectorType const const_vector; // will not be used
  auto const       iter = pde_operator->solve_nonlinear(
    solution, const_vector, 0.0 /*no mass term*/, load_factor /* = time */, update_preconditioner);

  unsigned int const N_iter_nonlinear = std::get<0>(iter);
  unsigned int const N_iter_linear    = std::get<1>(iter);

  if(not(is_test))
    print_solver_info_nonlinear(pcout, N_iter_nonlinear, N_iter_linear, timer.wall_time());

  return iter;
}

template<int dim, typename Number>
void
DriverQuasiStatic<dim, Number>::postprocessing() const
{
  dealii::Timer timer;
  timer.restart();

  postprocessor->do_postprocessing(solution);

  timer_tree->insert({"DriverQuasiStatic", "Postprocessing"}, timer.wall_time());
}

template class DriverQuasiStatic<2, float>;
template class DriverQuasiStatic<2, double>;

template class DriverQuasiStatic<3, float>;
template class DriverQuasiStatic<3, double>;

} // namespace Structure
} // namespace ExaDG
