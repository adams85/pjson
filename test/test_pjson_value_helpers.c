#include <assert.h>
#include <float.h>
#include <stdint.h>
#include <string.h>

#include "unity_fixture.h"
#include "pjson.h"
#include "toplevel_value_parser.h"

TEST_GROUP(value_helpers);

TEST_SETUP(value_helpers) {}

TEST_TEAR_DOWN(value_helpers) {}

static pjson_parsing_status parse_string(tlv_parser *parser, const char *input, pjson_parsing_status *feed_status) {
  pjson_tokenizer tokenizer;
  pjson_init(&tokenizer, &parser->base.base);

  pjson_parsing_status status = pjson_feed(&tokenizer, (uint8_t *)input, strlen(input));

  if (feed_status) *feed_status = status;
  return pjson_close(&tokenizer);
}

/* int32 */

static bool parse_int32_value(int32_t *value, const char *input) {
  tlv_parser parser;
  tlv_parser_init(&parser);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, input, &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);

  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.token.type);
  bool success = pjson_parse_int32(value, parser.token.start, parser.token.length);

  pjson_free((uint8_t *)parser.token.start);

  return success;
}

TEST(value_helpers, test_parse_int32_less_than_min) {
  int32_t value;
  TEST_ASSERT_FALSE(parse_int32_value(&value, "-2147483649"));
}

TEST(value_helpers, test_parse_int32_min) {
  int32_t value;
  TEST_ASSERT_TRUE(parse_int32_value(&value, "-2147483648"));
  TEST_ASSERT_EQUAL(INT32_MIN, value);
}

TEST(value_helpers, test_parse_int32_minus_zero) {
  int32_t value;
  TEST_ASSERT_TRUE(parse_int32_value(&value, "-0"));
  TEST_ASSERT_EQUAL(0, value);
}

TEST(value_helpers, test_parse_int32_max) {
  int32_t value;
  TEST_ASSERT_TRUE(parse_int32_value(&value, "2147483647"));
  TEST_ASSERT_EQUAL(INT32_MAX, value);
}

TEST(value_helpers, test_parse_int32_greater_than_max) {
  int32_t value;
  TEST_ASSERT_FALSE(parse_int32_value(&value, "2147483648"));
}

/* uint32 */

static bool parse_uint32_value(uint32_t *value, const char *input) {
  tlv_parser parser;
  tlv_parser_init(&parser);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, input, &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);

  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.token.type);
  bool success = pjson_parse_uint32(value, parser.token.start, parser.token.length);

  pjson_free((uint8_t *)parser.token.start);

  return success;
}

TEST(value_helpers, test_parse_uint32_less_than_min) {
  uint32_t value;
  TEST_ASSERT_FALSE(parse_uint32_value(&value, "-1"));
}

TEST(value_helpers, test_parse_uint32_min) {
  uint32_t value;
  TEST_ASSERT_TRUE(parse_uint32_value(&value, "0"));
  TEST_ASSERT_EQUAL(0, value);
}

TEST(value_helpers, test_parse_uint32_minus_zero) {
  uint32_t value;
  TEST_ASSERT_FALSE(parse_uint32_value(&value, "-0"));
}

TEST(value_helpers, test_parse_uint32_max) {
  uint32_t value;
  TEST_ASSERT_TRUE(parse_uint32_value(&value, "4294967295"));
  TEST_ASSERT_EQUAL(UINT32_MAX, value);
}

TEST(value_helpers, test_parse_uint32_greater_than_max) {
  uint32_t value;
  TEST_ASSERT_FALSE(parse_uint32_value(&value, "4294967296"));
}

/* int64 */

static bool parse_int64_value(int64_t *value, const char *input) {
  tlv_parser parser;
  tlv_parser_init(&parser);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, input, &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);

  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.token.type);
  bool success = pjson_parse_int64(value, parser.token.start, parser.token.length);

  pjson_free((uint8_t *)parser.token.start);

  return success;
}

