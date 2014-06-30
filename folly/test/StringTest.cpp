/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <folly/String.h>

#include <random>
#include <boost/algorithm/string.hpp>
#include <gtest/gtest.h>

#include <folly/Benchmark.h>

using namespace folly;
using namespace std;

TEST(StringPrintf, BasicTest) {
  EXPECT_EQ("abc", stringPrintf("%s", "abc"));
  EXPECT_EQ("abc", stringPrintf("%sbc", "a"));
  EXPECT_EQ("abc", stringPrintf("a%sc", "b"));
  EXPECT_EQ("abc", stringPrintf("ab%s", "c"));

  EXPECT_EQ("abc", stringPrintf("abc"));
}

TEST(StringPrintf, NumericFormats) {
  EXPECT_EQ("12", stringPrintf("%d", 12));
  EXPECT_EQ("5000000000", stringPrintf("%ld", 5000000000UL));
  EXPECT_EQ("5000000000", stringPrintf("%ld", 5000000000L));
  EXPECT_EQ("-5000000000", stringPrintf("%ld", -5000000000L));
  EXPECT_EQ("-1", stringPrintf("%d", 0xffffffff));
  EXPECT_EQ("-1", stringPrintf("%ld", 0xffffffffffffffff));
  EXPECT_EQ("-1", stringPrintf("%ld", 0xffffffffffffffffUL));

  EXPECT_EQ("7.7", stringPrintf("%1.1f", 7.7));
  EXPECT_EQ("7.7", stringPrintf("%1.1lf", 7.7));
  EXPECT_EQ("7.70000000000000018",
            stringPrintf("%.17f", 7.7));
  EXPECT_EQ("7.70000000000000018",
            stringPrintf("%.17lf", 7.7));
}

TEST(StringPrintf, Appending) {
  string s;
  stringAppendf(&s, "a%s", "b");
  stringAppendf(&s, "%c", 'c');
  EXPECT_EQ(s, "abc");
  stringAppendf(&s, " %d", 123);
  EXPECT_EQ(s, "abc 123");
}

TEST(StringPrintf, VariousSizes) {
  // Test a wide variety of output sizes
  for (int i = 0; i < 100; ++i) {
    string expected(i + 1, 'a');
    EXPECT_EQ("X" + expected + "X", stringPrintf("X%sX", expected.c_str()));
  }

  EXPECT_EQ("abc12345678910111213141516171819202122232425xyz",
            stringPrintf("abc%d%d%d%d%d%d%d%d%d%d%d%d%d%d"
                         "%d%d%d%d%d%d%d%d%d%d%dxyz",
                         1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
                         17, 18, 19, 20, 21, 22, 23, 24, 25));
}

TEST(StringPrintf, oldStringPrintfTests) {
  EXPECT_EQ(string("a/b/c/d"),
            stringPrintf("%s/%s/%s/%s", "a", "b", "c", "d"));

  EXPECT_EQ(string("    5    10"),
            stringPrintf("%5d %5d", 5, 10));

  // check printing w/ a big buffer
  for (int size = (1 << 8); size <= (1 << 15); size <<= 1) {
    string a(size, 'z');
    string b = stringPrintf("%s", a.c_str());
    EXPECT_EQ(a.size(), b.size());
  }
}

TEST(StringPrintf, oldStringAppendf) {
  string s = "hello";
  stringAppendf(&s, "%s/%s/%s/%s", "a", "b", "c", "d");
  EXPECT_EQ(string("helloa/b/c/d"), s);
}

BENCHMARK(new_stringPrintfSmall, iters) {
  for (int64_t i = 0; i < iters; ++i) {
    int32_t x = int32_t(i);
    int32_t y = int32_t(i + 1);
    string s =
      stringPrintf("msg msg msg msg msg msg msg msg:  %d, %d, %s",
                   x, y, "hello");
  }
}

TEST(Escape, cEscape) {
  EXPECT_EQ("hello world", cEscape<std::string>("hello world"));
  EXPECT_EQ("hello \\\\world\\\" goodbye",
            cEscape<std::string>("hello \\world\" goodbye"));
  EXPECT_EQ("hello\\nworld", cEscape<std::string>("hello\nworld"));
  EXPECT_EQ("hello\\377\\376", cEscape<std::string>("hello\xff\xfe"));
}

TEST(Escape, cUnescape) {
  EXPECT_EQ("hello world", cUnescape<std::string>("hello world"));
  EXPECT_EQ("hello \\world\" goodbye",
            cUnescape<std::string>("hello \\\\world\\\" goodbye"));
  EXPECT_EQ("hello\nworld", cUnescape<std::string>("hello\\nworld"));
  EXPECT_EQ("hello\nworld", cUnescape<std::string>("hello\\012world"));
  EXPECT_EQ("hello\nworld", cUnescape<std::string>("hello\\x0aworld"));
  EXPECT_EQ("hello\xff\xfe", cUnescape<std::string>("hello\\377\\376"));
  EXPECT_EQ("hello\xff\xfe", cUnescape<std::string>("hello\\xff\\xfe"));

  EXPECT_THROW({cUnescape<std::string>("hello\\");},
               std::invalid_argument);
  EXPECT_THROW({cUnescape<std::string>("hello\\x");},
               std::invalid_argument);
  EXPECT_THROW({cUnescape<std::string>("hello\\q");},
               std::invalid_argument);
}

TEST(Escape, uriEscape) {
  EXPECT_EQ("hello%2c%20%2fworld", uriEscape<std::string>("hello, /world"));
  EXPECT_EQ("hello%2c%20/world", uriEscape<std::string>("hello, /world",
                                                        UriEscapeMode::PATH));
  EXPECT_EQ("hello%2c+%2fworld", uriEscape<std::string>("hello, /world",
                                                        UriEscapeMode::QUERY));
}

