/** @file gsVisitorBiharmonic.h

    @brief Visitor for a simple Biharmonic equation.

    This file is part of the G+Smo library.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

    Author(s): J. Sogn
*/

#pragma once

# include <gsG1Basis/gsG1MultiBasis.h>

namespace gismo
{

/** \brief Visitor for the biharmonic equation.
 *
 * Assembles the bilinear terms
 * \f[ (\Delta u,\Delta v)_\Omega \text{ and } (f,v)_\Omega \f]
 * For \f[ u = g \quad on \quad \partial \Omega \f],
 *
 */

    template <class T>
    class gsVisitorBiharmonicArgyrisDirect
    {
    public:

        gsVisitorBiharmonicArgyrisDirect(const gsPde<T> & pde)
        {
            rhs_ptr = static_cast<const gsBiharmonicPde<T>&>(pde).rhs() ;
        }

        /** \brief Constructor for gsVisitorBiharmonic.
         *
         * \param[in] rhs Given right-hand-side function/source term that, for
         */
        gsVisitorBiharmonicArgyrisDirect(const gsFunction<T> & rhs) :
                rhs_ptr(&rhs)
        {
            GISMO_ASSERT( rhs.targetDim() == 1 ,"Not yet tested for multiple right-hand-sides");
        }

        void initialize(const gsBasis<T> & basis,
                        gsQuadRule<T>    & rule)
        {
            gsVector<index_t> numQuadNodes( basis.dim() );
            for (int i = 0; i < basis.dim(); ++i) // to do: improve
                numQuadNodes[i] = basis.degree(i) + 1;

            // Setup Quadrature
            rule = gsGaussRule<T>(numQuadNodes);// NB!

            // Set Geometry evaluation flags
            md.flags = NEED_VALUE | NEED_MEASURE | NEED_GRAD_TRANSFORM | NEED_2ND_DER;
        }

        void initialize(const gsBasis<T> & basis,
                        const index_t ,
                        const gsOptionList & options,
                        gsQuadRule<T>    & rule)
        {
            // Setup Quadrature
            rule = gsQuadrature::get(basis, options);

            // Set Geometry evaluation flags
            md.flags = NEED_VALUE | NEED_MEASURE | NEED_GRAD_TRANSFORM | NEED_2ND_DER;
        }

        // Evaluate on element.
        inline void evaluate(const gsBasis<T>       & basis, // to do: more unknowns
                             gsG1MultiBasis<T> & g1MultiBasis,
                             const gsGeometry<T>    & geo,
                             gsMatrix<T>            & quNodes)
        {
            md.points = quNodes;
            // Compute the active basis functions
            // Assumes actives are the same for all quadrature points on the elements
            basis.active_into(md.points.col(0), actives);
            numActive = actives.rows();

            //deriv2_into()
            //col(point) = B1_xx B2_yy B1_zz B_xy B1_xz B1_xy B2_xx ...

            gsStopwatch clock;
            clock.restart();
            // Evaluate basis functions on element
            basis.evalAllDers_into(md.points, 2, basisData);
            //gsInfo << "Time Basis: " << clock.stop() << "\n";

            // Compute image of Gauss nodes under geometry mapping as well as Jacobians
            geo.computeMap(md);

            // Evaluate right-hand side at the geometry points
            rhs_ptr->eval_into(md.values[0], rhsVals); // Dim: 1 X NumPts

            // Pascal
            if (g1MultiBasis.nPatches() == 2) // TODO For now: only for two Patch domains
            {
                numg1Active = 0;
                g1MultiBasis.active_into(md.points.col(0), g1actives, geo.id());
                numg1Active = g1actives.rows();
                if (g1actives.rows() > 0)
                {
                    clock.restart();
                    g1MultiBasis.evalAllDers_into(md.points, 2, g1basisData, geo.id());

                    basisData[0].conservativeResize(basisData[0].rows() + g1basisData[0].rows(), basisData[0].cols());
                    basisData[0].bottomRows(g1basisData[0].rows()) = g1basisData[0];

                    basisData[1].conservativeResize(basisData[1].rows() + g1basisData[1].rows(), basisData[1].cols());
                    basisData[1].bottomRows(g1basisData[1].rows()) = g1basisData[1];

                    basisData[2].conservativeResize(basisData[2].rows() + g1basisData[2].rows(), basisData[2].cols());
                    basisData[2].bottomRows(g1basisData[2].rows()) = g1basisData[2];

                    numActive += numg1Active;
                    actives.conservativeResize(actives.rows() + g1actives.rows(), actives.cols());
                    actives.bottomRows(g1actives.rows()) = g1actives;
                    //gsInfo << "Time G1 Basis: " << clock.stop() << "\n";
                }
            }
            // Initialize local matrix/rhs
            localMat.setZero(numActive, numActive);
            localRhs.setZero(numActive, rhsVals.rows());//multiple right-hand sides
        }


