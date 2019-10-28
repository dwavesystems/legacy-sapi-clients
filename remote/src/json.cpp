//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifdef _MSC_VER
#include <cstdarg>
#endif

#include <cstddef>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <limits>
#include <memory>
#include <stack>
#include <string>
#include <vector>

#include <boost/variant.hpp>
#include <boost/foreach.hpp>

#include <json.hpp>

#ifdef ENABLE_DEBUG_NEW
#include "debug-new.hpp"
#endif

using std::size_t;
using std::isspace;
using std::isdigit;
using std::strtod;
using std::numeric_limits;
using std::stack;
using std::string;
using std::unique_ptr;
using std::vector;

#ifdef _MSC_VER
#define strtoll _strtoi64
#else
using std::strtoll;
using std::snprintf;
#endif


namespace {

#ifdef _MSC_VER
int vsnprintf(char* str, size_t size, const char* format, va_list ap) {
    int count = -1;
    if (size != 0) count = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
    if (count == -1) count = _vscprintf(format, ap);
    return count;
}

int snprintf(char* str, size_t size, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    int count = vsnprintf(str, size, format, ap);
    va_end(ap);
    return count;
}
#endif

//=========================================================================================================
//
// writing
//

class ToStringVisitor : public boost::static_visitor<> {
private:
  string s_;
  vector<char> buf_;

  void appendDouble(double d) {
    bool done = true;
    do {
      size_t written = static_cast<size_t>(
          snprintf(buf_.data(), buf_.size(), "%.*g", 2 + std::numeric_limits<double>::digits * 3010/10000, d));
      done = written < buf_.size();
      if (!done) buf_.resize(written + 1);
    } while (!done);
    s_.append(buf_.data());
  }

  void appendLongLong(long long l) {
    bool done = true;
    do {
      size_t written = static_cast<size_t>(snprintf(buf_.data(), buf_.size(), "%lld", l));
      done = written < buf_.size();
      if (!done) buf_.resize(written + 1);
    } while (!done);
    s_.append(buf_.data());
  }

  void appendUEscape(unsigned c) {
    bool done = true;
    do {
      size_t written = static_cast<size_t>(snprintf(buf_.data(), buf_.size(), "\\u%04x", c));
      done = written < buf_.size();
      if (!done) buf_.resize(written + 1);
    } while (!done);
    s_.append(buf_.data());
  }

  void appendString(const string& s) {
    s_.append(1, '"');
    BOOST_FOREACH( char c, s ) {
      switch (c) {
        case 0x08: s_.append("\\b"); break; // backspace
        case 0x0c: s_.append("\\f"); break; // form feed
        case 0x0a: s_.append("\\n"); break; // newline
        case 0x0d: s_.append("\\r"); break; // carriage return
        case 0x09: s_.append("\\t"); break; // tab
        case 0x22: s_.append("\\\""); break; // quotation mark
        case 0x5c: s_.append("\\\\"); break; // backslash
        default:
          if (static_cast<unsigned char>(c) >= 0x20) {
            s_.append(1, c);
          } else {
            appendUEscape(static_cast<unsigned char>(c));
          }
          break;
      }
    }
    s_.append(1, '"');
  }

public:
  ToStringVisitor() : buf_(20) {}
  void operator()(const json::Null&) { s_.append("null"); }
  void operator()(bool b) { s_.append(b ? "true" : "false"); }
  void operator()(double d) { appendDouble(d); }
  void operator()(long long l) { appendLongLong(l); }
  void operator()(const string& s) { appendString(s); }

  void operator()(const json::Array& array) {
    s_.append(1, '[');
    size_t itemsLeft = array.size();
    BOOST_FOREACH( const json::Value& item, array ) {
      boost::apply_visitor(*this, item.variant());
      if (--itemsLeft > 0) s_.append(1, ',');
    }
    s_.append(1, ']');
  }

  void operator()(const json::Object& object) {
    s_.append(1, '{');
    size_t itemsLeft = object.size();
    BOOST_FOREACH( const json::Object::value_type& item, object ) {
      appendString(item.first);
      s_.append(1, ':');
      boost::apply_visitor(*this, item.second.variant());
      if (--itemsLeft > 0) s_.append(1, ',');
    }
    s_.append(1, '}');
  }

