// SPDX-License-Identifier: GPL-3.0-or-later
module;

#include <string_view>

#include "simdsum_export.h"

export module simdsum;

export namespace simdsum {

/// Library version, "MAJOR.MINOR.PATCH".
SIMDSUM_API std::string_view version() noexcept;

} // namespace simdsum