TEST(value_helpers, test_parse_int64_less_than_min) {
  int64_t value;
  TEST_ASSERT_FALSE(parse_int64_value(&value, "-9223372036854775809"));
}

TEST(value_helpers, test_parse_int64_min) {
  int64_t value;
  TEST_ASSERT_TRUE(parse_int64_value(&value, "-9223372036854775808"));
  TEST_ASSERT_EQUAL(INT64_MIN, value);
}

TEST(value_helpers, test_parse_int64_minus_zero) {
  int64_t value;
  TEST_ASSERT_TRUE(parse_int64_value(&value, "-0"));
  TEST_ASSERT_EQUAL(0, value);
}

TEST(value_helpers, test_parse_int64_max) {
  int64_t value;
  TEST_ASSERT_TRUE(parse_int64_value(&value, "9223372036854775807"));
  TEST_ASSERT_EQUAL(INT64_MAX, value);
}

TEST(value_helpers, test_parse_int64_greater_than_max) {
  int64_t value;
  TEST_ASSERT_FALSE(parse_int64_value(&value, "9223372036854775808"));
}

/* uint64 */

static bool parse_uint64_value(uint64_t *value, const char *input) {
  tlv_parser parser;
  tlv_parser_init(&parser);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, input, &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);

  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.token.type);
  bool success = pjson_parse_uint64(value, parser.token.start, parser.token.length);

  pjson_free((uint8_t *)parser.token.start);

  return success;
}

TEST(value_helpers, test_parse_uint64_less_than_min) {
  uint64_t value;
  TEST_ASSERT_FALSE(parse_uint64_value(&value, "-1"));
}

TEST(value_helpers, test_parse_uint64_min) {
  uint64_t value;
  TEST_ASSERT_TRUE(parse_uint64_value(&value, "0"));
  TEST_ASSERT_EQUAL(0, value);
}

TEST(value_helpers, test_parse_uint64_minus_zero) {
  uint64_t value;
  TEST_ASSERT_FALSE(parse_uint64_value(&value, "-0"));
}

TEST(value_helpers, test_parse_uint64_max) {
  uint64_t value;
  TEST_ASSERT_TRUE(parse_uint64_value(&value, "18446744073709551615"));
  TEST_ASSERT_EQUAL(UINT64_MAX, value);
}

TEST(value_helpers, test_parse_uint64_greater_than_max) {
  uint64_t value;
  TEST_ASSERT_FALSE(parse_uint64_value(&value, "18446744073709551616"));
}

/* float */

static bool parse_float_value(float *value, const char *input) {
  tlv_parser parser;
  tlv_parser_init(&parser);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, input, &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);

  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.token.type);
  bool success = pjson_parse_float(value, parser.token.start, parser.token.length);

  pjson_free((uint8_t *)parser.token.start);

  return success;
}

TEST(value_helpers, test_parse_float_minus_smallest) {
  float value;
  TEST_ASSERT_TRUE(parse_float_value(&value, "-1.175494351e-38"));
  TEST_ASSERT_EQUAL_FLOAT(-1.175494351e-38f, value);
}

TEST(value_helpers, test_parse_float_minus_zero) {
  float value;
  TEST_ASSERT_TRUE(parse_float_value(&value, "-0.000"));
  TEST_ASSERT_EQUAL_FLOAT(0, value);
}

TEST(value_helpers, test_parse_float_zero) {
  float value;
  TEST_ASSERT_TRUE(parse_float_value(&value, "0.000"));
  TEST_ASSERT_EQUAL_FLOAT(0, value);
}

TEST(value_helpers, test_parse_float_plus_largest) {
  float value;
  TEST_ASSERT_TRUE(parse_float_value(&value, "3.402823466E+38"));
  TEST_ASSERT_EQUAL_FLOAT(3.402823466e+38f, value);
}

TEST(value_helpers, test_parse_float_long) {
  float value;
  TEST_ASSERT_TRUE(parse_float_value(&value, "3.14159265358979323846264338327950288419716939937510"));
  TEST_ASSERT_EQUAL_FLOAT(3.14159265358979323846264338327950288419716939937510f, value);
}

