#ifndef SOASORT_H
#define SOASORT_H
#include <algorithm>
#include <functional>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>
namespace soa_sort {

	constexpr bool THREADING = false;
	namespace {

		template <class Iterator>
		void apply_permutation(const std::vector<int>& indices,
			Iterator first)
		{

			std::vector<bool> done(indices.size());
			for (std::size_t i = 0; i < indices.size(); i++) {
				if (!done[i]) {
					done[i] = true;
					std::size_t prev_j = i;
					std::size_t j = indices[i];
					while (i != j) {
						std::iter_swap(first + prev_j, first + j);
						done[j] = true;
						prev_j = j;
						j = indices[j];
					}
				}
			}
		}

		// Base case for parameter packing.
		template <class Iterator>
		void sort(const std::vector<int>& indices, Iterator it)
		{
			apply_permutation(indices, it);
		}

		// Start a new thread for every apply permutation.
		template <class Iterator, class... Iterators>
		void sort(const std::vector<int>& indices, Iterator i1,
			Iterators... args)
		{
			if (THREADING) {
				std::thread t1(apply_permutation<Iterator>, indices, i1);
				sort(indices, args...);
				t1.join();
			}
			else {
				apply_permutation(indices, i1);
				sort(indices, args...);
			}
		}
	} // namespace

	// Sort the elements in range [first, last) with a custom comparator.
	// Apply the permutation determined by the [first, last) sort order to the remaining iterators
	// given by args.
	//
	// First and last determine the range of elements to sort.
	// The value of the elements from [first, last) determine the permutation which is
	// then applied to the remaining args.
	//
	// The args are iterators which point to starting point where the permutation will be applied.
	template <class Iterator, typename Compare, class... Iterators>
	void sort_cmp(
		Iterator first, Iterator last,
		Compare cmp,
		Iterators... args)
	{
		std::vector<int> indices(std::distance(first, last));
		std::iota(indices.begin(), indices.end(), 0);

		// Sort the indices using the values found in the first iterator.
		std::sort(indices.begin(), indices.end(),
			[first, cmp](const int& a, const int& b) {
				return cmp(*(first + a), *(first + b));
			});

		// Apply the calculated permutation to all other iterators.
		sort(indices, first, args...);
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
	template <class Iterator, class... Iterators>
	void sort(Iterator first, Iterator last, Iterators... args)
	{
		auto cmp = [](const decltype(*first)& a, const decltype(*first)& b) {
			return a < b;
		};

		sort_cmp(first, last, cmp, args...);
	}

} // namespace soa
#endif // SOASORT_H