TEST(Escape, uriUnescape) {
  EXPECT_EQ("hello, /world", uriUnescape<std::string>("hello, /world"));
  EXPECT_EQ("hello, /world", uriUnescape<std::string>("hello%2c%20%2fworld"));
  EXPECT_EQ("hello,+/world", uriUnescape<std::string>("hello%2c+%2fworld"));
  EXPECT_EQ("hello, /world", uriUnescape<std::string>("hello%2c+%2fworld",
                                                      UriEscapeMode::QUERY));
  EXPECT_EQ("hello/", uriUnescape<std::string>("hello%2f"));
  EXPECT_EQ("hello/", uriUnescape<std::string>("hello%2F"));
  EXPECT_THROW({uriUnescape<std::string>("hello%");},
               std::invalid_argument);
  EXPECT_THROW({uriUnescape<std::string>("hello%2");},
               std::invalid_argument);
  EXPECT_THROW({uriUnescape<std::string>("hello%2g");},
               std::invalid_argument);
}

namespace {
void expectPrintable(StringPiece s) {
  for (char c : s) {
    EXPECT_LE(32, c);
    EXPECT_GE(127, c);
  }
}
}  // namespace

TEST(Escape, uriEscapeAllCombinations) {
  char c[3];
  c[2] = '\0';
  StringPiece in(c, 2);
  fbstring tmp;
  fbstring out;
  for (int i = 0; i < 256; ++i) {
    c[0] = i;
    for (int j = 0; j < 256; ++j) {
      c[1] = j;
      tmp.clear();
      out.clear();
      uriEscape(in, tmp);
      expectPrintable(tmp);
      uriUnescape(tmp, out);
      EXPECT_EQ(in, out);
    }
  }
}

namespace {
bool isHex(int v) {
  return ((v >= '0' && v <= '9') ||
          (v >= 'A' && v <= 'F') ||
          (v >= 'a' && v <= 'f'));
}
}  // namespace

TEST(Escape, uriUnescapePercentDecoding) {
  char c[4] = {'%', '\0', '\0', '\0'};
  StringPiece in(c, 3);
  fbstring out;
  unsigned int expected = 0;
  for (int i = 0; i < 256; ++i) {
    c[1] = i;
    for (int j = 0; j < 256; ++j) {
      c[2] = j;
      if (isHex(i) && isHex(j)) {
        out.clear();
        uriUnescape(in, out);
        EXPECT_EQ(1, out.size());
        EXPECT_EQ(1, sscanf(c + 1, "%x", &expected));
        unsigned char v = out[0];
        EXPECT_EQ(expected, v);
      } else {
        EXPECT_THROW({uriUnescape(in, out);}, std::invalid_argument);
      }
    }
  }
}

namespace {
fbstring cbmString;
fbstring cbmEscapedString;
fbstring cEscapedString;
fbstring cUnescapedString;
const size_t kCBmStringLength = 64 << 10;
const uint32_t kCPrintablePercentage = 90;

fbstring uribmString;
fbstring uribmEscapedString;
fbstring uriEscapedString;
fbstring uriUnescapedString;
const size_t kURIBmStringLength = 256;
const uint32_t kURIPassThroughPercentage = 50;

void initBenchmark() {
  std::mt19937 rnd;

  // C escape
  std::uniform_int_distribution<uint32_t> printable(32, 126);
  std::uniform_int_distribution<uint32_t> nonPrintable(0, 160);
  std::uniform_int_distribution<uint32_t> percentage(0, 99);

  cbmString.reserve(kCBmStringLength);
  for (size_t i = 0; i < kCBmStringLength; ++i) {
    unsigned char c;
    if (percentage(rnd) < kCPrintablePercentage) {
      c = printable(rnd);
    } else {
      c = nonPrintable(rnd);
      // Generate characters in both non-printable ranges:
      // 0..31 and 127..255
      if (c >= 32) {
        c += (126 - 32) + 1;
      }
    }
    cbmString.push_back(c);
  }

  cbmEscapedString = cEscape<fbstring>(cbmString);

  // URI escape
  std::uniform_int_distribution<uint32_t> passthrough('a', 'z');
  std::string encodeChars = " ?!\"',+[]";
  std::uniform_int_distribution<uint32_t> encode(0, encodeChars.size() - 1);

  uribmString.reserve(kURIBmStringLength);
  for (size_t i = 0; i < kURIBmStringLength; ++i) {
    unsigned char c;
    if (percentage(rnd) < kURIPassThroughPercentage) {
      c = passthrough(rnd);
    } else {
      c = encodeChars[encode(rnd)];
    }
    uribmString.push_back(c);
  }

  uribmEscapedString = uriEscape<fbstring>(uribmString);
}

BENCHMARK(BM_cEscape, iters) {
  while (iters--) {
    cEscapedString = cEscape<fbstring>(cbmString);
    doNotOptimizeAway(cEscapedString.size());
  }
}

BENCHMARK(BM_cUnescape, iters) {
  while (iters--) {
    cUnescapedString = cUnescape<fbstring>(cbmEscapedString);
    doNotOptimizeAway(cUnescapedString.size());
  }
}

BENCHMARK(BM_uriEscape, iters) {
  while (iters--) {
    uriEscapedString = uriEscape<fbstring>(uribmString);
    doNotOptimizeAway(uriEscapedString.size());
  }
}

BENCHMARK(BM_uriUnescape, iters) {
  while (iters--) {
    uriUnescapedString = uriUnescape<fbstring>(uribmEscapedString);
    doNotOptimizeAway(uriUnescapedString.size());
  }
}

}  // namespace

namespace {

double pow2(int exponent) {
  return double(int64_t(1) << exponent);
}

}  // namespace
struct PrettyTestCase{
  std::string prettyString;
  double realValue;
  PrettyType prettyType;
};