  string str() { return s_; }
};

//=========================================================================================================
//
// parsing
//

class OpenContainer {
public:
  virtual ~OpenContainer() {}
  virtual bool addValue(json::Value&, char next) = 0;
  virtual json::Value finalResult() { badFinalResult(); return json::Value(); }
  virtual json::Value closeArray() { badCloseArray(); return json::Value(); }
  virtual json::Value closeObject() { badCloseObject(); return json::Value(); }
  static void badFinalResult() { throw json::ParseException("unexpected end of input"); }
  static void badCloseArray() { throw json::ParseException("mismatched ']'"); }
  static void badCloseObject() { throw json::ParseException("mismatched '}'"); }
};

class Sentinel : public OpenContainer {
private:
  json::Value v_;
  bool done_;
public:
  Sentinel() : v_(), done_(false) {}
  virtual bool addValue(json::Value& value, char next) {
    if (next != '\0') throw json::ParseException("trailing garbage");
    v_ = std::move(value);
    done_ = true;
    return false;
  }
  virtual json::Value finalResult() {
    if (!done_) OpenContainer::badFinalResult();
    return std::move(v_);
  }
};


class OpenArray : public OpenContainer {
private:
  json::Value v_;
  json::Array& array_; // ref to v_
  bool closeable_;
public:
  OpenArray() : v_(json::Array()), array_(v_.getArray()), closeable_(true) {}
  virtual bool addValue(json::Value& value, char next) {
    array_.push_back(std::move(value));
    switch (next) {
      case ',': closeable_ = false; return true;
      case ']': closeable_ = true; return false;
      default: throw json::ParseException("invalid array separator");
    }
  }
  virtual json::Value closeArray() {
    if (!closeable_) throw json::ParseException("dangling array separator");
    return std::move(v_);
  }
};

class OpenObject : public OpenContainer {
private:
  json::Value v_;
  json::Object& object_; // ref to v_
  string key_;
  bool expectKey_;
public:
  OpenObject() : v_(json::Object()), object_(v_.getObject()), key_(), expectKey_(true) {}
  virtual bool addValue(json::Value& value, char next) {
    if (expectKey_) {
      if (!value.isString()) throw json::ParseException("invalid object key");
      key_.swap(value.getString());
      expectKey_ = false;
      if (next != ':') throw json::ParseException("invalid key/value separator");
      return true;
    } else {
      object_[std::move(key_)].swap(value);
      expectKey_ = true;
      switch (next) {
        case ',': return true;
        case '}': return false;
        default: throw json::ParseException("invalid array separator");
      }
    }
  }
  virtual json::Value closeObject() {
    if (!expectKey_) throw json::ParseException("dangling object separator");
    return std::move(v_);
  }
};

typedef stack<unique_ptr<OpenContainer>> ContainerStack;

class JsonStack {
  ContainerStack containerStack_;
  bool done_;

public:
  JsonStack() : containerStack_(), done_(false) {
    containerStack_.push(unique_ptr<OpenContainer>(new Sentinel));
  }
  void openArray() { containerStack_.emplace(new OpenArray); }
  void openObject() { containerStack_.emplace(new OpenObject); }

  bool addValue(json::Value value, char next) {
    return containerStack_.top()->addValue(value, next);
  }

  bool closeArray(char next) {
    json::Value closed = containerStack_.top()->closeArray();
    containerStack_.pop();
    return containerStack_.top()->addValue(closed, next);
  }

  bool closeObject(char next) {
    json::Value closed = containerStack_.top()->closeObject();
    containerStack_.pop();
    return containerStack_.top()->addValue(closed, next);
  }

  json::Value result() {
    return std::move(containerStack_.top()->finalResult());
  }
};

json::Value parseNumber(const char*& sp) {
  char* endPtr;

  errno = 0;
  long long l = strtoll(sp, &endPtr, 10);
  if (errno == 0 && *endPtr != '.' && *endPtr != 'e' && *endPtr != 'E') {
    sp = endPtr;
    return json::Value(l);
  }

  errno = 0;
  double d = strtod(sp, &endPtr);
  if (errno == ERANGE) throw json::Value("number out of range");
  sp = endPtr;
  return json::Value(d);
}

unsigned hexval(char c) {
  switch (tolower(c)) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'a': return 10;
    case 'b': return 11;
    case 'c': return 12;
    case 'd': return 13;
    case 'e': return 14;
    case 'f': return 15;
    default: throw json::ParseException("invalid \\u value");
  }
}

unsigned unescape(const char*& sp) {
  switch (*sp) {
    case '"': return '"';
    case '\\': return '\\';
    case '/': return '/';
    case 'b': return 0x08;
    case 'f': return 0x0c;
    case 'n': return 0x0a;
    case 'r': return 0x0d;
    case 't': return 0x09;
    case 'u':
    {
      unsigned u = 0;
      u = u * 16 + hexval(*++sp);
      u = u * 16 + hexval(*++sp);
      u = u * 16 + hexval(*++sp);
      u = u * 16 + hexval(*++sp);
      return u;
    }
    default: throw json::ParseException("invalid escape sequence");
  }
}

void appendUtf8(string& s, unsigned long u) {
  if (u < 0x80u) {
    s.append(1, static_cast<char>(u));
  } else if (u < 0x800u) {
    s.append(1, static_cast<char>(0xc0 + (u >> 6)));
    s.append(1, static_cast<char>(0x80 + (u & 0x3f)));
  } else if (u < 0x10000u) {
    s.append(1, static_cast<char>(0xe0 + (u >> 12)));
    s.append(1, static_cast<char>(0x80 + ((u >> 6) & 0x3f)));
    s.append(1, static_cast<char>(0x80 + (u & 0x3f)));
  } else {
    s.append(1, static_cast<char>(0xf0 + (u >> 18)));
    s.append(1, static_cast<char>(0x80 + ((u >> 12) & 0x3f)));
    s.append(1, static_cast<char>(0x80 + ((u >> 6) & 0x3f)));
    s.append(1, static_cast<char>(0x80 + (u & 0x3f)));
  }
}

