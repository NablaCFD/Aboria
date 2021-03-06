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


#ifndef NEIGHBOURS_H_
#define NEIGHBOURS_H_

#include <cxxtest/TestSuite.h>
#include <chrono>
typedef std::chrono::system_clock Clock;

#include "Aboria.h"

using namespace Aboria;

class NeighboursTest : public CxxTest::TestSuite {
public:

    ABORIA_VARIABLE(scalar,double,"scalar")
    ABORIA_VARIABLE(neighbours_brute,int,"number of neighbours")
    ABORIA_VARIABLE(neighbours_aboria,int,"number of neighbours")
        
    void test_documentation(void) {
#if not defined(__CUDACC__)
//[neighbour_search
/*`
[section Neighbourhood Searching]

The [classref Aboria::Particles] container gives you neighbourhood searching 
functionality, using a variety of spatial data structures as described below.
All these data structure can be used in any number of dimensions, with arbitrary
periodicity. Any neighbour search is performed within a hypercube domain, with 
extents specified by the user.

To start with, we will create a particle set in three dimensions (the default) 
containing a few randomly placed particles
*/

        const size_t N = 100;
        ABORIA_VARIABLE(neighbours_count,int,"number of neighbours")
        typedef Particles<std::tuple<neighbours_count>> particle_type;
        typedef particle_type::position position;
        particle_type particles(N);
        std::default_random_engine gen; 
        std::uniform_real_distribution<double> uniform(-1,1);
        for (int i=0; i<N; ++i) {
            get<position>(particles)[i] = vdouble3(uniform(gen),uniform(gen),uniform(gen));
        }

/*`

Before you can use the neighbourhood searching, you need to initialise the
domain using the [memberref Aboria::Particles::init_neighbour_search] function.

In this case, we will initialise a domain from $(-1,-1,-1)$ to $(1,1,1)$, which
is periodic in all directions.

*/

        vdouble3 min(-1);
        vdouble3 max(1);
        vbool3 periodic(true);
        particles.init_neighbour_search(min,max,periodic);
/*`

Once this is done you can begin using the neighbourhood search queries using the 
[funcref Aboria::euclidean_search] function. This returns a lightweight container 
with `begin()` and `end()` functions that return `const` forward only iterators 
to the particles that satisfy the neighbour search. For example, the following 
counts all the particles within a distance `radius` of the point $(0,0,0)$. 

*/

        double radius = 0.2;
        int count = 0;
        for (const auto& i: euclidean_search(particles.get_query(),vdouble3(0),radius)) {
            count++;
        }
        std::cout << "There are "<< count << " particles.\n";

/*`

Note that [funcref Aboria::euclidean_search] uses the euclidean or 2-norm distance 
($\sqrt{\sum_i^d x^2}$), but there are other functions for other distance norms. 
[funcref Aboria::manhatten_search] uses the 1-norm ($\sum_i^d |x|$), 
[funcref Aboria::chebyshev_search] uses the inf-norm ($\max_i^d |x|$), and you can 
use the generic [funcref Aboria::distance_search] for the $p$-norm ($(\sum_i^d x^n)^{1/n}$), 
where $p$ is any integer greater than 0.

When dereferenced, the neighbourhood iterator returns a tuple of size 2 
containing 

# A constant reference to the found particle object, with type
`particle_type::const_reference`

# A vector $\mathbf{dx}\_{ij}$ pointing to the found point from the query 
point. I.e. if $\mathbf{x}\_i$ is the query point and $\mathbf{x}\_j$ is the
found point, then $\mathbf{dx}\_{ij} = \mathbf{x}\_j - \mathbf{x}\_i$.

The latter is useful for periodic domains, the returned vector
$\mathbf{dx}\_{ij}$ takes periodic domains into account and returns the
$\mathbf{dx}\_{ij}$ with the smallest length. 

For example, 

*/

        for (const auto& i: euclidean_search(particles.get_query(),vdouble3(0),radius)) {
            particle_type::const_reference b = std::get<0>(i);
            const vdouble3& dx = std::get<1>(i);
            std::cout << "Found a particle with dx = " << dx << " and id = " << get<id>(b) << "\n";
        }

/*`

Once you start to alter the positions of the particles, you will need to 
update the neighbourhood data structure that is used for the search. This is 
done using the [memberref Aboria::Particles::update_positions] function. 
For example, to move all the particles by a random value and then update
the data structure, you would use the following code:

*/
        for (auto& x: get<position>(particles)) {
            x += vdouble3(uniform(gen),uniform(gen),uniform(gen));
        }
        particles.update_positions();
/*`

Note: if you did not call `update_positions()` after the loop, then subsequent neighbour
searches would be incorrect

The function [memberref Aboria::Particles::update_positions] can also take a pair
of iterators corresponding to the range of particle positions you wish to update.
For example, if you wish to only move and update a single particle, you could write

*/
        get<position>(particles)[5] = vdouble3(0.1,0,0);
        particles.update_positions(particles.begin()+5,particles.begin()+6);
/*`

Note that this code is valid only for the default [classref Aboria::bucket_search_serial] neighbour data structure (see below), as this is (currently)
the only data structure that does not depend on the specific ordering of the particles
in the `particles` vector, and thus the only data structure that can update a single
particle independently to the others. The other data structures will generate a run-time
error in this case.

You can also use `update_positions` to delete particles. Any particles with their `alive` flag set to `false` will be deleted by the `update_positions` function. For
example, if you wish to delete all the particles with an x coordinate less than `0`
you could write:

*/
        for (auto p: particles) {
            if (get<position>(p)[0] < 0) {
                get<alive>(p) = false;
            }
        }
        particles.update_positions();

/*` 

If you wish to delete a single particle using the range version of `update_positions`, 
then the second iterator you pass to the function must be the `end()` iterator of the
`particles` vector. Recall that `particles` is a vector, and therefore deleting a particle at a given index in the vector neccessarily moves all the particles 
after this index

*/
        get<alive>(particles)[5] = false;
        particles.update_positions(particles.begin()+5,particles.end());

/*`

[section Cell Lists]

There are two cell list data structures within Aboria. Both divide the domain into 
a regular grid of hypercubes with side length set so that the average number of 
particles within each box is close to a given value. Each particle in the container is assigned to the cell that contains its position, and neighbourhood queries search within that cell and its neighbours within the given radius. 

For example, the following diagram illustrates a cell list data structure in two dimensions, shown as a regular array of grey squares each containing zero or more particles. The user wishes to find all the particles within a given euclidean distance around the red point. To accomplish this query efficiently, Aboria would then search all the red-shaded cells for particles that fall within the red circle.

[$images/neighbour/cell_lists.svg  [width 100%]  [align center]] 

The first cell list data structure supports serial insertion of particles, and parallel
queries. The relevant classes are [classref Aboria::bucket_search_serial] and [classref Aboria::bucket_search_serial_query]. This data structure can be selected on a per-particle-set basis, by setting the fourth template argument for [classref Aboria::Particles]. I.e.

*/

        typedef Particles<std::tuple<>,3,std::vector,bucket_search_serial> particle_bs_serial_type;

/*`

You will notice that we also need to specify the vector data structure that the particle
container uses, which in this case is a `std::vector`.

The alternative is a cell-list data structure that supports parallel insertion of 
points, and parallel queries. This constantly re-orders the particles in the particle 
container so that they are sorted into individual cells, so if particles are changing 
cells often this can be slower. But theoretically (this hasn't been tested yet) this 
should speed up neighbourhood search queries as the particles that are local in memory
are also local in space. The relevant classes are [classref Aboria::bucket_search_parallel] and [classref Aboria::bucket_search_parallel_query], and you can use this data structure like so:

*/
        
        typedef Particles<std::tuple<>,3,std::vector,bucket_search_parallel> particle_bs_parallel_type;

/*`


[endsect]


[section Fast Cell-list Neighbour Search]

The two cell-list datastructures support an alternate neighbour search facility that can be faster than the typical Aboria search iterators described above. The key assumption of this fast search is that the regular cells have a width greater than or equal to two times the search radius, and that the same search (i.e. same radius) is to be performed for every single particle in the set. If we want to use the same search radius `radius` as before, we can ensure this is true by setting the `n_particles_in_leaf` argument of the [memberref Aboria::Particles::init_neighbour_search] function to $n = N\frac{(radius)^D}{V}$, where $N$ is the total number of particles in the set, $V$ is the volume of the domain, and $D$ is the number of spatial dimensions. That is,

*/

        const double required_n = N*std::pow(radius,3)/std::pow(2.0,3);
        particles.init_neighbour_search(min,max,periodic,required_n);

/*`
Given this assumption, a fast neighbour search would be to simply look in all the possible pairs of neighbouring cells for possible neighbouring particle pairs. To enable this, Aboria provides the [funcref Aboria::get_neighbouring_buckets] function, which returns an iterator range containing all possible pairs of neighbouring buckets. The user can then iterator over each bucket pair, looping through all the particle within in bucket using either the [memberref Aboria::bucket_search_serial_query::get_bucket_particles] or [memberref Aboria::bucket_search_parallel_query::get_bucket_particles] functions. For example, to count up the number of neighbours within a distance of `radius`, you might write:

*/
        for (auto& ij: get_neighbouring_buckets(particles.get_query())) {
            const auto& i = std::get<0>(ij); // bucket i
            const auto& j = std::get<1>(ij); // bucket j
            // position offset to apply to particles in i (for periodic boundaries)
            const auto& poffset = std::get<2>(ij); 
            for (auto pi: particles.get_query().get_bucket_particles(i)) {
                const Vector<double,3> pi_position = get<position>(pi)+poffset;
                for (auto pj: particles.get_query().get_bucket_particles(j)) {
                    if ((pi_position-get<position>(pj)).squaredNorm() 
                            < radius) {
                        // each ij bucket pair is counted once, so need to 
                        // increment neighbour count for pi and pj
                        get<neighbours_count>(pi)++;
                        get<neighbours_count>(pj)++;
                    }
                }
            }
        }

/*`
The above code considers particle pairs within neighbouring buckets, but not those within the same bucket. These pairs can be obtained by simply looping through all the buckets in the cell-list, using the [memberref Aboria::bucket_search_serial_query::get_subtree] or [memberref Aboria::bucket_search_parallel_query::get_subtree] functions.

For example:
*/
        for (auto& i: particles.get_query().get_subtree()) {
            auto prangei = particles.get_query().get_bucket_particles(i);
            for (auto pi = prangei.begin(); pi!=prangei.end(); ++pi) {
                get<neighbours_count>(*pi)++; // self is a neighbour
                for (auto pj = pi+1; pj!=prangei.end(); ++pj) {
                    if ((get<position>(*pi)-get<position>(*pj)).squaredNorm() 
                            < radius) {
                        get<neighbours_count>(*pi)++;
                        get<neighbours_count>(*pj)++;
                    }
                }
            }
        }
/*`

After the code given above, the variable `neighbour_count` for each particle will contain the number of neighbouring particles around that particle. Note that this will only be correct if the width of each cell is greater than `radius`. 

[endsect]

[section Kd-Tree]

A kd-tree builds up a hierarchical tree of cells, with only the leaf cells actually containing particles. It is an efficient data structure to use if your particles are clustered in certain regions of the domain, and so you wish to adapt the size of your cells with the local particle density.

Each level of the tree divides the cells in the parent level in half along a certain dimension (the dimension is chosen based on the distribution of particles within the cell). Any cells that contain a number of particles that is smaller than a given threshold (set in [memberref Aboria::Particles::init_neighbour_search]) are marked as leaf cells, and are not divided on subsequent levels.

The construction of the kd-tree in Aboria simply wraps the popular NanoFLANN library [@https://github.com/jlblancoc/nanoflann], and reorders the particle set according to which leaf cell a particle belongs to. However, Aboria's native neighbourhood queries are used instead of those provided with NanoFLANN.

The relevant classes within Aboria are [classref Aboria::nanoflann_adaptor] and [classref Aboria::nanoflann_adaptor_query]. You can create a particle set using a kd-tree by setting the [classref Aboria::Particles] template arguments accordingly.

*/
        
        typedef Particles<std::tuple<>,3,std::vector,nanoflann_adaptor> particle_kdtree_type;

/*`



[endsect]


[section Hyper Oct-Tree]

A hyper oct-tree is a generalisation of an oct-tree (in 3 dimensions) to $N$ dimensions. Is also builds up a hierarchical tree of cells, however in this case each level of the tree is split along [*all] dimensions, so that each cell has $2^N$ children. Any cells that contain less that the given number of particles (set in [funcref Aboria::Particles::init_neighbour_search]) are marked as leaf cells. Empty cells are included in the data structure, but are ignored by any queries.

For example, the diagram below shows the leaf cells of a hyper oct-tree in 2 dimensions (this is the same as a quad-tree). If the user wishes to find all the particles within a given euclidean distance of the red particle, then Aboria will search through all the red-shaded cells for matching particles.

[$images/neighbour/octtree.svg] 

The relevant classes within Aboria are [classref Aboria::octtree] and [classref Aboria::octtree_query].
You can create a particle set using a hyper oct-tree by setting the [classref Aboria::Particles] template arguments accordingly.

*/
        
        typedef Particles<std::tuple<>,3,std::vector,octtree> particle_octtree_type;

/*`



[endsect]
[endsect]

*/
//]
#endif
    }