/* double */

static bool parse_double_value(double *value, const char *input) {
  tlv_parser parser;
  tlv_parser_init(&parser);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, input, &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);

  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.token.type);
  bool success = pjson_parse_double(value, parser.token.start, parser.token.length);

  pjson_free((uint8_t *)parser.token.start);

  return success;
}

TEST(value_helpers, test_parse_double_minus_smallest) {
  double value;
  TEST_ASSERT_TRUE(parse_double_value(&value, "-2.2250738585072014E-308"));
  TEST_ASSERT_EQUAL_DOUBLE(-2.2250738585072014E-308, value);
}

TEST(value_helpers, test_parse_double_minus_zero) {
  double value;
  TEST_ASSERT_TRUE(parse_double_value(&value, "-0.000"));
  TEST_ASSERT_EQUAL_DOUBLE(0, value);
}

TEST(value_helpers, test_parse_double_zero) {
  double value;
  TEST_ASSERT_TRUE(parse_double_value(&value, "0.000"));
  TEST_ASSERT_EQUAL_DOUBLE(0, value);
}

TEST(value_helpers, test_parse_double_plus_largest) {
  double value;
  TEST_ASSERT_TRUE(parse_double_value(&value, "1.7976931348623158e+308"));
  TEST_ASSERT_EQUAL_DOUBLE(1.7976931348623158e+308, value);
}

TEST(value_helpers, test_parse_double_long) {
  double value;
  TEST_ASSERT_TRUE(parse_double_value(&value, "3.14159265358979323846264338327950288419716939937510"));
  TEST_ASSERT_EQUAL_DOUBLE(3.14159265358979323846264338327950288419716939937510, value);
}

/* string */

static bool parse_string_value(char **value, const char *input, bool replace_lone_surrogates) {
  tlv_parser parser;
  tlv_parser_init(&parser);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, input, &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);

  TEST_ASSERT_EQUAL(PJSON_TOKEN_STRING, parser.token.type);
  *value = (char *)pjson_malloc(parser.token.unescaped_length + 1);
  bool success = pjson_parse_string((uint8_t *)*value, parser.token.unescaped_length, parser.token.start, parser.token.length, replace_lone_surrogates);
  (*value)[parser.token.unescaped_length] = 0;

  pjson_free((uint8_t *)parser.token.start);

  return success;
}

TEST(value_helpers, test_parse_string_ascii) {
  char *value;
  TEST_ASSERT_TRUE(parse_string_value(&value, "\"abc/123\"", false));
  TEST_ASSERT_EQUAL_STRING("abc/123", value);
  pjson_free(value);
}

TEST(value_helpers, test_parse_string_utf8) {
  char *value;
  TEST_ASSERT_TRUE(parse_string_value(&value, "\"\177 \337\277 \357\277\277 \360\220\200\200 \364\217\277\277\"", false));
  TEST_ASSERT_EQUAL_STRING("\177 \337\277 \357\277\277 \360\220\200\200 \364\217\277\277", value);
  pjson_free(value);
}

TEST(value_helpers, test_parse_string_basic_escape_sequences) {
  char *value;
  TEST_ASSERT_TRUE(parse_string_value(&value, "\"\\\" \\\\ \\b \\f \\n \\r \\t \\/ \\\"\"", false));
  TEST_ASSERT_EQUAL_STRING("\" \\ \b \f \n \r \t / \"", value);
  pjson_free(value);
}

TEST(value_helpers, test_parse_string_unicode_escape_sequences) {
  char *value;
  TEST_ASSERT_TRUE(parse_string_value(&value, "\"\\u007f \\u07ff \\uFFFF \\uD800\\udc00 \\udbff\\uDFFF\"", false));
  TEST_ASSERT_EQUAL_STRING("\177 \337\277 \357\277\277 \360\220\200\200 \364\217\277\277", value);
  pjson_free(value);
}

