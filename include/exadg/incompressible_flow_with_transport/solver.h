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

#ifndef INCLUDE_EXADG_INCOMPRESSIBLE_FLOW_WITH_TRANSPORT_SOLVER_H_
#define INCLUDE_EXADG_INCOMPRESSIBLE_FLOW_WITH_TRANSPORT_SOLVER_H_

// deal.II
#include <deal.II/base/parameter_handler.h>

// ExaDG

// configuration
#include <exadg/configuration/config.h>

// driver
#include <exadg/incompressible_flow_with_transport/driver.h>

// utilities
#include <exadg/utilities/general_parameters.h>

// application
#include <exadg/incompressible_flow_with_transport/user_interface/declare_get_application.h>

namespace ExaDG
{
struct ResolutionParameters
{
  ResolutionParameters()
  {
  }

  ResolutionParameters(const std::string & input_file)
  {
    dealii::ParameterHandler prm;
    add_parameters(prm);
    prm.parse_input(input_file, "", true, true);
  }

  void
  add_parameters(dealii::ParameterHandler & prm)
  {
    // clang-format off
    prm.enter_subsection("SpatialResolution");
      prm.add_parameter("Degree",
                        degree,
                        "Polynomial degree of shape functions.",
                        Patterns::Integer(1,EXADG_DEGREE_MAX),
                        true);
      prm.add_parameter("RefineSpace",
                        refine_space,
                        "Number of global, uniform mesh refinements.",
                        Patterns::Integer(0,20),
                        true);
    prm.leave_subsection();
    // clang-format on
  }

  unsigned int degree = 3;

  unsigned int refine_space = 0;
};

void
create_input_file(std::string const & input_file)
{
  dealii::ParameterHandler prm;

  GeneralParameters general;
  general.add_parameters(prm);

  ResolutionParameters resolution;
  resolution.add_parameters(prm);

  try
  {
    // we have to assume a default dimension and default Number type
    // for the automatic generation of a default input file
    unsigned int const Dim = 2;
    typedef double     Number;
    FTI::get_application<Dim, Number>(input_file, MPI_COMM_WORLD)->add_parameters(prm);
  }
  catch(...)
  {
  }

  prm.print_parameters(input_file,
                       dealii::ParameterHandler::Short |
                         dealii::ParameterHandler::KeepDeclarationOrder);
}

template<int dim, typename Number>
void
run(std::string const & input_file, MPI_Comm const & mpi_comm, bool const is_test)
{
  Timer timer;
  timer.restart();

  std::shared_ptr<FTI::ApplicationBase<dim, Number>> application =
    FTI::get_application<dim, Number>(input_file, mpi_comm);

  ExaDG::ResolutionParameters resolution(input_file);
  application->set_parameters_convergence_study(resolution.degree, resolution.refine_space);

  std::shared_ptr<FTI::Driver<dim, Number>> driver =
    std::make_shared<FTI::Driver<dim, Number>>(mpi_comm, is_test);

  driver->setup(application);

  driver->solve();

  if(not(is_test))
    driver->print_performance_results(timer.wall_time());
}
} // namespace ExaDG

int
main(int argc, char ** argv)
{
  dealii::Utilities::MPI::MPI_InitFinalize mpi(argc, argv, 1);

  MPI_Comm mpi_comm(MPI_COMM_WORLD);

  std::string input_file;

  if(argc == 1)
  {
    if(dealii::Utilities::MPI::this_mpi_process(mpi_comm) == 0)
    {
      // clang-format off
      std::cout << "To run the program, use:      ./solver input_file" << std::endl
                << "To setup the input file, use: ./solver input_file --help" << std::endl;
      // clang-format on
    }

    return 0;
  }
  else if(argc >= 2)
  {
    input_file = std::string(argv[1]);

    if(argc == 3 and std::string(argv[2]) == "--help")
    {
      if(dealii::Utilities::MPI::this_mpi_process(mpi_comm) == 0)
        ExaDG::create_input_file(input_file);

      return 0;
    }
  }

  ExaDG::GeneralParameters general(input_file);

  // run the simulation
  if(general.dim == 2 && general.precision == "float")
    ExaDG::run<2, float>(input_file, mpi_comm, general.is_test);
  else if(general.dim == 2 && general.precision == "double")
    ExaDG::run<2, double>(input_file, mpi_comm, general.is_test);
  else if(general.dim == 3 && general.precision == "float")
    ExaDG::run<3, float>(input_file, mpi_comm, general.is_test);
  else if(general.dim == 3 && general.precision == "double")
    ExaDG::run<3, double>(input_file, mpi_comm, general.is_test);
  else
    AssertThrow(false,
                dealii::ExcMessage("Only dim = 2|3 and precision=float|double implemented."));

  return 0;
}

#endif /* INCLUDE_EXADG_INCOMPRESSIBLE_FLOW_WITH_TRANSPORT_SOLVER_H_ */
