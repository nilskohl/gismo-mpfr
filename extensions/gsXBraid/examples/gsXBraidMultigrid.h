#include <gismo.h>
#include <string>

namespace gismo {

/** @brief The p-multigrid base class provides the basic
 *  methods (smoothing, prolongation, restriction) for
 *  implementing p-multigrid methods
 */

template<class T>
struct gsXBraidMultigridBase
{

public:

  /// @brief Apply p-multigrid solver to given right-hand side on level l
  virtual void solve(const gsMatrix<T> & rhs,
                     std::vector<memory::shared_ptr<gsMultiBasis<T> > > m_basis,
                     gsMatrix<T>& x,
                     const int& numLevels,
                     const int& numCoarsening,
                     const int& numRefine,
                     const int& numSmoothing,
                     int& numCoarseCycles,
                     const int& typeCycle_p,
                     int& typeCycle_h,
                     const int& typeSolver,
                     const int& typeBCHandling,
                     gsBoundaryConditions<T> bcInfo,
                     gsMultiPatch<> mp,
                     gsGeometry<>::Ptr geo,
                     const int& typeLumping,
                     const int& typeProjection,
                     const int& typeSmoother,
                     std::vector<gsSparseMatrix<T> >& m_prolongation_P,
                     std::vector<gsSparseMatrix<T> >& m_restriction_P,
                     std::vector<gsMatrix<T> >& m_prolongation_M,
                     std::vector<gsMatrix<T> >& m_restriction_M,
                     std::vector<gsSparseMatrix<T> >& m_prolongation_H,
                     std::vector<gsSparseMatrix<T> >& m_restriction_H,
                     const gsMatrix<>& hp)
  {
    if( numLevels == 1)
      {
        solvecoarse(rhs, x, numLevels);
        return;
      }


    if(hp(std::max(numLevels-2,0),0) == 0 )
      {
        gsMatrix<T> fineRes, coarseRes, fineCorr, coarseCorr, postRes;
        presmoothing(rhs, x, numLevels, numSmoothing, fineRes, numRefine, typeSmoother,hp); 
        restriction(fineRes, coarseRes, numLevels, numCoarsening, m_basis, typeLumping,
                    typeBCHandling, bcInfo, mp, geo, typeProjection,
                    m_prolongation_P, m_restriction_P,
                    m_prolongation_M, m_restriction_M,
                    m_prolongation_H, m_restriction_H, hp);
        //coarseRes.setZero(coarseRes.rows(),1);
        coarseCorr.setZero(coarseRes.rows(),1);
        for( int j = 0 ; j < (typeCycle_p == 2 ? 2 : 1) ; j++)
          {   
            solve(coarseRes, m_basis, coarseCorr, numLevels-1, numCoarsening, numRefine,
                  numSmoothing, numCoarseCycles, typeCycle_p, typeCycle_h, typeSolver,
                  typeBCHandling, bcInfo, mp, geo, typeLumping, typeProjection, typeSmoother,
                  m_prolongation_P, m_restriction_P,
                  m_prolongation_M, m_restriction_M,
                  m_prolongation_H, m_restriction_H, hp);  
          }
        prolongation(coarseCorr, fineCorr, numLevels, numCoarsening, m_basis, typeLumping,
                     typeBCHandling, bcInfo, mp, geo, typeProjection,
                     m_prolongation_P, m_restriction_P,
                     m_prolongation_M, m_restriction_M,
                     m_prolongation_H, m_restriction_H, hp);
        postsmoothing(rhs, x, numLevels, numSmoothing, fineCorr, postRes, typeSolver,
                      numRefine, typeSmoother, hp);
      }
   
    if(hp(std::max(numLevels-2,0),0) == 1 )
      {
        gsMatrix<T> fineRes, coarseRes, fineCorr, coarseCorr, postRes;
        presmoothing(rhs, x, numLevels, numSmoothing, fineRes, numRefine, typeSmoother, hp); 
        restriction(fineRes, coarseRes, numLevels, numCoarsening, m_basis, typeLumping,
                    typeBCHandling, bcInfo, mp, geo, typeProjection,
                    m_prolongation_P, m_restriction_P,
                    m_prolongation_M, m_restriction_M,
                    m_prolongation_H, m_restriction_H, hp);
        //coarseRes.setZero(coarseRes.rows(),1);
        coarseCorr.setZero(coarseRes.rows(),1);
        for( int i = 0 ; i < (typeCycle_h == 2 ? 2 : 1) ; i++)
          {  
            solve(coarseRes, m_basis, coarseCorr, numLevels-1, numCoarsening, numRefine,
                  numSmoothing, numCoarseCycles, typeCycle_p, typeCycle_h, typeSolver,
                  typeBCHandling, bcInfo, mp, geo, typeLumping, typeProjection, typeSmoother,
                  m_prolongation_P, m_restriction_P,
                  m_prolongation_M, m_restriction_M,
                  m_prolongation_H, m_restriction_H, hp);  
          }   
        prolongation(coarseCorr, fineCorr, numLevels, numCoarsening, m_basis, typeLumping,
                     typeBCHandling, bcInfo, mp, geo, typeProjection,
                     m_prolongation_P, m_restriction_P,
                     m_prolongation_M, m_restriction_M,
                     m_prolongation_H, m_restriction_H,  hp);
        postsmoothing(rhs,x, numLevels, numSmoothing, fineCorr, postRes, typeSolver,
                      numRefine, typeSmoother, hp);
      }
  }

  /// @brief Setup p-multigrid to given linear system
  virtual void setup(const gsMatrix<T> & rhs,
                     std::vector<memory::shared_ptr<gsMultiBasis<T> > > m_basis,
                     gsMatrix<T>& x,
                     const int& numLevels,
                     const int& numCoarsening,
                     const int& numRefine,
                     const int& numSmoothing,
                     int& numCoarseCycles,
                     const int& typeCycle_p,
                     const int& typeCycle_h,
                     const int& typeSolver,
                     const int& typeBCHandling,
                     gsBoundaryConditions<T> bcInfo,
                     gsMultiPatch<> mp,
                     gsGeometry<>::Ptr geo,
                     const int& typeLumping,
                     const int& typeProjection,
                     const int& typeSmoother,
                     std::vector<gsSparseMatrix<T> >& m_prolongation_P,
                     std::vector<gsSparseMatrix<T> >& m_restriction_P,
                     std::vector<gsMatrix<T> >& m_prolongation_M,
                     std::vector<gsMatrix<T> >& m_restriction_M,
                     std::vector<gsSparseMatrix<T> >& m_prolongation_H,
                     std::vector<gsSparseMatrix<T> >& m_restriction_H,
                     const gsMatrix<>& hp) {}
  
