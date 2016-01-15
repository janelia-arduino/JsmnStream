// ----------------------------------------------------------------------------
// JsmnStream.h
//
//
// Authors:
// Serge Zaitsev zaitsev.serge@gmail.com
// Peter Polidoro polidorop@janelia.hhmi.org
// ----------------------------------------------------------------------------
#ifndef _JSMN_STREAM_H_
#define _JSMN_STREAM_H_
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif


class JsmnStream
{
public:
  /**
   * JSON type identifier. Basic types are:
   * 	o Object
   * 	o Array
   * 	o String
   * 	o Other primitive: number, boolean (true/false) or null
   */
  enum jsmntype_t
    {
      JSMN_UNDEFINED = 0,
      JSMN_OBJECT = 1,
      JSMN_ARRAY = 2,
      JSMN_STRING = 3,
      JSMN_PRIMITIVE = 4
    };

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
   * @param		type	type (object, array, string etc.)
   * @param		start	start position in JSON data string
   * @param		end		end position in JSON data string
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

  enum CharParseStates
    {
      PARSING_ROOT,
      PARSING_STRING,
      PARSING_PRIMATIVE,
    };

  /**
   * Create JSON parser over an array of tokens
   */
  // void jsmn_init(jsmn_parser *parser);
  template <size_t NUM_TOKENS>
  JsmnStream(jsmntok_t (&tokens)[NUM_TOKENS]);

  /**
   * Run JSON parser. It parses a JSON data string into and array of tokens, each describing
   * a single JSON object.
   */
  int parseJson(const char *js);
  int parseChar(const char c);
  // size_t getTokenLen(jsmntok_t &token);
private:
  jsmn_parser parser_;
  jsmntok_t *tokens_;
  size_t num_tokens_;
  int count_;
  CharParseStates char_parse_state_;
  void setup();
  jsmntok_t *allocToken();
  void fillToken(jsmntok_t *token, jsmntype_t type, int start, int end);
  int parsePrimitive(const char *js, size_t len);
  int parseString(const char *js, size_t len);
};
#include "JsmnStreamDefinitions.h"

#endif /* _JSMN_STREAM_H_ */