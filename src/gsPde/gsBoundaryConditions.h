/** @file gsBoundaryConditions.h

    @brief Provides gsBoundaryConditions class.

    This file is part of the G+Smo library.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

    Author(s): A. Mantzaflaris
*/

#pragma once

#include <gsCore/gsBoundary.h>


namespace gismo
{

template <class T> class gsFunction;

/// @brief Specifies the type of boundary condition
///
/// \ingroup Pde
struct condition_type
{
 enum type 
 {
     dirichlet = 0, ///< Dirichlet type
     neumann   = 1, ///< Neumann type
     robin     = 2  ///< Robin type
    //mixed BD means: there are both dirichlet and neumann sides
    //robin: a linear combination of value and derivative
    //cauchy: there are 2 conditions (value+deriv) defined on the same side
 };

};

// Print (as string) a boundary type
inline std::ostream &operator<<(std::ostream &os, const condition_type::type& o)
{
    switch (o)
    {
    case condition_type::dirichlet:
        os<< "Dirichlet";
    case condition_type::neumann:
        os<< "Neumann";
    case condition_type::robin:
        os<< "Mixed";
    default:
        gsInfo<<"condition type not known.\n";
    };
    return os;
}

/** 
    @brief Class that defines a boundary condition for a side of a
    patch for some unknown variable of a PDE.
    
    \todo rename to boundaryCondition
    
    \ingroup Pde
*/
template<class T>
struct boundary_condition
{
    boundary_condition( int p, boxSide s, gsFunction<T> * f, condition_type::type t, int unknown = 0)
	: ps(p, s), m_function(f), m_type(t), m_unknown(unknown) { }
      
    boundary_condition( int p, boxSide s, condition_type::type t, int unknown = 0)
	: ps(p, s), m_function(NULL), m_type(t), m_unknown(unknown)  { }
      
    /// homogeneous boundary condition ?
    bool isHomogeneous()       const { return m_function == 0; }

    /// Returns the function data of the boundary condition
    gsFunction<T> * function() const { return m_function; }

    /// Returns the type of the boundary condition
    condition_type::type  type()     const { return m_type; }

    /// Returns the patch to which this boundary condition refers to
    int             patch()    const { return ps.patch; }

    /// Returns the side to which this boundary condition refers to
    boxSide  side()     const { return ps.side(); }

    /// Returns the unknown to which this boundary condition refers to
    int             unknown()  const { return m_unknown; }


    patchSide ps;                ///< Side of a patch for this boundary condition
    gsFunction<T> * m_function;  ///< Function data for this boundary condition
    // TO DO : robin coefficients?
    condition_type::type m_type; ///< Type of the boundary condition
    int m_unknown;               ///< Unknown to which this boundary condition refers to
};

/** 
    @brief Class prescribing a value related to a corner of a patch
*/
template<class T>
struct corner_value
{
    corner_value(int p, boxCorner c, T v, int unk = 0)
	: patch(p), corner(c), value(v), unknown(unk) { }

    index_t patch;    ///< The index of the patch.
    boxCorner corner; ///< The corner
    T value;          ///< The value
    int   unknown;    ///< Unknown to which this boundary condition refers to
};
    
/** @brief
    Class containing a set of  boundary conditions.
    
    The boundary conditions are stored in the form of a list of boundary_condition
    instances.
    
    \ingroup Pde
*/
template<class T>
class gsBoundaryConditions 
{
    
public:

    typedef typename std::vector<boundary_condition<T> >::iterator iterator;
    typedef typename std::vector<boundary_condition<T> >::const_iterator const_iterator;

public:

    /// Default empty constructor
    gsBoundaryConditions() 
    { }

    ~gsBoundaryConditions() // Destructor
    { }
    
public:

    /// Return a reference to the Dirichlet sides 
    const std::vector<boundary_condition<T> > & dirichletSides() const {return drchlt_sides; }

    /// Return a reference to the Neumann sides 
    const std::vector<boundary_condition<T> > & neumannSides()   const {return nmnn_sides;   }

    /// Return a reference to the Dirichlet sides 
    const std::vector<boundary_condition<T> > & robinSides()     const {return robin_sides;  }

    std::vector<boundary_condition<T> > allConditions() const
    {
        std::vector<boundary_condition<T> > all;
        all.reserve( drchlt_sides.size()+nmnn_sides.size()+robin_sides.size());
        all.insert( all.end(), drchlt_sides.begin(), drchlt_sides.end() );
        all.insert( all.end(), nmnn_sides.begin()  , nmnn_sides.end()   );
        all.insert( all.end(), robin_sides.begin() , robin_sides.end()  );
        return all;
    }
    