  /// @brief Apply fixed number of smoothing steps (pure virtual method)
  virtual void presmoothing(const gsMatrix<T>& rhs,
                            gsMatrix<T>& x,
                            const int& numLevels,
                            const int& numSmoothing,
                            gsMatrix<T> & fineRes ,
                            const int& numRefine,
                            const int& typeSmoother,
                            const gsMatrix<>& hp) = 0;

  /// @brief Apply fixed number of smoothing steps (pure virtual method)
  virtual void postsmoothing(const gsMatrix<T>& rhs,
                             gsMatrix<T>& x,
                             const int& numLevels,
                             const int& numSmoothing,
                             gsMatrix<T> & fineCorr,
                             gsMatrix<T> & postRes,
                             const int& typeSolver,
                             const int& numRefine,
                             const int& typeSmoother,
                             const gsMatrix<>& hp) = 0;

  /// @brief Apply coarse solver (pure virtual method)
  virtual void solvecoarse(const gsMatrix<T>& rhs,
                           gsMatrix<T>& x,
                           const int& numLevels) = 0;
   
  /// @brief Prolongate coarse space function to fine space
  virtual gsSparseMatrix<T> prolongation_P(const int& numLevels,
                                           std::vector<memory::shared_ptr<gsMultiBasis<T> > > m_basis,
                                           const int& typeLumping,
                                           const int& typeBCHandling,
                                           gsGeometry<>::Ptr geo,
                                           const int& typeProjection) = 0;
   
  /// @brief Prolongate coarse space function to fine space
  virtual gsSparseMatrix<T> restriction_P(const int& numLevels,
                                          std::vector<memory::shared_ptr<gsMultiBasis<T> > > m_basis,
                                          const int& typeLumping,
                                          const int& typeBCHandling,
                                          gsGeometry<>::Ptr geo,
                                          const int& typeProjection) = 0;
  
  /// @brief Prolongate coarse space function to fine space
  virtual gsMatrix<T> prolongation_M(const int& numLevels,
                                     std::vector<memory::shared_ptr<gsMultiBasis<T> > > m_basis,
                                     const int& typeLumping,
                                     const int& typeBCHandling,
                                     gsGeometry<>::Ptr geo,
                                     const int& typeProjection) = 0;
   
  /// @brief Prolongate coarse space function to fine space
  virtual gsMatrix<T> restriction_M(const int& numLevels,
                                    std::vector<memory::shared_ptr<gsMultiBasis<T> > > m_basis,
                                    const int& typeLumping,
                                    const int& typeBCHandling,
                                    gsGeometry<>::Ptr geo,
                                    const int& typeProjection) = 0;
 
  /// @brief Prolongate coarse space function to fine space
  virtual void prolongation(const gsMatrix<T>& Xcoarse,
                            gsMatrix<T>& Xfine,
                            const int& numLevels,
                            const int& numCoarsening,
                            std::vector<memory::shared_ptr<gsMultiBasis<T> > > m_basis,
                            const int& typeLumping,
                            const int& typeBCHandling,
                            gsBoundaryConditions<T> bcInfo,
                            gsMultiPatch<> mp,
                            gsGeometry<>::Ptr geo,
                            const int& typeProjection,
                            std::vector<gsSparseMatrix<T> >& m_prolongation_P,
                            std::vector<gsSparseMatrix<T> >& m_restriction_P,
                            std::vector<gsMatrix<T> >& m_prolongation_M,
                            std::vector<gsMatrix<T> >& m_restriction_M,
                            std::vector<gsSparseMatrix<T> >& m_prolongation_H,
                            std::vector<gsSparseMatrix<T> >& m_restriction_H,
                            const gsMatrix<>& hp)
  {
    if(hp(numLevels-2,0) == 1)
      {
        Xfine = m_prolongation_H[numLevels-2]*Xcoarse;
      }
    else
      {
        if(typeLumping == 1)
          {
            gsVector<> temp = m_prolongation_P[numLevels-2]*Xcoarse;
            gsMatrix<> M_L_inv = (m_prolongation_M[numLevels-2]).array().inverse();
            Xfine = (M_L_inv).cwiseProduct(temp);
          }
        else
          {
            // Define the low and high order basis
            gsMultiBasis<> basisL = *m_basis[numLevels-2];
            gsMultiBasis<> basisH = *m_basis[numLevels-1];
            typedef gsExprAssembler<real_t>::geometryMap geometryMap;
            typedef gsExprAssembler<real_t>::variable variable;
            typedef gsExprAssembler<real_t>::space space;
      
            // Determine matrix M (high_order * high_order)
            gsExprAssembler<real_t> ex2(1,1);
            geometryMap G2 = ex2.getMap(mp);
            space w_n = ex2.getSpace(basisH ,1, 0);
            w_n.setInterfaceCont(0);
            if(typeBCHandling == 1)
              {
                w_n.addBc(bcInfo.get("Dirichlet"));
              }
            ex2.setIntegrationElements(basisH);
            ex2.initSystem();
            ex2.assemble(w_n * meas(G2) * w_n.tr()); 
        
            // Prolongate Xcoarse to Xfine
            gsVector<> temp = m_prolongation_P[numLevels-2]*Xcoarse;
            gsSparseMatrix<> M = ex2.matrix();  
            gsConjugateGradient<> CGSolver(M);
            CGSolver.setTolerance(1e-12);
            CGSolver.solve(temp,Xfine);        
          }
      }
  }

