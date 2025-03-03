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

#ifndef INCLUDE_FUNCTIONALITIES_ENUM_TYPES_H_
#define INCLUDE_FUNCTIONALITIES_ENUM_TYPES_H_

#include <string>

namespace ExaDG
{
/**************************************************************************************/
/*                                                                                    */
/*                                         MESH                                       */
/*                                                                                    */
/**************************************************************************************/

/*
 * Triangulation type
 */
enum class TriangulationType
{
  Serial,
  Distributed,
  FullyDistributed
};

std::string
enum_to_string(TriangulationType const enum_type);

/*
 * Partitioning type (relevant for fully-distributed triangulation)
 */
enum class PartitioningType
{
  Metis,
  z_order
};

std::string
enum_to_string(PartitioningType const enum_type);

/*
 *  Mapping type (polynomial degree)
 */
enum class MappingType
{
  Affine,
  Quadratic,
  Cubic,
  Isoparametric
};

std::string
enum_to_string(MappingType const enum_type);

} // namespace ExaDG

#endif /* INCLUDE_FUNCTIONALITIES_ENUM_TYPES_H_ */