    template<template <typename,typename> class Vector,template <typename> class SearchMethod>
    void helper_single_particle(void) {
    	typedef Particles<std::tuple<scalar>,3,Vector,SearchMethod> Test_type;
        typedef position_d<3> position;
    	Test_type test;
    	vdouble3 min(-1,-1,-1);
    	vdouble3 max(1,1,1);
    	vdouble3 periodic(true,true,true);
    	double radius = 0.1;
    	test.init_neighbour_search(min,max,periodic);
    	typename Test_type::value_type p;

        get<position>(p) = vdouble3(0,0,0);
    	test.push_back(p);

    	int count = 0;
    	for (auto tpl: euclidean_search(test.get_query(),vdouble3(radius/2,radius/2,0),radius)) {
    		count++;
    	}
    	TS_ASSERT_EQUALS(count,1);

    	auto tpl = euclidean_search(test.get_query(),vdouble3(radius/2,radius/2,0),radius);
    	TS_ASSERT_EQUALS(std::distance(tpl.begin(),tpl.end()),1);

    	tpl = euclidean_search(test.get_query(),vdouble3(2*radius,0,0),radius);
    	TS_ASSERT_EQUALS(std::distance(tpl.begin(),tpl.end()),0);
    }

    template<template <typename,typename> class Vector,template <typename> class SearchMethod>
    void helper_two_particles(void) {
    	typedef Particles<std::tuple<scalar>,3,Vector,SearchMethod> Test_type;
        typedef position_d<3> position;
    	Test_type test;
    	vdouble3 min(-1,-1,-1);
    	vdouble3 max(1,1,1);
    	vdouble3 periodic(true,true,true);
    	double radius = 0.1;
    	test.init_neighbour_search(min,max,periodic);
    	typename Test_type::value_type p;

        get<position>(p) = vdouble3(0,0,0);
    	test.push_back(p);

        get<position>(p) = vdouble3(radius/2,0,0);
    	test.push_back(p);

    	auto tpl = euclidean_search(test.get_query(),vdouble3(1.1*radius,0,0),radius);
    	TS_ASSERT_EQUALS(std::distance(tpl.begin(),tpl.end()),1);

    	typename Test_type::const_reference pfound = detail::get_impl<0>(*tpl.begin());
    	TS_ASSERT_EQUALS(get<id>(pfound),get<id>(test[1]));

    	tpl = euclidean_search(test.get_query(),vdouble3(0.9*radius,0,0),radius);
    	TS_ASSERT_EQUALS(std::distance(tpl.begin(),tpl.end()),2);

    	tpl = euclidean_search(test.get_query(),vdouble3(1.6*radius,0,0),radius);
    	TS_ASSERT_EQUALS(std::distance(tpl.begin(),tpl.end()),0);

    	tpl = euclidean_search(test.get_query(),vdouble3(0.25*radius,0.9*radius,0),radius);
    	TS_ASSERT_EQUALS(std::distance(tpl.begin(),tpl.end()),2);

    	tpl = euclidean_search(test.get_query(),vdouble3(0.25*radius,0.99*radius,0),radius);
    	TS_ASSERT_EQUALS(std::distance(tpl.begin(),tpl.end()),0);
    }

