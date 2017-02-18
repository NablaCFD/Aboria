/*

Copyright (c) 2005-2016, University of Oxford.
All rights reserved.

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Aboria.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of the University of Oxford nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/


#ifndef OPERATORSTEST_H_
#define OPERATORSTEST_H_


#include <cxxtest/TestSuite.h>

#include "Aboria.h"

using namespace Aboria;


class OperatorsTest : public CxxTest::TestSuite {
public:

    void test_Eigen(void) {
#ifdef HAVE_EIGEN
        ABORIA_VARIABLE(scalar1,double,"scalar1")
        ABORIA_VARIABLE(scalar2,double,"scalar2")

    	typedef Particles<std::tuple<scalar1,scalar2>> ParticlesType;
        typedef position_d<3> position;
       	ParticlesType particles;

       	double diameter = 0.1;
        double3 min(-1);
        double3 max(1);
        double3 periodic(false);
        
        double s_init1 = 1.0;
        double s_init2 = 2.0;
        ParticlesType::value_type p;
        get<position>(p) = double3(0,0,0);
        get<scalar1>(p) = s_init1;
        get<scalar2>(p) = s_init2;
       	particles.push_back(p);
        get<position>(p) = double3(diameter*0.9,0,0);
       	particles.push_back(p);
        get<position>(p) = double3(diameter*1.8,0,0);
       	particles.push_back(p);

        const size_t n = 3;

        particles.init_neighbour_search(min,max,diameter,periodic);

        auto A = create_dense_operator(particles,particles,
                    [](const position::value_type &dx,
                       ParticlesType::const_reference a,
                       ParticlesType::const_reference b) {
                    return get<scalar1>(a) + get<scalar2>(b);
                    });

        Eigen::VectorXd v(3);
        v << 1, 2, 3;
        Eigen::VectorXd ans(3);
        ans = A*v;
        for (int i=0; i<n; i++) {
            TS_ASSERT_EQUALS(ans[i],(s_init1+s_init2)*v.sum()); 
        }
        v << 0, 2, 1;
        ans = A*v;
        for (int i=0; i<n; i++) {
            TS_ASSERT_EQUALS(ans[i],(s_init1+s_init2)*v.sum()); 
        }


        /*
        auto C = create_sparse_operator(particles,particles,
                    diameter,
                    [](ParticlesType::const_reference &a,
                       ParticlesType::const_reference &b) {
                    get<scalar1>(a) + get<scalar2>(b)
                    }));

        v << 1, 2, 3;
        ans = C*v;
        
        for (int i=0; i<n; i++) {
            double sum = 0;
            for (int j=0; j<n; j++) {
                if ((get<id>(particles[i]) == 0) && (get<id>(particles[j]) == 2)) {
                    sum += 0;
                } else if ((get<id>(particles[i]) == 2) && (get<id>(particles[j]) == 0)) {
                    sum += 0;
                } else {
                    sum += (s_init1+s_init2)*v[j];
                }
            }
            TS_ASSERT_EQUALS(ans[i],sum); 
        }

        Eigen::MatrixXd C_copy(n,n);
        Eigen::VectorXd ans_copy(n);
        C.assemble(C_copy);
        for (int i=0; i<n; i++) {
            for (int j=0; j<n; j++) {
                TS_ASSERT_EQUALS(C_copy(i,j),C.coeff(i,j)); 
            }
        }

        ans_copy = C_copy*v;
        
        for (int i=0; i<n; i++) {
            TS_ASSERT_EQUALS(ans[i],ans_copy[i]); 
        }

        Eigen::SparseMatrix<double> C_sparse(n,n);
        C.assemble(C_sparse);
        TS_ASSERT_EQUALS(C_sparse.nonZeros(),7);
        for (int k=0; k<C_sparse.outerSize(); ++k) {
            for (Eigen::SparseMatrix<double>::InnerIterator it(C_sparse,k); it; ++it) {
                TS_ASSERT_EQUALS(it.value(),C.coeff(it.row(),it.col())); 
            }
        }

        ans_copy = C_sparse*v;
        
        for (int i=0; i<n; i++) {
            TS_ASSERT_EQUALS(ans[i],ans_copy[i]); 
        }
        */

