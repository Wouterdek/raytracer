#ifndef SOASORT_H
#define SOASORT_H
#include <algorithm>
#include <functional>
#include <numeric>
#include <vector>
#include <tuple>
#include <cassert>

#if defined(SOASORT_USE_TBB_PARALLEL)
#include <tbb/tbb.h>
#include <tbb/parallel_sort.h>
#define PAR_FOR(start, end, func) tbb::parallel_for(start, end, func);
#define PAR_SORT(begin, end, comparator) tbb::parallel_sort(begin, end, comparator);
#elif defined(SOASORT_USE_STD_PARALLEL)
#include <thread>
  #include <execution>
  // Can't do parallel index-based for loop in std cpp17
  #define PAR_FOR(start, end, func) for(auto i = start; i < end; ++i) { func(i); }
  #define PAR_SORT(begin, end, comparator) std::sort(std::execution::par_unseq, begin, end, comparator);
#else
  // Fallback to sequential algorithms
  #define PAR_FOR(start, end, func) for(auto i = start; i < end; ++i) { func(i); }
  #define PAR_SORT(begin, end, comparator) std::sort(begin, end, comparator);
#endif


namespace soa_sort {

    template<bool AllowParallelization>
    struct soa_sort_implementation {
        // Apply a permutation to one iterator.
        // Each element at position i is moved to indices[i]
        template <class Iterator>
        static void apply_permutation(const std::vector<int>& indices, Iterator begin)
        {
            auto indicesSize = indices.size();
            std::vector<bool> done(indicesSize);
            for (std::size_t i = 0; i < indicesSize; ++i) {
                if (!done[i]) {
                    done[i] = true;
                    std::size_t prev_j = i;
                    std::size_t j = indices[i];
                    while (i != j) {
                        std::iter_swap(begin + prev_j, begin + j);
                        done[j] = true;
                        prev_j = j;
                        j = indices[j];
                    }
                }
            }
        }

        // Base case, apply a permutation to the head element if i == acc, error otherwise
        template <class Head>
        static void apply_permutation_to_ith_iterator(const std::vector<int>& indices, unsigned int i, unsigned int acc, Head head)
        {
            if(acc == i) {
                apply_permutation(indices, head);
            } else {
                // This should never happen
                assert(false);
            }
        }

        // Apply a permutation to the ith element in a list.
        // If i == acc, then we have reached the target element and apply_permutation is called.
        // Else, recurse with the tail of the list and incremented acc.
        // Recursion is used because parameter packs cannot be indexed by a runtime index.
        template <class Head, class... Tail>
        static void apply_permutation_to_ith_iterator(const std::vector<int>& indices, unsigned int i, unsigned int acc, Head head, Tail... tail)
        {
            if(acc == i){
                apply_permutation(indices, head);
            }else{
                apply_permutation_to_ith_iterator(indices, i, acc+1, tail...);
            }
        }

        // Apply a permutation to multiple iterators.
        template <class... Iterators>
        static void apply_permutation(const std::vector<int>& indices, Iterators... args)
        {
            auto iteratorCount = sizeof...(Iterators);
            auto func = [&indices, &args...](auto i){
                apply_permutation_to_ith_iterator(indices, i, 0, args...);
            };
            decltype(iteratorCount) start = 0;

            if(AllowParallelization) {
                PAR_FOR(start, iteratorCount, func)
            } else {
                for (auto i = start; i < iteratorCount; ++i) {
                    func(i);
                }
            }
        }
    };

    // Sort the elements in range [first, last) with a custom comparator.
    // Apply the permutation determined by the [first, last) sort order to the remaining iterators
    // given by args.
    //
    // First and last determine the range of elements to sort.
    // The value of the elements from [first, last) determine the permutation which is
    // then applied to the remaining args.
    //
    // The args are iterators which point to starting point where the permutation will be applied.
    template <bool AllowParallelization, class Iterator, typename Compare, class... Iterators>
    void sort_cmp(
            Iterator first, Iterator last,
            Compare cmp,
            Iterators... args)
    {
        std::vector<int> indices(std::distance(first, last));
        std::iota(indices.begin(), indices.end(), 0);

        // Sort the indices using the values found in the first iterator.
        auto begin = indices.begin();
        auto end = indices.end();
        auto comparator = [first, cmp](const int& a, const int& b) {
            return cmp(*(first + a), *(first + b));
        };
        if(AllowParallelization) {
            PAR_SORT(begin, end, comparator)
        } else {
            std::sort(begin, end, comparator);
        }

        // Apply the calculated permutation to all other iterators.
        soa_sort_implementation<AllowParallelization>::apply_permutation(indices, first, args...);
    }

    // Sort the elements in range [first, last) in ascending order.
    // Apply the permutation determined by the [first, last) sort order to the remaining iterators
    // given by args.
    //
    // The parameters first, last determine the range of elements to sort.
    // The value of the elements from [first, last) determine the permutation which is then applied
    // to the remaining args.
    //
    // The args are iterators which point to starting point where the permutation will be applied.
    template <bool AllowParallelization, class Iterator, class... Iterators>
    void sort(Iterator first, Iterator last, Iterators... args)
    {
        auto cmp = [](const decltype(*first)& a, const decltype(*first)& b) {
            return a < b;
        };

        sort_cmp<AllowParallelization>(first, last, cmp, args...);
    }

} // namespace soa
#endif // SOASORT_H