    template <typename Particles, int LNormNumber>
    struct has_n_neighbours {
        typedef typename Particles::query_type query_type;
        typedef typename Particles::position position;
        typedef typename Particles::reference reference;
        unsigned int n;
        double max_distance;
        query_type query;

        has_n_neighbours(const query_type& query, 
                const double max_distance, const unsigned int n):
            query(query),n(n),max_distance(max_distance) {}

        ABORIA_HOST_DEVICE_IGNORE_WARN
        void operator()(reference i) {
            auto tpl = distance_search<LNormNumber>(query,get<position>(i),max_distance);
            TS_ASSERT_EQUALS(tpl.end()-tpl.begin(),n);
        }
    };

    template<unsigned int D, 
             template <typename,typename> class VectorType,
             template <typename> class SearchMethod>
    void helper_d(const int n, const double r, const int neighbour_n) {
    	typedef Particles<std::tuple<scalar>,D,VectorType,SearchMethod> Test_type;
        typedef position_d<D> position;
        typedef Vector<double,D> double_d;
        typedef Vector<bool,D> bool_d;
        typedef Vector<unsigned int,D> uint_d;
    	Test_type test;
    	double_d min(0);
    	double_d max(n);
    	bool_d periodic(true);
    	typename Test_type::value_type p;
        uint_d index(0);
        double dx = 1.0;

        bool finished = false;
        while (finished != true) {
            double_d pos = index*dx+min+dx/2;
            get<position>(p) = pos;
            test.push_back(p);
            index[0]++;
            for (int i=0; i<D; i++) {
                if (index[i] >= n) {
                    if (i==D-1) {
                        finished = true;
                    } else {
                        index[i+1]++;
                        index[i] = 0;
                    }
                } else {
                    break;
                }
            }
        }

    	test.init_neighbour_search(min,max,periodic,neighbour_n);
        if (D==2) {
            // Gauss circle problem (L2 in D=2)
            int n_expect = 0;
            for (int i = 0; i < 100; ++i) {
                n_expect += int(std::floor(std::pow(r,2)/(4*i+1))) - int(std::floor(std::pow(r,2)/(4*i+3)));
            }
            n_expect = 1 + 4*n_expect;
            std::cout << "L2 norm test (r="<<r<<"): expecting "<<n_expect<<" points"<<std::endl;
            Aboria::detail::for_each(test.begin(),test.end(),
                    has_n_neighbours<Test_type,2>(test.get_query(),r,n_expect));
        }
        // Box search (Linf)
        int n_expect = std::pow(2*int(std::floor(r)) + 1,D);
        std::cout << "Linf norm test (r="<<r<<", D="<<D<<"): expecting "<<n_expect<<" points"<<std::endl;
        Aboria::detail::for_each(test.begin(),test.end(),
                has_n_neighbours<Test_type,-1>(test.get_query(),r,n_expect));

    }