    /// Get a const-iterator to the beginning of the Dirichlet sides
    /// \return an iterator to the beginning of the Dirichlet sides
    const_iterator dirichletBegin() const
	{ return drchlt_sides.begin(); }
    
    /// Get a const-iterator to the end of the Dirichlet sides
    /// \return an iterator to the end of the Dirichlet sides
    const_iterator dirichletEnd() const
	{ return drchlt_sides.end(); }
    
    /// Get an iterator to the beginning of the Dirichlet sides
    /// \return an iterator to the beginning of the Dirichlet sides
    iterator dirichletBegin()
	{ return drchlt_sides.begin(); }
    
    /// Get an iterator to the end of the Dirichlet sides
    /// \return an iterator to the end of the Dirichlet sides
    iterator dirichletEnd()
	{ return drchlt_sides.end(); }

    /// Get a const-iterator to the beginning of the Neumann sides
    /// \return an iterator to the beginning of the Neumann sides
    const_iterator neumannBegin() const
	{ return nmnn_sides.begin(); }
    
    /// Get a const-iterator to the end of the Neumann sides
    /// \return an iterator to the end of the Neumann sides
    const_iterator neumannEnd() const
	{ return nmnn_sides.end(); }
    
    /// Get an iterator to the beginning of the Neumann sides
    /// \return an iterator to the beginning of the Neumann sides
    iterator neumannBegin()
	{ return nmnn_sides.begin(); }
    
    /// Get an iterator to the end of the Neumann sides
    /// \return an iterator to the end of the Neumann sides
    iterator neumannEnd()
	{ return nmnn_sides.end(); }

    /// Get a const-iterator to the beginning of the Robin sides
    /// \return an iterator to the beginning of the Robin sides
    const_iterator robinBegin() const
	{ return robin_sides.begin(); }
    
    /// Get a const-iterator to the end of the Robin sides
    /// \return an iterator to the end of the Robin sides
    const_iterator robinEnd() const
	{ return robin_sides.end(); }
    
    /// Get an iterator to the beginning of the Robin sides
    /// \return an iterator to the beginning of the Robin sides
    iterator robinBegin()
	{ return robin_sides.begin(); }
    
    /// Get an iterator to the end of the Robin sides
    /// \return an iterator to the end of the Robin sides
    iterator robinEnd()
	{ return robin_sides.end(); }
    
    void addCondition(int p, boxSide s, condition_type::type t, 
                      gsFunction<T> * f, int unknown = 0)
    {
        switch (t) {
        case condition_type::dirichlet :
            drchlt_sides.push_back( boundary_condition<T>(p,s,f,t,unknown) );
            break;
        case condition_type::neumann :
            nmnn_sides.push_back( boundary_condition<T>(p,s,f,t,unknown) );
            break;
        case condition_type::robin :
            robin_sides.push_back( boundary_condition<T>(p,s,f,t,unknown) );
            break;
        default:
            std::cout<<"gsBoundaryConditions: Unknown boundary condition.\n";
        }
    }

    void addCondition( boxSide s, condition_type::type t, 
                       gsFunction<T> * f, int unknown = 0)
    {
        // for single-patch only
        addCondition(0,s,t,f,unknown);
    }

    void addCornerValue(int p, boxCorner c, T value, int unknown = 0)
    {
        corner_values.push_back( corner_value<T>(p,c,value,unknown) );
    }

    void addCornerValue(boxCorner c, T value, int unknown = 0)
    {
        // for single-patch only
        corner_values.push_back( corner_value<T>(0,c,value,unknown) );
    }
    
    void addCondition(const patchSide& ps, condition_type::type t, 
                      gsFunction<T> * f, int unknown = 0)
    {
        addCondition(ps.patch, ps.side(), t, f, unknown);
    }
    
    /// Prints the object as a string.
    std::ostream &print(std::ostream &os) const
    { 
        os << "gsBoundaryConditions :\n";
        os << "* Dirichlet boundaries: "<< drchlt_sides.size() <<"\n";
        os << "* Neumann boundaries  : "<< nmnn_sides.size() <<"\n";
        return os; 
    };

// Data members
private:

    std::vector<boundary_condition<T> > drchlt_sides; ///< List of Dirichlet sides
    std::vector<boundary_condition<T> > nmnn_sides;   ///< List of Neumann sides
    std::vector<boundary_condition<T> > robin_sides;  ///< List of Robin sides
    std::vector<corner_value<T> >       corner_values; ///< List of corners with fixed value

}; // class gsBoundaryConditions


//////////////////////////////////////////////////
//////////////////////////////////////////////////

/// Print (as string)
template<class T>
std::ostream &operator<<(std::ostream &os, const gsBoundaryConditions<T>& bvp)
{return bvp.print(os); }
    
} // namespace gismo
