
#ifndef ALGORITHMS_H_ 
#define ALGORITHMS_H_ 

#include "CudaInclude.h"
#include "Get.h"
#include "Traits.h"
#include <algorithm>

namespace Aboria {

namespace detail {

template <typename Traits>
size_t concurrent_processes() {
#ifdef __aboria_have_thrust__
    if (std::is_same<Traits::vector_unsigned_int,
                     thrust::device_vector<unsigned int>>::value) {
        // if using GPU just return "lots"
        return 9999;
        /*
        int deviceCount, device;
        struct cudaDeviceProp properties;
        cudaError_t cudaResultCode = cudaGetDeviceCount(&deviceCount);
        if (cudaResultCode != cudaSuccess)
            deviceCount = 0;
        ASSERT(deviceCount > 0, "trying to use device vector without a device");
        cudaGetDeviceProperties(&properties, 0);
        if (properties.major != 9999) { // 9999 means emulation only
            return properties.multiProcessorCount*properties.maxThreadsPerMultiProcessor;
        } else {
            //TODO: what should return here?
            return 123;
        }
        */
    }
#endif
#ifdef HAVE_OPENMP
    return omp_get_max_threads();
#else
    return 1;
#endif
}


template <typename T>
struct  is_std_iterator: 
    std::is_same<
            typename std::iterator_traits<T>::iterator_category,
            std::random_access_iterator_tag
    > {};

// reorders v such that v_new[i] == v[order[i]]
template< typename order_iterator, typename value_iterator >
void reorder_destructive( order_iterator order_begin, order_iterator order_end, value_iterator v )  {
    typedef typename std::iterator_traits< value_iterator >::value_type value_t;
    typedef typename std::iterator_traits< value_iterator >::reference reference;
    typedef typename std::iterator_traits< order_iterator >::value_type index_t;
    typedef typename std::iterator_traits< order_iterator >::difference_type diff_t;

    diff_t size = order_end - order_begin;
    
    size_t i, j, k;
    value_t temp;
    for(i = 0; i < size; i++){
        if(i != order_begin[i]){
            temp = v[i];
            k = i;
            while(i != (j = order_begin[k])){
                // every move places a value in it's final location
                // NOTE: need the static cast or assignment has no effect (TODO: why?)
                static_cast<reference>(v[k])  = v[j];
                order_begin[k] = k;
                k = j;
            }
            v[k] = temp;
            order_begin[k] = k;
        }
    }
}


template <typename value_type> 
struct iter_comp {
    bool operator()(const value_type& t1, const value_type& t2) { return get<0>(t1.get_tuple()) < get<0>(t2.get_tuple()); }
};

template <typename T> 
struct lower_bound_impl {
    const T& values_first,values_last;
    lower_bound_impl(const T& values_first, const T& values_last):
        values_first(values_first),values_last(values_last) {}
    typedef typename std::iterator_traits<T>::value_type value_type;
    typedef typename std::iterator_traits<T>::difference_type difference_type;
    difference_type operator()(const value_type & search) { 
        return std::distance(values_first,std::lower_bound(values_first,values_last,search)); 
    }
};

template <typename T> 
struct upper_bound_impl {
    const T& values_first,values_last;
    upper_bound_impl(const T& values_first, const T& values_last):
        values_first(values_first),values_last(values_last) {}
    typedef typename std::iterator_traits<T>::value_type value_type;
    typedef typename std::iterator_traits<T>::difference_type difference_type;
    difference_type operator()(const value_type & search) { 
        return std::distance(values_first,std::upper_bound(values_first,values_last,search)); 
    }
};

#if defined(__CUDACC__)

template <typename T>
using counting_iterator = thrust::counting_iterator<T>;

template <typename T>
using counting_iterator = thrust::counting_iterator<T>;

template <typename T>
using uniform_real_distribution = thrust::uniform_real_distribution<T>;

template <typename T>
using normal_distribution = thrust::normal_distribution<T>;

static const thrust::detail::functional::placeholder<0>::type _1;
static const thrust::detail::functional::placeholder<1>::type _2;
static const thrust::detail::functional::placeholder<2>::type _3;

template <typename T>
using plus = thrust::plus<T>;

using thrust::make_transform_iterator;
using thrust::make_zip_iterator;
using thrust::make_tuple;

#else

template <typename T>
using counting_iterator = boost::counting_iterator<T>;

template <typename T>
using uniform_real_distribution = std::uniform_real_distribution<T>;

template <typename T>
using normal_distribution = std::normal_distribution<T>;

template <class UnaryFunction, class Iterator>
using transform_iterator = boost::transform_iterator<UnaryFunction, Iterator>;

const boost::lambda::placeholder1_type _1;
const boost::lambda::placeholder2_type _2;
const boost::lambda::placeholder3_type _3;

template <typename T>
using plus = std::plus<T>;

using boost::make_transform_iterator;
using boost::make_zip_iterator;
using boost::make_tuple;

//template <class UnaryFunction, class Iterator>
//transform_iterator<UnaryFunction, Iterator>
//make_transform_iterator(Iterator&& it, UnaryFunction&& fun) {
//    return boost::make_transform_iterator(
//            std::forward<Iterator>(it), 
//            std::forward<UnaryFunction>(fun));
//}
#endif

template< class ForwardIt, class T >
void fill( ForwardIt first, ForwardIt last, const T& value, std::true_type ) {
    std::fill(first,last,value);
}

#ifdef __aboria_have_thrust__
template< class ForwardIt, class T >
void fill( ForwardIt first, ForwardIt last, const T& value, std::false_type ) {
    thrust::fill(first,last,value);
}
#endif

template< class ForwardIt, class T >
void fill( ForwardIt first, ForwardIt last, const T& value) {
    fill(first,last,value,typename is_std_iterator<ForwardIt>::type());
}

template< class InputIt, class UnaryFunction >
#ifdef __aboria_use_thrust_algorithms__
InputIt for_each( InputIt first, InputIt last, UnaryFunction f ) {
    return thrust::for_each(first,last,f);
#else
UnaryFunction for_each( InputIt first, InputIt last, UnaryFunction f ) {
    return std::for_each(first,last,f);
#endif
}

template<typename T1, typename T2>
void sort(T1 start, T1 end, std::true_type) {
    std::sort(start,end);
}

#ifdef __aboria_have_thrust__
template<typename T1, typename T2>
void sort(T1 start, T1 end, std::false_type) {
    thrust::sort(start,end);
}
#endif

template<typename T1, typename T2>
void sort(T1 start, T1 end) {
    sort(start,end,typename is_std_iterator<T1>::type());
}

template<typename T1, typename T2>
void sort_by_key(T1 start_keys,
        T1 end_keys,
        T2 start_data,std::true_type) {
    typedef zip_iterator<std::tuple<T1,T2>,mpl::vector<>> pair_zip_type;
    typedef typename pair_zip_type::reference reference;
    typedef typename pair_zip_type::value_type value_type;

    std::sort(
            pair_zip_type(start_keys,start_data),
            pair_zip_type(end_keys,start_data+std::distance(start_keys,end_keys)),
            detail::iter_comp<value_type>());
}

#ifdef __aboria_have_thrust__
template<typename T1, typename T2>
void sort_by_key(T1 start_keys,
        T1 end_keys,
        T2 start_data,std::false_type) {
    thrust::sort_by_key(start_keys,end_keys,start_data);
}
#endif

//TODO: only works for random access iterators
template<typename T1, typename T2>
void sort_by_key(T1 start_keys,
        T1 end_keys,
        T2 start_data) {
    //TODO: how to check its generically a std iterator as opposed to a thrust::iterator
    sort_by_key(start_keys,end_keys,start_data,typename is_std_iterator<T1>::type());
}

template<typename ForwardIterator, typename InputIterator, typename OutputIterator>
void lower_bound(
        ForwardIterator first,
        ForwardIterator last,
        InputIterator values_first,
        InputIterator values_last,
        OutputIterator result, std::true_type) {
    std::transform(values_first,values_last,result,
            detail::lower_bound_impl<ForwardIterator>(first,last));
}

#ifdef __aboria_have_thrust__
template<typename ForwardIterator, typename InputIterator, typename OutputIterator>
void lower_bound(
        ForwardIterator first,
        ForwardIterator last,
        InputIterator values_first,
        InputIterator values_last,
        OutputIterator result, std::false_type) {
    thrust::lower_bound(first,last,values_first,values_last,result);
}
#endif

template<typename ForwardIterator, typename InputIterator, typename OutputIterator>
void lower_bound(
        ForwardIterator first,
        ForwardIterator last,
        InputIterator values_first,
        InputIterator values_last,
        OutputIterator result) {

    lower_bound(first,last,values_first,values_last,result,
            typename is_std_iterator<ForwardIterator>::type());
}

template<typename ForwardIterator, typename InputIterator, typename OutputIterator>
void upper_bound(
        ForwardIterator first,
        ForwardIterator last,
        InputIterator values_first,
        InputIterator values_last,
        OutputIterator result, std::true_type) {

    std::transform(values_first,values_last,result,
            detail::upper_bound_impl<ForwardIterator>(first,last));
}

#ifdef __aboria_have_thrust__
template<typename ForwardIterator, typename InputIterator, typename OutputIterator>
void upper_bound(
        ForwardIterator first,
        ForwardIterator last,
        InputIterator values_first,
        InputIterator values_last,
        OutputIterator result, std::false_type) {
    thrust::upper_bound(first,last,values_first,values_last,result);
}
#endif



template<typename ForwardIterator, typename InputIterator, typename OutputIterator>
void upper_bound(
        ForwardIterator first,
        ForwardIterator last,
        InputIterator values_first,
        InputIterator values_last,
        OutputIterator result) {

    upper_bound(first,last,values_first,values_last,result,
            typename is_std_iterator<ForwardIterator>::type());
}

template< class InputIt, class T, class BinaryOperation >
T reduce( 
    InputIt first, 
    InputIt last, T init,
    BinaryOperation op, std::true_type) {

    return std::accumulate(first,last,init,op);
}

#ifdef __aboria_have_thrust__
template< class InputIt, class T, class BinaryOperation >
T reduce( 
    InputIt first, 
    InputIt last, T init,
    BinaryOperation op, std::false_type) {

    return thrust::reduce(first,last,init,op);
}
#endif

template< class InputIt, class T, class BinaryOperation >
T reduce( 
    InputIt first, 
    InputIt last, T init,
    BinaryOperation op) {

    reduce(first,last,init,op,typename is_std_iterator<InputIt>::type());
}
 
template <class InputIterator, class OutputIterator, class UnaryOperation>
OutputIterator transform (
        InputIterator first, InputIterator last,
        OutputIterator result, UnaryOperation op, std::true_type) {
    return std::transform(first,last,result,op);
}

#ifdef __aboria_have_thrust__
template <class InputIterator, class OutputIterator, class UnaryOperation>
OutputIterator transform (
        InputIterator first, InputIterator last,
        OutputIterator result, UnaryOperation op, std::false_type) {
    return thrust::transform(first,last,result,op);
}
#endif

template <class InputIterator, class OutputIterator, class UnaryOperation>
OutputIterator transform (
        InputIterator first, InputIterator last,
        OutputIterator result, UnaryOperation op) {
    return transform(first,last,result,op,typename is_std_iterator<InputIterator>::type());
}

template <class ForwardIterator>
void sequence (ForwardIterator first, ForwardIterator last, std::true_type) {
    counting_iterator<unsigned int> count(0);
    std::transform(first,last,count,first,
        [](const typename std::iterator_traits<ForwardIterator>::reference, const unsigned int i) {
            return i;
        });
}

template <class ForwardIterator, typename T>
void sequence (ForwardIterator first, ForwardIterator last, T init, std::true_type) {
    counting_iterator<unsigned int> count(init);
    std::transform(first,last,count,first,
        [](const typename std::iterator_traits<ForwardIterator>::reference, const unsigned int i) {
            return i;
        });
}

#ifdef __aboria_have_thrust__
template <class ForwardIterator>
void sequence (ForwardIterator first, ForwardIterator last, std::false_type) {
    thrust::sequence(first,last);
}

template <class ForwardIterator, typename T>
void sequence (ForwardIterator first, ForwardIterator last, T init, std::false_type) {
    thrust::sequence(first,last,init);
}
#endif

template <class ForwardIterator>
void sequence (ForwardIterator first, ForwardIterator last) {
    sequence(first,last, typename is_std_iterator<ForwardIterator>::type());
}

template <class ForwardIterator, typename T>
void sequence (ForwardIterator first, ForwardIterator last, T init) {
    sequence(first,last,init, typename is_std_iterator<ForwardIterator>::type());
}

template<typename ForwardIterator , typename UnaryOperation >
void tabulate (
        ForwardIterator first,
        ForwardIterator last,
        UnaryOperation  unary_op, std::true_type) {	

    counting_iterator<unsigned int> count(0);
    std::transform(first,last,count,first,
        [&unary_op](const typename std::iterator_traits<ForwardIterator>::reference, const unsigned int i) {
            return unary_op(i);
        });
}

#ifdef __aboria_have_thrust__
template<typename ForwardIterator , typename UnaryOperation >
void tabulate (
        ForwardIterator first,
        ForwardIterator last,
        UnaryOperation  unary_op, std::false_type) {	
    thrust::tabulate(first,last,unary_op);
}
#endif

template<typename ForwardIterator , typename UnaryOperation >
void tabulate (
        ForwardIterator first,
        ForwardIterator last,
        UnaryOperation  unary_op) {	
    tabulate(first,last,unary_op, typename is_std_iterator<ForwardIterator>::type());
}

template< class ForwardIt, class UnaryPredicate >
ForwardIt partition( ForwardIt first, ForwardIt last, UnaryPredicate p,std::true_type ) {
    std::partition(first,last,p);
}

#ifdef __aboria_have_thrust__
template< class ForwardIt, class UnaryPredicate >
ForwardIt partition( ForwardIt first, ForwardIt last, UnaryPredicate p,std::false_type) {
    thrust::partition(first,last,p);
}
#endif

template< class ForwardIt, class UnaryPredicate >
ForwardIt partition( ForwardIt first, ForwardIt last, UnaryPredicate p) {
    partition(first,last,p,typename is_std_iterator<ForwardIt>::type());
}

template< class ForwardIt >
ForwardIt unique( ForwardIt first, ForwardIt last, std::true_type ) {
    return std::unique(first,last);
}

#ifdef __aboria_have_thrust__
template< class ForwardIt >
ForwardIt unique( ForwardIt first, ForwardIt last, std::false_type ) {
    return trust::unique(first,last);
}
#endif

template< class ForwardIt >
ForwardIt unique( ForwardIt first, ForwardIt last ) {
    return unique(first,last,typename is_std_iterator<ForwardIt>::type());
}

template<typename InputIterator , typename OutputIterator >
OutputIterator copy(InputIterator first, InputIterator last, OutputIterator result, std::true_type) {
    return std::copy(first,last,result);
}

#ifdef __aboria_have_thrust__
template<typename InputIterator , typename OutputIterator >
OutputIterator copy(InputIterator first, InputIterator last, OutputIterator result, std::false_type) {
    return thrust::copy(first,last,result);
}
#endif

template<typename InputIterator , typename OutputIterator >
OutputIterator copy(InputIterator first, InputIterator last, OutputIterator result) {
    return copy(first,last,result, typename is_std_iterator<InputIterator>::type());
}

template<typename InputIterator, typename OutputIterator, typename UnaryFunction, 
    typename T, typename AssociativeOperator>
OutputIterator transform_exclusive_scan(
    InputIterator first, InputIterator last,
    OutputIterator result,
    UnaryFunction unary_op, T init, AssociativeOperator binary_op, std::true_type) {

    const size_t n = last-first;
    result[0] = init;
    for (int i=1; i<n; ++i) {
        result[i] = binary_op(result[i-1],unary_op(first[i-1]));
    }
    return result + n;
}

#ifdef __aboria_have_thrust__
template<typename InputIterator, typename OutputIterator, typename UnaryFunction, 
    typename T, typename AssociativeOperator>
OutputIterator transform_exclusive_scan(
    InputIterator first, InputIterator last,
    OutputIterator result,
    UnaryFunction unary_op, T init, AssociativeOperator binary_op, std::false_type) {

    return thrust::transform_exclusive_scan(first,last,result,unary_op,init,binary_op);
}
#endif

template<typename InputIterator, typename OutputIterator, typename UnaryFunction, 
    typename T, typename AssociativeOperator>
OutputIterator transform_exclusive_scan(
    InputIterator first, InputIterator last,
    OutputIterator result,
    UnaryFunction unary_op, T init, AssociativeOperator binary_op) {

    return transform_exclusive_scan(first,last,result,unary_op,init,binary_op, 
            typename is_std_iterator<InputIterator>::type());
}



template<typename InputIterator1, typename InputIterator2, 
    typename InputIterator3, typename RandomAccessIterator , typename Predicate >
void scatter_if(
        InputIterator1 first, InputIterator1 last,
        InputIterator2 map, InputIterator3 stencil,
        RandomAccessIterator output, Predicate pred, std::true_type) {

    const size_t n = last-first;
    for (int i=0; i<n; ++i) {
        if (pred(stencil[i])) {
            output[map[i]] = first[i];
        }
    }
}

#ifdef __aboria_have_thrust__
template<typename InputIterator1, typename InputIterator2, 
    typename InputIterator3, typename RandomAccessIterator , typename Predicate >
void scatter_if(
        InputIterator1 first, InputIterator1 last,
        InputIterator2 map, InputIterator3 stencil,
        RandomAccessIterator output, Predicate pred, std::false_type) {

    thrust::scatter_if(first,last,map,stencil,output,pred);
}
#endif

template<typename InputIterator1, typename InputIterator2, 
    typename InputIterator3, typename RandomAccessIterator , typename Predicate >
void scatter_if(
        InputIterator1 first, InputIterator1 last,
        InputIterator2 map, InputIterator3 stencil,
        RandomAccessIterator output, Predicate pred) {

    scatter_if(first,last,map,stencil,output,pred, typename is_std_iterator<InputIterator1>::type());
}

template<typename InputIterator , typename RandomAccessIterator , typename OutputIterator>
OutputIterator gather(InputIterator map_first, InputIterator map_last, 
                      RandomAccessIterator input_first, OutputIterator result, std::true_type) {
    std::transform(map_first,map_last,result, 
            [&input_first](typename InputIterator::value_type const &i){
                return input_first[i];
            });
}

#ifdef __aboria_have_thrust__
template<typename InputIterator , typename RandomAccessIterator , typename OutputIterator>
OutputIterator gather(InputIterator map_first, InputIterator map_last, 
                      RandomAccessIterator input_first, OutputIterator result, std::false_type) {
    thrust::gather(map_first,map_last,input_first,result);
}
#endif

template<typename InputIterator , typename RandomAccessIterator , typename OutputIterator>
OutputIterator gather(InputIterator map_first, InputIterator map_last, 
                      RandomAccessIterator input_first, OutputIterator result) {
    gather(map_first,map_last,input_first,result, typename is_std_iterator<RandomAccessIterator>::type());
}

template<typename InputIterator1, typename InputIterator2, 
    typename OutputIterator, typename Predicate>
OutputIterator copy_if(
        InputIterator1 first, InputIterator1 last, 
        InputIterator2 stencil, OutputIterator result, Predicate pred, std::true_type) {

    return std::copy_if(first,last,stencil,result,pred);
}



#ifdef __aboria_have_thrust__
template<typename InputIterator1, typename InputIterator2, 
    typename OutputIterator, typename Predicate>
OutputIterator copy_if(
        InputIterator1 first, InputIterator1 last, 
        InputIterator2 stencil, OutputIterator result, Predicate pred, std::false_type) {

    return thrust::copy_if(first,last,stencil,result,pred);
}
#endif

template<typename InputIterator1, typename InputIterator2, 
    typename OutputIterator, typename Predicate>
OutputIterator copy_if(
        InputIterator1 first, InputIterator1 last, 
        InputIterator2 stencil, OutputIterator result, Predicate pred) {

    return copy_if(first,last,stencil,result,pred,
                            typename is_std_iterator<InputIterator1>::type());
}

}
}

#endif