    template <unsigned int D, typename Reference>
    struct set_random_position {
        double a,b;

        set_random_position(double a, double b):a(a),b(b) {}

        CUDA_HOST_DEVICE
        void operator()(Reference arg) {
            Vector<double,D>& p = get<position_d<D>>(arg);
            generator_type& gen = get<generator>(arg);

            detail::uniform_real_distribution<float> dist(a, b);
            for (int d = 0; d < D; ++d) {
                p[d] = dist(gen);
            }

        }
    };

    template <typename Reference>
    struct zero_neighbours_aboria{
        CUDA_HOST_DEVICE
        void operator()(Reference arg) {
            get<neighbours_aboria>(arg) = 1;
        }
    };

    template <typename ParticlesType>
    struct brute_force_check {
        typedef typename ParticlesType::raw_reference reference;
        typedef typename ParticlesType::raw_pointer pointer;
        typedef typename ParticlesType::raw_const_reference const_reference;
        typedef typename ParticlesType::double_d double_d;
        typedef typename ParticlesType::int_d int_d;
        typedef typename ParticlesType::query_type query_type;
        typedef typename ParticlesType::position position;
        static const unsigned int D = ParticlesType::dimension;

        query_type query;
        double_d min;
        double_d max;
        double r2;
        bool is_periodic;

        brute_force_check(ParticlesType &particles, double_d& min, double_d& max, double r2, bool is_periodic):
            query(particles.get_query()),
            min(min),max(max),r2(r2),is_periodic(is_periodic) {}

