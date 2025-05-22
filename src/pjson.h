/*
3-Clause BSD Non-AI License

Copyright (c) 2024 Adam Simon. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

4. The source code, and any modifications made to it may not be used for the
   purpose of training or improving machine learning algorithms, including but
   not limited to artificial intelligence, natural language processing, or
   data mining. This condition applies to any derivatives, modifications, or
   updates based on the Software code. Any usage of the source code in an
   AI-training dataset is considered a breach of this License.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __PJSON_H__
#define __PJSON_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef PJSON_CONFIG_H
#define PJSON_CONFIG_H "pjson_config.h"
#endif

#include PJSON_CONFIG_H

#define pjson_countof(arr) \
  (sizeof(arr) / sizeof((arr)[0]))

#if defined(__cplusplus)
extern "C" {
#endif
  // Note for maintainers: enum values must not be changed as parsing logic relies on them!
  // Errors must be represented using negative values less than -1.
  // Value -1 is reserved for special purposes (see STATE_EOS).
  // (Values other than PJSON_STATUS_DATA_NEEDED causes pjson_feed to return.)

  typedef enum pjson_parsing_status {
    PJSON_STATUS_USER_ERROR = -0x20,
    PJSON_STATUS_SYNTAX_ERROR = -0x10,
    PJSON_STATUS_UTF8_ERROR = -0xF,
    PJSON_STATUS_OUT_OF_MEMORY = -5,
    PJSON_STATUS_NONCOMPLIANT_PARSER = -4,
    PJSON_STATUS_MAX_DEPTH_EXCEEDED = -3,
    PJSON_STATUS_NO_TOKENS_FOUND = -2,
    PJSON_STATUS_SUCCESS = 0,
    PJSON_STATUS_DATA_NEEDED = PJSON_STATUS_SUCCESS,
    PJSON_STATUS_COMPLETED = 1,
  } pjson_parsing_status;

  // Note for maintainers: enum values must not be changed as parsing logic relies on them!
  // (Values of PJSON_TOKEN_NULL, PJSON_TOKEN_FALSE and PJSON_TOKEN_TRUE are used for indexing into an array.)

  typedef enum json_token_type {
    PJSON_TOKEN_ERROR = -1,
    PJSON_TOKEN_NONE = 0,
    PJSON_TOKEN_NULL = 1,
    PJSON_TOKEN_FALSE = 2,
    PJSON_TOKEN_TRUE = 3,
    PJSON_TOKEN_NUMBER = 4,
    PJSON_TOKEN_STRING = 5,
    PJSON_TOKEN_OPEN_BRACKET = 6,
    PJSON_TOKEN_OPEN_BRACE = 7,
    PJSON_TOKEN_CLOSE_BRACKET = 8,
    PJSON_TOKEN_CLOSE_BRACE = 9,
    PJSON_TOKEN_COLON = 10,
    PJSON_TOKEN_COMMA = 11,
    PJSON_TOKEN_EOS = 12,
  } pjson_token_type;

  typedef struct pjson_token {
    pjson_token_type type;
    size_t start_index;
    /** Pointer to the first byte of the token. Valid for the current call only, must not be stored outside the stack. */
    const uint8_t /* non-owning */ *start;
    /** Length of token in bytes. */
    size_t length;
    /** In the case of string tokens, the length of the unescaped, UTF-8 encoded string value (excluding quotes) in bytes. */
    size_t unescaped_length;
  } pjson_token;

  typedef struct pjson_parser_base pjson_parser_base;

  /* Tokenizer */

  /** Stores mostly internal state. Do not modify members directly. */
  typedef struct pjson_tokenizer {
    pjson_parser_base /* non-owning */ *parser;
    size_t index;
    size_t token_start_index;
    /**
     * Used internally only, except when PJSON_STATUS_COMPLETED is returned by pjson_feed.
     * In that case, it points to where the next token may start in the user-provided buffer.
     * (If the next token does not start in the buffer, it points to the byte NEXT to the last byte of the buffer.)
     * This allows user to continue receiving further items when parsing a stream of JSON values.
     */
    const uint8_t /* non-owning */ *token_start;
    pjson_token_type token_type;
    int state;
    union {
      uint8_t utf8_sequence_buf[4];
      uint16_t utf16_surrogate_pair[2];
    } string_state;
    size_t unescaped_length;
    uint8_t fixed_size_buf[PJSON_INTERNAL_BUFFER_FIXED_SIZE];
    uint8_t /* owning */ *buf; // owned by pjson_tokenizer, mananged by pjson_init & pjson_close.
    size_t buf_length;
    size_t buf_capacity;
  } pjson_tokenizer;

  /**
   * Initializes a JSON tokenizer with an optional JSON parser.
   * @param tokenizer Pointer to a `pjson_tokenizer` struct. Required, cannot be `NULL`.
   * @param parser Pointer to a `pjson_parser_base` struct. Optional, can be `NULL`.
   */
  void PJSON_API(pjson_init)(pjson_tokenizer *tokenizer, pjson_parser_base *parser);

  pjson_parsing_status PJSON_API(pjson_feed)(pjson_tokenizer *tokenizer, const uint8_t *data, size_t length);

  pjson_parsing_status PJSON_API(pjson_close)(pjson_tokenizer *tokenizer);

  /* Parser */

  typedef pjson_parsing_status(*pjson_parser_eat)(pjson_parser_base *parser, const pjson_token *token);

  typedef struct pjson_parser_base {
    pjson_parser_eat eat;
  } pjson_parser_base;

  typedef struct pjson_parser pjson_parser;

  typedef struct pjson_parser_context pjson_parser_context;

  typedef pjson_parsing_status(*pjson_parser_context_callback)(pjson_parser *parser, pjson_parser_context *context, const pjson_token *token);

  typedef struct pjson_parser_context {
    /** Stores internal state. Do not modify it directly. */
    pjson_parser_eat next_eat;

    /**
     * User-provided function that is called when consuming a JSON value (null, boolean, number, string, array or object). Optional, can be `NULL`.
     *
     * @remarks
     * For arrays and objects it is called twice: once when beginning and once when finishing parsing the value. The actual case can be detected by looking at `token->type`.
     */
    pjson_parser_context_callback on_value;

    /**
     * User-provided function that is called when consuming an object property name. Optional, can be `NULL`.
     */
    pjson_parser_context_callback on_object_property_name;
  } pjson_parser_context;

  typedef pjson_parsing_status(*pjson_parser_push_context)(pjson_parser *parser);
  typedef pjson_parser_context *(*pjson_parser_peek_context)(pjson_parser *parser, bool previous);
  typedef void(*pjson_parser_pop_context)(pjson_parser *parser);

  /** Stores mostly internal state. Do not modify members directly. */
  typedef struct pjson_parser {
    pjson_parser_base base;

    pjson_parser_push_context push_context;
    pjson_parser_peek_context peek_context;
    pjson_parser_pop_context pop_context;
  } pjson_parser;

  /**
   * Initializes a user-defined JSON parser.
   * @param parser Pointer to a `pjson_parser` struct. Required, cannot be `NULL`.
   * @param is_lazy Specifies whether to return `PJSON_STATUS_COMPLETED` as soon as a complete JSON value is parsed.
   * Set this to `true` if you want to parse a stream of multiple JSON values.
   * @param push_context User-provided function that is used to store the current context when entering a new context
   * (top-level, array or object). Required, cannot be `NULL`.
   * @param peek_context User-provided function that is used to retrieve the previous or current context
   * (top-level, array or object). Required, cannot be `NULL`.
   * @param pop_context User-provided function that is used to restore the previous context when leaving the current context
   * (top-level, array or object). Required, cannot be `NULL`.
   */
  void PJSON_API(pjson_parser_init)(pjson_parser *parser, bool is_lazy,
    pjson_parser_push_context push_context,
    pjson_parser_peek_context peek_context,
    pjson_parser_pop_context pop_context
  );

  void PJSON_API(pjson_parser_reset)(pjson_parser *parser, bool is_lazy);

  /* Helpers */

  bool PJSON_API(pjson_parse_string)(uint8_t *dest, size_t dest_size, const uint8_t *token_start, size_t token_length, bool replace_lone_surrogates);
  bool PJSON_API(pjson_parse_uint32)(uint32_t *num, const uint8_t *token_start, size_t token_length);
  bool PJSON_API(pjson_parse_int32)(int32_t *num, const uint8_t *token_start, size_t token_length);
  bool PJSON_API(pjson_parse_uint64)(uint64_t *num, const uint8_t *token_start, size_t token_length);
  bool PJSON_API(pjson_parse_int64)(int64_t *num, const uint8_t *token_start, size_t token_length);
  bool PJSON_API(pjson_parse_float)(float *num, const uint8_t *token_start, size_t token_length);
  bool PJSON_API(pjson_parse_double)(double *num, const uint8_t *token_start, size_t token_length);

#if defined(__cplusplus)
}
#endif

#endif // __PJSON_H__