PrettyTestCase prettyTestCases[] =
{
  {string("8.53e+07 s "), 85.3e6,  PRETTY_TIME},
  {string("8.53e+07 s "), 85.3e6,  PRETTY_TIME},
  {string("85.3 ms"), 85.3e-3,  PRETTY_TIME},
  {string("85.3 us"), 85.3e-6,  PRETTY_TIME},
  {string("85.3 ns"), 85.3e-9,  PRETTY_TIME},
  {string("85.3 ps"), 85.3e-12,  PRETTY_TIME},
  {string("8.53e-14 s "), 85.3e-15,  PRETTY_TIME},

  {string("0 s "), 0,  PRETTY_TIME},
  {string("1 s "), 1.0,  PRETTY_TIME},
  {string("1 ms"), 1.0e-3,  PRETTY_TIME},
  {string("1 us"), 1.0e-6,  PRETTY_TIME},
  {string("1 ns"), 1.0e-9,  PRETTY_TIME},
  {string("1 ps"), 1.0e-12,  PRETTY_TIME},

  // check bytes printing
  {string("853 B "), 853.,  PRETTY_BYTES},
  {string("833 kB"), 853.e3,  PRETTY_BYTES},
  {string("813.5 MB"), 853.e6,  PRETTY_BYTES},
  {string("7.944 GB"), 8.53e9,  PRETTY_BYTES},
  {string("794.4 GB"), 853.e9,  PRETTY_BYTES},
  {string("775.8 TB"), 853.e12,  PRETTY_BYTES},

  {string("0 B "), 0,  PRETTY_BYTES},
  {string("1 B "), pow2(0),  PRETTY_BYTES},
  {string("1 kB"), pow2(10),  PRETTY_BYTES},
  {string("1 MB"), pow2(20),  PRETTY_BYTES},
  {string("1 GB"), pow2(30),  PRETTY_BYTES},
  {string("1 TB"), pow2(40),  PRETTY_BYTES},

  {string("853 B  "), 853.,  PRETTY_BYTES_IEC},
  {string("833 KiB"), 853.e3,  PRETTY_BYTES_IEC},
  {string("813.5 MiB"), 853.e6,  PRETTY_BYTES_IEC},
  {string("7.944 GiB"), 8.53e9,  PRETTY_BYTES_IEC},
  {string("794.4 GiB"), 853.e9,  PRETTY_BYTES_IEC},
  {string("775.8 TiB"), 853.e12,  PRETTY_BYTES_IEC},

  {string("0 B  "), 0,  PRETTY_BYTES_IEC},
  {string("1 B  "), pow2(0),  PRETTY_BYTES_IEC},
  {string("1 KiB"), pow2(10),  PRETTY_BYTES_IEC},
  {string("1 MiB"), pow2(20),  PRETTY_BYTES_IEC},
  {string("1 GiB"), pow2(30),  PRETTY_BYTES_IEC},
  {string("1 TiB"), pow2(40),  PRETTY_BYTES_IEC},

  // check bytes metric printing
  {string("853 B "), 853.,  PRETTY_BYTES_METRIC},
  {string("853 kB"), 853.e3,  PRETTY_BYTES_METRIC},
  {string("853 MB"), 853.e6,  PRETTY_BYTES_METRIC},
  {string("8.53 GB"), 8.53e9,  PRETTY_BYTES_METRIC},
  {string("853 GB"), 853.e9,  PRETTY_BYTES_METRIC},
  {string("853 TB"), 853.e12,  PRETTY_BYTES_METRIC},

  {string("0 B "), 0,  PRETTY_BYTES_METRIC},
  {string("1 B "), 1.0,  PRETTY_BYTES_METRIC},
  {string("1 kB"), 1.0e+3,  PRETTY_BYTES_METRIC},
  {string("1 MB"), 1.0e+6,  PRETTY_BYTES_METRIC},

  {string("1 GB"), 1.0e+9,  PRETTY_BYTES_METRIC},
  {string("1 TB"), 1.0e+12,  PRETTY_BYTES_METRIC},

  // check metric-units (powers of 1000) printing
  {string("853  "), 853.,  PRETTY_UNITS_METRIC},
  {string("853 k"), 853.e3,  PRETTY_UNITS_METRIC},
  {string("853 M"), 853.e6,  PRETTY_UNITS_METRIC},
  {string("8.53 bil"), 8.53e9,  PRETTY_UNITS_METRIC},
  {string("853 bil"), 853.e9,  PRETTY_UNITS_METRIC},
  {string("853 tril"), 853.e12,  PRETTY_UNITS_METRIC},

  // check binary-units (powers of 1024) printing
  {string("0  "), 0,  PRETTY_UNITS_BINARY},
  {string("1  "), pow2(0),  PRETTY_UNITS_BINARY},
  {string("1 k"), pow2(10),  PRETTY_UNITS_BINARY},
  {string("1 M"), pow2(20),  PRETTY_UNITS_BINARY},
  {string("1 G"), pow2(30),  PRETTY_UNITS_BINARY},
  {string("1 T"), pow2(40),  PRETTY_UNITS_BINARY},

  {string("1023  "), pow2(10) - 1,  PRETTY_UNITS_BINARY},
  {string("1024 k"), pow2(20) - 1,  PRETTY_UNITS_BINARY},
  {string("1024 M"), pow2(30) - 1,  PRETTY_UNITS_BINARY},
  {string("1024 G"), pow2(40) - 1,  PRETTY_UNITS_BINARY},

  {string("0   "), 0,  PRETTY_UNITS_BINARY_IEC},
  {string("1   "), pow2(0),  PRETTY_UNITS_BINARY_IEC},
  {string("1 Ki"), pow2(10),  PRETTY_UNITS_BINARY_IEC},
  {string("1 Mi"), pow2(20),  PRETTY_UNITS_BINARY_IEC},
  {string("1 Gi"), pow2(30),  PRETTY_UNITS_BINARY_IEC},
  {string("1 Ti"), pow2(40),  PRETTY_UNITS_BINARY_IEC},

  {string("1023   "), pow2(10) - 1,  PRETTY_UNITS_BINARY_IEC},
  {string("1024 Ki"), pow2(20) - 1,  PRETTY_UNITS_BINARY_IEC},
  {string("1024 Mi"), pow2(30) - 1,  PRETTY_UNITS_BINARY_IEC},
  {string("1024 Gi"), pow2(40) - 1,  PRETTY_UNITS_BINARY_IEC},

  //check border SI cases

  {string("1 Y"), 1e24,  PRETTY_SI},
  {string("10 Y"), 1e25,  PRETTY_SI},
  {string("1 y"), 1e-24,  PRETTY_SI},
  {string("10 y"), 1e-23,  PRETTY_SI},

  // check that negative values work
  {string("-85.3 s "), -85.3,  PRETTY_TIME},
  {string("-85.3 ms"), -85.3e-3,  PRETTY_TIME},
  {string("-85.3 us"), -85.3e-6,  PRETTY_TIME},
  {string("-85.3 ns"), -85.3e-9,  PRETTY_TIME},
  // end of test
  {string("endoftest"), 0, PRETTY_NUM_TYPES}
};

