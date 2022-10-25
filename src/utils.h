#pragma once

#include <robin_hood.h>
#include <toml++/toml.h>

struct StrHash : robin_hood::hash<std::string>
{
    using is_transparent = void;

    inline std::size_t operator()(const std::string& key) const { return robin_hood::hash_bytes(key.c_str(), key.size()); }
    inline std::size_t operator()(std::string_view key) const { return robin_hood::hash_bytes(key.data(), key.size()); }
    inline std::size_t operator()(const char* key) const { return robin_hood::hash_bytes(key, std::strlen(key)); }
};

struct StrEq
{
    using is_transparent = int;

    inline bool operator()(std::string_view lhs, const std::string& rhs) const
    {
        const std::string_view view = rhs;
        return lhs == view;
    }

    inline bool operator()(const char* lhs, const std::string& rhs) const
    {
        return std::strcmp(lhs, rhs.c_str()) == 0;
    }

    inline bool operator()(const std::string& lhs, const std::string& rhs) const
    {
        return lhs == rhs;
    }
};

template <typename T>
using StrMap = robin_hood::unordered_map<std::string, T, StrHash, StrEq>;
using StrSet = robin_hood::unordered_set<std::string, StrHash, StrEq>;

std::string joinTags(const StrSet& tags, bool sorted = true);
StrSet      splitTags(const std::string& str);

inline void mergeStrSet(StrSet& to, const StrSet& from)
{
    for (auto& str : from)
        to.emplace(str);
}
inline toml::array strSet2TomlArray(const StrSet& set)
{
    toml::array arr;
    for (auto& v : set)
        arr.push_back(v);
    return arr;
}
inline StrSet tomlArray2StrSet(const toml::array& arr)
{
    StrSet set;
    for (auto& v : arr)
        if (v.is_string())
            set.insert(v.ref<std::string>());
    return set;
}

// only for the simple flat structure used in the code
// don't use it for nested tables etc.
bool isSameStructure(const toml::table& a, const toml::table& b);