  /// @brief Restrict fine space function to coarse space
  virtual void restriction(const gsMatrix<T>& Xfine,
                           gsMatrix<T>& Xcoarse,
                           const int& numLevels,
                           const int& numCoarsening,
                           std::vector<memory::shared_ptr<gsMultiBasis<T> > > m_basis,
                           const int& typeLumping,
                           const int& typeBCHandling,
                           gsBoundaryConditions<T> bcInfo,
                           gsMultiPatch<> mp,
                           gsGeometry<>::Ptr geo,
                           const int& typeProjection,
                           std::vector<gsSparseMatrix<T> >& m_prolongation_P,
                           std::vector<gsSparseMatrix<T> >& m_restriction_P,
                           std::vector<gsMatrix<T> >& m_prolongation_M,
                           std::vector<gsMatrix<T> >& m_restriction_M,
                           std::vector<gsSparseMatrix<T> >& m_prolongation_H,
                           std::vector<gsSparseMatrix<T> >& m_restriction_H,
                           const gsMatrix<>& hp)
  {
    if(hp(numLevels-2,0) == 1)
      {
        Xcoarse = m_restriction_H[numLevels-2]*Xfine;
      }
    else
      {
        if(typeLumping == 1)
          {
            // Standard way
            gsVector<> temp = m_restriction_P[numLevels-2]*Xfine;
            gsMatrix<> M_L_inv = (m_restriction_M[numLevels-2]).array().inverse();
            Xcoarse = (M_L_inv).cwiseProduct(temp);
          }
        else
          {
            // Define the low and high order basis
            gsMultiBasis<> basisL = *m_basis[numLevels-2];
            gsMultiBasis<> basisH = *m_basis[numLevels-1];
            typedef gsExprAssembler<real_t>::geometryMap geometryMap;
            typedef gsExprAssembler<real_t>::variable variable;
            typedef gsExprAssembler<real_t>::space space;
      
            // Determine matrix M (low_order * low_order)
            gsExprAssembler<real_t> ex2(1,1);
            geometryMap G2 = ex2.getMap(mp);
            space w_n = ex2.getSpace(basisL, 1, 0);
            w_n.setInterfaceCont(0);
            if(typeBCHandling == 1)
              {
                w_n.addBc(bcInfo.get("Dirichlet"));
              }
            ex2.setIntegrationElements(basisL);
            ex2.initSystem();
            ex2.assemble(w_n * meas(G2) * w_n.tr());
        
            // Restrict Xfine to Xcoarse
            gsMatrix<> temp = m_restriction_P[numLevels-2]*Xfine;
            gsSparseMatrix<> M = ex2.matrix();  
            gsConjugateGradient<> CGSolver(M);
            CGSolver.setTolerance(1e-12);
            CGSolver.solve(temp, Xcoarse);      
          }
      }
  }
};

/** @brief The p-multigrid class implements a generic p-multigrid solver
 *  that can be customized by passing assembler and coarse
 *  solver as template arguments.
 *
 *  @note: This implementation assumes that all required prolongation/
 *  restriction operators are generated internally. Therefore, a
 *  problem-specific assembler has to be passed as template argument.
 */
template<class T, class CoarseSolver, class Assembler>
struct gsXBraidMultigrid : public gsXBraidMultigridBase<T>
{
private:

  /// Base class type
  typedef gsXBraidMultigridBase<T> Base;

  /// Shared pointer to multi-patch geometry
  memory::shared_ptr<gsMultiPatch<T> > m_mp_ptr;

  /// Shared pointer to boundary conditions
  memory::shared_ptr<gsBoundaryConditions<T> > m_bcInfo_ptr;
 
  /// std::vector of multi-basis objects
  std::vector<memory::shared_ptr<gsMultiBasis<T> > > m_basis;

  /// std::vector of prolongation operators
  std::vector< gsSparseMatrix<T> > m_prolongation_P;

  /// std::vector of restriction operators
  std::vector< gsSparseMatrix<T> > m_restriction_P;
  
  /// std::vector of prolongation operators
  std::vector< gsMatrix<T> > m_prolongation_M;

  /// std::vector of restriction operators
  std::vector< gsMatrix<T> > m_restriction_M;

  /// std::vector of prolongation operators
  std::vector< gsSparseMatrix<T> > m_prolongation_H;

  /// std::vector of restriction operators
  std::vector< gsSparseMatrix<T> > m_restriction_H;

  /// std::vector of factorized operators
  std::vector< std::vector< gsSparseMatrix<T> > > m_ILUT;

  /// std::vector of factorized operators
  std::vector< std::vector < Eigen::PermutationMatrix<Dynamic,Dynamic,index_t> > > m_P;
  
  /// std::vector of factorized operators
  std::vector < std::vector < Eigen::PermutationMatrix<Dynamic,Dynamic,index_t> > > m_Pinv;

  /// std::vector of SCM smoother object
  std::vector< gsPreconditionerOp<>::Ptr > m_SCMS;
  
  /// std::vector of operator objects
  std::vector< gsSparseMatrix<T> > m_operator;

  /// std::vector of std::vector of block operator objects
  std::vector < std::vector< gsSparseMatrix<T> > > m_block_operator;

  /// std::vector of std::vector of block operator objects
  std::vector < std::vector  < gsSparseMatrix<T> > > m_ddB;

  /// std::vector of std::vector of block operator objects
  std::vector < std::vector  < gsSparseMatrix<T> > > m_ddC;

  /// std::vector of std::vector of block operator objects
  std::vector < std::vector <  gsMatrix<T>  > > m_ddBtilde;

  /// std::vector of std::vector of block operator objects
  std::vector < std::vector <  gsMatrix<T>  > > m_ddCtilde;

  /// std::vector of std::vector of block operator objects
  std::vector <  gsMatrix<T> > m_A_aprox;

  /// std::vector of std::vector of block operator objects
  std::vector <  gsSparseMatrix<T> > m_S;
  
  /// std::vector of std::vector of shift objects
  std::vector < std::vector< int > > m_shift;

  /// std::vector of assembler objects
  std::vector<Assembler> m_assembler;

public:

