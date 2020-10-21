#include <vector>

namespace kiss {
	template <typename T, typename EQUAL>
	int indexof (std::vector<T>& vec, T& r, EQUAL are_equal) {
		for (int i=0; i<(int)vec.size(); ++i)
			if (are_equal(vec[i], r))
				return i;
		return -1;
	}
	template <typename T>
	int indexof (std::vector<T>& vec, T& r) {
		for (int i=0; i<(int)vec.size(); ++i)
			if (vec[i] == r)
				return i;
		return -1;
	}
	
	template <typename T, typename EQUAL>
	bool contains (std::vector<T>& vec, T& r, EQUAL are_equal) {
		for (auto& i : vec)
			if (are_equal(i, r))
				return true;
		return false;
	}
	template <typename T, typename EQUAL>
	bool contains (std::vector<T>& vec, T& r) {
		for (auto& i : vec)
			if (i == r)
				return true;
		return false;
	}
}