TEST(PrettyPrint, Basic) {
  for (int i = 0; prettyTestCases[i].prettyType != PRETTY_NUM_TYPES; ++i){
    const PrettyTestCase& prettyTest = prettyTestCases[i];
    EXPECT_EQ(prettyTest.prettyString,
              prettyPrint(prettyTest.realValue, prettyTest.prettyType));
  }
}

TEST(PrettyToDouble, Basic) {
  // check manually created tests
  for (int i = 0; prettyTestCases[i].prettyType != PRETTY_NUM_TYPES; ++i){
    PrettyTestCase testCase = prettyTestCases[i];
    PrettyType formatType = testCase.prettyType;
    double x = testCase.realValue;
    std::string testString = testCase.prettyString;
    double recoveredX;
    try{
      recoveredX = prettyToDouble(testString, formatType);
    } catch (std::range_error &ex){
      EXPECT_TRUE(false);
    }
    double relativeError = fabs(x) < 1e-5 ? (x-recoveredX) :
                                            (x - recoveredX) / x;
    EXPECT_NEAR(0, relativeError, 1e-3);
  }

  // checks for compatibility with prettyPrint over the whole parameter space
  for (int i = 0 ; i < PRETTY_NUM_TYPES; ++i){
    PrettyType formatType = static_cast<PrettyType>(i);
    for (double x = 1e-18; x < 1e40; x *= 1.9){
      bool addSpace = static_cast<PrettyType> (i) == PRETTY_SI;
      for (int it = 0; it < 2; ++it, addSpace = true){
        double recoveredX;
        try{
          recoveredX = prettyToDouble(prettyPrint(x, formatType, addSpace),
                                             formatType);
        } catch (std::range_error &ex){
          EXPECT_TRUE(false);
        }
        double relativeError = (x - recoveredX) / x;
        EXPECT_NEAR(0, relativeError, 1e-3);
      }
    }
  }

  // check for incorrect values
  EXPECT_THROW(prettyToDouble("10Mx", PRETTY_SI), std::range_error);
  EXPECT_THROW(prettyToDouble("10 Mx", PRETTY_SI), std::range_error);
  EXPECT_THROW(prettyToDouble("10 M x", PRETTY_SI), std::range_error);

  StringPiece testString = "10Mx";
  EXPECT_DOUBLE_EQ(prettyToDouble(&testString, PRETTY_UNITS_METRIC), 10e6);
  EXPECT_EQ(testString, "x");
}

TEST(PrettyPrint, HexDump) {
  std::string a("abc\x00\x02\xa0", 6);  // embedded NUL
  EXPECT_EQ(
    "00000000  61 62 63 00 02 a0                                 "
    "|abc...          |\n",
    hexDump(a.data(), a.size()));

  a = "abcdefghijklmnopqrstuvwxyz";
  EXPECT_EQ(
    "00000000  61 62 63 64 65 66 67 68  69 6a 6b 6c 6d 6e 6f 70  "
    "|abcdefghijklmnop|\n"
    "00000010  71 72 73 74 75 76 77 78  79 7a                    "
    "|qrstuvwxyz      |\n",
    hexDump(a.data(), a.size()));
}

TEST(System, errnoStr) {
  errno = EACCES;
  EXPECT_EQ(EACCES, errno);
  EXPECT_EQ(EACCES, errno);  // twice to make sure EXPECT_EQ doesn't change it

  fbstring expected = strerror(ENOENT);

  errno = EACCES;
  EXPECT_EQ(expected, errnoStr(ENOENT));
  // Ensure that errno isn't changed
  EXPECT_EQ(EACCES, errno);

  // Per POSIX, all errno values are positive, so -1 is invalid
  errnoStr(-1);

  // Ensure that errno isn't changed
  EXPECT_EQ(EACCES, errno);
}

namespace folly_test {
struct ThisIsAVeryLongStructureName {
};
}  // namespace folly_test