  // Constructor
  gsXBraidMultigrid(const gsMultiPatch<T> & mp,
                    const gsMultiBasis<T> & basis,
                    const gsBoundaryConditions<T> & bcInfo)
  {
    m_mp_ptr = memory::make_shared_not_owned(&mp);
    m_bcInfo_ptr = memory::make_shared_not_owned(&bcInfo);
    m_basis.push_back(memory::make_shared_not_owned(&basis));
  }

public:

  ///  @brief Set-up p-multigrid solver 
  void setup(const gsFunctionExpr<T> & rhs,
             const gsFunctionExpr<T> & sol_exact,
             gsMatrix<T>& x,
             const int& numSmoothing,
             gsMatrix<T> f,
             const int& typeSolver,
             int& iterTot,
             int& typeCycle_p,
             int& typeCycle_h,
             int numLevels,
             const int& numCoarsening,
             const int& numDegree,
             const int& numRefine,
             const int& numBenchmark,
             const int& typeMultigrid,
             const int& typeBCHandling,
             gsGeometry<>::Ptr geo,
             const int& typeLumping,
             const gsMatrix<>& hp,
             const int& typeProjection,
             const int& typeSmoother,
             const int& typeCoarseOperator,
             const gsFunctionExpr<> coeff_diff,
             const gsFunctionExpr<> coeff_conv,
             const gsFunctionExpr<> coeff_reac)
  {
    for (int i = 1; i < numLevels; i++)
      {
        m_basis.push_back(give(m_basis.back()->clone()));
        switch((int) hp(i-1,0) )
          {
          case 0 : (typeProjection == 1 ?
                    m_basis.back()->degreeIncrease(numDegree-1) :
                    m_basis.back()->degreeIncrease()); break;

          case 1 : m_basis.back()->uniformRefine();  break;

          case 2:  m_basis.back()->uniformRefine();
            m_basis.back()->degreeIncrease(); break;
          }
      }
    
    // Generate sequence of assembler objects and assemble
    for (typename std::vector<memory::shared_ptr<gsMultiBasis<T> > >::iterator it = m_basis.begin();
         it != m_basis.end(); ++it)
      {
        m_assembler.push_back(Assembler(*m_mp_ptr,
                                        *(*it).get(),
                                        *m_bcInfo_ptr,
                                        rhs,
                                        coeff_diff,
                                        coeff_conv,
                                        coeff_reac,
                                        (typeBCHandling == 1 ?
                                         dirichlet::elimination :
                                         dirichlet::nitsche),
                                        iFace::glue));
      }
   
    // Resize vector of operators
    m_operator.resize(numLevels);
    m_prolongation_P.resize(numLevels-1);
    m_prolongation_M.resize(numLevels-1);
    m_prolongation_H.resize(numLevels-1);
    m_restriction_P.resize(numLevels-1);
    m_restriction_M.resize(numLevels-1);
    m_restriction_H.resize(numLevels-1);

    // Assemble operators at finest level
    gsStopwatch clock;
    gsInfo << "|| Multigrid hierarchy ||" <<std::endl;
    for (int i = 0; i < numLevels; i++)
      {
        gsInfo << "Level " << i+1 << " " ;
        if(typeCoarseOperator == 1)
          {
            m_assembler[i].assemble();
            m_operator[i] = m_assembler[i].matrix();
            gsInfo << "Degree: " << m_basis[i]->degree() << ", Ndof: " << m_basis[i]->totalSize() <<std::endl; 
          }
        else
          {
            if(hp(std::min(i,hp.rows()-1),0) == 0 || i == numLevels-1)
              {
                m_assembler[i].assemble();
                m_operator[i] = m_assembler[i].matrix();
                gsInfo << "\nDegree of the basis: " << m_basis[i]->degree() <<std::endl;
                gsInfo << "Size of the basis functions: " << m_basis[i]->totalSize() <<std::endl;
              }
          }
      }
    real_t Time_Assembly = clock.stop();
    
    // Determine prolongation/restriction operators in p
    clock.restart();
    for (int i = 1; i < numLevels; i++)
      {
        if(hp(i-1,0) == 0)
          {
            m_prolongation_P[i-1] =  prolongation_P(i+1, m_basis, typeLumping, typeBCHandling, geo, typeProjection);
            m_restriction_P[i-1] =  m_prolongation_P[i-1].transpose(); //restriction_P(i+1, m_basis, typeLumping, typeBCHandling, geo, typeProjection);
            m_prolongation_M[i-1] =  prolongation_M(i+1, m_basis, typeLumping, typeBCHandling, geo, typeProjection);
            m_restriction_M[i-1] = restriction_M(i+1, m_basis, typeLumping, typeBCHandling, geo, typeProjection);
          }
      }

    // Determine prolongation/restriction operators in h
    gsSparseMatrix<real_t, RowMajor> transferMatrix;    
    gsOptionList options;
    typeBCHandling == 1 ? options.addInt("DirichletStrategy","",dirichlet::elimination) : options.addInt("DirichletStrategy","",dirichlet::nitsche);
    for(int i = 1; i < numLevels; i++)
      {
        if(hp(i-1,0) == 1)
          {
            gsMultiBasis<T> m_basis_copy = *m_basis[i]; 
            m_basis_copy.uniformCoarsen_withTransfer(transferMatrix,*m_bcInfo_ptr,options); 
            m_prolongation_H[i-1] = transferMatrix;
            m_restriction_H[i-1] = m_prolongation_H[i-1].transpose();
          }  
      }
    real_t Time_Transfer = clock.stop();
    
    // Obtain operators with Galerkin projection
    clock.restart();
    if(typeCoarseOperator == 2)
      {
        for (int i = numLevels-1; i > -1; i--)
          {
            if(hp(hp.rows()-1,0) == 0)
              {
                if(hp(std::min(i,hp.rows()-1),0) == 1)
                  {
                    m_operator[i] = m_restriction_H[i]*m_operator[i+1]*m_prolongation_H[i];  
                  }
              }
            else
              {
                if(hp(std::min(i,hp.rows()-1),0) == 1 && i > 0)
                  {
                    m_operator[i-1] = m_restriction_H[i-1]*m_operator[i]*m_prolongation_H[i-1];    
                  }
              }
          }
      }
    real_t Time_Assembly_Galerkin = clock.stop();


    // Setting up the subspace corrected mass smoother
    clock.restart();
    if(typeSmoother == 3)
      {
        // Generate sequence of SCM smoothers
        m_SCMS.resize(numLevels);
        gsOptionList opt;
        opt.addReal("Scaling","",0.12);
        for(int i = 0 ; i < numLevels ; i++)
          {
            m_SCMS[i] = setupSubspaceCorrectedMassSmoother(m_operator[i], *m_basis[i], *m_bcInfo_ptr, opt, typeBCHandling);
          }
      }
    real_t Time_SCMS = clock.stop();

    // Determine ILUT factorizations at each level
    clock.restart();  
    int numPatch = m_mp_ptr->nPatches();
      
    if(typeSmoother == 1)
      {
        // Generate factorizations (ILUT)
        m_ILUT.resize(numLevels);
        m_P.resize(numLevels);
        m_Pinv.resize(numLevels);
        for(int i = 0; i < numLevels; i++)
          {
            m_ILUT[i].resize(1);
            m_P[i].resize(1);
            m_Pinv[i].resize(1);
            if(typeProjection == 2)
              {
                Eigen::IncompleteLUT<real_t> ilu;
                ilu.setFillfactor(1);
                ilu.compute(m_operator[i]);
                m_ILUT[i][0] = ilu.m_lu;
                m_P[i][0] = ilu.m_P;
                m_Pinv[i][0] = ilu.m_Pinv;
              }
            else
              {
                if(i == numLevels-1) // Only at finest level
                  {
                    Eigen::IncompleteLUT<real_t> ilu;
                    ilu.setFillfactor(1);
                    ilu.compute(m_operator[i]);
                    m_ILUT[i][0] = ilu.m_lu;
                    m_P[i][0] = ilu.m_P;
                    m_Pinv[i][0] = ilu.m_Pinv;
                  }
              } 
          }
      } 
    real_t Time_ILUT_Factorization = clock.stop();    
    clock.restart();   
    if(typeSmoother == 5)
      {
        int shift0 = 0;
        m_ddB.resize(numLevels);
        m_ddC.resize(numLevels);
        m_ddBtilde.resize(numLevels);
        m_ddCtilde.resize(numLevels);

        m_ILUT.resize(numLevels);
        m_P.resize(numLevels);
        m_Pinv.resize(numLevels);
        m_shift.resize(numLevels);
        m_S.resize(numLevels);
        
        for(int i = 0 ; i < numLevels ; i++)
          {
            m_shift[i].resize(numPatch+1);
            m_ILUT[i].resize(numPatch+1);
            m_P[i].resize(numPatch+1);
            m_Pinv[i].resize(numPatch+1);
         
            // Use of partition functions
            std::vector<gsVector<index_t> > interior, boundary;
            std::vector<std::vector<gsVector<index_t> > > interface;
            std::vector<gsMatrix<index_t> >  global_interior, global_boundary;
            std::vector<std::vector<gsMatrix<index_t> > > global_interface;
            //m_basis[i]->partition(interior,boundary,interface,global_interior,global_boundary,global_interface);
            for(int l=0; l< numPatch; l++)
              {
                m_shift[i][l] = global_interior[l].rows();
              }
            m_shift[i][numPatch] = 0; 
            m_shift[i][numPatch] = m_operator[i].rows() - accumulate(m_shift[i].begin(),m_shift[i].end(),0);

            // Put shift on zero
            shift0 = 0;
            for(int j = 0 ; j < numPatch ; j++)
              {
                const gsSparseMatrix<> block = m_operator[i].block(shift0,shift0,m_shift[i][j],m_shift[i][j]);
                Eigen::IncompleteLUT<real_t> ilu;
                ilu.setFillfactor(1);
                ilu.compute(block);
                m_ILUT[i][j] = ilu.m_lu;

                m_P[i][j] = ilu.m_P;
                m_Pinv[i][j] = ilu.m_Pinv;
                shift0 = shift0 + m_shift[i][j]; 
        
              } 

            shift0 = 0;
            // Obtain the blocks of the matrix
            m_ddB[i].resize(numPatch+1);
            m_ddC[i].resize(numPatch+1);
        
            for(int j = 0 ; j < numPatch+1 ; j++)
              {
                m_ddB[i][j] = m_operator[i].block(m_operator[i].rows()-m_shift[i][numPatch],shift0,m_shift[i][numPatch],m_shift[i][j]);
                m_ddC[i][j] = m_operator[i].block(shift0,m_operator[i].cols()-m_shift[i][numPatch],m_shift[i][j],m_shift[i][numPatch]);
                shift0 = shift0 + m_shift[i][j];
              }     
            shift0 = 0;
          }

        m_A_aprox.resize(numLevels);
        for(int i = 0 ; i < numLevels ; i++)
          {
            // Define the A_aprox matrix
            m_A_aprox[i] = gsSparseMatrix<>(m_operator[i].rows(),m_operator[i].cols());

            // Retrieve a block of each patch
            for(int k=0; k< numPatch; k++)
              {
                m_A_aprox[i].block(shift0,shift0,m_shift[i][k],m_shift[i][k]) = m_ILUT[i][k];  
                shift0 = shift0 + m_shift[i][k];
              }
            shift0 = 0; 
            m_ddBtilde[i].resize(numPatch);
            m_ddCtilde[i].resize(numPatch);

            for(int j=0 ; j < numPatch ; j ++)
              {
                m_ddBtilde[i][j] = gsSparseMatrix<>(m_shift[i][j],m_shift[i][numPatch]);
                m_ddCtilde[i][j] = gsSparseMatrix<>(m_shift[i][j],m_shift[i][numPatch]);
                for(int k=0 ; k < m_shift[i][numPatch]; k++)
                  {
                    gsMatrix<> Brhs = m_ddC[i][j].col(k);
                    gsMatrix<> Crhs = m_ddC[i][j].col(k);
                    m_ddBtilde[i][j].col(k) = m_ILUT[i][j].template triangularView<Eigen::Upper>().transpose().solve(Brhs);
                    m_ddCtilde[i][j].col(k) = m_ILUT[i][j].template triangularView<Eigen::UnitLower>().solve(Crhs);
                  }   
              }

            // Define matrix S
            m_S[i] = m_ddC[i][numPatch];
            for(int l = 0 ; l < numPatch ; l++)
              {
                m_S[i] = m_S[i] - m_ddBtilde[i][l].transpose()*m_ddCtilde[i][l];
              }  
        
            // Fill matrix A_aprox
            for(int m = 0 ; m < numPatch ; m++)
              {
                m_A_aprox[i].block(shift0,m_A_aprox[i].rows() - m_shift[i][numPatch],m_shift[i][m],m_shift[i][numPatch]) = m_ddCtilde[i][m];  
                m_A_aprox[i].block(m_A_aprox[i].rows() - m_shift[i][numPatch],shift0,m_shift[i][numPatch],m_shift[i][m]) = m_ddBtilde[i][m].transpose();      
                shift0 = shift0 + m_shift[i][m];
              }
            shift0 = 0;  
        
            // Preform ILUT on the S-matrix!
            Eigen::IncompleteLUT<real_t> ilu;
            ilu.setFillfactor(1);
            gsSparseMatrix<> II = m_S[i];
            ilu.compute(II);
            m_A_aprox[i].block(m_A_aprox[i].rows() - m_shift[i][numPatch],m_A_aprox[i].rows() - m_shift[i][numPatch],m_shift[i][numPatch],m_shift[i][numPatch]) = ilu.m_lu;
          }
      }
      
    real_t Time_Block_ILUT_Factorization = clock.stop();
    gsInfo << "\n|| Setup Timings || " <<std::endl;
    gsInfo << "Total Assembly time: " << Time_Assembly <<std::endl;
    gsInfo << "Total ILUT factorization time: " << Time_ILUT_Factorization <<std::endl;
    gsInfo << "Total block ILUT factorization time: " << Time_Block_ILUT_Factorization <<std::endl;
    gsInfo << "Total SCMS time: " << Time_SCMS <<std::endl;
    gsInfo << "Total setup time: " << Time_Assembly_Galerkin + Time_Assembly + Time_Transfer + Time_ILUT_Factorization + Time_SCMS <<std::endl;
  }

