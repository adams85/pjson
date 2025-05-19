#include <assert.h>
#include <string.h>

#include "unity_fixture.h"
#include "pjson.h"
#include "stats_parser.h"

TEST_GROUP(basics);

TEST_SETUP(basics) {}

TEST_TEAR_DOWN(basics) {}

static pjson_parsing_status feed_string(pjson_tokenizer *tokenizer, const char *input) {
  return pjson_feed(tokenizer, (uint8_t *)input, strlen(input));
}

static pjson_parsing_status parse_string(stats_parser *parser, const char *input, pjson_parsing_status *feed_status) {
  pjson_tokenizer tokenizer;
  pjson_init(&tokenizer, &parser->base.base);
  pjson_parsing_status status = feed_string(&tokenizer, input);
  if (feed_status) *feed_status = status;
  return pjson_close(&tokenizer);
}

/* Empty & whitespace input */

TEST(basics, test_parse_empty_input_greedy) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_NO_TOKENS_FOUND, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NONE, parser.toplevel_datatype);
}

TEST(basics, test_parse_empty_input_lazy) {
  stats_parser parser;
  stats_parser_init(&parser, true);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_NO_TOKENS_FOUND, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NONE, parser.toplevel_datatype);
}

TEST(basics, test_parse_whitespace_input_greedy) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "   ", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_NO_TOKENS_FOUND, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NONE, parser.toplevel_datatype);
}

TEST(basics, test_parse_whitespace_input_lazy) {
  stats_parser parser;
  stats_parser_init(&parser, true);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "   ", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_NO_TOKENS_FOUND, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NONE, parser.toplevel_datatype);
}

/* Keywords */

TEST(basics, test_parse_null_greedy) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "null", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NULL, parser.toplevel_datatype);
}

TEST(basics, test_parse_null_lazy) {
  stats_parser parser;
  stats_parser_init(&parser, true);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "null", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NULL, parser.toplevel_datatype);
}

TEST(basics, test_parse_false_greedy) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "\rfalse\t", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_FALSE, parser.toplevel_datatype);
}

TEST(basics, test_parse_false_lazy) {
  stats_parser parser;
  stats_parser_init(&parser, true);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "\rfalse\t", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_FALSE, parser.toplevel_datatype);
}

TEST(basics, test_parse_true_greedy) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, " true\r", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_TRUE, parser.toplevel_datatype);
}

TEST(basics, test_parse_true_lazy) {
  stats_parser parser;
  stats_parser_init(&parser, true);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, " true\n", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_TRUE, parser.toplevel_datatype);
}

/* String */

TEST(basics, test_parse_string_greedy) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "\n\"abc|\x61\xC2\x88\xE0\xA2\x88\xF0\x98\xA2\x88|\\\\\\/\\\"\\b\\t\\f\\n\\r|\\u0065\\uD83D\\uD83d\\uDca9\\uDCA9\\ubEeB\\uffFF\"\r", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_STRING, parser.toplevel_datatype);
}

TEST(basics, test_parse_string_lazy) {
  stats_parser parser;
  stats_parser_init(&parser, true);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "\n\"abc|\x61\xC2\x88\xE0\xA2\x88\xF0\x98\xA2\x88|\\\\\\/\\\"\\b\\t\\f\\n\\r|\\u0065\\uD83D\\uD83d\\uDca9\\uDCA9\\ubEeB\\uffFF\"\r", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_STRING, parser.toplevel_datatype);
}

/* Number */

TEST(basics, test_parse_number_greedy) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "-1.0203e+4", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.toplevel_datatype);
}

TEST(basics, test_parse_number_lazy) {
  stats_parser parser;
  stats_parser_init(&parser, true);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "-1.0203e+4", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.toplevel_datatype);
}

TEST(basics, test_parse_number_zero) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, " 0 ", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.toplevel_datatype);
}

TEST(basics, test_parse_number_zero_fractional) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, " 0.1e2\t", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.toplevel_datatype);
}

TEST(basics, test_parse_number_zero_exponential) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, " 0E+0\r\n", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.toplevel_datatype);
}

TEST(basics, test_parse_number_negative_zero) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, " -0 ", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.toplevel_datatype);
}

TEST(basics, test_parse_number_negative_zero_fractional) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, " -0.1E2\t", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.toplevel_datatype);
}

TEST(basics, test_parse_number_negative_zero_exponential) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, " -0e-0\r\n", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.toplevel_datatype);
}

TEST(basics, test_parse_number_nonzero_integer) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "\n1234567890\r", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.toplevel_datatype);
}