#if FOLLY_HAVE_CPLUS_DEMANGLE_V3_CALLBACK
TEST(System, demangle) {
  char expected[] = "folly_test::ThisIsAVeryLongStructureName";
  EXPECT_STREQ(
      expected,
      demangle(typeid(folly_test::ThisIsAVeryLongStructureName)).c_str());

  {
    char buf[sizeof(expected)];
    EXPECT_EQ(sizeof(expected) - 1,
              demangle(typeid(folly_test::ThisIsAVeryLongStructureName),
                       buf, sizeof(buf)));
    EXPECT_STREQ(expected, buf);

    EXPECT_EQ(sizeof(expected) - 1,
              demangle(typeid(folly_test::ThisIsAVeryLongStructureName),
                       buf, 11));
    EXPECT_STREQ("folly_test", buf);
  }
}
#endif

namespace {

template<template<class,class> class VectorType>
void splitTest() {
  VectorType<string,std::allocator<string> > parts;

  folly::split(',', "a,b,c", parts);
  EXPECT_EQ(parts.size(), 3);
  EXPECT_EQ(parts[0], "a");
  EXPECT_EQ(parts[1], "b");
  EXPECT_EQ(parts[2], "c");
  parts.clear();

  folly::split(',', StringPiece("a,b,c"), parts);
  EXPECT_EQ(parts.size(), 3);
  EXPECT_EQ(parts[0], "a");
  EXPECT_EQ(parts[1], "b");
  EXPECT_EQ(parts[2], "c");
  parts.clear();

  folly::split(',', string("a,b,c"), parts);
  EXPECT_EQ(parts.size(), 3);
  EXPECT_EQ(parts[0], "a");
  EXPECT_EQ(parts[1], "b");
  EXPECT_EQ(parts[2], "c");
  parts.clear();

  folly::split(',', "a,,c", parts);
  EXPECT_EQ(parts.size(), 3);
  EXPECT_EQ(parts[0], "a");
  EXPECT_EQ(parts[1], "");
  EXPECT_EQ(parts[2], "c");
  parts.clear();

  folly::split(',', string("a,,c"), parts);
  EXPECT_EQ(parts.size(), 3);
  EXPECT_EQ(parts[0], "a");
  EXPECT_EQ(parts[1], "");
  EXPECT_EQ(parts[2], "c");
  parts.clear();

  folly::split(',', "a,,c", parts, true);
  EXPECT_EQ(parts.size(), 2);
  EXPECT_EQ(parts[0], "a");
  EXPECT_EQ(parts[1], "c");
  parts.clear();

  folly::split(',', string("a,,c"), parts, true);
  EXPECT_EQ(parts.size(), 2);
  EXPECT_EQ(parts[0], "a");
  EXPECT_EQ(parts[1], "c");
  parts.clear();

  folly::split(',', string(",,a,,c,,,"), parts, true);
  EXPECT_EQ(parts.size(), 2);
  EXPECT_EQ(parts[0], "a");
  EXPECT_EQ(parts[1], "c");
  parts.clear();

  // test multiple split w/o clear
  folly::split(',', ",,a,,c,,,", parts, true);
  EXPECT_EQ(parts.size(), 2);
  EXPECT_EQ(parts[0], "a");
  EXPECT_EQ(parts[1], "c");
  folly::split(',', ",,a,,c,,,", parts, true);
  EXPECT_EQ(parts.size(), 4);
  EXPECT_EQ(parts[2], "a");
  EXPECT_EQ(parts[3], "c");
  parts.clear();

  // test splits that with multi-line delimiter
  folly::split("ab", "dabcabkdbkab", parts, true);
  EXPECT_EQ(parts.size(), 3);
  EXPECT_EQ(parts[0], "d");
  EXPECT_EQ(parts[1], "c");
  EXPECT_EQ(parts[2], "kdbk");
  parts.clear();

  string orig = "ab2342asdfv~~!";
  folly::split("", orig, parts, true);
  EXPECT_EQ(parts.size(), 1);
  EXPECT_EQ(parts[0], orig);
  parts.clear();

  folly::split("452x;o38asfsajsdlfdf.j", "asfds", parts, true);
  EXPECT_EQ(parts.size(), 1);
  EXPECT_EQ(parts[0], "asfds");
  parts.clear();

  folly::split("a", "", parts, true);
  EXPECT_EQ(parts.size(), 0);
  parts.clear();

  folly::split("a", "", parts);
  EXPECT_EQ(parts.size(), 1);
  EXPECT_EQ(parts[0], "");
  parts.clear();

  folly::split("a", StringPiece(), parts, true);
  EXPECT_EQ(parts.size(), 0);
  parts.clear();

  folly::split("a", StringPiece(), parts);
  EXPECT_EQ(parts.size(), 1);
  EXPECT_EQ(parts[0], "");
  parts.clear();

  folly::split("a", "abcdefg", parts, true);
  EXPECT_EQ(parts.size(), 1);
  EXPECT_EQ(parts[0], "bcdefg");
  parts.clear();

  orig = "All, , your base, are , , belong to us";
  folly::split(", ", orig, parts, true);
  EXPECT_EQ(parts.size(), 4);
  EXPECT_EQ(parts[0], "All");
  EXPECT_EQ(parts[1], "your base");
  EXPECT_EQ(parts[2], "are ");
  EXPECT_EQ(parts[3], "belong to us");
  parts.clear();
  folly::split(", ", orig, parts);
  EXPECT_EQ(parts.size(), 6);
  EXPECT_EQ(parts[0], "All");
  EXPECT_EQ(parts[1], "");
  EXPECT_EQ(parts[2], "your base");
  EXPECT_EQ(parts[3], "are ");
  EXPECT_EQ(parts[4], "");
  EXPECT_EQ(parts[5], "belong to us");
  parts.clear();

  orig = ", Facebook, rul,es!, ";
  folly::split(", ", orig, parts, true);
  EXPECT_EQ(parts.size(), 2);
  EXPECT_EQ(parts[0], "Facebook");
  EXPECT_EQ(parts[1], "rul,es!");
  parts.clear();
  folly::split(", ", orig, parts);
  EXPECT_EQ(parts.size(), 4);
  EXPECT_EQ(parts[0], "");
  EXPECT_EQ(parts[1], "Facebook");
  EXPECT_EQ(parts[2], "rul,es!");
  EXPECT_EQ(parts[3], "");
}

template<template<class,class> class VectorType>
void piecesTest() {
  VectorType<StringPiece,std::allocator<StringPiece> > pieces;
  VectorType<StringPiece,std::allocator<StringPiece> > pieces2;

  folly::split(',', "a,b,c", pieces);
  EXPECT_EQ(pieces.size(), 3);
  EXPECT_EQ(pieces[0], "a");
  EXPECT_EQ(pieces[1], "b");
  EXPECT_EQ(pieces[2], "c");

  pieces.clear();

  folly::split(',', "a,,c", pieces);
  EXPECT_EQ(pieces.size(), 3);
  EXPECT_EQ(pieces[0], "a");
  EXPECT_EQ(pieces[1], "");
  EXPECT_EQ(pieces[2], "c");
  pieces.clear();

  folly::split(',', "a,,c", pieces, true);
  EXPECT_EQ(pieces.size(), 2);
  EXPECT_EQ(pieces[0], "a");
  EXPECT_EQ(pieces[1], "c");
  pieces.clear();

  folly::split(',', ",,a,,c,,,", pieces, true);
  EXPECT_EQ(pieces.size(), 2);
  EXPECT_EQ(pieces[0], "a");
  EXPECT_EQ(pieces[1], "c");
  pieces.clear();

  // test multiple split w/o clear
  folly::split(',', ",,a,,c,,,", pieces, true);
  EXPECT_EQ(pieces.size(), 2);
  EXPECT_EQ(pieces[0], "a");
  EXPECT_EQ(pieces[1], "c");
  folly::split(',', ",,a,,c,,,", pieces, true);
  EXPECT_EQ(pieces.size(), 4);
  EXPECT_EQ(pieces[2], "a");
  EXPECT_EQ(pieces[3], "c");
  pieces.clear();

  // test multiple split rounds
  folly::split(",", "a_b,c_d", pieces);
  EXPECT_EQ(pieces.size(), 2);
  EXPECT_EQ(pieces[0], "a_b");
  EXPECT_EQ(pieces[1], "c_d");
  folly::split("_", pieces[0], pieces2);
  EXPECT_EQ(pieces2.size(), 2);
  EXPECT_EQ(pieces2[0], "a");
  EXPECT_EQ(pieces2[1], "b");
  pieces2.clear();
  folly::split("_", pieces[1], pieces2);
  EXPECT_EQ(pieces2.size(), 2);
  EXPECT_EQ(pieces2[0], "c");
  EXPECT_EQ(pieces2[1], "d");
  pieces.clear();
  pieces2.clear();

  // test splits that with multi-line delimiter
  folly::split("ab", "dabcabkdbkab", pieces, true);
  EXPECT_EQ(pieces.size(), 3);
  EXPECT_EQ(pieces[0], "d");
  EXPECT_EQ(pieces[1], "c");
  EXPECT_EQ(pieces[2], "kdbk");
  pieces.clear();

  string orig = "ab2342asdfv~~!";
  folly::split("", orig.c_str(), pieces, true);
  EXPECT_EQ(pieces.size(), 1);
  EXPECT_EQ(pieces[0], orig);
  pieces.clear();

  folly::split("452x;o38asfsajsdlfdf.j", "asfds", pieces, true);
  EXPECT_EQ(pieces.size(), 1);
  EXPECT_EQ(pieces[0], "asfds");
  pieces.clear();

  folly::split("a", "", pieces, true);
  EXPECT_EQ(pieces.size(), 0);
  pieces.clear();

  folly::split("a", "", pieces);
  EXPECT_EQ(pieces.size(), 1);
  EXPECT_EQ(pieces[0], "");
  pieces.clear();

  folly::split("a", "abcdefg", pieces, true);
  EXPECT_EQ(pieces.size(), 1);
  EXPECT_EQ(pieces[0], "bcdefg");
  pieces.clear();

  orig = "All, , your base, are , , belong to us";
  folly::split(", ", orig, pieces, true);
  EXPECT_EQ(pieces.size(), 4);
  EXPECT_EQ(pieces[0], "All");
  EXPECT_EQ(pieces[1], "your base");
  EXPECT_EQ(pieces[2], "are ");
  EXPECT_EQ(pieces[3], "belong to us");
  pieces.clear();
  folly::split(", ", orig, pieces);
  EXPECT_EQ(pieces.size(), 6);
  EXPECT_EQ(pieces[0], "All");
  EXPECT_EQ(pieces[1], "");
  EXPECT_EQ(pieces[2], "your base");
  EXPECT_EQ(pieces[3], "are ");
  EXPECT_EQ(pieces[4], "");
  EXPECT_EQ(pieces[5], "belong to us");
  pieces.clear();

  orig = ", Facebook, rul,es!, ";
  folly::split(", ", orig, pieces, true);
  EXPECT_EQ(pieces.size(), 2);
  EXPECT_EQ(pieces[0], "Facebook");
  EXPECT_EQ(pieces[1], "rul,es!");
  pieces.clear();
  folly::split(", ", orig, pieces);
  EXPECT_EQ(pieces.size(), 4);
  EXPECT_EQ(pieces[0], "");
  EXPECT_EQ(pieces[1], "Facebook");
  EXPECT_EQ(pieces[2], "rul,es!");
  EXPECT_EQ(pieces[3], "");
  pieces.clear();

  const char* str = "a,b";
  folly::split(',', StringPiece(str), pieces);
  EXPECT_EQ(pieces.size(), 2);
  EXPECT_EQ(pieces[0], "a");
  EXPECT_EQ(pieces[1], "b");
  EXPECT_EQ(pieces[0].start(), str);
  EXPECT_EQ(pieces[1].start(), str + 2);

  std::set<StringPiece> unique;
  folly::splitTo<StringPiece>(":", "asd:bsd:asd:asd:bsd:csd::asd",
    std::inserter(unique, unique.begin()), true);
  EXPECT_EQ(unique.size(), 3);
  if (unique.size() == 3) {
    EXPECT_EQ(*unique.begin(), "asd");
    EXPECT_EQ(*--unique.end(), "csd");
  }

  VectorType<fbstring,std::allocator<fbstring> > blah;
  folly::split('-', "a-b-c-d-f-e", blah);
  EXPECT_EQ(blah.size(), 6);
}

}