  ///  @brief Apply p-multigrid solver to given right-hand side on level l
  void solve(const gsFunctionExpr<T> & rhs,
             const gsFunctionExpr<T> & sol_exact,
             gsMatrix<T>& x,
             const int& numSmoothing,
             gsMatrix<T> f,
             const int& typeSolver,
             int& iterTot,
             int& typeCycle_p,
             int& typeCycle_h,
             int numLevels,
             const int& numCoarsening,
             const int& numDegree,
             const int& numRefine,
             const int& numBenchmark,
             const int& typeMultigrid,
             const int& typeBCHandling,
             gsGeometry<>::Ptr geo,
             const int& typeLumping,
             const gsMatrix<>& hp,
             const int& typeProjection,
             const int& typeSmoother,
             const int& typeCoarseOperator)
  {
    gsStopwatch clock;
    
    if(typeSolver == 1)
      {
        x = gsMatrix<>::Random(m_operator[numLevels-1].rows(),1);
      }
     
    gsMatrix<> b;    
    typeSolver == 1 ? b = m_assembler.back().rhs() : b = f;


    // Determine residual and L2 error
    real_t r0 = (m_operator[numLevels-1]*x - b).norm();
    real_t r = r0;
    real_t tol = 1e-8;
    int iter = 1;
    int numCoarseCycles = 0;

    // Solve with p-multigrid method 
    real_t r_old = r0;
    clock.restart();
    while( (typeSolver == 1 || typeSolver == 5) ? r/r0 > tol && iter < 100000 : iter < 2)
      {
        // Call solver from base class
        Base::solve(b, m_basis, x, numLevels, numCoarsening, numRefine, numSmoothing, numCoarseCycles,
                    typeCycle_p, typeCycle_h, typeSolver, typeBCHandling, *m_bcInfo_ptr, *m_mp_ptr, geo,
                    typeLumping, typeProjection, typeSmoother,
                    m_prolongation_P, m_restriction_P,
                    m_prolongation_M, m_restriction_M,
                    m_prolongation_H, m_restriction_H, hp);
        numCoarseCycles = 0;
        r = (m_operator[numLevels-1]*x - b).norm();
        if( r_old < r)
          {
            gsInfo << "Residual increased during solving!!! " <<std::endl;
          }
        r_old = r;
        //gsInfo << "Residual after cycle " << iter << " equals: " << r <<std::endl;
        iter++;
        iterTot++;
      }
    real_t Time_Solve = clock.stop();
    gsInfo << "\n|| Solver information || " <<std::endl;
    gsInfo << "Solver converged in " << Time_Solve << " seconds!" <<std::endl;
    gsInfo << "Solver converged in: " << iter-1 << " iterations!" <<std::endl;
           
    if(typeSolver == 1)
      {
        // // Determine residual and L2 errpr
        // gsField<> solMG = m_assembler.back().constructSolution(x);
        // gsNormL2<real_t> L2Norm(solMG,sol_exact);
        // real_t errorL2 = L2Norm.compute();
        // gsInfo << "Residual after solving: "  << r <<std::endl;
        // gsInfo << "Residual after solving: "  << (b-m_operator[numLevels-1]*x).norm() <<std::endl;
        // gsInfo << "L2 error: " << errorL2 <<std::endl; 

        // // Plot solution in Paraview
        // gsInfo << "Plotting in Paraview...\n";
        // gsWriteParaview<>(solMG, "Multigrid_solution", 100*x.rows());
        // gsField<> Exact( *m_mp_ptr, sol_exact, false );
        // gsWriteParaview<>( Exact, "Exact_solution", 100*x.rows());          
      }
  }

private:

  /// @brief Apply coarse solver
  virtual void solvecoarse(const gsMatrix<T>& rhs,
                           gsMatrix<T>& x,
                           const int& numLevels)
  {
    gsInfo << "Coarse solver is applied! " <<std::endl;

    // Direct solver (LU factorization)
    CoarseSolver solver;
    solver.analyzePattern(m_operator[0]);
    solver.factorize(m_operator[0]);
    x = solver.solve(rhs); 
  }
  
  /// @brief Construct prolongation operator at level numLevels
  virtual gsMatrix<T> prolongation_M(const int& numLevels,
                                     std::vector<memory::shared_ptr<gsMultiBasis<T> > > m_basis,
                                     const int& typeLumping,
                                     const int& typeBCHandling,
                                     gsGeometry<>::Ptr geo,
                                     const int& typeProjection)
  {
    // Define the low and high order basis
    gsMultiBasis<> basisL = *m_basis[numLevels-2];
    gsMultiBasis<> basisH = *m_basis[numLevels-1];

    // Determine matrix M (high_order * high_order)
    typedef gsExprAssembler<real_t>::geometryMap geometryMap;
    typedef gsExprAssembler<real_t>::variable variable;
    typedef gsExprAssembler<real_t>::space    space;
    gsExprAssembler<real_t> ex2(1,1);
    geometryMap G2 = ex2.getMap(*m_mp_ptr);
    space w_n = ex2.getSpace(basisH ,1, 0);
    w_n.setInterfaceCont(0);
    if(typeBCHandling == 1)
      {
        w_n.addBc(m_bcInfo_ptr->get("Dirichlet"));
      }
    ex2.setIntegrationElements(basisH);
    ex2.initSystem();
    ex2.assemble(w_n * meas(G2) );   
    return ex2.rhs();
  }

