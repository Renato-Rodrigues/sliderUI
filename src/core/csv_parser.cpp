#include "core/csv_parser.h"
#include "core/file_utils.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cerrno>

namespace core {

CSVReader::CSVReader(char delimiter, bool allow_crlf)
  : delimiter_(delimiter), allow_crlf_(allow_crlf), rows_(), last_error_(std::nullopt) {}

CSVReader::~CSVReader() = default;

void CSVReader::clear() {
  rows_.clear();
  last_error_.reset();
}

std::optional<std::string> CSVReader::last_error() const {
  return last_error_;
}

bool CSVReader::load(const std::string &path) {
  clear();
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    last_error_ = std::string("open failed: ") + std::to_string(errno);
    return false;
  }

  // Read whole file into string to allow quoted newlines
  std::ostringstream ss;
  ss << in.rdbuf();
  std::string data = ss.str();

  std::vector<std::string> cur_row;
  std::string cur_field;
  enum State { OUTSIDE, IN_FIELD, IN_QUOTED_FIELD, IN_QUOTED_QUOTE } state = OUTSIDE;

  auto push_field = [&]() {
    cur_row.push_back(std::move(cur_field));
    cur_field.clear();
  };

  auto push_row = [&]() {
    // Even an empty line should become an empty row
    rows_.push_back(cur_row);
    cur_row.clear();
  };

  size_t i = 0;
  const size_t n = data.size();
  while (i < n) {
    char c = data[i];

    // handle CRLF by optionally skipping CR and treating LF as newline
    if (c == '\r' && allow_crlf_) {
      // skip CR, don't treat as newline by itself.
      ++i;
      continue;
    }

    switch (state) {
      case OUTSIDE:
        if (c == '"') {
          state = IN_QUOTED_FIELD;
          ++i;
        } else if (c == delimiter_) {
          // empty field
          push_field();
          state = OUTSIDE;
          ++i;
        } else if (c == '\n') {
          // end of row
          push_field();
          push_row();
          state = OUTSIDE;
          ++i;
        } else {
          // start unquoted field
          state = IN_FIELD;
          cur_field.push_back(c);
          ++i;
        }
        break;

      case IN_FIELD:
        if (c == delimiter_) {
          push_field();
          state = OUTSIDE;
          ++i;
        } else if (c == '\n') {
          push_field();
          push_row();
          state = OUTSIDE;
          ++i;
        } else {
          cur_field.push_back(c);
          ++i;
        }
        break;

      case IN_QUOTED_FIELD:
        if (c == '"') {
          // possible end or escaped quote
          state = IN_QUOTED_QUOTE;
          ++i;
        } else {
          // accept any char including newline and delimiter
          cur_field.push_back(c);
          ++i;
        }
        break;

      case IN_QUOTED_QUOTE:
        if (c == '"') {
          // escaped quote -> add one quote and return to quoted state
          cur_field.push_back('"');
          state = IN_QUOTED_FIELD;
          ++i;
        } else if (c == delimiter_) {
          // end of field
          push_field();
          state = OUTSIDE;
          ++i;
        } else if (c == '\n') {
          // end of row
          push_field();
          push_row();
          state = OUTSIDE;
          ++i;
        } else {
          // according to RFC, after a closing quote only delimiter or newline allowed.
          // However be permissive: treat as end of quoted field and then the char belongs to next unquoted field.
          state = IN_FIELD;
          // do not consume c here; loop will re-handle it in IN_FIELD
        }
        break;
    }
  } // while

  // End of file reached: finalize depending on state
  if (state == IN_QUOTED_FIELD || state == IN_QUOTED_QUOTE) {
    // If we ended inside a quoted field, accept it as end-of-field (be tolerant)
    if (state == IN_QUOTED_QUOTE) {
      // closing quote seen; treat as end of field
      push_field();
      push_row();
    } else {
      // unterminated quoted field: accept field as-is
      push_field();
      push_row();
    }
  } else if (state == IN_FIELD) {
    push_field();
    push_row();
  } else if (state == OUTSIDE) {
    // If the file ended right after a delimiter, we should have pushed an empty field earlier.
    // But if no chars were read at all and nothing was pushed, we don't add an empty row.
    if (!cur_row.empty() || !cur_field.empty()) {
      if (!cur_field.empty()) push_field();
      push_row();
    }
  }

  return true;
}

static bool needs_quoting(const std::string &field, char delimiter) {
  if (field.empty()) return false; // optional: empty field doesn't strictly need quotes
  for (char c : field) {
    if (c == '"' || c == '\n' || c == '\r' || c == delimiter) return true;
  }
  return false;
}

static std::string quote_field(const std::string &field) {
  std::string out;
  out.reserve(field.size() + 2);
  out.push_back('"');
  for (char c : field) {
    if (c == '"') {
      out.push_back('"'); // escape by doubling
      out.push_back('"');
    } else {
      out.push_back(c);
    }
  }
  out.push_back('"');
  return out;
}

bool CSVReader::save(const std::string &path, const std::vector<std::vector<std::string>> &rows) const {
  std::ostringstream ss;
  for (size_t r = 0; r < rows.size(); ++r) {
    const auto &row = rows[r];
    for (size_t f = 0; f < row.size(); ++f) {
      const std::string &field = row[f];
      if (needs_quoting(field, delimiter_)) {
        ss << quote_field(field);
      } else {
        ss << field;
      }
      if (f + 1 < row.size()) ss << delimiter_;
    }
    // LF line ending
    ss << '\n';
  }

  return file_utils::atomic_write(path, ss.str());
}

std::vector<std::vector<std::string>> CSVReader::rows() const {
  return rows_;
}

} // namespace core
