/** @file gsG1BiharmonicAssembler.h
    @brief Provides assembler for a homogenius Biharmonic equation.
    This file is part of the G+Smo library.
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    Author(s): P. Weinmueller
*/

#pragma once

#include <gsAssembler/gsAssembler.h>

#include <gsPde/gsBiharmonicPde.h>

#include <gsAssembler/gsVisitorBiharmonic.h>
#include <gsAssembler/gsVisitorNeumann.h>
#include <gsAssembler/gsVisitorNeumannBiharmonic.h>
//#include <gsAssembler/gsVisitorNitscheBiharmonic.h>

# include <gsG1Basis/gsG1Mapper_pascal.h>

# include <gsG1Basis/gsG1System.h>

namespace gismo
{

/** @brief
    Implementation of a homogeneous Biharmonic Assembler.
    It sets up an assembler and assembles the system patch wise and
    combines the patch-local stiffness matrices into a global system.
    Dirichlet boundary can only be enforced strongly (i.e Nitsche is
    not implemented).
*/
template <class T, class bhVisitor = gsVisitorBiharmonic<T> >
class gsG1BiharmonicAssembler : public gsAssembler<T>
{
public:
    typedef gsAssembler<T> Base;

public:
/** @brief
    Constructor of the assembler object.
    \param[in] patches is a gsMultiPatch object describing the geometry.
    \param[in] bases a multi-basis that contains patch-wise bases
    \param[in] bconditions  is a gsBoundaryConditions object that holds boundary conditions on the form:
    \f[ \text{Dirichlet: } u = g \text{ on } \Gamma, \text{ and Neumann: } \nabla \Delta u \cdot \mathbf{n} = h \text{ on } \Gamma\f]
    \param[in] bconditions2 is a gsBoundaryConditions object that holds Neumann boundary conditions on the form:
    \f[\text{Neumann: } \nabla \Delta u \cdot \mathbf{n} = g\, \rightarrow \,(g,\nabla v \cdot \mathbf{n})_\Gamma, \f] where \f$ g \f$ is the Neumann data,
    \f$ v \f$ is the test function and \f$ \Gamma\f$ is the boundary side.
    \param[in] rhs is the right-hand side of the Biharmonic equation, \f$\mathbf{f}\f$.
    \param[in] dirStrategy option for the treatment of Dirichlet boundary in the \em bconditions object.
    \param[in] intStrategy option for the treatment of patch interfaces
*/
    gsG1BiharmonicAssembler(gsMultiPatch<T> const        & patches,
                            gsMultiBasis<T> const         & bases,
                            gsBoundaryConditions<T> const & bconditions,
                            gsBoundaryConditions<T> const & bconditions2,
                            const gsPiecewiseFunction<T>          & rhs)
        : m_ppde(patches,bconditions,bconditions2,rhs)
    {
        iFace::strategy intStrategy = iFace::none;
        dirichlet::strategy dirStrategy = dirichlet::none;

        m_options.setInt("DirichletStrategy", dirStrategy);
        m_options.setInt("InterfaceStrategy", intStrategy);

        Base::initialize(m_ppde, bases, m_options);
    }

    void refresh();

    void assemble();

    void constructSolution(const gsMatrix<T>& solVector,
                           gsMultiPatch<T>& result, int unk = 0);


    void computeDirichletDofsL2Proj(std::vector<gsG1AuxiliaryPatch> & g1_edges, std::vector<gsG1AuxiliaryPatch> & g1_vertices, gsG1Mapper_pascal<real_t> &  g1Mapper);
    void constructDirichletSolution(std::vector<gsG1AuxiliaryPatch> & g1_edges, std::vector<gsG1AuxiliaryPatch> & g1_vertices, gsG1Mapper_pascal<real_t> & g1Mapper);

    void computeDirichletDofsL2Proj(gsG1System<real_t> &  g1System);

    gsDofMapper get_mapper() { return m_system.colMapper(0); };



protected:

    // fixme: add constructor and remove this
    gsBiharmonicPde<T> m_ppde;

    // Members from gsAssembler
    using Base::m_pde_ptr;
    using Base::m_bases;
    using Base::m_ddof;
    using Base::m_options;
    using Base::m_system;

protected:

    gsMatrix<T> m_g1_ddof;

};

template <class T, class bhVisitor>
void gsG1BiharmonicAssembler<T,bhVisitor>::constructDirichletSolution(std::vector<gsG1AuxiliaryPatch> & g1_edges, std::vector<gsG1AuxiliaryPatch> &  g1_vertices, gsG1Mapper_pascal<real_t> & g1Mapper)
{
    gsDofMapper map_edge(g1Mapper.getMapper_Edges());
    gsDofMapper map_vertex(g1Mapper.getMapper_Vertex());

    std::vector<gsMultiPatch<T>> edges, vertices;
    for ( index_t i = 0; i < g1_edges.size(); i++ )
    {
        const gsMultiPatch<T> multiPatch_Edges = g1_edges.at(i).getG1Basis();

        for (size_t j = 0; j < multiPatch_Edges.nPatches(); j++)
        {
            if( map_edge.is_boundary(j,i) )
                multiPatch_Edges.patch(j).setCoefs(multiPatch_Edges.patch(j).coefs() * m_g1_ddof.at(map_edge.bindex(j,i)));
            else
                multiPatch_Edges.patch(j).setCoefs(multiPatch_Edges.patch(j).coefs() * 0);
        }
        edges.push_back(multiPatch_Edges);
    }
    for (index_t i = 0; i< g1_vertices.size(); i++)
    {
        const gsMultiPatch<T> multiPatch_Vertex = g1_vertices.at(i).getG1Basis();

        for (size_t j = 0; j < multiPatch_Vertex.nPatches(); j++)
        {
            if( map_vertex.is_boundary(j,i) )
                multiPatch_Vertex.patch(j).setCoefs(multiPatch_Vertex.patch(j).coefs() * m_g1_ddof.at(map_edge.boundarySize() + map_vertex.bindex(j,i)));
            else
                multiPatch_Vertex.patch(j).setCoefs(multiPatch_Vertex.patch(j).coefs() * 0);
        }
        vertices.push_back(multiPatch_Vertex);
    }

    std::string basename = "Boundary";
    gsParaviewCollection collection(basename);
    std::string fileName;
    index_t iter = 0;
    for (index_t j = 0; j < edges.size(); ++j)
    {
        for (size_t i = 0; i < edges.at(j).nPatches(); i++)
        {
            fileName = basename + "_" + util::to_string(iter);
            gsField<> temp_field(m_pde_ptr->domain().patch(g1_edges.at(j).getGlobalPatchIndex()),edges.at(j).patch(i));
            gsWriteParaview(temp_field,fileName,5000);
            collection.addTimestep(fileName,iter,"0.vts");
            iter ++;
        }
    }
    for (index_t j = 0; j < vertices.size(); ++j)
    {
        for (size_t i = 0; i < vertices.at(j).nPatches(); i++)
        {
            fileName = basename + "_" + util::to_string(iter);
            gsField<> temp_field(m_pde_ptr->domain().patch(g1_vertices.at(j).getGlobalPatchIndex()),vertices.at(j).patch(i));
            gsWriteParaview(temp_field,fileName,5000);
            collection.addTimestep(fileName,iter,"0.vts");
            iter ++;
        }
    }
    collection.save();

    std::string fn = "boundary_value";
    index_t npts = 5000;
    gsParaviewCollection collection2(fn);
    std::string fileName2;

    for ( unsigned pp =0; pp < m_pde_ptr->domain().nPatches(); ++pp ) // Patches
    {
        //const gsBasis<T> & dom = field.isParametrized() ?
        //                         field.igaFunction(i).basis() : field.patch(i).basis();

        fileName2 = fn + util::to_string(pp);
        //writeSinglePatchField( field, i, fileName, npts );

        const gsFunction<T> & geometry = m_pde_ptr->domain().patch(pp);
        //const gsFunction<T> & parField = m_pde_ptr->domain().function(i);

        const int n = geometry.targetDim();
        const int d = geometry.domainDim();

        gsMatrix<T> ab = geometry.support();
        gsVector<T> a = ab.col(0);
        gsVector<T> b = ab.col(1);

        gsVector<unsigned> np = uniformSampleCount(a, b, npts);
        gsMatrix<T> pts = gsPointGrid(a, b, np);

        gsMatrix<T> eval_geo = geometry.eval(pts);//pts
        gsMatrix<T> eval_field; // = field.isParametric() ? parField.eval(pts) : parField.eval(eval_geo);
        eval_field.setZero(1,pts.dim().second);
        // Hier g1 basis dazu addieren!!!
        for (index_t j = 0; j < edges.size(); ++j)
            if (g1_edges.at(j).getGlobalPatchIndex() == pp)
                for (size_t i = 0; i < edges.at(j).nPatches(); i++)
                {
                    gsField<> temp_field(m_pde_ptr->domain().patch(g1_edges.at(j).getGlobalPatchIndex()),edges.at(j).patch(i));
                    eval_field += temp_field.value(pts);
                }

        for (index_t j = 0; j < vertices.size(); ++j)
            if (g1_vertices.at(j).getGlobalPatchIndex() == pp)
                for (size_t i = 0; i < vertices.at(j).nPatches(); i++)
                {
                    gsField<> temp_field(m_pde_ptr->domain().patch(g1_vertices.at(j).getGlobalPatchIndex()),vertices.at(j).patch(i));
                    eval_field += temp_field.value(pts);
                }

        //gsFunctionExpr<> solVal("(cos(4*pi*x) - 1) * (cos(4*pi*y) - 1)",2);
        //gsField<> exact( field.patch(i), solVal, false );
        //eval_field -= exact.value(pts);

        if ( 3 - d > 0 )
        {
            np.conservativeResize(3);
            np.bottomRows(3-d).setOnes();
        }
        else if (d > 3)
        {
            gsWarn<< "Cannot plot 4D data.\n";
            return;
        }

        if ( 3 - n > 0 )
        {
            eval_geo.conservativeResize(3,eval_geo.cols() );
            eval_geo.bottomRows(3-n).setZero();
        }
        else if (n > 3)
        {
            gsWarn<< "Data is more than 3 dimensions.\n";
        }

        if ( eval_field.rows() == 2)
        {
            eval_field.conservativeResize(3,eval_geo.cols() );
            eval_field.bottomRows(1).setZero(); // 3-field.dim()
        }

        gsWriteParaviewTPgrid(eval_geo, eval_field, np.template cast<index_t>(), fileName2);


        collection2.addPart(fileName2, ".vts");
    }
    collection2.save();
}

template <class T, class bhVisitor>
void gsG1BiharmonicAssembler<T,bhVisitor>::refresh()
{
    // We use predefined helper which initializes the system matrix
    // rows and columns using the same test and trial space
    Base::scalarProblemGalerkinRefresh();

}

template <class T, class bhVisitor>
void gsG1BiharmonicAssembler<T,bhVisitor>::assemble()
{
    GISMO_ASSERT(m_system.initialized(), "Sparse system is not initialized, call refresh()");

    // Reserve sparse system
    const index_t nz = gsAssemblerOptions::numColNz(m_bases[0][0], 2, 1, 0.333333);
    m_system.reserve(nz, this->pde().numRhs());

    // Compute the Dirichlet Degrees of freedom (if needed by m_options)
    m_ddof.resize(m_system.numUnknowns());
    m_ddof[0].setZero(m_system.colMapper(0).boundarySize(), m_system.unkSize(0) * m_system.rhs().cols());

    // Assemble volume integrals
    Base::template push<bhVisitor>();

    // Neuman conditions of first kind
    //Base::template push<gsVisitorNeumann<T> >(
    //    m_ppde.bcFirstKind().neumannSides() );

    // Neuman conditions of second kind
    Base::template push<gsVisitorNeumannBiharmonic<T> >(
        m_ppde.bcSecondKind().neumannSides());

    if (m_options.getInt("InterfaceStrategy") == iFace::dg)
        gsWarn << "DG option ignored.\n";

    /*
    // If requested, force Dirichlet boundary conditions by Nitsche's method
    this->template push<gsVisitorNitscheBiharmonic<T> >(
    m_ppde.bcSecondKind().dirichletSides() );
    */

    // Assembly is done, compress the matrix
    Base::finalize();
}


template <class T, class bhVisitor>
void gsG1BiharmonicAssembler<T,bhVisitor>::computeDirichletDofsL2Proj(gsG1System<real_t> &  g1System)
{
    size_t unk_ = 0;

    m_g1_ddof.resize( g1System.boundarySize(), m_system.unkSize(unk_)*m_system.rhs().cols());  //m_pde_ptr->numRhs() );

    // Set up matrix, right-hand-side and solution vector/matrix for
    // the L2-projection
    gsSparseEntries<T> projMatEntries;
    gsMatrix<T>        globProjRhs;
    globProjRhs.setZero( g1System.boundarySize(), m_system.unkSize(unk_)*m_system.rhs().cols() );

    // Temporaries
    gsVector<T> quWeights;

    gsMatrix<T> rhsVals;
    gsMatrix<unsigned> globIdxAct, locIdxAct;
    gsMatrix<unsigned> globIdxAct_0;
    gsMatrix<unsigned> globIdxAct_1;
    gsMatrix<T> basisVals;

    gsMapData<T> md(NEED_MEASURE);
}

template <class T, class bhVisitor>
void gsG1BiharmonicAssembler<T,bhVisitor>::computeDirichletDofsL2Proj(std::vector<gsG1AuxiliaryPatch> & g1_edges,
                                                                      std::vector<gsG1AuxiliaryPatch> & g1_vertices,
                                                                      gsG1Mapper_pascal<real_t> & g1Mapper)
{
    size_t unk_ = 0;

    gsDofMapper map_edge(g1Mapper.getMapper_Edges());
    gsDofMapper map_vertex(g1Mapper.getMapper_Vertex());

    gsInfo << " : " << map_vertex.asVector() << "\n";
    gsInfo << " : " << map_vertex.global_to_bindex(19) << "\n";
    gsInfo << " : " << map_vertex.global_to_bindex(30) << "\n";
    gsInfo << " : " << map_vertex.global_to_bindex(31) << "\n";

    m_g1_ddof.resize( map_edge.boundarySize() + map_vertex.boundarySize(), m_system.unkSize(unk_)*m_system.rhs().cols());  //m_pde_ptr->numRhs() );

    // Set up matrix, right-hand-side and solution vector/matrix for
    // the L2-projection
    gsSparseEntries<T> projMatEntries;
    gsMatrix<T>        globProjRhs;
    globProjRhs.setZero( map_edge.boundarySize() + map_vertex.boundarySize(), m_system.unkSize(unk_)*m_system.rhs().cols() );

    // Temporaries
    gsVector<T> quWeights;

    gsMatrix<T> rhsVals;
    gsMatrix<unsigned> globIdxAct, locIdxAct;
    gsMatrix<unsigned> globIdxAct_0;
    gsMatrix<unsigned> globIdxAct_1;
    gsMatrix<T> basisVals;

    gsMapData<T> md(NEED_MEASURE);

    // Iterate over all patch-sides with Dirichlet-boundary conditions
    for ( typename gsBoundaryConditions<T>::const_iterator
              iter = m_pde_ptr->bc().dirichletBegin();
          iter != m_pde_ptr->bc().dirichletEnd(); ++iter )
    {
        if (iter->isHomogeneous() )
            continue;

        GISMO_ASSERT(iter->function()->targetDim() == m_system.unkSize(unk_)*m_system.rhs().cols(),
                     "Given Dirichlet boundary function does not match problem dimension."
                         <<iter->function()->targetDim()<<" != "<<m_system.unkSize(unk_)<<"\n");

        const size_t unk = iter->unknown();
        if(unk!=unk_)
            continue;
        const int patchIdx   = iter->patch();
        const index_t sideIdx = iter->side();

        const gsMultiPatch<T> multiPatch_Edges = g1_edges.at(g1Mapper.localToGlobal_Edge(sideIdx,patchIdx)).getG1Basis();
        const gsMultiPatch<T> multiPatch_Vertex_0 = g1_vertices.at(g1Mapper.localToGlobal_Vertex(g1Mapper.findVertexOfEdge(sideIdx).first,patchIdx)).getG1Basis();
        const gsMultiPatch<T> multiPatch_Vertex_1 = g1_vertices.at(g1Mapper.localToGlobal_Vertex(g1Mapper.findVertexOfEdge(sideIdx).second,patchIdx)).getG1Basis();

        const gsBasis<T> & basis = multiPatch_Edges.basis(0); // Assume that the basis is the same for all the basis functions

        const gsGeometry<T> & patch = m_pde_ptr->patches()[patchIdx];

        // Set up quadrature to degree+1 Gauss points per direction,
        // all lying on iter->side() except from the direction which
        // is NOT along the element

        gsGaussRule<T> bdQuRule(basis, 1.0, 1, iter->side().direction());

        // Create the iterator along the given part boundary.
        typename gsBasis<T>::domainIter bdryIter = basis.makeDomainIterator(iter->side());

        for(; bdryIter->good(); bdryIter->next() )
        {
            bdQuRule.mapTo( bdryIter->lowerCorner(), bdryIter->upperCorner(),
                            md.points, quWeights);

            //geoEval->evaluateAt( md.points );
            patch.computeMap(md);

            // the values of the boundary condition are stored
            // to rhsVals. Here, "rhs" refers to the right-hand-side
            // of the L2-projection, not of the PDE.
            rhsVals = iter->function()->eval( m_pde_ptr->domain()[patchIdx].eval( md.points ) );

            basisVals.setZero(multiPatch_Edges.nPatches() + multiPatch_Vertex_0.nPatches() + multiPatch_Vertex_1.nPatches(),md.points.dim().second);
            for (size_t i = 0; i < multiPatch_Edges.nPatches(); i++)
            {
                basisVals.row(i) += multiPatch_Edges.patch(i).eval(md.points);
            }
            for (size_t i = 0; i < multiPatch_Vertex_0.nPatches(); i++)
            {
                basisVals.row(multiPatch_Edges.nPatches() + i) += multiPatch_Vertex_0.patch(i).eval(md.points);
                basisVals.row(multiPatch_Edges.nPatches() + multiPatch_Vertex_0.nPatches() + i) +=
                    multiPatch_Vertex_1.patch(i).eval(md.points); // Assume the same length than vertex0
            }

            // Indices involved here:
            // --- Local index:
            // Index of the basis function/DOF on the patch.
            // Does not take into account any boundary or interface conditions.
            // --- Global Index:
            // Each DOF has a unique global index that runs over all patches.
            // This global index includes a re-ordering such that all eliminated
            // DOFs come at the end.
            // The global index also takes care of glued interface, i.e., corresponding
            // DOFs on different patches will have the same global index, if they are
            // glued together.
            // --- Boundary Index (actually, it's a "Dirichlet Boundary Index"):
            // The eliminated DOFs, which come last in the global indexing,
            // have their own numbering starting from zero.

            // Get the global indices (second line) of the local
            // active basis (first line) functions/DOFs:

            //basis.active_into(md.points.col(0), locIdxAct );
            //map_boundary.localToGlobal( locIdxAct, patchIdx, globIdxAct);
            gsVector<unsigned> vec;
            vec.setLinSpaced(multiPatch_Edges.nPatches(),0,multiPatch_Edges.nPatches());
            locIdxAct.setZero(multiPatch_Edges.nPatches(),1);
            locIdxAct.col(0) = vec;
            map_edge.localToGlobal(locIdxAct, g1Mapper.localToGlobal_Edge(sideIdx,patchIdx), globIdxAct);

            vec.setLinSpaced(multiPatch_Vertex_0.nPatches(),0,multiPatch_Vertex_0.nPatches());
            locIdxAct.setZero(multiPatch_Vertex_0.nPatches(),1);
            locIdxAct.col(0) = vec;
            map_vertex.localToGlobal(locIdxAct, g1Mapper.localToGlobal_Vertex(g1Mapper.findVertexOfEdge(sideIdx).first,patchIdx), globIdxAct_0);
            map_vertex.localToGlobal(locIdxAct, g1Mapper.localToGlobal_Vertex(g1Mapper.findVertexOfEdge(sideIdx).second,patchIdx), globIdxAct_1); // The same local indices

            // Out of the active functions/DOFs on this element, collect all those
            // which correspond to a boundary DOF.
            // This is checked by calling mapper.is_boundary_index( global Index )

            // eltBdryFcts stores the row in basisVals/globIdxAct, i.e.,
            // something like a "element-wise index"
            std::vector<index_t> eltBdryFcts;
            eltBdryFcts.reserve(map_edge.boundarySize() + map_vertex.boundarySize());
            for( size_t i=0; i < multiPatch_Edges.nPatches(); i++)
                if( map_edge.is_boundary(i,g1Mapper.localToGlobal_Edge(sideIdx,patchIdx)) )
                    eltBdryFcts.push_back( i );

            for( size_t i=0; i < multiPatch_Vertex_0.nPatches(); i++)
                if( map_vertex.is_boundary(i,g1Mapper.localToGlobal_Vertex(g1Mapper.findVertexOfEdge(sideIdx).first,patchIdx)))
                    eltBdryFcts.push_back( i + multiPatch_Edges.nPatches());

            for( size_t i=0; i < multiPatch_Vertex_1.nPatches(); i++)
                if( map_vertex.is_boundary(i,g1Mapper.localToGlobal_Vertex(g1Mapper.findVertexOfEdge(sideIdx).second,patchIdx)))
                    eltBdryFcts.push_back( i + multiPatch_Vertex_0.nPatches() + multiPatch_Edges.nPatches());

            // Do the actual assembly:
            for( index_t k=0; k < md.points.cols(); k++ )
            {
                const T weight_k = quWeights[k] * md.measure(k);

                // Only run through the active boundary functions on the element:
                for( size_t i0=0; i0 < eltBdryFcts.size(); i0++ )
                {
                    unsigned ii, jj;
                    // Each active boundary function/DOF in eltBdryFcts has...
                    // ...the above-mentioned "element-wise index"
                    const index_t i = eltBdryFcts[i0];
                    // ...the boundary index.
                    if (i < multiPatch_Edges.nPatches())
                        ii = map_edge.global_to_bindex(globIdxAct.at(i));
                    else if (i >= multiPatch_Edges.nPatches() && i < multiPatch_Vertex_0.nPatches() + multiPatch_Edges.nPatches())
                        ii = map_edge.boundarySize() + map_vertex.global_to_bindex(globIdxAct_0.at(i - multiPatch_Edges.nPatches()));
                    else if (i >= multiPatch_Edges.nPatches() + multiPatch_Vertex_0.nPatches())
                        ii = map_edge.boundarySize() + map_vertex.global_to_bindex(globIdxAct_1.at(i - multiPatch_Edges.nPatches() - multiPatch_Vertex_0.nPatches()));

                    for( size_t j0=0; j0 < eltBdryFcts.size(); j0++ )
                    {
                        const index_t j = eltBdryFcts[j0];
                        if (j < multiPatch_Edges.nPatches())
                            jj = map_edge.global_to_bindex(globIdxAct.at(j));
                        else if (j >= multiPatch_Edges.nPatches() && j < multiPatch_Vertex_0.nPatches() + multiPatch_Edges.nPatches())
                            jj = map_edge.boundarySize() + map_vertex.global_to_bindex(globIdxAct_0.at(j - multiPatch_Edges.nPatches()));
                        else if (j >= multiPatch_Edges.nPatches() + multiPatch_Vertex_0.nPatches())
                            jj = map_edge.boundarySize() + map_vertex.global_to_bindex(globIdxAct_1.at(j - multiPatch_Edges.nPatches() - multiPatch_Vertex_0.nPatches()));

                        // Use the "element-wise index" to get the needed
                        // function value.
                        // Use the boundary index to put the value in the proper
                        // place in the global projection matrix.
                        projMatEntries.add(ii, jj, weight_k * basisVals(i,k) * basisVals(j,k));
                    } // for j
                    globProjRhs.row(ii) += weight_k *  basisVals(i,k) * rhsVals.col(k).transpose();
                } // for i
            } // for k
        } // bdryIter
    } // boundaryConditions-Iterator

    gsSparseMatrix<T> globProjMat( map_edge.boundarySize() + map_vertex.boundarySize() ,
                                   map_edge.boundarySize() + map_vertex.boundarySize() );
    globProjMat.setFrom( projMatEntries );
    globProjMat.makeCompressed();

    // Solve the linear system:
    // The position in the solution vector already corresponds to the
    // numbering by the boundary index. Hence, we can simply take them
    // for the values of the eliminated Dirichlet DOFs.
    typename gsSparseSolver<T>::CGDiagonal solver;
    m_g1_ddof = solver.compute( globProjMat ).solve ( globProjRhs );
}

} // namespace gismo