  /// @brief Construct prolongation operator at level numLevels
  virtual gsSparseMatrix<T> prolongation_P(const int& numLevels,
                                           std::vector<memory::shared_ptr<gsMultiBasis<T> > > m_basis,
                                           const int& typeLumping,
                                           const int& typeBCHandling,
                                           gsGeometry<>::Ptr geo,
                                           const int& typeProjection)
  {
    // Define the low and high order basis
    gsMultiBasis<> basisL = *m_basis[numLevels-2];
    gsMultiBasis<> basisH = *m_basis[numLevels-1];

    // Determine matrix P (high_order * low_order)
    typedef gsExprAssembler<real_t>::geometryMap geometryMap;
    gsExprAssembler<real_t> ex(1,1);
    geometryMap G = ex.getMap(*m_mp_ptr);
    typedef gsExprAssembler<real_t>::variable variable;
    typedef gsExprAssembler<real_t>::space    space;
    space v_n = ex.getSpace(basisH ,1, 0);
    v_n.setInterfaceCont(0);
    space u_n = ex.getTestSpace(v_n , basisL);
    u_n.setInterfaceCont(0);
    if(typeBCHandling == 1)
      {
        v_n.addBc(m_bcInfo_ptr->get("Dirichlet"));
        u_n.addBc(m_bcInfo_ptr->get("Dirichlet"));
      }
    ex.setIntegrationElements(basisH);
    ex.initSystem();
    ex.assemble(u_n*meas(G) * v_n.tr()); 
    gsSparseMatrix<> P = ex.matrix().transpose();
    return P;    
  }

  /// @brief Construct restriction operator at level numLevels
  virtual gsMatrix<T> restriction_M(const int& numLevels,
                                    std::vector<memory::shared_ptr<gsMultiBasis<T> > > m_basis,
                                    const int& typeLumping,
                                    const int& typeBCHandling,
                                    gsGeometry<>::Ptr geo,
                                    const int& typeProjection)
  {
    // Define the low and high order basis
    gsMultiBasis<> basisL = *m_basis[numLevels-2];
    gsMultiBasis<> basisH = *m_basis[numLevels-1];
      
    // Determine matrix M (low_order * low_order)
    typedef gsExprAssembler<real_t>::geometryMap geometryMap;
    typedef gsExprAssembler<real_t>::variable variable;
    typedef gsExprAssembler<real_t>::space    space;
    gsExprAssembler<real_t> ex2(1,1);
    geometryMap G2 = ex2.getMap(*m_mp_ptr);
    space w_n = ex2.getSpace(basisL ,1, 0);
    w_n.setInterfaceCont(0);
    if(typeBCHandling == 1)
      {
        w_n.addBc(m_bcInfo_ptr->get("Dirichlet"));
      }
    ex2.setIntegrationElements(basisL);
    ex2.initSystem();
    ex2.assemble(w_n * meas(G2) ); 
    return ex2.rhs();     
  }

  /// @brief Construct restriction operator at level numLevels
  virtual gsSparseMatrix<T> restriction_P(const int& numLevels,
                                          std::vector<memory::shared_ptr<gsMultiBasis<T> > > m_basis,
                                          const int& typeLumping,
                                          const int& typeBCHandling,
                                          gsGeometry<>::Ptr geo,
                                          const int& typeProjection)
  {
    // Define the low and high order basis
    gsMultiBasis<> basisL = *m_basis[numLevels-2];
    gsMultiBasis<> basisH = *m_basis[numLevels-1];
      
    // Determine matrix P (high_order * low_order)
    gsExprAssembler<real_t> ex(1,1);
    typedef gsExprAssembler<real_t>::geometryMap geometryMap;
    geometryMap G = ex.getMap(*m_mp_ptr);
      
    typedef gsExprAssembler<real_t>::variable variable;
    typedef gsExprAssembler<real_t>::space    space;
    space v_n = ex.getSpace(basisH ,1, 0);
    v_n.setInterfaceCont(0);
    space u_n = ex.getTestSpace(v_n , basisL);
    u_n.setInterfaceCont(0);
    if( typeBCHandling == 1)
      {
        u_n.addBc(m_bcInfo_ptr->get("Dirichlet"));
        v_n.addBc(m_bcInfo_ptr->get("Dirichlet"));
      }
    ex.setIntegrationElements(basisH);
    ex.initSystem();
    ex.assemble(u_n * meas(G)* v_n.tr());
    gsSparseMatrix<> P = ex.matrix();
    return P;   
  }
  
