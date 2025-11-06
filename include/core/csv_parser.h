#pragma once
#ifndef SLIDERUI_CORE_CSV_PARSER_H
#define SLIDERUI_CORE_CSV_PARSER_H

#include <string>
#include <vector>
#include <optional>

namespace core {

/**
 * CSVReader
 *
 * Minimal CSV reader/writer tailored for sliderUI:
 * - UTF-8 input
 * - newline: LF (but loader should be tolerant)
 * - delimiter: semicolon ';' (configurable via constructor)
 * - supports quoted fields: "field;with;delim" and escaped quotes using double quotes: ""
 *
 * This header declares the interface only; implementation lives in src/core/csv_parser.cpp.
 */
class CSVReader {
public:
  /**
   * Construct a CSVReader.
   * @param delimiter character used to split fields (default ';' for sliderUI)
   * @param allow_crlf if true accept CRLF line endings; otherwise treat CRLF as CR + LF (default true)
   */
  explicit CSVReader(char delimiter = ';', bool allow_crlf = true);

  ~CSVReader();

  CSVReader(const CSVReader&) = delete;
  CSVReader& operator=(const CSVReader&) = delete;
  CSVReader(CSVReader&&) = default;
  CSVReader& operator=(CSVReader&&) = default;

  /**
   * Load CSV from disk into memory.
   * Returns true on success, false on I/O or parse error.
   * On success, rows() returns parsed content.
   */
  bool load(const std::string &path);

  /**
   * Return parsed rows as vector of rows, each row is a vector<string> of fields.
   * Use this after successful load().
   */
  std::vector<std::vector<std::string>> rows() const;

  /**
   * Save rows to disk using LF line endings and quoting as needed.
   * Returns true on success (uses atomic write in implementation).
   */
  bool save(const std::string &path, const std::vector<std::vector<std::string>> &rows) const;

  /**
   * Clear internal rows buffer.
   */
  void clear();

  /**
   * If load() fails due to parse error, this returns an optional error message.
   */
  std::optional<std::string> last_error() const;

private:
  char delimiter_;
  bool allow_crlf_;
  std::vector<std::vector<std::string>> rows_;
  std::optional<std::string> last_error_;

  // Implementation helper functions will be in .cpp:
  // - parse_line(const std::string& line, std::vector<std::string>& out_fields)
  // - quote_field_for_output(const std::string& field)
};

} // namespace core

#endif // SLIDERUI_CORE_CSV_PARSER_H
