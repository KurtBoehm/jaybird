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