TEST(Split, split_vector) {
  splitTest<std::vector>();
}
TEST(Split, split_fbvector) {
  splitTest<folly::fbvector>();
}
TEST(Split, pieces_vector) {
  piecesTest<std::vector>();
}
TEST(Split, pieces_fbvector) {
  piecesTest<folly::fbvector>();
}

TEST(Split, fixed) {
  StringPiece a, b, c, d;

  EXPECT_TRUE(folly::split<false>('.', "a.b.c.d", a, b, c, d));
  EXPECT_TRUE(folly::split<false>('.', "a.b.c", a, b, c));
  EXPECT_TRUE(folly::split<false>('.', "a.b", a, b));
  EXPECT_TRUE(folly::split<false>('.', "a", a));

  EXPECT_TRUE(folly::split('.', "a.b.c.d", a, b, c, d));
  EXPECT_TRUE(folly::split('.', "a.b.c", a, b, c));
  EXPECT_TRUE(folly::split('.', "a.b", a, b));
  EXPECT_TRUE(folly::split('.', "a", a));

  EXPECT_TRUE(folly::split<false>('.', "a.b.c", a, b, c));
  EXPECT_EQ("a", a);
  EXPECT_EQ("b", b);
  EXPECT_EQ("c", c);
  EXPECT_FALSE(folly::split<false>('.', "a.b", a, b, c));
  EXPECT_TRUE(folly::split<false>('.', "a.b.c", a, b));
  EXPECT_EQ("a", a);
  EXPECT_EQ("b.c", b);

  EXPECT_TRUE(folly::split('.', "a.b.c", a, b, c));
  EXPECT_EQ("a", a);
  EXPECT_EQ("b", b);
  EXPECT_EQ("c", c);
  EXPECT_FALSE(folly::split('.', "a.b.c", a, b));
  EXPECT_FALSE(folly::split('.', "a.b", a, b, c));

  EXPECT_TRUE(folly::split<false>('.', "a.b", a, b));
  EXPECT_EQ("a", a);
  EXPECT_EQ("b", b);
  EXPECT_FALSE(folly::split<false>('.', "a", a, b));
  EXPECT_TRUE(folly::split<false>('.', "a.b", a));
  EXPECT_EQ("a.b", a);

  EXPECT_TRUE(folly::split('.', "a.b", a, b));
  EXPECT_EQ("a", a);
  EXPECT_EQ("b", b);
  EXPECT_FALSE(folly::split('.', "a", a, b));
  EXPECT_FALSE(folly::split('.', "a.b", a));
}