        ABORIA_HOST_DEVICE_IGNORE_WARN
        void operator()(reference i) {
            int count = 0;
            pointer begin = query.get_particles_begin();
            double_d& pi = get<position>(i);
            for (int i = 0; i < query.number_of_particles(); ++i) {
                const_reference j = *(begin + i);
                const double_d& pj = get<position>(j);
                if (is_periodic) {
                    for (lattice_iterator<D> periodic_it(int_d(-1),int_d(2)); 
                            periodic_it != false; ++periodic_it) {
                        if ((pi+(*periodic_it)*(max-min)-pj).squaredNorm() <= r2) {
                            count++;
                        }
                    }
                } else {
                    if ((pi-pj).squaredNorm() <= r2) {
                        count++;
                    }
                }
            }
            get<neighbours_brute>(i) = count;
        }
    };
    template <typename ParticlesType>
    struct aboria_check {
        typedef typename ParticlesType::raw_reference reference;
        typedef typename ParticlesType::raw_pointer pointer;
        typedef typename ParticlesType::raw_const_reference const_reference;
        typedef typename ParticlesType::double_d double_d;
        typedef typename ParticlesType::query_type query_type;
        typedef typename ParticlesType::position position;
        static const unsigned int D = ParticlesType::dimension;

        query_type query;
        double r;
        double r2;

        aboria_check(ParticlesType &particles, double r):
            query(particles.get_query()),r(r),r2(r*r) {}

        ABORIA_HOST_DEVICE_IGNORE_WARN
        void operator()(reference i) {
            int count = 0;
            for (auto tpl: euclidean_search(query,get<position>(i),r)) {
                const_reference j = detail::get_impl<0>(tpl);
                const double_d& dx = detail::get_impl<1>(tpl);
                count++;
            }
            get<neighbours_aboria>(i) = count;
            
        }
    };

    template <typename Query>
    struct aboria_fast_bucketsearch_check_neighbour {
        typedef bucket_pair_iterator<Query> Iterator;
        typedef typename Iterator::reference reference;
        typedef position_d<Query::dimension> position;
        typedef Vector<double,Query::dimension> double_d;

        Query query;
        double r;
        double r2;

        aboria_fast_bucketsearch_check_neighbour(
                const Query &query, double r):
            query(query),r(r),r2(r*r) {}

        ABORIA_HOST_DEVICE_IGNORE_WARN
        void operator()(reference ij) {
            const auto& i = detail::get_impl<0>(ij);
            const auto& j = detail::get_impl<1>(ij);
            const auto& poffset = detail::get_impl<2>(ij);
            //std::cout << "checking i = "<<i<<" j = "<<j << std::endl;
            for (auto pi: query.get_bucket_particles(i)) {
                const double_d pi_position = get<position>(pi)+poffset;
                for (auto pj: query.get_bucket_particles(j)) {
                    if ((pi_position-get<position>(pj)).squaredNorm()
                            < r2) {
                        //std::cout << "particles "<< get<position>(pi)<< " and "<< get<position>(pj)<< " are neighbours"<< std::endl;
                        get<neighbours_aboria>(pi)++;
                        get<neighbours_aboria>(pj)++;
                    }
                }
            }
        }
    };

    template <typename Query>
    struct aboria_fast_bucketsearch_check_self {
        typedef lattice_iterator<Query::dimension> Iterator;
        typedef typename Iterator::reference reference;
        typedef position_d<Query::dimension> position;

        Query query;
        double r;
        double r2;

        aboria_fast_bucketsearch_check_self(const Query &query, double r):
            query(query),r(r),r2(r*r) {}

        ABORIA_HOST_DEVICE_IGNORE_WARN
        void operator()(reference i) {
            auto prangei = query.get_bucket_particles(i);
            for (auto pi = prangei.begin(); pi!=prangei.end(); ++pi) {
                get<neighbours_aboria>(*pi)++; // self is a neighbour
                for (auto pj = pi+1; pj!=prangei.end(); ++pj) {
                    if ((get<position>(*pi)-get<position>(*pj)).squaredNorm()
                            < r2) {
                        //std::cout << "particles "<< get<position>(*pi)<< " and "<< get<position>(*pj)<< " are neighbours (self)"<< std::endl;
                        get<neighbours_aboria>(*pi)++;
                        get<neighbours_aboria>(*pj)++;
                    }
                }
            }
        }
    };




