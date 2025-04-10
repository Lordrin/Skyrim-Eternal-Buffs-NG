#pragma once

#include <string>
#include <string_view>  // C++17, efficient for splitting/trimming
#include <vector>
#include <algorithm>  // For std::transform, std::find_if
#include <cctype>     // For ::isspace, ::tolower
#include <sstream>    // Used in one SplitString implementation option

namespace Utilities {

    /**
     * @brief Removes leading and trailing whitespace from a string.
     * @param str The string to trim.
     * @return A new string with whitespace removed from both ends.
     */
    std::string TrimString(const std::string& str);

    /**
     * @brief Removes leading and trailing whitespace from a string_view (C++17). More efficient as it avoids copying.
     * @param sv The string_view to trim.
     * @return A string_view representing the trimmed portion.
     */
    std::string_view TrimStringView(std::string_view sv);

    /**
     * @brief Splits a string into a vector of strings based on a delimiter character.
     * @param str The string to split.
     * @param delimiter The character to split by.
     * @param skipEmpty If true, empty strings resulting from consecutive delimiters are omitted.
     * @return A vector of strings.
     */
    std::vector<std::string> SplitString(const std::string& str, char delimiter, bool skipEmpty = true);

    /**
     * @brief Splits a string_view into a vector of string_views (C++17). Avoids string allocations during splitting.
     * @param sv The string_view to split.
     * @param delimiter The character to split by.
     * @param skipEmpty If true, empty string_views resulting from consecutive delimiters are omitted.
     * @return A vector of string_views.
     */
    std::vector<std::string_view> SplitStringView(std::string_view sv, char delimiter, bool skipEmpty = true);

    /**
     * @brief Converts a string to lowercase.
     * @param str The string to convert.
     * @return A new string in lowercase.
     */
    std::string ToLower(const std::string& str);

}