TEST(Split, fixed_convert) {
  StringPiece a, d;
  int b;
  double c;

  EXPECT_TRUE(folly::split(':', "a:13:14.7:b", a, b, c, d));
  EXPECT_EQ("a", a);
  EXPECT_EQ(13, b);
  EXPECT_NEAR(14.7, c, 1e-10);
  EXPECT_EQ("b", d);

  EXPECT_TRUE(folly::split<false>(':', "b:14:15.3:c", a, b, c, d));
  EXPECT_EQ("b", a);
  EXPECT_EQ(14, b);
  EXPECT_NEAR(15.3, c, 1e-10);
  EXPECT_EQ("c", d);

  EXPECT_FALSE(folly::split(':', "a:13:14.7:b", a, b, d));

  EXPECT_TRUE(folly::split<false>(':', "a:13:14.7:b", a, b, d));
  EXPECT_EQ("a", a);
  EXPECT_EQ(13, b);
  EXPECT_EQ("14.7:b", d);

  EXPECT_THROW(folly::split<false>(':', "a:13:14.7:b", a, b, c),
               std::range_error);
}

TEST(String, join) {
  string output;

  std::vector<int> empty = { };
  join(":", empty, output);
  EXPECT_TRUE(output.empty());

  std::vector<std::string> input1 = { "1", "23", "456", "" };
  join(':', input1, output);
  EXPECT_EQ(output, "1:23:456:");
  output = join(':', input1);
  EXPECT_EQ(output, "1:23:456:");

  auto input2 = { 1, 23, 456 };
  join("-*-", input2, output);
  EXPECT_EQ(output, "1-*-23-*-456");
  output = join("-*-", input2);
  EXPECT_EQ(output, "1-*-23-*-456");

  auto input3 = { 'f', 'a', 'c', 'e', 'b', 'o', 'o', 'k' };
  join("", input3, output);
  EXPECT_EQ(output, "facebook");

  join("_", { "", "f", "a", "c", "e", "b", "o", "o", "k", "" }, output);
  EXPECT_EQ(output, "_f_a_c_e_b_o_o_k_");
}

TEST(String, hexlify) {
  string input1 = "0123";
  string output1;
  EXPECT_TRUE(hexlify(input1, output1));
  EXPECT_EQ(output1, "30313233");

  fbstring input2 = "abcdefg";
  input2[1] = 0;
  input2[3] = 0xff;
  input2[5] = 0xb6;
  fbstring output2;
  EXPECT_TRUE(hexlify(input2, output2));
  EXPECT_EQ(output2, "610063ff65b667");
}

