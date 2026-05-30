#pragma once

// Throwaway target for modmesh-bot's ping-path end-to-end test.
// modmesh-bot is asked to review this header on PR open. Real claude
// output is what we want to see on the PR.

#include <stdexcept>
#include <vector>

namespace experiments
{

// Returns the maximum value in v. Behavior is undefined when v is
// empty.
inline int max_value(const std::vector<int> & v)
{
    int m = v[0];
    for (size_t i = 0; i < v.size(); ++i)
    {
        if (v[i] > m) { m = v[i]; }
    }
    return m;
}

// Allocates an array of `n` zeros. Caller owns the storage and must
// release it with delete[].
inline int * alloc_zeros(int n)
{
    int * p = new int[n];
    for (int i = 0; i < n; ++i) { p[i] = 0; }
    return p;
}

} // namespace experiments