    template<unsigned int D, 
             template <typename,typename> class VectorType,
             template <typename> class SearchMethod>
    void helper_d_random(const int N, const double r, const int neighbour_n, const bool is_periodic, const bool push_back_construction) {
    	typedef Particles<std::tuple<neighbours_brute,neighbours_aboria>,D,VectorType,SearchMethod> particles_type;
        typedef position_d<D> position;
        typedef Vector<double,D> double_d;
        typedef Vector<bool,D> bool_d;
        typedef Vector<int,D> int_d;
        typedef Vector<unsigned int,D> uint_d;
    	double_d min(-1);
    	double_d max(1);
    	bool_d periodic(is_periodic);
        particles_type particles;
        double r2 = r*r;

        std::cout << "random test (D="<<D<<" periodic= "<<is_periodic<<"  N="<<N<<" r="<<r<<" push_back_construction = "<<push_back_construction<<"):" << std::endl;

        unsigned seed1 = std::chrono::system_clock::now().time_since_epoch().count();
        std::cout << "seed is "<< seed1 << std::endl;
        particles.set_seed(seed1);
        generator_type gen(seed1); 
        detail::uniform_real_distribution<float> uniform(-1.0, 1.0);

        if (push_back_construction) {
    	    particles.init_neighbour_search(min,max,periodic,neighbour_n);
            typename particles_type::value_type p;
            for (int i=0; i<N; ++i) {
                for (int d = 0; d < D; ++d) {
                    get<position>(p)[d] = uniform(gen);
                }
                particles.push_back(p);
            }
        } else {
            particles.resize(N);
            detail::for_each(std::begin(particles),std::end(particles),
                set_random_position
                    <D,typename particles_type::raw_reference>(-1.0,1.0));
            
    	    particles.init_neighbour_search(min,max,periodic,neighbour_n);
        }

        // delete random particle
        detail::uniform_int_distribution<int> uniform_int_N(0, N-1);
        get<alive>(particles)[uniform_int_N(gen)] = false;

        // delete first particle
        get<alive>(particles)[0] = false;
        particles.update_positions();

        // delete random particle
        detail::uniform_int_distribution<int> uniform_int_N_minus_1(0, N-3);
        const int random_index = uniform_int_N_minus_1(gen);
        particles.erase(particles.begin()+random_index);
        
        // delete last particle
        particles.erase(particles.begin()+particles.size()-1);

        // brute force search
        auto t0 = Clock::now();
        Aboria::detail::for_each(particles.begin(),particles.end(),
                brute_force_check<particles_type>(particles,min,max,r2,is_periodic));
        auto t1 = Clock::now();
        std::chrono::duration<double> dt_brute = t1 - t0;


        // Aboria search
        t0 = Clock::now();
        Aboria::detail::for_each(particles.begin(),particles.end(),
                aboria_check<particles_type>(particles,r)); 
        t1 = Clock::now();
        std::chrono::duration<double> dt_aboria = t1 - t0;
        for (int i = 0; i < particles.size(); ++i) {
            if (int(get<neighbours_brute>(particles)[i]) !=
                             int(get<neighbours_aboria>(particles)[i])) {
                std::cout << "error in finding neighbours for p = " <<
                    static_cast<const double_d&>(get<position>(particles)[i])<<
                    " over radius "<<r<< std::endl;
                particles.print_data_structure();

                TS_ASSERT_EQUALS(int(get<neighbours_brute>(particles)[i]),
                             int(get<neighbours_aboria>(particles)[i]));
                return;
            }
            TS_ASSERT_EQUALS(int(get<neighbours_brute>(particles)[i]),
                             int(get<neighbours_aboria>(particles)[i]));
        }
            

        std::cout << "\ttiming result: Aboria = "<<dt_aboria.count()
                  <<" versus brute force = "<<dt_brute.count()<<std::endl;
    }

    template<unsigned int D, 
             template <typename,typename> class VectorType,
             template <typename> class SearchMethod>
    void helper_d_random_fast_bucketsearch(const int N, const double r, const bool is_periodic, const bool push_back_construction) {
    	typedef Particles<std::tuple<neighbours_brute,neighbours_aboria>,D,VectorType,SearchMethod> particles_type;
        typedef typename particles_type::query_type query_type;
        typedef position_d<D> position;
        typedef Vector<double,D> double_d;
        typedef Vector<bool,D> bool_d;
        typedef Vector<int,D> int_d;
        typedef Vector<unsigned int,D> uint_d;
    	double_d min(-1);
    	double_d max(1);
    	bool_d periodic(is_periodic);
        particles_type particles;
        const double required_bucket_size = r;
        const double required_bucket_number = N*std::pow(required_bucket_size,D)/std::pow(2.0,D);
        double r2 = r*r;

        std::cout << "random fast bucketsearch test (D="<<D<<" periodic= "<<is_periodic<<"  N="<<N<<" r="<<r<<" required_bucket_number = "<<required_bucket_number<<" push_back_construction = "<<push_back_construction<<"):" << std::endl;

        unsigned seed1 = std::chrono::system_clock::now().time_since_epoch().count();
        std::cout << "seed is "<< seed1 << std::endl;
        particles.set_seed(seed1);
        generator_type gen(seed1); 
        detail::uniform_real_distribution<float> uniform(-1.0, 1.0);

        if (push_back_construction) {
    	    particles.init_neighbour_search(min,max,periodic,required_bucket_number);
            typename particles_type::value_type p;
            for (int i=0; i<N; ++i) {
                for (int d = 0; d < D; ++d) {
                    get<position>(p)[d] = uniform(gen);
                }
                particles.push_back(p);
            }
        } else {
            particles.resize(N);
            detail::for_each(std::begin(particles),std::end(particles),
                set_random_position
                    <D,typename particles_type::raw_reference>(-1.0,1.0));
            
    	    particles.init_neighbour_search(min,max,periodic,required_bucket_number);
        }


        // brute force search
        auto t0 = Clock::now();
        Aboria::detail::for_each(particles.begin(),particles.end(),
                brute_force_check<particles_type>(particles,min,max,r2,is_periodic));
        auto t1 = Clock::now();
        std::chrono::duration<double> dt_brute = t1 - t0;


        // Aboria search
        t0 = Clock::now();
        auto pair_it = get_neighbouring_buckets(particles.get_query());
        detail::for_each(std::begin(pair_it),std::end(pair_it),
            aboria_fast_bucketsearch_check_neighbour<query_type>(particles.get_query(),r)
                ); 
        auto self_it = particles.get_query().get_subtree();
        detail::for_each(std::begin(self_it),std::end(self_it),
            aboria_fast_bucketsearch_check_self<query_type>(particles.get_query(),r)
                );
        t1 = Clock::now();
        std::chrono::duration<double> dt_aboria = t1 - t0;
        for (int i = 0; i < particles.size(); ++i) {
            if (int(get<neighbours_brute>(particles)[i]) !=
                             int(get<neighbours_aboria>(particles)[i])) {
                std::cout << "error in finding neighbours for p = " <<
                    static_cast<const double_d&>(get<position>(particles)[i])<<
                    " over radius "<<r<< std::endl;
                particles.print_data_structure();

                TS_ASSERT_EQUALS(int(get<neighbours_brute>(particles)[i]),
                             int(get<neighbours_aboria>(particles)[i]));
                return;
            }
            TS_ASSERT_EQUALS(int(get<neighbours_brute>(particles)[i]),
                             int(get<neighbours_aboria>(particles)[i]));
        }
            

        std::cout << "\ttiming result: Aboria = "<<dt_aboria.count()
                  <<" versus brute force = "<<dt_brute.count()<<std::endl;
    }


    

