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

#include <exadg/functions_and_boundary_conditions/linear_interpolation.h>
#include <exadg/incompressible_navier_stokes/postprocessor/inflow_data_calculator.h>
#include <exadg/vector_tools/interpolate_solution.h>
#include <exadg/vector_tools/point_value.h>

namespace ExaDG
{
namespace IncNS
{
using namespace dealii;

template<int dim, typename Number>
InflowDataCalculator<dim, Number>::InflowDataCalculator(InflowData<dim> const & inflow_data_in,
                                                        MPI_Comm const &        comm)
  : inflow_data(inflow_data_in), inflow_data_has_been_initialized(false), mpi_comm(comm)
{
}

template<int dim, typename Number>
void
InflowDataCalculator<dim, Number>::setup(DoFHandler<dim> const & dof_handler_velocity_in,
                                         Mapping<dim> const &    mapping_in)
{
  dof_handler_velocity = &dof_handler_velocity_in;
  mapping              = &mapping_in;

  array_dof_indices_and_shape_values.resize(inflow_data.n_points_y * inflow_data.n_points_z);
  array_counter.resize(inflow_data.n_points_y * inflow_data.n_points_z);
}

template<int dim, typename Number>
void
InflowDataCalculator<dim, Number>::calculate(
  LinearAlgebra::distributed::Vector<Number> const & velocity)
{
  if(inflow_data.write_inflow_data == true)
  {
    // initial data: do this expensive step only once at the beginning of the simulation
    if(inflow_data_has_been_initialized == false)
    {
      for(unsigned int iy = 0; iy < inflow_data.n_points_y; ++iy)
      {
        for(unsigned int iz = 0; iz < inflow_data.n_points_z; ++iz)
        {
          Point<dim> point;

          if(inflow_data.inflow_geometry == InflowGeometry::Cartesian)
          {
            AssertThrow(inflow_data.normal_direction == 0, ExcMessage("Not implemented."));

            point = Point<dim>(inflow_data.normal_coordinate,
                               (*inflow_data.y_values)[iy],
                               (*inflow_data.z_values)[iz]);
          }
          else if(inflow_data.inflow_geometry == InflowGeometry::Cylindrical)
          {
            AssertThrow(inflow_data.normal_direction == 2, ExcMessage("Not implemented."));

            Number const x = (*inflow_data.y_values)[iy] * std::cos((*inflow_data.z_values)[iz]);
            Number const y = (*inflow_data.y_values)[iy] * std::sin((*inflow_data.z_values)[iz]);
            point          = Point<dim>(x, y, inflow_data.normal_coordinate);
          }
          else
          {
            AssertThrow(false, ExcMessage("Not implemented."));
          }

          unsigned int array_index = iy * inflow_data.n_points_z + iz;

          auto adjacent_cells = GridTools::find_all_active_cells_around_point(
            *mapping, dof_handler_velocity->get_triangulation(), point, 1.e-10);

          array_dof_indices_and_shape_values[array_index] = get_dof_indices_and_shape_values(
            adjacent_cells, *dof_handler_velocity, *mapping, velocity);
        }
      }

      inflow_data_has_been_initialized = true;
    }

    // evaluate velocity in all points of the 2d grid
    for(unsigned int iy = 0; iy < inflow_data.n_points_y; ++iy)
    {
      for(unsigned int iz = 0; iz < inflow_data.n_points_z; ++iz)
      {
        // determine the array index, will be needed several times below
        unsigned int array_index = iy * inflow_data.n_points_z + iz;

        // initialize with zeros since we accumulate into these variables
        (*inflow_data.array)[array_index] = 0.0;
        array_counter[array_index]        = 0;

        auto & vector(array_dof_indices_and_shape_values[array_index]);

        // loop over all adjacent, locally owned cells for the current point
        for(auto iter = vector.begin(); iter != vector.end(); ++iter)
        {
          // increment counter (because this is a locally owned cell)
          array_counter[array_index] += 1;

          // interpolate solution using the precomputed shape values and the global dof index
          Tensor<1, dim, Number> velocity_value = Interpolator<1, dim, Number>::value(
            *dof_handler_velocity, velocity, iter->first, iter->second);

          // add result to array with velocity inflow data
          (*inflow_data.array)[array_index] += velocity_value;
        }
      }
    }

    // sum over all processors
    Utilities::MPI::sum(array_counter, mpi_comm, array_counter);
    Utilities::MPI::sum(
      ArrayView<double const>(&(*inflow_data.array)[0][0], dim * inflow_data.array->size()),
      mpi_comm,
      ArrayView<double>(&(*inflow_data.array)[0][0], dim * inflow_data.array->size()));

    // divide by counter in order to get the mean value (averaged over all
    // adjacent cells for a given point)
    for(unsigned int iy = 0; iy < inflow_data.n_points_y; ++iy)
    {
      for(unsigned int iz = 0; iz < inflow_data.n_points_z; ++iz)
      {
        unsigned int array_index = iy * inflow_data.n_points_z + iz;
        if(array_counter[array_index] >= 1)
          (*inflow_data.array)[array_index] /= Number(array_counter[array_index]);
      }
    }
  }
}

template class InflowDataCalculator<2, float>;
template class InflowDataCalculator<2, double>;

template class InflowDataCalculator<3, float>;
template class InflowDataCalculator<3, double>;

} // namespace IncNS
} // namespace ExaDG
