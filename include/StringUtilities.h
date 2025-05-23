#pragma once

#include <cctype>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <string_view>

namespace StringUtilities {

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


    /**
     * @brief Joins a vector of strings into a single string with a specified delimiter.
     * @param strings The vector of strings to join.
     * @param delimiter The delimiter to use between strings.
     * @return A single string with the joined values.
     */
    std::string Join(const std::vector<std::string>& strings, const std::string& delimiter = ", ");



    /**
     * @brief Removes all whitespace from a string.
     * @param str The string to process.
     * @return A new string with all whitespace removed.
     */
    std::string RemoveWhitespace(const std::string& str);

}