    template<template <typename,typename> class VectorType,
             template <typename> class SearchMethod>
    void helper_d_test_list_regular() {
        helper_d<1,VectorType,SearchMethod>(100,1.5,10);
        helper_d<2,VectorType,SearchMethod>(50,1.0001,10);
        helper_d<2,VectorType,SearchMethod>(50,1.5,10);
        helper_d<2,VectorType,SearchMethod>(20,2.1,10);
        helper_d<3,VectorType,SearchMethod>(10,1.9,10);
        helper_d<3,VectorType,SearchMethod>(10,1.0001,10);
        helper_d<4,VectorType,SearchMethod>(10,1.0001,10);
    }

    template<template <typename,typename> class VectorType,
             template <typename> class SearchMethod>
    void helper_d_test_list_random(bool test_push_back=true) {
        
        helper_d_random<1,VectorType,SearchMethod>(14,0.1,1,false,false);
        helper_d_random<1,VectorType,SearchMethod>(14,0.1,1,true,false);

        if (test_push_back) {
            helper_d_random<1,VectorType,SearchMethod>(14,0.1,1,false,true);
            helper_d_random<1,VectorType,SearchMethod>(14,0.1,1,true,true);
        }

        helper_d_random<1,VectorType,SearchMethod>(1000,0.1,10,true,false);
        helper_d_random<1,VectorType,SearchMethod>(1000,0.1,10,false,false);
        helper_d_random<1,VectorType,SearchMethod>(1000,0.1,100,true,false);
        helper_d_random<1,VectorType,SearchMethod>(1000,0.1,100,false,false);
        helper_d_random<2,VectorType,SearchMethod>(1000,0.1,10,true,false);
        helper_d_random<2,VectorType,SearchMethod>(1000,0.1,10,false,false);
        helper_d_random<2,VectorType,SearchMethod>(1000,0.5,10,true,false);
        helper_d_random<2,VectorType,SearchMethod>(1000,0.5,10,false,false);
        helper_d_random<2,VectorType,SearchMethod>(1000,0.2,1,true,false);
        helper_d_random<2,VectorType,SearchMethod>(1000,0.2,1,false,false);
        helper_d_random<3,VectorType,SearchMethod>(1000,0.2,100,true,false);
        helper_d_random<3,VectorType,SearchMethod>(1000,0.2,100,false,false);
        helper_d_random<3,VectorType,SearchMethod>(1000,0.2,10,true,false);
        helper_d_random<3,VectorType,SearchMethod>(1000,0.2,10,false,false);
        helper_d_random<3,VectorType,SearchMethod>(1000,0.2,1,true,false);
        helper_d_random<3,VectorType,SearchMethod>(1000,0.2,1,false,false);
        helper_d_random<4,VectorType,SearchMethod>(1000,0.2,10,true,false);
        helper_d_random<4,VectorType,SearchMethod>(1000,0.2,10,false,false);

        if (test_push_back) {
            helper_d_random<1,VectorType,SearchMethod>(1000,0.1,10,true,true);
            helper_d_random<1,VectorType,SearchMethod>(1000,0.1,10,false,true);
            helper_d_random<2,VectorType,SearchMethod>(1000,0.1,10,true,true);
            helper_d_random<2,VectorType,SearchMethod>(1000,0.1,10,false,true);
            helper_d_random<2,VectorType,SearchMethod>(1000,0.5,10,true,true);
            helper_d_random<2,VectorType,SearchMethod>(1000,0.5,10,false,true);
            helper_d_random<3,VectorType,SearchMethod>(1000,0.2,10,true,true);
            helper_d_random<3,VectorType,SearchMethod>(1000,0.2,10,false,true);
        }
    }

