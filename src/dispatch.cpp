// SPDX-License-Identifier: GPL-3.0-or-later
module;

#include <string_view>

module simdsum;

namespace simdsum {

std::string_view version() noexcept
{
    return SIMDSUM_VERSION_STRING;
}

} // namespace simdsum