TEST(value_helpers, test_parse_string_lone_high_surrogate_followed_by_nothing_no_replace) {
  char *value;
  TEST_ASSERT_FALSE(parse_string_value(&value, "\"\\uD800\"", false));
  pjson_free(value);
}

TEST(value_helpers, test_parse_string_lone_high_surrogate_followed_by_nothing_replace) {
  char *value;
  TEST_ASSERT_TRUE(parse_string_value(&value, "\"\\uD800\"", true));
  TEST_ASSERT_EQUAL_STRING("\357\277\275", value);
  pjson_free(value);
}

TEST(value_helpers, test_parse_string_lone_high_surrogate_followed_by_nonescaped_no_replace) {
  char *value;
  TEST_ASSERT_FALSE(parse_string_value(&value, "\"\\uD800x\"", false));
  pjson_free(value);
}

TEST(value_helpers, test_parse_string_lone_high_surrogate_followed_by_nonescaped_replace) {
  char *value;
  TEST_ASSERT_TRUE(parse_string_value(&value, "\"\\uD800x\"", true));
  TEST_ASSERT_EQUAL_STRING("\357\277\275x", value);
  pjson_free(value);
}


TEST(value_helpers, test_parse_string_lone_high_surrogate_followed_by_basic_escape_no_replace) {
  char *value;
  TEST_ASSERT_FALSE(parse_string_value(&value, "\"\\uD800\\t\"", false));
  pjson_free(value);
}

TEST(value_helpers, test_parse_string_lone_high_surrogate_followed_by_basic_escape_replace) {
  char *value;
  TEST_ASSERT_TRUE(parse_string_value(&value, "\"\\uD800\\t\"", true));
  TEST_ASSERT_EQUAL_STRING("\357\277\275\t", value);
  pjson_free(value);
}

TEST(value_helpers, test_parse_string_lone_high_surrogate_followed_by_unicode_escape_no_replace) {
  char *value;
  TEST_ASSERT_FALSE(parse_string_value(&value, "\"\\uD800\\uDBFF\"", false));
  pjson_free(value);
}

TEST(value_helpers, test_parse_string_lone_high_surrogate_followed_by_unicode_escape_replace) {
  char *value;
  TEST_ASSERT_TRUE(parse_string_value(&value, "\"\\uD800\\uDBFF\"", true));
  TEST_ASSERT_EQUAL_STRING("\357\277\275\357\277\275", value);
  pjson_free(value);
}

TEST(value_helpers, test_parse_string_lone_low_surrogate_followed_by_nothing_no_replace) {
  char *value;
  TEST_ASSERT_FALSE(parse_string_value(&value, "\"\\uDC00\"", false));
  pjson_free(value);
}

TEST(value_helpers, test_parse_string_lone_low_surrogate_followed_by_nothing_replace) {
  char *value;
  TEST_ASSERT_TRUE(parse_string_value(&value, "\"\\uDC00\"", true));
  TEST_ASSERT_EQUAL_STRING("\357\277\275", value);
  pjson_free(value);
}

TEST(value_helpers, test_parse_string_lone_low_surrogate_followed_by_nonescaped_no_replace) {
  char *value;
  TEST_ASSERT_FALSE(parse_string_value(&value, "\"\\uDC00x\"", false));
  pjson_free(value);
}

TEST(value_helpers, test_parse_string_lone_low_surrogate_followed_by_nonescaped_replace) {
  char *value;
  TEST_ASSERT_TRUE(parse_string_value(&value, "\"\\uDC00x\"", true));
  TEST_ASSERT_EQUAL_STRING("\357\277\275x", value);
  pjson_free(value);
}