        inline void assemble(gsDomainIterator<T>    &,
                             const gsVector<T>      & quWeights)
        {
            gsMatrix<T> & basisVals  = basisData[0];
            gsMatrix<T> & basisGrads = basisData[1];
            gsMatrix<T> & basis2ndDerivs = basisData[2];

            //gsInfo << "Basisvals: " << basisVals.col(0) << "\n";

            //gsInfo << "basisGrads: " << basisGrads.col(0) << "\n";

            //gsInfo << "basis2ndDerivs: " << basis2ndDerivs.col(0) << "\n";

            for (index_t k = 0; k < quWeights.rows(); ++k) // loop over quadrature nodes
            {
                // Multiply weight by the geometry measure
                const T weight = quWeights[k] * md.measure(k);

                // Compute physical laplacian at k as a 1 x numActive matrix
                transformLaplaceHgrad(md, k, basisGrads, basis2ndDerivs, physBasisLaplace);

                // (\Delta u, \Delta v)
                localMat.noalias() += weight * (physBasisLaplace.transpose() * physBasisLaplace);

                localRhs.noalias() += weight * ( basisVals.col(k) * rhsVals.col(k).transpose() ) ;
            }
        }

        inline void localToGlobal(const index_t                     patchIndex,
                                  const std::vector<gsMatrix<T> > & eliminatedDofs,
                                  gsSparseSystem<T>               & system)
        {
            // Map patch-local DoFs to global DoFs
            system.mapColIndices(actives, patchIndex, actives);

            // Add contributions to the system matrix and right-hand side
            system.pushWithTagged(localMat, localRhs, actives, eliminatedDofs[0], 0, 0); // TODO modify!!!
        }

        /*
        inline void localToGlobal(const gsDofMapper & mapper,
                                  const gsMatrix<T> & eliminatedDofs,
                                  const index_t       patchIndex,
                                  gsSparseMatrix<T> & sysMatrix,
                                  gsMatrix<T>       & rhsMatrix )
        {
            // Local Dofs to global dofs
            mapper.localToGlobal(actives, patchIndex, actives);
            //const int numActive = actives.rows();

            for (index_t i=0; i < numActive; ++i)
            {
                const int ii = actives(i);
                if ( mapper.is_free_index(ii) )
                {
                    rhsMatrix.row(ii) += localRhs.row(i);

                    for (index_t j=0; j < numActive; ++j)
                    {
                        const int jj = actives(j);
                        if ( mapper.is_free_index(jj) )
                        {
                            sysMatrix.coeffRef(ii, jj) += localMat(i, j);
                        }
                        else
                        {
                            rhsMatrix.row(ii).noalias() -= localMat(i, j) *
                                eliminatedDofs.row( mapper.global_to_bindex(jj) );
                        }
                    }
                }
            }
        }
        */


    protected:
        // Right hand side
        const gsFunction<T> * rhs_ptr;

    protected:
        // Basis values
        std::vector<gsMatrix<T> > basisData;
        gsMatrix<T>        physBasisLaplace;
        gsMatrix<index_t> actives;
        index_t numActive;


    protected:
        // Local values of the right hand side
        gsMatrix<T> rhsVals;

    protected:
        // Local matrices
        gsMatrix<T> localMat;
        gsMatrix<T> localRhs;

        gsMapData<T> md;

// Pascal
    protected:
        // Basis values
        std::vector<gsMatrix<T> > g1basisData;
        gsMatrix<T>        physG1BasisLaplace;
        gsMatrix<index_t> g1actives;
        index_t numg1Active;
    };


} // namespace gismo