TEST(basics, test_parse_number_negative_nonzero_integer) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "\t-9876543210\t", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.toplevel_datatype);
}

TEST(basics, test_parse_number_nonzero_integer_with_exponent) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "\n1234567890E00123456789\r", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.toplevel_datatype);
}

TEST(basics, test_parse_number_nonzero_integer_with_positive_exponent) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "\n1234567890e+00123456789\r", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.toplevel_datatype);
}

TEST(basics, test_parse_number_nonzero_integer_with_negative_exponent) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "\n1234567890e-00123456789\r", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.toplevel_datatype);
}

TEST(basics, test_parse_number_nonzero_fractional) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "1.234567890\t", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.toplevel_datatype);
}

TEST(basics, test_parse_number_negative_nonzero_fractional) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "-1.234567890 ", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.toplevel_datatype);
}

TEST(basics, test_parse_number_nonzero_fraction_with_exponent) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "1.234567890E00123456789\t", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.toplevel_datatype);
}

TEST(basics, test_parse_number_nonzero_fraction_with_positive_exponent) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "1.234567890e+00123456789\t", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.toplevel_datatype);
}

TEST(basics, test_parse_number_nonzero_fraction_with_negative_exponent) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "1.234567890e-00123456789\t", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.toplevel_datatype);
}

/* Array */

TEST(basics, test_parse_array_greedy) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "[\n0, \"1\" , null, \nfalse, \ntrue,\r\n[], {}\t]", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_CLOSE_BRACKET, parser.toplevel_datatype);
}

TEST(basics, test_parse_array_lazy) {
  stats_parser parser;
  stats_parser_init(&parser, true);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "[\n0, \"1\" , null, \nfalse, \ntrue,\r\n[], {}\t]", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_CLOSE_BRACKET, parser.toplevel_datatype);
}

TEST(basics, test_parse_empty_array) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "[]", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_CLOSE_BRACKET, parser.toplevel_datatype);
}

TEST(basics, test_parse_nested_arrays) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "[[ ]]", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_CLOSE_BRACKET, parser.toplevel_datatype);
}

/* Object */

TEST(basics, test_parse_object_greedy) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "{\n\"num\":0, \"str\" : \"1\" , \n\"null\": null, \n\"bool\": false, \n\"bool\": true,\r\n\"arr\": [], \"obj\": {}\t}", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_CLOSE_BRACE, parser.toplevel_datatype);
}

TEST(basics, test_parse_object_lazy) {
  stats_parser parser;
  stats_parser_init(&parser, true);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "{\n\"num\":0, \"str\" : \"1\" , \n\"null\": null, \n\"bool\": false, \n\"bool\": true,\r\n\"arr\": [], \"obj\": {}\t}", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_CLOSE_BRACE, parser.toplevel_datatype);
}

TEST(basics, test_parse_empty_object) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "{}", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_CLOSE_BRACE, parser.toplevel_datatype);
}

TEST(basics, test_parse_nested_objects) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_parsing_status feed_status;
  pjson_parsing_status close_status = parse_string(&parser, "{\"o\":{ }}", &feed_status);

  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, close_status);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_CLOSE_BRACE, parser.toplevel_datatype);
}

/* Multiple values */

TEST(basics, test_parse_multiple_toplevel_values_greedy) {
  stats_parser parser;
  stats_parser_init(&parser, false);

  pjson_tokenizer tokenizer;
  pjson_init(&tokenizer, &parser.base.base);

  static char *input = "0.12{ }";
  pjson_parsing_status feed_status = feed_string(&tokenizer, input);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, feed_status);
  TEST_ASSERT_EQUAL(4, tokenizer.index);

  feed_status = feed_string(&tokenizer, input);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, feed_status);

  pjson_parsing_status close_status = pjson_close(&tokenizer);
  TEST_ASSERT_EQUAL(PJSON_STATUS_SYNTAX_ERROR, close_status);
  TEST_ASSERT_EQUAL(4, tokenizer.index);

  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.toplevel_datatype);
}