#endif // HAVE_EIGEN
    }

    void test_Eigen_block(void) {
#ifdef HAVE_EIGEN
        ABORIA_VARIABLE(scalar1,double,"scalar1")
        ABORIA_VARIABLE(scalar2,double,"scalar2")

    	typedef Particles<std::tuple<scalar1,scalar2>> ParticlesType;
        typedef position_d<3> position;
       	ParticlesType particles,augment;

       	double diameter = 0.1;
        double3 min(-1);
        double3 max(1);
        double3 periodic(false);
        
        ParticlesType::value_type p;
        get<position>(p) = double3(0,0,0);
        get<scalar1>(p) = 1;
        get<scalar2>(p) = 0.1;
       	particles.push_back(p);
        get<position>(p) = double3(diameter*0.9,0,0);
        get<scalar1>(p) = 2;
        get<scalar2>(p) = 0.2;
       	particles.push_back(p);
        get<position>(p) = double3(diameter*1.8,0,0);
        get<scalar1>(p) = 3;
        get<scalar2>(p) = 0.3;
       	particles.push_back(p);
        get<scalar1>(p) = 0;
        get<scalar2>(p) = 0;
        get<position>(p) = double3(-diameter*1.8,0,0);
        augment.push_back(p);


        const size_t n = 3;

        particles.init_neighbour_search(min,max,diameter,periodic);

        
        //      1  1  1
        // A =  2  2  2
        //      3  3  3
        auto A = create_dense_operator(particles,particles,
                    [](const position::value_type &dx,
                       ParticlesType::const_reference a,
                       ParticlesType::const_reference b) {
                    return get<scalar1>(a);
                    });


        Eigen::VectorXd v(n);
        v << 1, 1, 1;
        Eigen::VectorXd ans(n);
        ans = A*v;
        TS_ASSERT_EQUALS(ans[0],3); 
        TS_ASSERT_EQUALS(ans[1],6); 
        TS_ASSERT_EQUALS(ans[2],9); 

        // B = 0.1
        //     0.2
        //     0.3
        auto B = create_dense_operator(particles,augment,
                    [](const position::value_type &dx,
                       ParticlesType::const_reference a,
                       ParticlesType::const_reference b) {
                    return get<scalar2>(a);
                    });


        // C = 0.1 0.2 0.3
        auto C = create_dense_operator(augment,particles,
                    [](const position::value_type &dx,
                       ParticlesType::const_reference a,
                       ParticlesType::const_reference b) {
                    return get<scalar2>(b);
                    });


        auto Zero = create_zero_operator(augment,augment);



        //         1   1   1   0.1
        //         2   2   2   0.2
        // Full =  3   3   3   0.3
        //         0.1 0.2 0.3 0
        auto Full = create_block_operator<2,2>(A,B,
                                               C,Zero);

        v.resize(n+1);
        v << 1, 1, 1, 1;
        ans.resize(n+1);

        //         1   1   1   0.1   1   3.1
        //         2   2   2   0.2   1   6.2
        // ans  =  3   3   3   0.3 * 1 = 9.3
        //         0.1 0.2 0.3 0     1   0.6
        ans = Full*v;
        TS_ASSERT_DELTA(ans[0],3.1,std::numeric_limits<double>::epsilon()); 
        TS_ASSERT_DELTA(ans[1],6.2,std::numeric_limits<double>::epsilon()); 
        TS_ASSERT_DELTA(ans[2],9.3,std::numeric_limits<double>::epsilon()); 
        TS_ASSERT_DELTA(ans[3],0.6,std::numeric_limits<double>::epsilon()); 

        Eigen::MatrixXd Full_copy(n+1,n+1);
        Eigen::VectorXd ans_copy(n+1);
        Full.assemble(Full_copy);
        for (int i=0; i<n; i++) {
            for (int j=0; j<n; j++) {
                TS_ASSERT_EQUALS(Full_copy(i,j),Full.coeff(i,j)); 
            }
        }

        ans_copy = Full_copy*v;
        
        for (int i=0; i<n; i++) {
            TS_ASSERT_EQUALS(ans[i],ans_copy[i]); 
        }

        Eigen::SparseMatrix<double> Full_sparse(n+1,n+1);
        Full.assemble(Full_sparse);
        TS_ASSERT_EQUALS(Full_sparse.nonZeros(),15);
        for (int k=0; k<Full_sparse.outerSize(); ++k) {
            for (Eigen::SparseMatrix<double>::InnerIterator it(Full_sparse,k); it; ++it) {
                TS_ASSERT_EQUALS(it.value(),Full.coeff(it.row(),it.col())); 
            }
        }

        ans_copy = Full_sparse*v;
        
        for (int i=0; i<n; i++) {
            TS_ASSERT_EQUALS(ans[i],ans_copy[i]); 
        }

#endif // HAVE_EIGEN
    }


};

#endif /* OPERATORSTEST_H_ */