TEST(String, unhexlify) {
  string input1 = "30313233";
  string output1;
  EXPECT_TRUE(unhexlify(input1, output1));
  EXPECT_EQ(output1, "0123");

  fbstring input2 = "610063ff65b667";
  fbstring output2;
  EXPECT_TRUE(unhexlify(input2, output2));
  EXPECT_EQ(output2.size(), 7);
  EXPECT_EQ(output2[0], 'a');
  EXPECT_EQ(output2[1], 0);
  EXPECT_EQ(output2[2], 'c');
  EXPECT_EQ(output2[3] & 0xff, 0xff);
  EXPECT_EQ(output2[4], 'e');
  EXPECT_EQ(output2[5] & 0xff, 0xb6);
  EXPECT_EQ(output2[6], 'g');

  string input3 = "x";
  string output3;
  EXPECT_FALSE(unhexlify(input3, output3));

  string input4 = "xy";
  string output4;
  EXPECT_FALSE(unhexlify(input4, output4));
}

TEST(String, backslashify) {
  EXPECT_EQ("abc", string("abc"));
  EXPECT_EQ("abc", backslashify(string("abc")));
  EXPECT_EQ("abc\\r", backslashify(string("abc\r")));
  EXPECT_EQ("abc\\x0d", backslashify(string("abc\r"), true));
  EXPECT_EQ("\\0\\0", backslashify(string(2, '\0')));
}

TEST(String, humanify) {
  // Simple cases; output is obvious.
  EXPECT_EQ("abc", humanify(string("abc")));
  EXPECT_EQ("abc\\\\r", humanify(string("abc\\r")));
  EXPECT_EQ("0xff", humanify(string("\xff")));
  EXPECT_EQ("abc\\xff", humanify(string("abc\xff")));
  EXPECT_EQ("abc\\b", humanify(string("abc\b")));
  EXPECT_EQ("0x00", humanify(string(1, '\0')));
  EXPECT_EQ("0x0000", humanify(string(2, '\0')));


  // Mostly printable, so backslash!  80, 60, and 40% printable, respectively
  EXPECT_EQ("aaaa\\xff", humanify(string("aaaa\xff")));
  EXPECT_EQ("aaa\\xff\\xff", humanify(string("aaa\xff\xff")));
  EXPECT_EQ("aa\\xff\\xff\\xff", humanify(string("aa\xff\xff\xff")));

  // 20% printable, and the printable portion isn't the prefix; hexify!
  EXPECT_EQ("0xff61ffffff", humanify(string("\xff" "a\xff\xff\xff")));

  // Same as previous, except swap first two chars; prefix is
  // printable and within the threshold, so backslashify.
  EXPECT_EQ("a\\xff\\xff\\xff\\xff", humanify(string("a\xff\xff\xff\xff")));

  // Just too much unprintable; hex, despite prefix.
  EXPECT_EQ("0x61ffffffffff", humanify(string("a\xff\xff\xff\xff\xff")));
}

//////////////////////////////////////////////////////////////////////

BENCHMARK(splitOnSingleChar, iters) {
  static const std::string line = "one:two:three:four";
  for (int i = 0; i < iters << 4; ++i) {
    std::vector<StringPiece> pieces;
    folly::split(':', line, pieces);
  }
}

BENCHMARK(splitOnSingleCharFixed, iters) {
  static const std::string line = "one:two:three:four";
  for (int i = 0; i < iters << 4; ++i) {
    StringPiece a, b, c, d;
    folly::split(':', line, a, b, c, d);
  }
}

BENCHMARK(splitOnSingleCharFixedAllowExtra, iters) {
  static const std::string line = "one:two:three:four";
  for (int i = 0; i < iters << 4; ++i) {
    StringPiece a, b, c, d;
    folly::split<false>(':', line, a, b, c, d);
  }
}

BENCHMARK(splitStr, iters) {
  static const std::string line = "one-*-two-*-three-*-four";
  for (int i = 0; i < iters << 4; ++i) {
    std::vector<StringPiece> pieces;
    folly::split("-*-", line, pieces);
  }
}

BENCHMARK(splitStrFixed, iters) {
  static const std::string line = "one-*-two-*-three-*-four";
  for (int i = 0; i < iters << 4; ++i) {
    StringPiece a, b, c, d;
    folly::split("-*-", line, a, b, c, d);
  }
}

BENCHMARK(boost_splitOnSingleChar, iters) {
  static const std::string line = "one:two:three:four";
  bool(*pred)(char) = [] (char c) -> bool { return c == ':'; };
  for (int i = 0; i < iters << 4; ++i) {
    std::vector<boost::iterator_range<std::string::const_iterator> > pieces;
    boost::split(pieces, line, pred);
  }
}

BENCHMARK(joinCharStr, iters) {
  static const std::vector<std::string> input = {
    "one", "two", "three", "four", "five", "six", "seven" };
  for (int i = 0; i < iters << 4; ++i) {
    std::string output;
    folly::join(':', input, output);
  }
}

BENCHMARK(joinStrStr, iters) {
  static const std::vector<std::string> input = {
    "one", "two", "three", "four", "five", "six", "seven" };
  for (int i = 0; i < iters << 4; ++i) {
    std::string output;
    folly::join(":", input, output);
  }
}

BENCHMARK(joinInt, iters) {
  static const auto input = {
    123, 456, 78910, 1112, 1314, 151, 61718 };
  for (int i = 0; i < iters << 4; ++i) {
    std::string output;
    folly::join(":", input, output);
  }
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  auto ret = RUN_ALL_TESTS();
  if (!ret) {
    initBenchmark();
    if (FLAGS_benchmark) {
      folly::runBenchmarks();
    }
  }
  return ret;
}