TEST_GROUP_RUNNER(value_helpers) {
  RUN_TEST_CASE(value_helpers, test_parse_int32_less_than_min);
  RUN_TEST_CASE(value_helpers, test_parse_int32_min);
  RUN_TEST_CASE(value_helpers, test_parse_int32_minus_zero);
  RUN_TEST_CASE(value_helpers, test_parse_int32_max);
  RUN_TEST_CASE(value_helpers, test_parse_int32_greater_than_max);

  RUN_TEST_CASE(value_helpers, test_parse_uint32_less_than_min);
  RUN_TEST_CASE(value_helpers, test_parse_uint32_min);
  RUN_TEST_CASE(value_helpers, test_parse_uint32_minus_zero);
  RUN_TEST_CASE(value_helpers, test_parse_uint32_max);
  RUN_TEST_CASE(value_helpers, test_parse_uint32_greater_than_max);

  RUN_TEST_CASE(value_helpers, test_parse_int64_less_than_min);
  RUN_TEST_CASE(value_helpers, test_parse_int64_min);
  RUN_TEST_CASE(value_helpers, test_parse_int64_minus_zero);
  RUN_TEST_CASE(value_helpers, test_parse_int64_max);
  RUN_TEST_CASE(value_helpers, test_parse_int64_greater_than_max);

  RUN_TEST_CASE(value_helpers, test_parse_uint64_less_than_min);
  RUN_TEST_CASE(value_helpers, test_parse_uint64_min);
  RUN_TEST_CASE(value_helpers, test_parse_uint64_minus_zero);
  RUN_TEST_CASE(value_helpers, test_parse_uint64_max);
  RUN_TEST_CASE(value_helpers, test_parse_uint64_greater_than_max);

  RUN_TEST_CASE(value_helpers, test_parse_float_minus_smallest);
  RUN_TEST_CASE(value_helpers, test_parse_float_minus_zero);
  RUN_TEST_CASE(value_helpers, test_parse_float_zero);
  RUN_TEST_CASE(value_helpers, test_parse_float_plus_largest);
  RUN_TEST_CASE(value_helpers, test_parse_float_long);

  RUN_TEST_CASE(value_helpers, test_parse_double_minus_smallest);
  RUN_TEST_CASE(value_helpers, test_parse_double_minus_zero);
  RUN_TEST_CASE(value_helpers, test_parse_double_zero);
  RUN_TEST_CASE(value_helpers, test_parse_double_plus_largest);
  RUN_TEST_CASE(value_helpers, test_parse_double_long);

  RUN_TEST_CASE(value_helpers, test_parse_string_ascii);
  RUN_TEST_CASE(value_helpers, test_parse_string_utf8);
  RUN_TEST_CASE(value_helpers, test_parse_string_basic_escape_sequences);
  RUN_TEST_CASE(value_helpers, test_parse_string_unicode_escape_sequences);
  RUN_TEST_CASE(value_helpers, test_parse_string_lone_high_surrogate_followed_by_nothing_no_replace);
  RUN_TEST_CASE(value_helpers, test_parse_string_lone_high_surrogate_followed_by_nothing_replace);
  RUN_TEST_CASE(value_helpers, test_parse_string_lone_high_surrogate_followed_by_nonescaped_no_replace);
  RUN_TEST_CASE(value_helpers, test_parse_string_lone_high_surrogate_followed_by_nonescaped_replace);
  RUN_TEST_CASE(value_helpers, test_parse_string_lone_high_surrogate_followed_by_basic_escape_no_replace);
  RUN_TEST_CASE(value_helpers, test_parse_string_lone_high_surrogate_followed_by_basic_escape_replace);
  RUN_TEST_CASE(value_helpers, test_parse_string_lone_high_surrogate_followed_by_unicode_escape_no_replace);
  RUN_TEST_CASE(value_helpers, test_parse_string_lone_high_surrogate_followed_by_unicode_escape_replace);
  RUN_TEST_CASE(value_helpers, test_parse_string_lone_low_surrogate_followed_by_nothing_no_replace);
  RUN_TEST_CASE(value_helpers, test_parse_string_lone_low_surrogate_followed_by_nothing_replace);
  RUN_TEST_CASE(value_helpers, test_parse_string_lone_low_surrogate_followed_by_nonescaped_no_replace);
  RUN_TEST_CASE(value_helpers, test_parse_string_lone_low_surrogate_followed_by_nonescaped_replace);
}
