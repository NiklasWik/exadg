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

// C/C++
#include <fstream>

// deal.II
#include <deal.II/numerics/data_out.h>

// ExaDG
#include <exadg/incompressible_navier_stokes/postprocessor/output_generator.h>
#include <exadg/incompressible_navier_stokes/spatial_discretization/spatial_operator_base.h>
#include <exadg/postprocessor/write_output.h>
#include <exadg/utilities/create_directories.h>

namespace ExaDG
{
namespace IncNS
{
template<int dim, typename Number>
void
write_output(OutputData const &                                         output_data,
             dealii::DoFHandler<dim> const &                            dof_handler_velocity,
             dealii::DoFHandler<dim> const &                            dof_handler_pressure,
             dealii::Mapping<dim> const &                               mapping,
             dealii::LinearAlgebra::distributed::Vector<Number> const & velocity,
             dealii::LinearAlgebra::distributed::Vector<Number> const & pressure,
             std::vector<SolutionField<dim, Number>> const &            additional_fields,
             unsigned int const                                         output_counter,
             MPI_Comm const &                                           mpi_comm)
{
  std::string folder = output_data.directory, file = output_data.filename;

  dealii::DataOutBase::VtkFlags flags;
  flags.write_higher_order_cells = output_data.write_higher_order;

  dealii::DataOut<dim> data_out;
  data_out.set_flags(flags);

  std::vector<std::string> velocity_names(dim, "velocity");
  std::vector<dealii::DataComponentInterpretation::DataComponentInterpretation>
    velocity_component_interpretation(
      dim, dealii::DataComponentInterpretation::component_is_part_of_vector);

  data_out.add_data_vector(dof_handler_velocity,
                           velocity,
                           velocity_names,
                           velocity_component_interpretation);

  data_out.add_data_vector(dof_handler_pressure, pressure, "p");

  // vector needs to survive until build_patches
  dealii::Vector<double> aspect_ratios;
  if(output_data.write_aspect_ratio)
  {
    aspect_ratios =
      dealii::GridTools::compute_aspect_ratio_of_cells(mapping,
                                                       dof_handler_velocity.get_triangulation(),
                                                       dealii::QGauss<dim>(4));
    data_out.add_data_vector(aspect_ratios, "aspect_ratio");
  }

  for(typename std::vector<SolutionField<dim, Number>>::const_iterator it =
        additional_fields.begin();
      it != additional_fields.end();
      ++it)
  {
    if(it->type == SolutionFieldType::scalar)
    {
      data_out.add_data_vector(*it->dof_handler, *it->vector, it->name);
    }
    else if(it->type == SolutionFieldType::cellwise)
    {
      data_out.add_data_vector(*it->vector, it->name);
    }
    else if(it->type == SolutionFieldType::vector)
    {
      std::vector<std::string> names(dim, it->name);
      std::vector<dealii::DataComponentInterpretation::DataComponentInterpretation>
        component_interpretation(dim,
                                 dealii::DataComponentInterpretation::component_is_part_of_vector);

      data_out.add_data_vector(*it->dof_handler, *it->vector, names, component_interpretation);
    }
    else
    {
      AssertThrow(false, dealii::ExcMessage("Not implemented."));
    }
  }

  data_out.build_patches(mapping, output_data.degree, dealii::DataOut<dim>::curved_inner_cells);

  data_out.write_vtu_with_pvtu_record(folder, file, output_counter, mpi_comm, 4);
}

template<int dim, typename Number>
OutputGenerator<dim, Number>::OutputGenerator(MPI_Comm const & comm)
  : mpi_comm(comm), output_counter(0), reset_counter(true), counter_mean_velocity(0)
{
}

template<int dim, typename Number>
void
OutputGenerator<dim, Number>::setup(NavierStokesOperator const &    navier_stokes_operator_in,
                                    dealii::DoFHandler<dim> const & dof_handler_velocity_in,
                                    dealii::DoFHandler<dim> const & dof_handler_pressure_in,
                                    dealii::Mapping<dim> const &    mapping_in,
                                    OutputData const &              output_data_in)
{
  navier_stokes_operator = &navier_stokes_operator_in;
  dof_handler_velocity   = &dof_handler_velocity_in;
  dof_handler_pressure   = &dof_handler_pressure_in;
  mapping                = &mapping_in;
  output_data            = output_data_in;

  // reset output counter
  output_counter = output_data.start_counter;

  initialize_additional_fields();

  if(output_data.write_output == true)
  {
    create_directories(output_data.directory, mpi_comm);

    // Visualize boundary IDs:
    // since boundary IDs typically do not change during the simulation, we only do this
    // once at the beginning of the simulation (i.e., in the setup function).
    if(output_data.write_boundary_IDs)
    {
      write_boundary_IDs(dof_handler_velocity->get_triangulation(),
                         output_data.directory,
                         output_data.filename,
                         mpi_comm);
    }

    // write surface mesh
    if(output_data.write_surface_mesh)
    {
      write_surface_mesh(dof_handler_velocity->get_triangulation(),
                         *mapping,
                         output_data.degree,
                         output_data.directory,
                         output_data.filename,
                         0,
                         mpi_comm);
    }

    if(output_data.write_grid)
    {
      write_grid(dof_handler_velocity->get_triangulation(),
                 output_data.directory,
                 output_data.filename);
    }

    // processor_id
    if(output_data.write_processor_id)
    {
      dealii::GridOut grid_out;

      grid_out.write_mesh_per_processor_as_vtu(dof_handler_velocity->get_triangulation(),
                                               output_data.directory + output_data.filename +
                                                 "_processor_id");
    }
  }
}

template<int dim, typename Number>
void
OutputGenerator<dim, Number>::evaluate(VectorType const & velocity,
                                       VectorType const & pressure,
                                       double const &     time,
                                       int const &        time_step_number)
{
  if(output_data.write_output == true)
  {
    dealii::ConditionalOStream pcout(std::cout,
                                     dealii::Utilities::MPI::this_mpi_process(mpi_comm) == 0);

    if(time_step_number >= 0) // unsteady problem
    {
      // small number which is much smaller than the time step size
      double const EPSILON = 1.0e-10;

      // In the first time step, the current time might be larger than start_time. In that
      // case, we first have to reset the counter in order to avoid that output is written every
      // time step.
      if(reset_counter)
      {
        if(time > output_data.start_time)
        {
          output_counter +=
            int((time - output_data.start_time + EPSILON) / output_data.interval_time);
        }
        reset_counter = false;
      }

      if(time > (output_data.start_time + output_counter * output_data.interval_time - EPSILON))
      {
        pcout << std::endl
              << "OUTPUT << Write data at time t = " << std::scientific << std::setprecision(4)
              << time << std::endl;

        calculate_additional_fields(velocity, time, time_step_number);

        write_output<dim>(output_data,
                          *dof_handler_velocity,
                          *dof_handler_pressure,
                          *mapping,
                          velocity,
                          pressure,
                          additional_fields,
                          output_counter,
                          mpi_comm);

        ++output_counter;
      }
    }
    else // steady problem (time_step_number = -1)
    {
      pcout << std::endl
            << "OUTPUT << Write " << (output_counter == 0 ? "initial" : "solution") << " data"
            << std::endl;

      calculate_additional_fields(velocity, time, time_step_number);

      write_output<dim>(output_data,
                        *dof_handler_velocity,
                        *dof_handler_pressure,
                        *mapping,
                        velocity,
                        pressure,
                        additional_fields,
                        output_counter,
                        mpi_comm);

      ++output_counter;
    }
  }
}

template<int dim, typename Number>
void
OutputGenerator<dim, Number>::initialize_additional_fields()
{
  if(output_data.write_output == true)
  {
    // vorticity
    if(output_data.write_vorticity == true)
    {
      navier_stokes_operator->initialize_vector_velocity(vorticity);

      SolutionField<dim, Number> sol;
      sol.type        = SolutionFieldType::vector;
      sol.name        = "vorticity";
      sol.dof_handler = &navier_stokes_operator->get_dof_handler_u();
      sol.vector      = &vorticity;
      this->additional_fields.push_back(sol);
    }

    // divergence
    if(output_data.write_divergence == true)
    {
      navier_stokes_operator->initialize_vector_velocity_scalar(divergence);

      SolutionField<dim, Number> sol;
      sol.type        = SolutionFieldType::scalar;
      sol.name        = "div_u";
      sol.dof_handler = &navier_stokes_operator->get_dof_handler_u_scalar();
      sol.vector      = &divergence;
      this->additional_fields.push_back(sol);
    }

    // velocity magnitude
    if(output_data.write_velocity_magnitude == true)
    {
      navier_stokes_operator->initialize_vector_velocity_scalar(velocity_magnitude);

      SolutionField<dim, Number> sol;
      sol.type        = SolutionFieldType::scalar;
      sol.name        = "velocity_magnitude";
      sol.dof_handler = &navier_stokes_operator->get_dof_handler_u_scalar();
      sol.vector      = &velocity_magnitude;
      this->additional_fields.push_back(sol);
    }

    // vorticity magnitude
    if(output_data.write_vorticity_magnitude == true)
    {
      navier_stokes_operator->initialize_vector_velocity_scalar(vorticity_magnitude);

      SolutionField<dim, Number> sol;
      sol.type        = SolutionFieldType::scalar;
      sol.name        = "vorticity_magnitude";
      sol.dof_handler = &navier_stokes_operator->get_dof_handler_u_scalar();
      sol.vector      = &vorticity_magnitude;
      this->additional_fields.push_back(sol);
    }


    // streamfunction
    if(output_data.write_streamfunction == true)
    {
      navier_stokes_operator->initialize_vector_velocity_scalar(streamfunction);

      SolutionField<dim, Number> sol;
      sol.type        = SolutionFieldType::scalar;
      sol.name        = "streamfunction";
      sol.dof_handler = &navier_stokes_operator->get_dof_handler_u_scalar();
      sol.vector      = &streamfunction;
      this->additional_fields.push_back(sol);
    }

    // q criterion
    if(output_data.write_q_criterion == true)
    {
      navier_stokes_operator->initialize_vector_velocity_scalar(q_criterion);

      SolutionField<dim, Number> sol;
      sol.type        = SolutionFieldType::scalar;
      sol.name        = "q_criterion";
      sol.dof_handler = &navier_stokes_operator->get_dof_handler_u_scalar();
      sol.vector      = &q_criterion;
      this->additional_fields.push_back(sol);
    }

    // mean velocity
    if(output_data.mean_velocity.calculate == true)
    {
      navier_stokes_operator->initialize_vector_velocity(mean_velocity);

      SolutionField<dim, Number> sol;
      sol.type        = SolutionFieldType::vector;
      sol.name        = "mean_velocity";
      sol.dof_handler = &navier_stokes_operator->get_dof_handler_u();
      sol.vector      = &mean_velocity;
      this->additional_fields.push_back(sol);
    }

    // cfl
    if(output_data.write_cfl)
    {
      SolutionField<dim, Number> sol;
      sol.type   = SolutionFieldType::cellwise;
      sol.name   = "cfl_relative";
      sol.vector = &cfl_vector;
      this->additional_fields.push_back(sol);
    }
  }
}

template<int dim, typename Number>
void
OutputGenerator<dim, Number>::compute_mean_velocity(VectorType &       mean_velocity,
                                                    VectorType const & velocity,
                                                    double const       time,
                                                    int const          time_step_number)
{
  if(time >= output_data.mean_velocity.sample_start_time &&
     time <= output_data.mean_velocity.sample_end_time &&
     time_step_number % output_data.mean_velocity.sample_every_timesteps == 0)
  {
    mean_velocity.sadd((double)counter_mean_velocity, 1.0, velocity);
    ++counter_mean_velocity;
    mean_velocity *= 1. / (double)counter_mean_velocity;
  }
}


template<int dim, typename Number>
void
OutputGenerator<dim, Number>::calculate_additional_fields(VectorType const & velocity,
                                                          double const &     time,
                                                          int const &        time_step_number)
{
  if(output_data.write_output)
  {
    bool vorticity_is_up_to_date = false;
    if(output_data.write_vorticity == true)
    {
      navier_stokes_operator->compute_vorticity(vorticity, velocity);
      vorticity_is_up_to_date = true;
    }

    if(output_data.write_divergence == true)
    {
      navier_stokes_operator->compute_divergence(divergence, velocity);
    }

    if(output_data.write_velocity_magnitude == true)
    {
      navier_stokes_operator->compute_velocity_magnitude(velocity_magnitude, velocity);
    }

    if(output_data.write_vorticity_magnitude == true)
    {
      AssertThrow(vorticity_is_up_to_date == true,
                  dealii::ExcMessage(
                    "Vorticity vector needs to be updated to compute its magnitude."));

      navier_stokes_operator->compute_vorticity_magnitude(vorticity_magnitude, vorticity);
    }

    if(output_data.write_streamfunction == true)
    {
      AssertThrow(vorticity_is_up_to_date == true,
                  dealii::ExcMessage(
                    "Vorticity vector needs to be updated to compute its magnitude."));

      navier_stokes_operator->compute_streamfunction(streamfunction, vorticity);
    }

    if(output_data.write_q_criterion == true)
    {
      navier_stokes_operator->compute_q_criterion(q_criterion, velocity);
    }

    if(output_data.mean_velocity.calculate == true)
    {
      if(time_step_number >= 0) // unsteady problems
        compute_mean_velocity(mean_velocity, velocity, time, time_step_number);
      else // time_step_number < 0 -> steady problems
        AssertThrow(
          false, dealii::ExcMessage("Mean velocity can only be computed for unsteady problems."));
    }

    if(output_data.write_cfl)
    {
      // This time step size corresponds to CFL = 1.
      auto const time_step_size = navier_stokes_operator->calculate_time_step_cfl(velocity);

      // The computed cell-vector of CFL values contains relative CFL numbers with a value of
      // CFL = 1 in the most critical cell and CFL < 1 in other cells.
      navier_stokes_operator->calculate_cfl_from_time_step(cfl_vector, velocity, time_step_size);
    }
  }
}

template class OutputGenerator<2, float>;
template class OutputGenerator<2, double>;

template class OutputGenerator<3, float>;
template class OutputGenerator<3, double>;

} // namespace IncNS
} // namespace ExaDG
