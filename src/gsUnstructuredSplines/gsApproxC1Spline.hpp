/** @file gsApproxC1Spline.hpp

    @brief Provides declaration of Basis abstract interface.

    This file is part of the G+Smo library.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

    Author(s): F. Buchegger
*/

#pragma once

#include<gsUnstructuredSplines/gsApproxC1Edge.h>
#include<gsUnstructuredSplines/gsApproxC1Vertex.h>

namespace gismo
{

template<short_t d,class T>
void gsApproxC1Spline<d,T>::defaultOptions()
{
    /*
        to do: general
    */
    gsTensorBSplineBasis<d, T> basis = dynamic_cast<gsTensorBSplineBasis<d, T> &>(m_multiBasis.basis(0));
    index_t p = basis.degree(0);

    for (size_t np = 0; np < m_patches.nPatches(); np++)
    {
        gsTensorBSplineBasis<d, T> basis = dynamic_cast<gsTensorBSplineBasis<d, T> &>(m_multiBasis.basis(np));
        if (p != basis.degree(0))
            gsWarn << "Not suitable for different degrees! \n";
    }

    // For the gluing data space
    m_options.addInt("gluingDataDegree","Polynomial degree of the gluing data space", p-1 );
    m_options.addInt("gluingDataRegularity","Regularity of the gluing data space",  p-2 );
    m_options.addSwitch("info","Print debug information",  false );
    m_options.addSwitch("plot","Print debug information",  false );

}

template<short_t d,class T>
void gsApproxC1Spline<d,T>::init()
{
    p_tilde = m_options.getInt("gluingDataDegree");//math::max(m_optionList.getInt("discreteDegree") - 1, 2);
    r_tilde = m_options.getInt("gluingDataRegularity");//p_tilde - 1;

    m_bases.clear();
    m_bases.reserve(m_patches.nPatches()); // For each Patch
    for (size_t np = 0; np < m_patches.nPatches(); np++)
    {
        // gsContainerBasisBase:
        // # basisContainer
        // - Interior space: [0] : inner,
        // - Edge spaces:    [1] : west, [2] : east, [3] : south, [4] : north,
        // - Vertex spaces:  [5] : southwest, [6] : southeast, [7] : northwest, [8] : northeast
        //
        // # helperBasisContainer
        // [0] : basis plus, [1] : basis minus, [2] : basis geo, [3] : basis gluing data
        gsContainerBasis<d,T> containerBasis(9, 4); // for 9 subspaces and 4 helper Basis
        m_bases.push_back(containerBasis);
    }

    // Create interior spline space
    for (size_t np = 0; np < m_patches.nPatches(); np++)
    {
        gsTensorBSplineBasis<d, T> basis_inner = dynamic_cast<gsTensorBSplineBasis<d, T> &>(m_multiBasis.basis(np));

        // Construct special space for r = p - 1:
        // The first and the last knot (not 0,1) are repeated +1, e.g.
        // deg 3, r = 2: |||| || | [...] | || ||||

        index_t m,p;
        for (index_t uv = 0; uv < 2; uv++)
        {
            p = basis_inner.degree(uv);
            m = basis_inner.knots(uv).multiplicityIndex(p+1);
            if (m == 1)
            {
                T knot_u = basis_inner.knot(uv,basis_inner.degree(uv)+1);
                if (knot_u != 1)
                    basis_inner.insertKnot(knot_u,uv,1);

                if (knot_u != 0.5 && knot_u != 1)
                    basis_inner.insertKnot(1-knot_u,uv,1);
            }
        }
        m_bases[np].setBasis(0, basis_inner); // Inner
    }

    // For loop over Interface to construct the spaces
    for (size_t numInt = 0; numInt < m_patches.interfaces().size(); numInt++)
    {
        const boundaryInterface & item = m_patches.interfaces()[numInt];

        const index_t side_1 = item.first().side().index();
        const index_t side_2 = item.second().side().index();
        const index_t patch_1 = item.first().patch;
        const index_t patch_2 = item.second().patch;

        const index_t dir_1 = side_1 > 2 ? 0 : 1;
        const index_t dir_2 = side_2 > 2 ? 0 : 1;

        gsBSplineBasis<T> basis_1 = dynamic_cast<gsBSplineBasis<T> &>(m_multiBasis.basis(patch_1).component(dir_1));
        gsBSplineBasis<T> basis_2 = dynamic_cast<gsBSplineBasis<T> &>(m_multiBasis.basis(patch_2).component(dir_2));

        gsBSplineBasis<T> basis_geo_1 = dynamic_cast<gsBSplineBasis<T> &>(m_multiBasis.basis(patch_1).component(1-dir_1));
        gsBSplineBasis<T> basis_geo_2 = dynamic_cast<gsBSplineBasis<T> &>(m_multiBasis.basis(patch_2).component(1-dir_2));

        gsKnotVector<T> kv_1 = basis_1.knots();
        gsKnotVector<T> kv_2 = basis_2.knots();

        gsBSplineBasis<T> patch_basis_1 = dynamic_cast<gsBSplineBasis<T> &>(m_patches.patch(patch_1).basis().component(dir_1));
        gsKnotVector<T> kv_patch_1 = patch_basis_1.knots();

        gsBSplineBasis<T> patch_basis_2 = dynamic_cast<gsBSplineBasis<T> &>(m_patches.patch(patch_2).basis().component(dir_2));
        gsKnotVector<T> kv_patch_2 = patch_basis_1.knots();

        gsKnotVector<T> kv_plus, kv_minus, kv_gluingData;
        createPlusMinusSpace(kv_1, kv_2, kv_patch_1, kv_patch_2, kv_plus, kv_minus);

        gsBSplineBasis<T> basis_plus(kv_plus); // S(p,r+1,h)
        gsBSplineBasis<T> basis_minus(kv_minus); // S(p-1,r,h)

        //\tilde{p} = max(p-1,2)
        //\tilde{r} = \tilde{p}-1
        createGluingDataSpace(kv_1, kv_2, kv_patch_1, kv_patch_2, kv_gluingData);
        gsBSplineBasis<T> basis_gluingData(kv_gluingData); // S(\tilde{p},\tilde{r},h)
/*
        if (m_options.getSwitch("info"))
        {
            gsInfo << "Basis geo 1 : " << basis_geo_1.knots().asMatrix() << "\n";
            gsInfo << "Basis geo 2 : " << basis_geo_2.knots().asMatrix() << "\n";
            gsInfo << "Basis plus : " << basis_plus.knots().asMatrix() << "\n";
            gsInfo << "Basis minus : " << basis_minus.knots().asMatrix() << "\n";

            gsInfo << "Basis gluingData : " << basis_gluingData.knots().asMatrix() << "\n";
        }
*/

        gsKnotVector<T> kv_geo_1 = basis_geo_1.knots();
        gsKnotVector<T> kv_geo_2 = basis_geo_2.knots();

        gsKnotVector<T> kv_edge_1, kv_edge_2;

        createLocalEdgeSpace(kv_plus, kv_minus, kv_gluingData, kv_gluingData, kv_patch_1, kv_patch_2, kv_edge_1, kv_edge_2);
        gsBSplineBasis<T> basis_edge(kv_edge_1);
/*
        if (m_options.getSwitch("info"))
            gsInfo << "Basis edge : " << basis_edge.knots().asMatrix() << "\n";
*/
        gsTensorBSplineBasis<d, T> basis_edge_1(dir_1 == 0 ? kv_edge_1 : kv_geo_1, dir_1 == 0 ? kv_geo_1 : kv_edge_1);
        gsTensorBSplineBasis<d, T> basis_edge_2(dir_2 == 0 ? kv_edge_2 : kv_geo_2, dir_2 == 0 ? kv_geo_2 : kv_edge_2);

        m_bases[patch_1].setHelperBasis(side_1-1, 0, basis_plus);
        m_bases[patch_2].setHelperBasis(side_2-1, 0, basis_plus);

        m_bases[patch_1].setHelperBasis(side_1-1, 1, basis_minus);
        m_bases[patch_2].setHelperBasis(side_2-1, 1, basis_minus);

        m_bases[patch_1].setHelperBasis(side_1-1, 2, basis_geo_1);
        m_bases[patch_2].setHelperBasis(side_2-1, 2, basis_geo_2);

        m_bases[patch_1].setHelperBasis(side_1-1, 3, basis_gluingData);
        m_bases[patch_2].setHelperBasis(side_2-1, 3, basis_gluingData);

        m_bases[patch_1].setBasis(side_1, basis_edge_1);
        m_bases[patch_2].setBasis(side_2, basis_edge_2);
    }

    // For loop over the Edge to construct the spaces
    for (size_t numBdy = 0; numBdy < m_patches.boundaries().size(); numBdy++)
    {
        const patchSide & bit = m_patches.boundaries()[numBdy];

        index_t patch_1 = bit.patch;
        index_t side_1 = bit.side().index();

        index_t dir_1 = m_patches.boundaries()[numBdy].m_index < 3 ? 1 : 0;

        // Using Standard Basis for boundary edges
        gsTensorBSplineBasis<d, T> basis_edge_1 = dynamic_cast<gsTensorBSplineBasis<d, real_t> &>(m_multiBasis.basis(patch_1));

        gsBSplineBasis<T> basis_1 = dynamic_cast<gsBSplineBasis<> &>(m_multiBasis.basis(patch_1).component(dir_1));
        gsBSplineBasis<T> basis_geo_1 = dynamic_cast<gsBSplineBasis<> &>(m_multiBasis.basis(patch_1).component(1-dir_1));

        // Assume that plus/minus space is the same as the inner space
        gsKnotVector<T> kv_1 = basis_1.knots();

        gsBSplineBasis<T> patch_basis_1 = dynamic_cast<gsBSplineBasis<> &>(m_patches.patch(patch_1).basis().component(dir_1));
        gsKnotVector<T> kv_patch_1 = patch_basis_1.knots();

        gsKnotVector<T> kv_plus, kv_minus;
        createPlusMinusSpace(kv_1,  kv_patch_1,  kv_plus, kv_minus);

        gsBSplineBasis<T> basis_plus = gsBSplineBasis<>(kv_plus);
        gsBSplineBasis<T> basis_minus = gsBSplineBasis<>(kv_minus);


        index_t m, p;
        p = basis_1.degree();
        m = basis_1.knots().multiplicityIndex(p+1);
        if (m == 1)
        {
/*
            T knot_u = basis_edge_1.knot(0,basis_edge_1.degree(0)+1);
            if (knot_u != 1)
            {
                basis_geo_1.insertKnot(knot_u,1); // the first
                //basis_geo_1.insertKnot(knot_u+knot_u,1);
            }
            if (knot_u != 0.5)
            {
                basis_geo_1.insertKnot(1-knot_u,1); // the last
                //basis_geo_1.insertKnot(1-knot_u-knot_u,1);
            }
*/
            basis_geo_1.reduceContinuity(1);
        }


        gsKnotVector<T> kv_edge_1;
        createLocalEdgeSpace(kv_plus, kv_minus, kv_patch_1, kv_edge_1);

        gsKnotVector<T> kv_geo_1 = basis_geo_1.knots();
        gsTensorBSplineBasis<d, T> basis_edge_1_temp(dir_1 == 0 ? kv_edge_1 : kv_geo_1, dir_1 == 0 ? kv_geo_1 : kv_edge_1);

        m_bases[patch_1].setHelperBasis(side_1-1, 0, basis_plus);
        m_bases[patch_1].setHelperBasis(side_1-1, 1, basis_minus);

        m_bases[patch_1].setHelperBasis(side_1-1, 2, basis_geo_1);

        m_bases[patch_1].setBasis(side_1, basis_edge_1_temp);

/*
        if (m_options.getSwitch("info"))
            gsInfo << "Basis boundary: " << basis_edge_1_temp << "\n";
*/


    }
    // For loop over the Vertex to construct the spaces
    for (size_t numVer = 0; numVer < m_patches.vertices().size(); numVer++)
    {
        std::vector<patchCorner> allcornerLists = m_patches.vertices()[numVer];
        std::vector<size_t> patchIndex;
        std::vector<size_t> vertIndex;
        for (size_t j = 0; j < allcornerLists.size(); j++)
        {
            patchIndex.push_back(allcornerLists[j].patch);
            vertIndex.push_back(allcornerLists[j].m_index);
        }

        if (patchIndex.size() == 1) // Boundary vertex
        {
            index_t patch_1 = patchIndex[0];
            index_t vertex_1 = vertIndex[0];

            gsTensorBSplineBasis<d, T> basis_vertex_1 = dynamic_cast<gsTensorBSplineBasis<d, real_t> &>(m_multiBasis.basis(patch_1));

            /*
                to do: fix for general regularity.
             */
            index_t m, p;
            p = basis_vertex_1.degree(0);
            m = basis_vertex_1.knots(0).multiplicityIndex(p+1);
            if (m == 1) // == basis_vertex_1.degree(1)
                basis_vertex_1.reduceContinuity(1); // In the case for the max. smoothness

            m_bases[patch_1].setBasis(vertex_1+4, basis_vertex_1);
        }
        else if (patchIndex.size() > 1)
        {
            gsMultiPatch<T> temp_mp;
            for (size_t j = 0; j < patchIndex.size(); j++)
                temp_mp.addPatch(m_patches.patch(patchIndex[j]));
            temp_mp.computeTopology();

            if (patchIndex.size() == temp_mp.interfaces().size()) // Internal vertex
            {
                for (size_t j = 0; j < patchIndex.size(); j++)
                {
                    index_t patch_1 = patchIndex[j];
                    index_t vertex_1 = vertIndex[j];

                    gsTensorBSplineBasis<d, T> basis_vertex_1 = dynamic_cast<gsTensorBSplineBasis<d, real_t> &>(m_multiBasis.basis(
                            patch_1));

                    //createLocalVertexSpace(basis_vertex_1, basis_vertex_result);

                    //index_t p_tilde_1 = math::max(m_multiBasis.basis(patch_1).degree(0)-1,3);
                    //index_t p_tilde_2 = math::max(m_multiBasis.basis(patch_1).degree(1)-1,3);

                    basis_vertex_1.degreeElevate(p_tilde-1,0); // Keep smoothness
                    basis_vertex_1.degreeElevate(p_tilde-1,1);

                    // todo: fix for general regularity
                    index_t r, p;
                    p = basis_vertex_1.degree(0);
                    r = p - basis_vertex_1.knots(0).multiplicityIndex(p+1);

                    //if (m_multiBasis.basis(patch_1).degree(0) - r == 1)
                    //    basis_vertex_1.reduceContinuity(r-1); // In the case for the max. smoothness
                    //else if (r > 2)
                    {
                        if (r != 1)
                            basis_vertex_1.reduceContinuity(1); // bcs of minus space
                        if (r_tilde < r-1)
                            basis_vertex_1.reduceContinuity(r-r_tilde-1);
                    }

                    m_bases[patch_1].setBasis(vertex_1+4, basis_vertex_1);
                }
            }
            else if (patchIndex.size() > temp_mp.interfaces().size())// Interface-Boundary vertex
            {
                for (size_t j = 0; j < patchIndex.size(); j++)
                {
                    index_t patch_1 = patchIndex[j];
                    index_t vertex_1 = vertIndex[j];

                    gsTensorBSplineBasis<d, T> basis_vertex_1 = dynamic_cast<gsTensorBSplineBasis<d, real_t> &>(m_multiBasis.basis(patch_1));

                    //index_t p_tilde_1 = math::max(m_multiBasis.basis(patch_1).degree(0)-1,3);
                    //index_t p_tilde_2 = math::max(m_multiBasis.basis(patch_1).degree(1)-1,3);
/*
                    gsBSplineBasis<> basis_1 = dynamic_cast<gsBSplineBasis<> &>(m_multiBasis.basis(patch_1).component(0));
                    gsBSplineBasis<> basis_geo_1 = dynamic_cast<gsBSplineBasis<> &>(m_multiBasis.basis(0).component(1));

                    gsKnotVector<T> kv_1 = basis_1.knots();
                    gsKnotVector<T> kv_2 = basis_geo_1.knots();
                    gsTensorBSplineBasis<d, T> basis_vertex_1(kv_1,kv_2);
*/
                    basis_vertex_1.degreeElevate(p_tilde-1,0); // Keep smoothness
                    basis_vertex_1.degreeElevate(p_tilde-1,1);

                    // todo: fix for general regularity
                    index_t r, p;
                    p = basis_vertex_1.degree(0);
                    r = p - basis_vertex_1.knots(0).multiplicityIndex(p+1);

                    //if (m_multiBasis.basis(patch_1).degree(0) - r == 1)
                    //    basis_vertex_1.reduceContinuity(r-1); // In the case for the max. smoothness
                    //else if (r > 2)
                    {
                        if (r != 1)
                            basis_vertex_1.reduceContinuity(1); // bcs of minus space
                        if (r_tilde < r-1)
                            basis_vertex_1.reduceContinuity(r-r_tilde-1);
                    }
/*
                    if (m_options.getSwitch("info"))
                    {
                        gsBSplineBasis<> basis_geo_1 = dynamic_cast<gsBSplineBasis<> &>(basis_vertex_1.component(0));
                        gsInfo << "Basis vertex 1: " << basis_geo_1.knots().asMatrix() << "\n";
                        gsBSplineBasis<> basis_geo_2 = dynamic_cast<gsBSplineBasis<> &>(basis_vertex_1.component(1));
                        gsInfo << "Basis vertex 2: " << basis_geo_2.knots().asMatrix() << "\n";
                    }
*/
                    m_bases[patch_1].setBasis(vertex_1+4, basis_vertex_1);

                }
            }
        }
    }


    index_t row_dofs = 0;
    // Inner basis
    for (size_t np = 0; np < m_patches.nPatches(); np++)
    {
        index_t dim_u = m_bases[np].getBasis(0).component(0).size();
        index_t dim_v = m_bases[np].getBasis(0).component(1).size();
        row_dofs += (dim_u - 4) * (dim_v - 4);
    }

    // Interfaces
    for (size_t numInt = 0; numInt < m_patches.interfaces().size(); numInt++)
    {
        const boundaryInterface &item = m_patches.interfaces()[numInt];

        const index_t side_1 = item.first().side().index();
        const index_t patch_1 = item.first().patch;

        index_t numDofs = math::max(m_bases[patch_1].getHelperBasis(side_1-1, 0).size() + m_bases[patch_1].getHelperBasis(side_1-1, 1).size() - 10, 0);
        row_dofs += numDofs; // The same as side_2
    }

    // Boundary Edges
    for (size_t numBdy = 0; numBdy < m_patches.boundaries().size(); numBdy++)
    {
        const patchSide &bit = m_patches.boundaries()[numBdy];

        index_t patch_1 = bit.patch;
        index_t side_1 = bit.side().index();

        index_t numDofs = math::max(m_bases[patch_1].getHelperBasis(side_1-1, 0).size() + m_bases[patch_1].getHelperBasis(side_1-1, 1).size() - 10, 0);
        row_dofs += numDofs;
    }

    // Vertices
    for (size_t numVer = 0; numVer < m_patches.vertices().size(); numVer++)
    {
        row_dofs += 6;
    }


    m_matrix.clear();
    index_t dim_col = 0;
    for (size_t i = 0; i < m_bases.size(); i++)
    {
        dim_col += m_bases[i].size();
    }

    m_matrix.resize(row_dofs, dim_col);
    const index_t nz = 7*row_dofs; // TODO
    m_matrix.reserve(nz);
}   


template<short_t d,class T>
void gsApproxC1Spline<d,T>::compute()
{
    // Compute Inner Basis functions
    index_t shift_row = 0, shift_col = 0;
    for(size_t np = 0; np < m_patches.nPatches(); ++np)
    {
        index_t dim_u = m_bases[np].getBasis(0).component(0).size();
        index_t dim_v = m_bases[np].getBasis(0).component(1).size();

        index_t row_i = 0;
        for (index_t j = 2; j < dim_v-2; ++j)
            for (index_t i = 2; i < dim_u-2; ++i)
            {
                m_matrix.insert(shift_row + row_i, shift_col + j*dim_u+i) = 1.0;
                ++row_i;
            }

        shift_row += row_i;
        shift_col += m_bases[np].size();
    }

    // Interfaces
    for (size_t numInt = 0; numInt < m_patches.interfaces().size(); numInt++)
    {
        const boundaryInterface & item = m_patches.interfaces()[numInt];
        index_t side_1 = item.first().side().index();
        index_t side_2 = item.second().side().index();

        index_t patch_1 = item.first().patch;
        index_t patch_2 = item.second().patch;

        gsApproxC1Edge<d, T> approxC1Edge(m_patches, m_bases, item, numInt, m_options);
        std::vector<gsMultiPatch<T>> basisEdge = approxC1Edge.getEdgeBasis();

        index_t begin_col = 0, end_col = 0, shift_col = 0;
        for (index_t np = 0; np < patch_1; ++np)
            shift_col += m_bases[np].size();

        for (index_t ns = 0; ns < side_1; ++ns)
            begin_col += m_bases[patch_1].getBasis(ns).size();
        for (index_t ns = 0; ns < side_1+1; ++ns)
            end_col += m_bases[patch_1].getBasis(ns).size();

        for (size_t ii = 0; ii < basisEdge[0].nPatches(); ++ii)
        {
            index_t jj = 0;
            for (index_t j = begin_col; j < end_col; ++j, ++jj) {
                if (basisEdge[0].patch(ii).coef(jj, 0) * basisEdge[0].patch(ii).coef(jj, 0) > 1e-25)
                    m_matrix.insert(shift_row + ii, shift_col + j) = basisEdge[0].patch(ii).coef(jj, 0);
            }
        }

        begin_col = 0, end_col = 0, shift_col = 0;
        for (index_t np = 0; np < patch_2; ++np)
            shift_col += m_bases[np].size();

        for (index_t ns = 0; ns < side_2; ++ns)
            begin_col += m_bases[patch_2].getBasis(ns).size();
        for (index_t ns = 0; ns < side_2+1; ++ns)
            end_col += m_bases[patch_2].getBasis(ns).size();

        for (size_t ii = 0; ii < basisEdge[1].nPatches(); ++ii)
        {
            index_t jj = 0;
            for (index_t j = begin_col; j < end_col; ++j, ++jj) {
                if (basisEdge[1].patch(ii).coef(jj, 0) * basisEdge[1].patch(ii).coef(jj, 0) > 1e-25)
                    m_matrix.insert(shift_row + ii, shift_col + j) = basisEdge[1].patch(ii).coef(jj, 0);
            }
        }

        shift_row += basisEdge[0].nPatches();
    }

    // Compute Edge Basis functions
    for (size_t numBdy = 0; numBdy < m_patches.boundaries().size(); numBdy++)
    {
        const patchSide & bit = m_patches.boundaries()[numBdy];
        index_t side_1 = bit.side().index();
        index_t patch_1 = bit.patch;

        gsApproxC1Edge<d, T> approxC1Edge(m_patches, m_bases, bit, numBdy, m_options);
        std::vector<gsMultiPatch<T>> basisEdge = approxC1Edge.getEdgeBasis();

        index_t begin_col = 0, end_col = 0, shift_col = 0;
        for (index_t np = 0; np < patch_1; ++np)
            shift_col += m_bases[np].size();

        for (index_t ns = 0; ns < side_1; ++ns)
            begin_col += m_bases[patch_1].getBasis(ns).size();
        for (index_t ns = 0; ns < side_1+1; ++ns)
            end_col += m_bases[patch_1].getBasis(ns).size();

        for (size_t ii = 0; ii < basisEdge[0].nPatches(); ++ii)
        {
            index_t jj = 0;
            for (index_t j = begin_col; j < end_col; ++j, ++jj) {
                if (basisEdge[0].patch(ii).coef(jj, 0) * basisEdge[0].patch(ii).coef(jj, 0) > 1e-20)
                    m_matrix.insert(shift_row + ii, shift_col + j) = basisEdge[0].patch(ii).coef(jj, 0);
            }
        }
        shift_row += basisEdge[0].nPatches();
    }

    // Compute Vertex Basis functions
    for (size_t numVer = 0; numVer < m_patches.vertices().size(); numVer++)
    {
        std::vector<patchCorner> allcornerLists = m_patches.vertices()[numVer];
        std::vector<size_t> patchIndex;
        std::vector<size_t> vertIndex;
        for (size_t j = 0; j < allcornerLists.size(); j++)
        {
            patchIndex.push_back(allcornerLists[j].patch);
            vertIndex.push_back(allcornerLists[j].m_index);
        }

        gsApproxC1Vertex<d, T> approxC1Vertex(m_patches, m_bases, patchIndex, vertIndex, numVer, m_options);
        std::vector<gsMultiPatch<T>> basisVertex = approxC1Vertex.getVertexBasis();

        for (size_t np = 0; np < patchIndex.size(); ++np)
        {
            index_t patch_1 = patchIndex[np];
            index_t corner = vertIndex[np];

            index_t begin_col = 0, end_col = 0, shift_col = 0;
            for (index_t np = 0; np < patch_1; ++np)
                shift_col += m_bases[np].size();

            for (index_t ns = 0; ns < corner+4; ++ns)
                begin_col += m_bases[patch_1].getBasis(ns).size();
            for (index_t ns = 0; ns < corner+4+1; ++ns)
                end_col += m_bases[patch_1].getBasis(ns).size();

            for (size_t ii = 0; ii < basisVertex[np].nPatches(); ++ii)
            {
                index_t jj = 0;
                for (index_t j = begin_col; j < end_col; ++j, ++jj) {
                    if (basisVertex[np].patch(ii).coef(jj, 0) * basisVertex[np].patch(ii).coef(jj, 0) > 1e-20)
                        m_matrix.insert(shift_row + ii, shift_col + j) = basisVertex[np].patch(ii).coef(jj, 0);


                }
            }
        }
        shift_row += basisVertex[0].nPatches(); // + 6

    }
    m_matrix.makeCompressed();

/*
    if (m_options.getSwitch("info"))
    {
        gsInfo << "Dim for Patches: \n";
        for(size_t np = 0; np < m_patches.nPatches(); ++np)
        {
            gsInfo << "(" << shift_row << "," << m_bases[np].size() << "), ";
        }
        gsInfo << "\n";
    }
*/
}


template<short_t d,class T>
void gsApproxC1Spline<d,T>::createPlusMinusSpace(gsKnotVector<T> & kv1, gsKnotVector<T> & kv2,
                                               gsKnotVector<T> & kv1_patch, gsKnotVector<T> & kv2_patch,
                                               gsKnotVector<T> & kv1_result, gsKnotVector<T> & kv2_result)
{
    std::vector<real_t> knots_unique_1 = kv1.unique();
    std::vector<real_t> knots_unique_2 = kv2.unique();

    std::vector<index_t> knots_mult_1 = kv1.multiplicities();
    std::vector<index_t> knots_mult_2 = kv2.multiplicities();

    std::vector<real_t> patch_kv_unique_1 = kv1_patch.unique();
    std::vector<index_t> patch_kv_mult_1 = kv1_patch.multiplicities();

    std::vector<real_t> patch_kv_unique_2 = kv2_patch.unique();
    std::vector<index_t> patch_kv_mult_2 = kv2_patch.multiplicities();

    std::vector<real_t> knot_vector_plus, knot_vector_minus;

    if (knots_unique_1 != knots_unique_2)
        gsInfo << "NOT IMPLEMENTED YET 1: Plus, Minus space \n";

    if (kv1.degree() != kv2.degree())
        gsInfo << "NOT IMPLEMENTED YET 2: Plus, Minus space \n";

    if (knots_mult_1 != knots_mult_2)
        gsInfo << "NOT IMPLEMENTED YET 4: Plus, Minus space \n";

    // todo: fix for general regularity
    index_t m, p;
    p = kv1.degree();
    m = kv1.multiplicityIndex(p+1);

    kv1_result = kv1; // == kv2
    if (m != 1)
        kv1_result.reduceMultiplicity(1);

    kv2_result = kv1; // == kv2
    kv2_result.degreeDecrease(1);
    if (m != 1)
        kv2_result.reduceMultiplicity(1);
}


template<short_t d,class T>
void gsApproxC1Spline<d,T>::createPlusMinusSpace(gsKnotVector<T> & kv1,
                                               gsKnotVector<T> & kv1_patch,
                                               gsKnotVector<T> & kv1_result, gsKnotVector<T> & kv2_result)
{
    std::vector<real_t> knots_unique_1 = kv1.unique();

    std::vector<real_t> patch_kv_unique_1 = kv1_patch.unique();
    std::vector<index_t> patch_kv_mult_1 = kv1_patch.multiplicities();

    index_t m,p;
    p = math::max(kv1.degree(), 0);
    m = kv1.multiplicityIndex(p+1);

    kv1_result = kv1;
    if (m != 1)
        kv1_result.reduceMultiplicity(1);

    kv2_result = kv1;
    kv2_result.degreeDecrease(1);
    if (m != 1)
        kv2_result.reduceMultiplicity(1);
}

template<short_t d,class T>
void gsApproxC1Spline<d,T>::createGluingDataSpace(gsKnotVector<T> & kv1, gsKnotVector<T> & kv2,
                                                gsKnotVector<T> & kv1_patch, gsKnotVector<T> & kv2_patch,
                                                gsKnotVector<T> & kv_result)
{
    std::vector<real_t> knots_unique_1 = kv1.unique();
    std::vector<real_t> knots_unique_2 = kv2.unique();

    std::vector<real_t> knot_vector;
    knot_vector = knots_unique_2; // = knots_unique_1

    gsKnotVector<> new_knotvector(knot_vector, p_tilde, r_tilde);
    kv_result = new_knotvector;
} // createGluingDataSpace


template<short_t d,class T>
void gsApproxC1Spline<d,T>::createLocalEdgeSpace(gsKnotVector<T> & kv_plus, gsKnotVector<T> & kv_minus,
                                               gsKnotVector<T> & kv_gD_1, gsKnotVector<T> & kv_gD_2,
                                               gsKnotVector<T> & kv_patch_1, gsKnotVector<T> & kv_patch_2,
                                               gsKnotVector<T> & kv1_result, gsKnotVector<T> & kv2_result)
{
    index_t p_1 = math::max(kv_plus.degree()+kv_gD_1.degree()-1, kv_minus.degree()+kv_gD_1.degree() );
    //index_t p_2 = math::max(kv_plus.degree()+kv_gD_2.degree()-1, kv_minus.degree()+kv_gD_2.degree() ); == p_1

    std::vector<real_t> knots_unique_plus = kv_plus.unique(); // == kv_minus.unique()

    if (knots_unique_plus != kv_minus.unique())
        gsInfo << "ERROR LOKAL EDGE SPACE \n";

    index_t r = 1;
    if (knots_unique_plus[1] != 1)
    {
        index_t r_plus = kv_plus.degree() - kv_plus.multiplicities()[1]; // The same for all
        index_t r_minus = kv_minus.degree() - kv_minus.multiplicities()[1]; // The same for all
        index_t r_tilde = kv_gD_1.degree() - kv_gD_1.multiplicities()[1]; // The same for all
        //gsInfo << "R_tilde " << r_tilde << "\n";
        //gsInfo << "r_plus " << r_plus << "\n";
        //gsInfo << "r_minus " << r_minus << "\n";

        r = math::min(r_tilde, math::min(r_plus, r_minus));
        //gsInfo << "r " << r << "\n";
    }
    kv1_result = gsKnotVector<>(knots_unique_plus, p_1, r);
    // ==
    kv2_result = kv1_result;
} // createLocalEdgeSpace

template<short_t d,class T>
void gsApproxC1Spline<d,T>::createLocalEdgeSpace(gsKnotVector<T> & kv_plus, gsKnotVector<T> & kv_minus,
                                               gsKnotVector<T> & kv_patch_1,
                                               gsKnotVector<T> & kv1_result)
{
    index_t p_1 = math::max(kv_plus.degree(), kv_minus.degree() );

    std::vector<real_t> knots_unique_plus = kv_plus.unique(); // == kv_minus.unique()

    if (knots_unique_plus != kv_minus.unique())
        gsInfo << "ERROR LOKAL EDGE SPACE \n";

    index_t r = 1;
    if (knots_unique_plus[1] != 1)
    {
        index_t r_plus = kv_plus.degree() - kv_plus.multiplicities()[1]; // The same for all
        index_t r_minus = kv_minus.degree() - kv_minus.multiplicities()[1]; // The same for all

        r = math::min(r_plus, r_minus);
    }
    kv1_result = gsKnotVector<>(knots_unique_plus, p_1, r);
} // createLocalEdgeSpace

} // namespace gismo