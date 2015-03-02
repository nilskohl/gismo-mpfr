/** @file gsSurface.hpp

    @brief Provides implementation of Surface common operations.

    This file is part of the G+Smo library. 

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    
    Author(s): A. Mantzaflaris
*/


#pragma once

#include <gsUtils/gsPointGrid.h>
#include <gsUtils/gsPointIterator.h>
#include <gsUtils/gsMesh/gsMesh.h>

namespace gismo
{


template<class T> 
void gsSurface<T>::toMesh(gsMesh<T> & msh, int npoints) const
{   
    const gsMatrix<T> param = this->parameterRange();
    const gsVector<T> a = param.col(0);
    const gsVector<T> b = param.col(1);
    const gsVector<unsigned> np    = uniformSampleCount(a, b, npoints );
    
    gsMatrix<T> cp;
    
    gsTensorPointGridIterator<T,2> gridPoint(np.cast<int>(), a, b );
    
    for(; gridPoint.good(); gridPoint.next() )
    {
        this->eval_into(gridPoint.currPoint(), cp);
        msh.addVertex( cp );
    }

    unsigned ind1 = 0;
    unsigned ind2 = 0;
    for(unsigned j = 0; j < np[1] - 1; j++)
    {
        for(unsigned i= 0; i <np[0] - 1; i++)
        {
            ind1 =  j * np[0] + i;
            ind2 = ind1 + np[0];
            //msh.addFace(ind1, ind1+1, ind2+1, ind2);
            msh.addFace(ind1  , ind1+1, ind2+1);
            msh.addFace(ind2+1, ind2  , ind1  );
        }
    }
}

template<class T> 
gsGeometryEvaluator<T> *
gsSurface<T>::evaluator(unsigned flags) const
{
    switch ( this->coDim() )
    {
    case 0:
        return new gsGenericGeometryEvaluator<T,2,0 >(*this, flags);
    case 1:
        return new gsGenericGeometryEvaluator<T,2,1>(*this, flags);
    case -1:
        return new gsGenericGeometryEvaluator<T,2,-1>(*this, flags);
    default:
        GISMO_ERROR("Codimension problem.");
    }
}

/*
   gsVector<unsigned> v;
    v.setZero(2);
    do 
    {
        msh.addFace(v[0], v[0]+1, v[1]+1, v[1]);
    }
    while( nextLexicographic(v, np) );
*/

}; // namespace gismo

