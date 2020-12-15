/*  This file is part of the OpenLB library
 *
 *  Copyright (C) 2018 Adrian Kummerlaender
 *  E-mail contact: info@openlb.net
 *  The most recent release of OpenLB can be downloaded at
 *  <http://www.openlb.net/>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
*/

#ifndef BLOCK_LATTICE_REFINEMENT_METRIC_F_3D_HH
#define BLOCK_LATTICE_REFINEMENT_METRIC_F_3D_HH

#include "blockLatticeRefinementMetricF3D.h"
#include "dynamics/lbHelpers.h"


namespace olb {


template<typename T, typename DESCRIPTOR>
BlockLatticeKnudsen3D<T, DESCRIPTOR>::BlockLatticeKnudsen3D(
  BlockLatticeStructure3D<T, DESCRIPTOR>& blockLattice)
  : BlockLatticeF3D<T, DESCRIPTOR>(blockLattice, 1)
{
  this->getName() = "knudsen";
}

template<typename T, typename DESCRIPTOR>
bool BlockLatticeKnudsen3D<T, DESCRIPTOR>::operator()(T output[], const int input[])
{
  if (input[0] < 0 || input[1] < 0 || input[2] < 0 
		  || input[0] >= this->_blockLattice.getNx() 
		  || input[1] >= this->_blockLattice.getNy()
		  || input[2] >= this->_blockLattice.getNz()) {
    return false;
  }

  const auto& cell = this->_blockLattice.get(input[0], input[1], input[2]);

  T rho  = { };
  T u[3] = { };
  cell.computeRhoU(rho, u);

  const T uSqr = u[0]*u[0] + u[1]*u[1] + u[2]*u[2];

  T sum = 0.;

  for (int iPop = 0; iPop < DESCRIPTOR::q; ++iPop) {
    const T fEq = olb::lbHelpers<T, DESCRIPTOR>::equilibrium(iPop, rho, u, uSqr);

    sum += std::abs((cell[iPop] - fEq) / (fEq + descriptors::t<T,DESCRIPTOR>(iPop)));
  }

  output[0] = sum / (DESCRIPTOR::q);

  return true;
}


template<typename T, typename DESCRIPTOR>
BlockLatticeRefinementMetricKnudsen3D<T, DESCRIPTOR>::BlockLatticeRefinementMetricKnudsen3D(
  BlockLatticeStructure3D<T, DESCRIPTOR>& blockLattice,
  const UnitConverter<T, DESCRIPTOR>&     converter)
  : BlockLatticeKnudsen3D<T, DESCRIPTOR>(blockLattice),
    _knudsen(converter.getKnudsenNumber())
{
  this->getName() = "refinementMetricKnudsen";
}

template<typename T, typename DESCRIPTOR>
bool BlockLatticeRefinementMetricKnudsen3D<T, DESCRIPTOR>::operator()(T output[])
{
  const std::size_t cellCount = this->_blockLattice.getNx() 
							  * this->_blockLattice.getNy()
							  * this->_blockLattice.getNz();

  T blockSum = 0.;

  T   localOutput[1] = { };
  int localInput[3]  = { };

  for (localInput[0] = 0; localInput[0] < this->_blockLattice.getNx(); ++localInput[0]) {
    for (localInput[1] = 0; localInput[1] < this->_blockLattice.getNy(); ++localInput[1]) {
		for (localInput[2] = 0; localInput[2] < this->_blockLattice.getNz(); ++localInput[2]) {
	      BlockLatticeKnudsen3D<T, DESCRIPTOR>::operator()(localOutput, localInput);
	
	      blockSum += localOutput[0];
		}
    }
  }

  const T blockC = blockSum / cellCount;

  output[0] = std::nearbyint(std::log2(blockC / _knudsen));

  if ( output[0] <= 0. || blockC <= 0. ) {
    output[0] = 0.;
  }

  return true;
}

template<typename T, typename DESCRIPTOR>
bool BlockLatticeRefinementMetricKnudsen3D<T, DESCRIPTOR>::operator()(
  T output[], const int input[])
{
  T measuredKnudsen[1] { };
  BlockLatticeKnudsen3D<T, DESCRIPTOR>::operator()(measuredKnudsen, input);

  output[0] = std::nearbyint(std::log2(measuredKnudsen[0] / _knudsen));

  if ( output[0] <= 0. || measuredKnudsen[0] <= 0. ) {
    output[0] = 0.;
  }

  return true;
}


}

#endif
