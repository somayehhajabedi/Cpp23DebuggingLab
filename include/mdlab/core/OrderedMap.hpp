#pragma once

// Price levels want a container kept sorted by price with cheap iteration of
// the best few levels. std::flat_map (C++23) is the natural fit, but it is
// new enough that not every standard library ships it yet, so we probe for
// it and fall back to std::map transparently.
#if __has_include(<flat_map>)
#include <flat_map>
#define MDLAB_HAVE_FLAT_MAP 1
#else
#include <map>
#define MDLAB_HAVE_FLAT_MAP 0
#endif

#include <functional>

namespace mdlab::core {

#if MDLAB_HAVE_FLAT_MAP
template <typename Key, typename Value, typename Compare = std::less<Key>>
using OrderedMap = std::flat_map<Key, Value, Compare>;
#else
template <typename Key, typename Value, typename Compare = std::less<Key>>
using OrderedMap = std::map<Key, Value, Compare>;
#endif

}  // namespace mdlab::core
