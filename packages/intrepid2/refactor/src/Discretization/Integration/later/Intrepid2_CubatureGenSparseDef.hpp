// @HEADER
// ************************************************************************
//
//                           Intrepid2 Package
//                 Copyright (2007) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Kyungjoo Kim  (kyukim@sandia.gov), or
//                    Mauro Perego  (mperego@sandia.gov)
//
// ************************************************************************
// @HEADER

/** \file   Intrepid_CubatureGenSparseDef.hpp
    \brief  Definition file for the Intrepid2::CubatureGenSparse class.
    \author Created by P. Bochev, D. Ridzal, and M. Keegan.
*/


namespace Intrepid2 {

/**************************************************************************
**  Function Definitions for Class CubatureGenSparse
***************************************************************************/

template <class Scalar, ordinal_type dimension_, class ArrayPoint, class ArrayWeight>
CubatureGenSparse<Scalar,dimension_,ArrayPoint,ArrayWeight>::CubatureGenSparse(const ordinal_type degree) :
    degree_(degree) {

  SGNodes<ordinal_type, dimension_> list;
  SGNodes<ordinal_type,dimension_> bigger_rules;

  bool continue_making_first_list = true;
  bool more_bigger_rules = true;

  ordinal_type poly_exp[dimension_];
  ordinal_type level[dimension_];
  ordinal_type temp_big_rule[dimension_];
  
  for(ordinal_type i = 0; i<dimension_; i++){
    poly_exp[i] = 0;
    temp_big_rule[i] = 0;
  }

  while(continue_making_first_list){
    for(ordinal_type i = 0; i < dimension_; i++)
    {
      ordinal_type max_exp = 0;
      if(i == 0)
        max_exp = std::max(degree_,1) - Sum(poly_exp,1,dimension_-1);
      else if(i == dimension_ -1)
        max_exp = std::max(degree_,1) - Sum(poly_exp,0,dimension_-2);
      else
        max_exp = std::max(degree_,1) - Sum(poly_exp,0,dimension_-1) + poly_exp[i];

      if(poly_exp[i] < max_exp)
      {
        poly_exp[i]++;
        break;
      }
      else
      {
        if(i == dimension_-1)
          continue_making_first_list = false;
        else
          poly_exp[i] = 0;
          
      }
    }

    if(continue_making_first_list)
    {
      for(ordinal_type j = 0; j < dimension_;j++)
      {
        /*******************
        **  Slow-Gauss
        ********************/
        level[j] = (ordinal_type)std::ceil((((Scalar)poly_exp[j])+3.0)/4.0);
        /*******************
        **  Fast-Gauss
        ********************/
        //level[j] = intstd::ceil(std::log(poly_exp[j]+3)/std::log(2) - 1);
      }
      list.addNode(level,1);
      
    }
  }


  while(more_bigger_rules)
  {
    bigger_rules.addNode(temp_big_rule,1);

    for(ordinal_type i = 0; i < dimension_; i++)
    {
      if(temp_big_rule[i] == 0){
        temp_big_rule[i] = 1;
        break;
      }
      else{
        if(i == dimension_-1)
          more_bigger_rules = false;
        else
          temp_big_rule[i] = 0;
      }
    }
  } 

  for(ordinal_type x = 0; x < list.size(); x++){
    for(ordinal_type y = 0; y < bigger_rules.size(); y++)
    { 
      SGPoint<ordinal_type, dimension_> next_rule;
      for(ordinal_type t = 0; t < dimension_; t++)
        next_rule.coords[t] = list.nodes[x].coords[t] + bigger_rules.nodes[y].coords[t];

      bool is_in_set = false;
      for(ordinal_type z = 0; z < list.size(); z++)
      {
        if(next_rule == list.nodes[z]){
          is_in_set = true;
          break;
        }
      }

      if(is_in_set)
      {
        ordinal_type big_sum[dimension_];
        for(ordinal_type i = 0; i < dimension_; i++)
          big_sum[i] = bigger_rules.nodes[y].coords[i];
        Scalar coeff = std::pow(-1.0, Sum(big_sum, 0, dimension_-1));
        
        Scalar point[dimension_];
        ordinal_type point_record[dimension_];

        for(ordinal_type j = 0; j<dimension_; j++)
          point_record[j] = 1;

        bool more_points = true;

        while(more_points)
        {
          Scalar weight = 1.0;
        
          for(ordinal_type w = 0; w < dimension_; w++){
            /*******************
            **  Slow-Gauss
            ********************/
            ordinal_type order1D = 2*list.nodes[x].coords[w]-1;
            /*******************
            **  Fast-Gauss
            ********************/
            //ordinal_type order1D = (ordinal_type)std::pow(2.0,next_rule.coords[w]) - 1;

            ordinal_type cubDegree1D = 2*order1D - 1;
            CubatureDirectLineGauss<Scalar> Cub1D(cubDegree1D);
            FieldContainer<Scalar> cubPoints1D(order1D, 1);
            FieldContainer<Scalar> cubWeights1D(order1D);

            Cub1D.getCubature(cubPoints1D, cubWeights1D);

            point[w] =  cubPoints1D(point_record[w]-1, 0);
            weight = weight * cubWeights1D(point_record[w]-1);
          }     
          weight = weight*coeff;
          grid.addNode(point, weight);

          for(ordinal_type v = 0; v < dimension_; v++)
          {
            if(point_record[v] < 2*list.nodes[x].coords[v]-1){
              (point_record[v])++;
              break;
            }
            else{
              if(v == dimension_-1)
                more_points = false;
              else
                point_record[v] = 1;
            }
          }
        }
      }
    }
  }

  numPoints_ = grid.size();
}



template <class Scalar, ordinal_type dimension_, class ArrayPoint, class ArrayWeight>
void CubatureGenSparse<Scalar,dimension_,ArrayPoint,ArrayWeight>::getCubature(ArrayPoint  & cubPoints,
                                                                              ArrayWeight & cubWeights) const{
  grid.copyToArrays(cubPoints, cubWeights);
} // end getCubature

template<class Scalar, ordinal_type dimension_, class ArrayPoint, class ArrayWeight>
void CubatureGenSparse<Scalar,dimension_, ArrayPoint,ArrayWeight>::getCubature(ArrayPoint& cubPoints,
                                                                               ArrayWeight& cubWeights,
                                                                               ArrayPoint& cellCoords) const
{
    TEUCHOS_TEST_FOR_EXCEPTION( (true), std::logic_error,
                      ">>> ERROR (CubatureGenSparse): Cubature defined in reference space calling method for physical space cubature.");
}


template <class Scalar, ordinal_type dimension_, class ArrayPoint, class ArrayWeight>
ordinal_type CubatureGenSparse<Scalar,dimension_,ArrayPoint,ArrayWeight>::getNumPoints() const {
  return numPoints_;
} // end getNumPoints



template <class Scalar, ordinal_type dimension_, class ArrayPoint, class ArrayWeight>
ordinal_type CubatureGenSparse<Scalar,dimension_,ArrayPoint,ArrayWeight>::getDimension() const {
  return dimension_;
} // end dimension



template <class Scalar, ordinal_type dimension_, class ArrayPoint, class ArrayWeight>
void CubatureGenSparse<Scalar,dimension_,ArrayPoint,ArrayWeight>::getAccuracy(std::vector<ordinal_type> & accuracy) const {
  accuracy.assign(1, degree_);
} //end getAccuracy


} // end namespace Intrepid2
