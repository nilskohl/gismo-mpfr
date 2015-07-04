/** @file gsHBSplineBasis.h

    @brief Provides declaration of HBSplineBasis class.

    This file is part of the G+Smo library.
    
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

    Author(s): G. Kiss, A. Mantzaflaris, J. Speh
*/

#pragma once

#include <gsHSplines/gsHTensorBasis.h>
#include <gsHSplines/gsHBSpline.h>

namespace gismo
{
    /** 
     * \brief
     * A hierarchical B-spline basis of parametric dimension \em d.
     *
     * See \cite Kraft1997 for the theory behind this kind of basis.
     * 
     * \tparam d the dimension of the parameter domain
     * \tparam T coefficient type
     *
     * \ingroup basis
     * \ingroup HSplines
    */ 
    
template<unsigned d, class T>
class gsHBSplineBasis : public gsHTensorBasis<d,T>
{
public:
    /// Associated geometry type
    typedef gsHBSpline<d,T> GeometryType;
    
    typedef typename gsHTensorBasis<d,T>::CMatrix CMatrix;

    typedef typename 
    choose<d==1, gsConstantBasis<T>, gsHBSplineBasis<d-1,T>
           >::type BoundaryBasisType;

    typedef memory::shared_ptr< gsHBSplineBasis > Ptr;

public:

// #ifdef _MSC_VER
// #pragma warning( push )
// #pragma warning( disable : 4702 )
// #endif

    /// Constructor out of a gsBSplineBasis
    gsHBSplineBasis(gsBSplineBasis<T> &  bsbasis);
    
    gsHBSplineBasis( gsBSplineBasis<T> &  bsbasis,
                     std::vector<unsigned> & boxes)
        : gsHTensorBasis<d,T>( gsTensorBSplineBasis<d,T>(&bsbasis), boxes) 
    {
        //  Note: The compiler adds automatically a return statement
        //  at the end of each constructor.  Throwing an exception
        //  causes this return statement to be unreachable, and
        //  warning 4702 is emitted.  To stop this warning we add
        //  "bsbasis.dim()==1", which is not known at compile time
        GISMO_ASSERT(bsbasis.dim()==1 && d==1, "Wrong dimension");
    }
    
    gsHBSplineBasis( gsBSplineBasis<T> &  bsbasis,
                     gsMatrix<T> const & boxes)
        : gsHTensorBasis<d,T>(gsTensorBSplineBasis<d,T>(&bsbasis), boxes) 
    {
        GISMO_ASSERT(bsbasis.dim()==1 && d==1, "Wrong dimension");
    }

    gsHBSplineBasis( gsBSplineBasis<T> &  bsbasis,
                     gsMatrix<T> const & boxes, std::vector<unsigned int> & levels)
        : gsHTensorBasis<d,T>(gsTensorBSplineBasis<d,T>(&bsbasis), boxes)
    {
        GISMO_ASSERT(bsbasis.dim()==1 && d==1, "Wrong dimension");
    }

// #ifdef _MSC_VER
// #pragma warning( pop ) 
// #endif

    /// Constructor out of a tensor BSpline Basis
    gsHBSplineBasis(gsBasis<T> const&  tbasis)
        : gsHTensorBasis<d,T>(tbasis) 
    {
        // initialize(); // is done in the base constructor
    }
    
    gsHBSplineBasis( gsTensorBSplineBasis<d,T> const&  tbasis,
                     std::vector<unsigned> & boxes)
        : gsHTensorBasis<d,T>(tbasis, boxes) 
    {
        // initialize(); // is done in the base constructor
    }
    
    gsHBSplineBasis( gsTensorBSplineBasis<d,T> const&  tbasis,
                     gsMatrix<T> const & boxes)
        : gsHTensorBasis<d,T>(tbasis, boxes) 
    {
        // initialize(); // is done in the base constructor
    }
    
    gsHBSplineBasis( gsTensorBSplineBasis<d,T> const&  tbasis,
                     gsMatrix<T> const & boxes, std::vector<unsigned int> & levels)
        : gsHTensorBasis<d,T>(tbasis, boxes)
    {
        // initialize(); // is done in the base constructor
    }

    /// Gives back the boundary basis at boxSide s
    BoundaryBasisType * boundaryBasis(boxSide const & s ) const
    {
        return basisSlice(s.direction(),s.parameter());
    }

    /// Gives back the basis at a slice in \a dir_fixed at \a par
    BoundaryBasisType * basisSlice(index_t dir_fixed,T par ) const;
    
public:
    
    int dim() const { return d; }
    
    void eval_into(const gsMatrix<T> & u, gsMatrix<T>& result) const;

    void deriv_into(const gsMatrix<T> & u, gsMatrix<T>& result) const;

    void deriv2_into(const gsMatrix<T> & u, gsMatrix<T>& result) const;
    
    void evalSingle_into  (unsigned i, const gsMatrix<T> & u, gsMatrix<T>& result) const;
    
    void derivSingle_into (unsigned i, const gsMatrix<T> & u, gsMatrix<T>& result) const;
    
    void deriv2Single_into(unsigned i, const gsMatrix<T> & u, gsMatrix<T>& result) const;
    
    virtual gsHBSplineBasis* clone() const
    { return new gsHBSplineBasis(*this); }
    
    /// Prints the object as a string.
    std::ostream &print(std::ostream &os) const;
    ///returns transfer matrices betweend the levels of the given hierarchical spline
    void transferbyLvl(std::vector<gsMatrix<T> >& result);

    GISMO_MAKE_GEOMETRY_NEW
    
private:
    
    /// Initialize the characteristic and coefficient matrices and the
    /// internal bspline representations
    void initialize();

    gsMatrix<T> coarsening(const std::vector<gsSortedVector<unsigned> >& old, const std::vector<gsSortedVector<unsigned> >& n, const gsSparseMatrix<T,RowMajor> & transfer);
    gsMatrix<T> coarsening_direct( const std::vector<gsSortedVector<unsigned> >& old, const std::vector<gsSortedVector<unsigned> >& n, const std::vector<gsSparseMatrix<T,RowMajor> >& transfer);

    gsMatrix<T> coarsening_direct2( const std::vector<gsSortedVector<unsigned> >& old, const std::vector<gsSortedVector<unsigned> >& n, const std::vector<gsSparseMatrix<T,RowMajor> >& transfer);
    
}; // class gsHBSplineBasis


//////////////////////////////////////////////////
//////////////////////////////////////////////////

} // namespace gismo

#ifndef GISMO_BUILD_LIB
#include GISMO_HPP_HEADER(gsHBSplineBasis.hpp)
#endif
