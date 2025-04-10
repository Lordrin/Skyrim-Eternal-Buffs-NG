#include "StringUtilities.h"

namespace Utilities {

    // --- TrimString Implementations ---

    std::string TrimString(const std::string& str) {
        // Find the first non-whitespace character
        auto first = std::find_if_not(str.begin(), str.end(), [](unsigned char c) { return std::isspace(c); });
        // If the string is all whitespace, return empty
        if (first == str.end()) {
            return "";
        }
        // Find the last non-whitespace character
        auto last = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char c) { return std::isspace(c); }).base();

        // Construct a new string from the non-whitespace range
        return std::string(first, last);
    }

    std::string_view TrimStringView(std::string_view sv) {
        // Find the first non-whitespace character
        const auto first = sv.find_first_not_of(" \t\n\r\f\v");  // Standard whitespace chars
        if (first == std::string_view::npos) {
            return "";  // All whitespace
        }
        // Find the last non-whitespace character
        const auto last = sv.find_last_not_of(" \t\n\r\f\v");
        // Return the subview
        return sv.substr(first, (last - first + 1));
    }

    // --- SplitString Implementations ---

    // Option 1: Using stringstream (often simpler to read)
    std::vector<std::string> SplitString(const std::string& str, char delimiter, bool skipEmpty) {
        std::vector<std::string> result;
        std::stringstream ss(str);
        std::string segment;

        while (std::getline(ss, segment, delimiter)) {
            if (!skipEmpty || !segment.empty()) {
                result.push_back(segment);
            }
        }
        // Handle case where the string ends with the delimiter (getline might miss the last empty segment)
        if (!str.empty() && str.back() == delimiter && !skipEmpty) {
            result.push_back("");
        }
        return result;
    }

    // Option 2: Using string_view (more efficient, C++17)
    std::vector<std::string_view> SplitStringView(std::string_view sv, char delimiter, bool skipEmpty) {
        std::vector<std::string_view> result;
        std::string_view::size_type start = 0;
        for (std::string_view::size_type i = 0; i < sv.size(); ++i) {
            if (sv[i] == delimiter) {
                std::string_view segment = sv.substr(start, i - start);
                if (!skipEmpty || !segment.empty()) {
                    result.push_back(segment);
                }
                start = i + 1;
            }
        }
        // Add the last segment after the loop
        std::string_view lastSegment = sv.substr(start);
        if (!skipEmpty || !lastSegment.empty()) {
            result.push_back(lastSegment);
        }
        return result;
    }

    // If you need SplitString to return std::vector<std::string> but want efficiency of string_view splitting:
    /*
    std::vector<std::string> SplitString(const std::string& str, char delimiter, bool skipEmpty = true) {
        std::vector<std::string> result;
        std::string_view sv(str);
        auto views = SplitStringView(sv, delimiter, skipEmpty);
        result.reserve(views.size()); // Optimize allocation
        for(const auto& view : views) {
            result.emplace_back(view); // Construct string from view
        }
        return result;
    }
    */

    // --- ToLower Implementation ---

    std::string ToLower(const std::string& str) {
        std::string lower_str = str;  // Create a mutable copy
        std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return lower_str;
    }

}  // namespace Utilities