TEST(basics, test_parse_multiple_toplevel_values_lazy) {
  stats_parser parser;
  stats_parser_init(&parser, true);

  pjson_tokenizer tokenizer;
  pjson_init(&tokenizer, &parser.base.base);

  static char *input = "0.12{ }";
  pjson_parsing_status feed_status = feed_string(&tokenizer, input);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, feed_status);
  TEST_ASSERT_EQUAL(4, tokenizer.index);
  TEST_ASSERT_EQUAL(input + 4, tokenizer.token_start);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NUMBER, parser.toplevel_datatype);

  stats_parser_reset(&parser, true);

  feed_status = feed_string(&tokenizer, input + 4);
  TEST_ASSERT_EQUAL(PJSON_STATUS_COMPLETED, feed_status);
  TEST_ASSERT_EQUAL(7, tokenizer.index);
  TEST_ASSERT_EQUAL(input + 7, tokenizer.token_start);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_CLOSE_BRACE, parser.toplevel_datatype);

  stats_parser_reset(&parser, true);

  feed_status = feed_string(&tokenizer, input + 7);
  TEST_ASSERT_EQUAL(PJSON_STATUS_DATA_NEEDED, feed_status);
  TEST_ASSERT_EQUAL(7, tokenizer.index);

  pjson_parsing_status close_status = pjson_close(&tokenizer);
  TEST_ASSERT_EQUAL(PJSON_STATUS_NO_TOKENS_FOUND, close_status);
  TEST_ASSERT_EQUAL(7, tokenizer.index);
  TEST_ASSERT_NULL(tokenizer.token_start);
  TEST_ASSERT_EQUAL(PJSON_TOKEN_NONE, parser.toplevel_datatype);
}

TEST_GROUP_RUNNER(basics) {
  RUN_TEST_CASE(basics, test_parse_empty_input_greedy);
  RUN_TEST_CASE(basics, test_parse_empty_input_lazy);
  RUN_TEST_CASE(basics, test_parse_whitespace_input_greedy);
  RUN_TEST_CASE(basics, test_parse_whitespace_input_lazy);

  RUN_TEST_CASE(basics, test_parse_null_greedy);
  RUN_TEST_CASE(basics, test_parse_null_lazy);
  RUN_TEST_CASE(basics, test_parse_false_greedy);
  RUN_TEST_CASE(basics, test_parse_false_lazy);
  RUN_TEST_CASE(basics, test_parse_true_greedy);
  RUN_TEST_CASE(basics, test_parse_true_lazy);

  RUN_TEST_CASE(basics, test_parse_string_greedy);
  RUN_TEST_CASE(basics, test_parse_string_lazy);

  RUN_TEST_CASE(basics, test_parse_number_greedy);
  RUN_TEST_CASE(basics, test_parse_number_lazy);
  RUN_TEST_CASE(basics, test_parse_number_zero);
  RUN_TEST_CASE(basics, test_parse_number_zero_fractional);
  RUN_TEST_CASE(basics, test_parse_number_zero_exponential);
  RUN_TEST_CASE(basics, test_parse_number_negative_zero);
  RUN_TEST_CASE(basics, test_parse_number_negative_zero_fractional);
  RUN_TEST_CASE(basics, test_parse_number_negative_zero_exponential);
  RUN_TEST_CASE(basics, test_parse_number_nonzero_integer);
  RUN_TEST_CASE(basics, test_parse_number_negative_nonzero_integer);
  RUN_TEST_CASE(basics, test_parse_number_nonzero_integer_with_exponent);
  RUN_TEST_CASE(basics, test_parse_number_nonzero_integer_with_positive_exponent);
  RUN_TEST_CASE(basics, test_parse_number_nonzero_integer_with_negative_exponent);
  RUN_TEST_CASE(basics, test_parse_number_nonzero_fractional);
  RUN_TEST_CASE(basics, test_parse_number_negative_nonzero_fractional);
  RUN_TEST_CASE(basics, test_parse_number_nonzero_fraction_with_exponent);
  RUN_TEST_CASE(basics, test_parse_number_nonzero_fraction_with_positive_exponent);
  RUN_TEST_CASE(basics, test_parse_number_nonzero_fraction_with_negative_exponent);

  RUN_TEST_CASE(basics, test_parse_array_greedy);
  RUN_TEST_CASE(basics, test_parse_array_lazy);
  RUN_TEST_CASE(basics, test_parse_empty_array);
  RUN_TEST_CASE(basics, test_parse_nested_arrays);

  RUN_TEST_CASE(basics, test_parse_object_greedy);
  RUN_TEST_CASE(basics, test_parse_object_lazy);
  RUN_TEST_CASE(basics, test_parse_empty_object);
  RUN_TEST_CASE(basics, test_parse_nested_objects);

  RUN_TEST_CASE(basics, test_parse_multiple_toplevel_values_greedy);
  RUN_TEST_CASE(basics, test_parse_multiple_toplevel_values_lazy);
}