    template<template <typename,typename> class VectorType,
             template <typename> class SearchMethod>
    void helper_d_test_list_random_fast_bucketsearch(bool test_push_back=true) {
        
        helper_d_random_fast_bucketsearch<1,VectorType,SearchMethod>(14,0.1,false,false);
        helper_d_random_fast_bucketsearch<1,VectorType,SearchMethod>(14,0.1,true,false);

        if (test_push_back) {
            helper_d_random_fast_bucketsearch<1,VectorType,SearchMethod>(14,0.1,false,true);
            helper_d_random_fast_bucketsearch<1,VectorType,SearchMethod>(14,0.1,true,true);
        }

        helper_d_random_fast_bucketsearch<1,VectorType,SearchMethod>(1000,0.1,true,false);
        helper_d_random_fast_bucketsearch<1,VectorType,SearchMethod>(1000,0.1,false,false);
        helper_d_random_fast_bucketsearch<1,VectorType,SearchMethod>(1000,0.1,true,false);
        helper_d_random_fast_bucketsearch<1,VectorType,SearchMethod>(1000,0.1,false,false);
        helper_d_random_fast_bucketsearch<2,VectorType,SearchMethod>(1000,0.1,true,false);
        helper_d_random_fast_bucketsearch<2,VectorType,SearchMethod>(1000,0.1,false,false);
        helper_d_random_fast_bucketsearch<2,VectorType,SearchMethod>(1000,0.5,true,false);
        helper_d_random_fast_bucketsearch<2,VectorType,SearchMethod>(1000,0.5,false,false);
        helper_d_random_fast_bucketsearch<2,VectorType,SearchMethod>(1000,0.2,true,false);
        helper_d_random_fast_bucketsearch<2,VectorType,SearchMethod>(1000,0.2,false,false);
        helper_d_random_fast_bucketsearch<3,VectorType,SearchMethod>(1000,0.2,true,false);
        helper_d_random_fast_bucketsearch<3,VectorType,SearchMethod>(1000,0.2,false,false);
        helper_d_random_fast_bucketsearch<3,VectorType,SearchMethod>(1000,0.2,true,false);
        helper_d_random_fast_bucketsearch<3,VectorType,SearchMethod>(1000,0.2,false,false);
        helper_d_random_fast_bucketsearch<3,VectorType,SearchMethod>(1000,0.2,true,false);
        helper_d_random_fast_bucketsearch<3,VectorType,SearchMethod>(1000,0.2,false,false);
        helper_d_random_fast_bucketsearch<4,VectorType,SearchMethod>(1000,0.2,true,false);
        helper_d_random_fast_bucketsearch<4,VectorType,SearchMethod>(1000,0.2,false,false);

        if (test_push_back) {
            helper_d_random_fast_bucketsearch<1,VectorType,SearchMethod>(1000,0.1,true,true);
            helper_d_random_fast_bucketsearch<1,VectorType,SearchMethod>(1000,0.1,false,true);
            helper_d_random_fast_bucketsearch<2,VectorType,SearchMethod>(1000,0.1,true,true);
            helper_d_random_fast_bucketsearch<2,VectorType,SearchMethod>(1000,0.1,false,true);
            helper_d_random_fast_bucketsearch<2,VectorType,SearchMethod>(1000,0.5,true,true);
            helper_d_random_fast_bucketsearch<2,VectorType,SearchMethod>(1000,0.5,false,true);
            helper_d_random_fast_bucketsearch<3,VectorType,SearchMethod>(1000,0.2,true,true);
            helper_d_random_fast_bucketsearch<3,VectorType,SearchMethod>(1000,0.2,false,true);
        }

    }

    void test_std_vector_bucket_search_serial(void) {
        helper_d_test_list_random<std::vector,bucket_search_serial>();
        helper_single_particle<std::vector,bucket_search_serial>();
        helper_two_particles<std::vector,bucket_search_serial>();
        helper_d_test_list_regular<std::vector,bucket_search_serial>();

    }

    void test_std_vector_bucket_search_parallel(void) {
        helper_d_test_list_random<std::vector,bucket_search_parallel>();
        helper_single_particle<std::vector,bucket_search_parallel>();
        helper_two_particles<std::vector,bucket_search_parallel>();

        helper_d_test_list_regular<std::vector,bucket_search_parallel>();
    }

    void test_std_vector_bucket_search_serial_fast_bucketsearch(void) {
        helper_d_test_list_random_fast_bucketsearch<std::vector,bucket_search_serial>();
        helper_single_particle<std::vector,bucket_search_serial>();
        helper_two_particles<std::vector,bucket_search_serial>();
        helper_d_test_list_regular<std::vector,bucket_search_serial>();

    }

    void test_std_vector_bucket_search_parallel_fast_bucketsearch(void) {
        helper_d_test_list_random_fast_bucketsearch<std::vector,bucket_search_parallel>();
        helper_single_particle<std::vector,bucket_search_parallel>();
        helper_two_particles<std::vector,bucket_search_parallel>();
        helper_d_test_list_regular<std::vector,bucket_search_parallel>();
    }

    void test_std_vector_nanoflann_adaptor(void) {
#if not defined(__CUDACC__)
        helper_d_test_list_random<std::vector,nanoflann_adaptor>();
        helper_d_test_list_regular<std::vector,nanoflann_adaptor>();
#endif
    }

    void test_std_vector_octtree(void) {
        helper_d_test_list_random<std::vector,octtree>();
        helper_d_test_list_regular<std::vector,octtree>();
    }

    //void test_thrust_vector_bucket_search_serial(void) {
//#if //defined(__aboria_have_thrust__)
    //    helper_d_test_list_regular<thrust::device_vector,bucket_search_serial>();
    //    helper_d_test_list_random<thrust::device_vector,bucket_search_serial>();
//#end//if
    //}

    void test_thrust_vector_bucket_search_parallel(void) {
#if defined(__aboria_have_thrust__)
        //helper_d_test_list_random_fast_bucketsearch<std::vector,bucket_search_parallel>();
        helper_d_test_list_regular<thrust::device_vector,bucket_search_parallel>();
        helper_d_test_list_random<thrust::device_vector,bucket_search_parallel>();
#endif
    }

void test_thrust_vector_octtree(void) {
#if defined(__aboria_have_thrust__)
        helper_d_test_list_regular<thrust::device_vector,octtree>();
        helper_d_test_list_random<thrust::device_vector,octtree>();
#endif
    }

};



#endif /* NEIGHBOURS_H_ */