json::Value parseString(const char*& sp) {
  const unsigned leadSurrogateBegin = 0xd800u;
  const unsigned surrogateMask = 0x3ff;
  const unsigned trailSurrogateBegin = 0xdc00u;
  const unsigned trailSurrogateEnd = 0xe000u;
  json::Value v((string()));
  string& s = v.getString();
  unsigned leadSurrogate = 0;

  for (;; ++sp) {
    if (leadSurrogate != 0) {
      if (*sp != '\\') throw json::ParseException("missing trail surrogate");
      ++sp;
      unsigned trailSurrogate = unescape(sp);
      if (trailSurrogate < trailSurrogateBegin || trailSurrogate >= trailSurrogateEnd) {
        throw json::ParseException("invalid trail surrogate");
      }
      appendUtf8(s, 0x10000u | ((leadSurrogate & surrogateMask) << 10) | (trailSurrogate & surrogateMask));
      leadSurrogate = 0;
    } else {
      switch (*sp) {
        case '\0': throw json::ParseException("unexpected end of input");
        case '"' : ++sp; return v;
        case '\\':
          ++sp;
          leadSurrogate = unescape(sp);
          if (leadSurrogate < leadSurrogateBegin || leadSurrogate >= trailSurrogateEnd) {
            appendUtf8(s, leadSurrogate);
            leadSurrogate = 0;
          } else if (leadSurrogate >= trailSurrogateBegin) {
            throw json::ParseException("unexpected trail surrogate");
          }
          break;
        default:
          {
            auto sp0 = std::strpbrk(sp, "\\\"");
            if (sp0) {
              s.append(sp, static_cast<size_t>(sp0 - sp));
              sp = sp0 - 1;
              break;
            } else {
              throw json::ParseException("unexpected end of input");
            }
          }
      }
    }
  }
}

const char* eatSpace(const char* sp) {
  while (isspace(*sp)) ++sp;
  return sp;
}

void badCharacter() {
  throw json::ParseException("invalid character");
}

} // namespace {anonymous}


//=========================================================================================================
//
// interface implementation
//

namespace json {

std::string jsonToString(const Value& v) {
#ifdef ENABLE_DEBUG_NEW
  mem_debug::DeactivateThisThread mddtt;
#endif
  ToStringVisitor visitor;
  boost::apply_visitor(visitor, v.variant());
  return visitor.str();
}

std::string jsonToString(const Array& v) {
#ifdef ENABLE_DEBUG_NEW
  mem_debug::DeactivateThisThread mddtt;
#endif
  ToStringVisitor visitor;
  visitor(v);
  return visitor.str();
}

std::string jsonToString(const Object& v) {
#ifdef ENABLE_DEBUG_NEW
  mem_debug::DeactivateThisThread mddtt;
#endif
  ToStringVisitor visitor;
  visitor(v);
  return visitor.str();
}

Value stringToJson(const std::string& s) {
#ifdef ENABLE_DEBUG_NEW
  mem_debug::DeactivateThisThread mddtt;
#endif
  const char* sp = s.c_str();
  JsonStack jsonStack;

  for (;;) {
    sp = eatSpace(sp);
    switch (*sp) {
      case '\0': return jsonStack.result();
      case '[': ++sp; jsonStack.openArray(); break;
      case '{': ++sp; jsonStack.openObject(); break;
      case ']':
        sp = eatSpace(sp + 1);
        if (jsonStack.closeArray(*sp)) ++sp;
        break;
      case '}':
        sp = eatSpace(sp + 1);
        if (jsonStack.closeObject(*sp)) ++sp;
        break;
      case 'n':
        if (*++sp != 'u' || *++sp != 'l' || *++sp != 'l') badCharacter();
        sp = eatSpace(sp + 1);
        if (jsonStack.addValue(json::Null(), *sp)) ++sp;
        break;
      case 'f':
        if (*++sp != 'a' || *++sp != 'l' || *++sp != 's' || *++sp != 'e') badCharacter();
        sp = eatSpace(sp + 1);
        if (jsonStack.addValue(false, *sp)) ++sp;
        break;
      case 't':
        if (*++sp != 'r' || *++sp != 'u' || *++sp != 'e') badCharacter();
        sp = eatSpace(sp + 1);
        if (jsonStack.addValue(true, *sp)) ++sp;
        break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '-':
      {
        json::Value val = parseNumber(sp);
        sp = eatSpace(sp);
        if (jsonStack.addValue(std::move(val), *sp)) ++sp;
        break;
      }
      case '"':
      {
        ++sp;
        json::Value val = parseString(sp);
        sp = eatSpace(sp);
        if (jsonStack.addValue(std::move(val), *sp)) ++sp;
        break;
      }
      default: badCharacter(); break;
    }
  }
}

} // namespace json
