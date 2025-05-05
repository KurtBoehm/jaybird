// This file is part of https://github.com/KurtBoehm/jaybird.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef INCLUDE_JAYBIRD_IO_IO_HPP
#define INCLUDE_JAYBIRD_IO_IO_HPP

#include <filesystem>

#include "thesauros/io/file-reader.hpp"

#include "jaybird/base/defs.hpp"

namespace jay {
inline Json read_file(thes::FileReader reader) {
  return Json::parse(reader.handle());
}
inline Json read_file(const std::filesystem::path& p) {
  return read_file(thes::FileReader{p});
}
} // namespace jay

#endif // INCLUDE_JAYBIRD_IO_IO_HPP
