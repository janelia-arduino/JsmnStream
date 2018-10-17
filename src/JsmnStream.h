// ----------------------------------------------------------------------------
// JsmnStream.h
//
//
// Authors:
// Serge Zaitsev zaitsev.serge@gmail.com
// Peter Polidoro peterpolidoro@gmail.com
// ----------------------------------------------------------------------------
#ifndef _JSMN_STREAM_H_
#define _JSMN_STREAM_H_
#include <Arduino.h>

#undef KEY_END


class JsmnStream
{
public:
  /**
   * JSON type identifier. Basic types are:
   *  o Object
   *  o Array
   *  o String
   *  o Other primitive: number, boolean (true/false) or null
   */
  typedef enum
  {
    JSMN_UNDEFINED = 0,
    JSMN_OBJECT = 1,
    JSMN_ARRAY = 2,
    JSMN_STRING = 3,
    JSMN_PRIMITIVE = 4
  } jsmntype_t;

  enum jsmnerr
  {
    /* Not enough tokens were provided */
    JSMN_ERROR_NOMEM = -1,
    /* Invalid character inside JSON string */
    JSMN_ERROR_INVAL = -2,
    /* The string is not a full JSON packet, more bytes expected */
    JSMN_ERROR_PART = -3
  };

  /**
   * JSON token description.
   * @param   type  type (object, array, string etc.)
   * @param   start start position in JSON data string
   * @param   end   end position in JSON data string
   */
  typedef struct
  {
    jsmntype_t type;
    int start;
    int end;
    int size;
#ifdef JSMN_PARENT_LINKS
    int parent;
#endif
  } jsmntok_t;

  /**
   * JSON parser. Contains an array of token blocks available. Also stores
   * the string being parsed now and current position in that string
   */
  typedef struct
  {
    unsigned int pos; /* offset in the JSON string */
    unsigned int toknext; /* next token to allocate */
    int toksuper; /* superior token node, e.g parent object or array */
  } jsmn_parser;

  enum CharParseResults
  {
    UNKNOWN,
    OBJECT_BEGIN,
    OBJECT_END,
    ARRAY_BEGIN,
    ARRAY_END,
    STRING_BEGIN,
    STRING_END,
    STRING_BACKSLASH,
    STRING_CHAR,
    WHITESPACE,
    KEY_END,
    VALUE_END,
    PRIMATIVE_BEGIN,
    PRIMATIVE_CHAR
  };

  /**
   * Create JSON parser over an array of tokens
   */
  template <size_t TOKEN_COUNT_MAX>
  JsmnStream(jsmntok_t (&tokens)[TOKEN_COUNT_MAX]);

  /**
   * Run JSON parser. It parses a JSON data string into and array of tokens, each describing
   * a single JSON object.
   */
  int parseJson(const char *js);
  int parseChar(const char c);
  size_t getTokenCount();
  int checkParse();
  void resetParser();
private:
  jsmn_parser parser_;
  jsmntok_t *tokens_;
  size_t token_count_max_;
  CharParseResults char_parse_result_;
  int start_;
  void setup();
  jsmntok_t *allocToken();
  void fillToken(jsmntok_t *token_ptr, jsmntype_t type, int start, int end);
  int parsePrimitiveChar(const char c);
  int parseStringChar(const char c);
};
#include "JsmnStream/JsmnStreamDefinitions.h"

#endif /* _JSMN_STREAM_H_ */