  /// @brief Apply fixed number of presmoothing steps
  virtual void presmoothing(const gsMatrix<T>& rhs,
                            gsMatrix<T>& x,
                            const int& numLevels,
                            const int& numSmoothing,
                            gsMatrix<T> & fineRes,
                            const int& numRefine,
                            const int& typeSmoother,
                            const gsMatrix<>& hp)
  { 
    gsInfo << "Residual before presmoothing: " << (rhs-m_operator[numLevels-1]*x).norm() << " at level " << numLevels <<std::endl;
    for(int i = 0 ; i < numSmoothing ; i++)
      {
        if(typeSmoother == 1)
          {
            if(hp(numLevels-2,0) == 1 && hp(hp.rows()-1,0) == 0)
              {
                internal::gaussSeidelSweep(m_operator[numLevels-1],x,rhs);
              }
            else
              {   
                gsMatrix<> e; 
                gsMatrix<> d = rhs-m_operator[numLevels-1]*x;
                e = m_Pinv[numLevels-1][0]*d;
                e = m_ILUT[numLevels-1][0].template triangularView<Eigen::UnitLower>().solve(e);
                e = m_ILUT[numLevels-1][0].template triangularView<Eigen::Upper>().solve(e);
                e = m_P[numLevels-1][0]*e;
                x = x + e; 
              }     
          }
        if(typeSmoother == 2)
          {
            internal::gaussSeidelSweep(m_operator[numLevels-1],x,rhs); 
          }
        if(typeSmoother == 3)
          {  
            m_SCMS[numLevels-1]->step(rhs,x);
          }
        if(typeSmoother == 5)
          {
            if(hp(numLevels-2,0) == 1 && hp(hp.rows()-1,0) == 0)
              {
                internal::gaussSeidelSweep(m_operator[numLevels-1],x,rhs);
              }
            else
              {
                gsMatrix<> e; 
                gsMatrix<> d = rhs-m_operator[numLevels-1]*x;
                e = m_A_aprox[numLevels-1].template triangularView<Eigen::UnitLower>().solve(d);
                e = m_A_aprox[numLevels-1].template triangularView<Eigen::Upper>().solve(e);
                x = x + e;   
              }     
          }
      }          
    //   gsInfo << "Residual after presmoothing: " << (rhs-m_operator[numLevels-1]*x).norm() << " at level " << numLevels <<std::endl;
    fineRes = m_operator[numLevels-1]*x - rhs;
  }

  /// @brief Apply fixed number of postsmoothing steps
  virtual void postsmoothing(const gsMatrix<T>& rhs,
                             gsMatrix<T>& x,
                             const int& numLevels,
                             const int& numSmoothing,
                             gsMatrix<T> & fineCorr,
                             gsMatrix<T> & postRes,
                             const int& typeSolver,
                             const int& numRefine,
                             const int& typeSmoother,
                             const gsMatrix<>& hp)
  {
    real_t alpha = 1;
    x = x - alpha*fineCorr;
    gsInfo << "Residual before postsmoothing: " << (rhs-m_operator[numLevels-1]*x).norm() << " at level " << numLevels <<std::endl;

    for(int i = 0 ; i < numSmoothing ; i++)
      {
        if(typeSmoother == 1)
          {
            if(hp(numLevels-2,0) == 1 && hp(hp.rows()-1,0) == 0)
              { 
                ( typeSolver == 3 ? internal::reverseGaussSeidelSweep(m_operator[numLevels-1],x,rhs) : internal::gaussSeidelSweep(m_operator[numLevels-1],x,rhs)); 
              }
            else
              { 
                gsMatrix<> e; 
                gsMatrix<> d = rhs-m_operator[numLevels-1]*x;
                e = m_Pinv[numLevels-1][0]*d;
                e = m_ILUT[numLevels-1][0].template triangularView<Eigen::UnitLower>().solve(e);
                e = m_ILUT[numLevels-1][0].template triangularView<Eigen::Upper>().solve(e);
                e = m_P[numLevels-1][0]*e;
                x = x + e;
              }
          }
        if(typeSmoother == 2)
          {
            ( typeSolver == 3 ? internal::reverseGaussSeidelSweep(m_operator[numLevels-1],x,rhs) : internal::gaussSeidelSweep(m_operator[numLevels-1],x,rhs));
          }
        if(typeSmoother == 3)
          {
            m_SCMS[numLevels-1]->step(rhs,x);
          }
        if(typeSmoother == 5)
          {
            if(hp(numLevels-2,0) == 1 && hp(hp.rows()-1,0) == 0)
              { 
                ( typeSolver == 3 ? internal::reverseGaussSeidelSweep(m_operator[numLevels-1],x,rhs) : internal::gaussSeidelSweep(m_operator[numLevels-1],x,rhs)); 
              }
            else
              { 
                gsMatrix<> e; 
                gsMatrix<> d = rhs-m_operator[numLevels-1]*x;
                e = m_A_aprox[numLevels-1].template triangularView<Eigen::UnitLower>().solve(d);
                e = m_A_aprox[numLevels-1].template triangularView<Eigen::Upper>().solve(e);
                x = x + e;    
              }
          }  
        postRes = rhs - m_operator[numLevels-1]*x;        
        //      gsInfo << "Residual after postsmoothing: " << (rhs-m_operator[numLevels-1]*x).norm() << " at level " << numLevels <<std::endl;
      }
  }
};

/** @brief The p-multigrid class implements a generic p-multigrid solver
 *  that can be customized by passing assembler and coarse
 *  solver as template arguments.
 *
 *  @note: This implementation assumes that all required prolongation/
 *  restriction operators are generated externally and provided as
 *  constant references through the constructor. Therefore, no assembler
 *  is passed as template parameter.
 */
template<class T, class CoarseSolver>
struct gsXBraidMultigrid<T, CoarseSolver, void> : public gsXBraidMultigridBase<T>
{
  // Default constructor
  gsXBraidMultigrid()
  {
    gsInfo << "The specific case";
  }
};

} // namespace gismo