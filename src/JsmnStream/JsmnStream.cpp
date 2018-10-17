// ----------------------------------------------------------------------------
// JsonStream.cpp
//
//
// Authors:
// Serge Zaitsev zaitsev.serge@gmail.com
// Peter Polidoro peterpolidoro@gmail.com
// ----------------------------------------------------------------------------
#include "../JsmnStream.h"


/**
 * Parse JSON string one char at a time and fill tokens.
 */
int JsmnStream::parseChar(const char c)
{
  int r;
  jsmntok_t *token_ptr;
  jsmntype_t type;
  int i;
  switch (char_parse_result_)
  {
    case UNKNOWN:
    case OBJECT_BEGIN:
    case OBJECT_END:
    case ARRAY_BEGIN:
    case ARRAY_END:
    case STRING_END:
    case WHITESPACE:
    case KEY_END:
    case VALUE_END:
      switch (c)
      {
        case '{': case '[':
          if (tokens_ == NULL)
          {
            break;
          }
          token_ptr = allocToken();
          if (token_ptr == NULL)
          {
            return JSMN_ERROR_NOMEM;
          }
          if (parser_.toksuper != -1)
          {
            tokens_[parser_.toksuper].size++;
#ifdef JSMN_PARENT_LINKS
            token_ptr->parent = parser_.toksuper;
#endif
          }
          char_parse_result_ = (c == '{' ? OBJECT_BEGIN : ARRAY_BEGIN);
          token_ptr->type = (c == '{' ? JSMN_OBJECT : JSMN_ARRAY);
          token_ptr->start = parser_.pos;
          parser_.toksuper = parser_.toknext - 1;
          break;
        case '}': case ']':
          if (tokens_ == NULL)
          {
            break;
          }
          char_parse_result_ = (c == '}' ? OBJECT_END : ARRAY_END);
          type = (c == '}' ? JSMN_OBJECT : JSMN_ARRAY);
#ifdef JSMN_PARENT_LINKS
          if (parser_.toknext < 1)
          {
            return JSMN_ERROR_INVAL;
          }
          token_ptr = &tokens_[parser_.toknext - 1];
          for (;;)
          {
            if (token_ptr->start != -1 && token_ptr->end == -1)
            {
              if (token_ptr->type != type || parser_.toksuper == -1)
              {
                return JSMN_ERROR_INVAL;
              }
              token_ptr->end = parser_.pos + 1;
              parser_.toksuper = token_ptr->parent;
              break;
            }
            if (token_ptr->parent == -1)
            {
              break;
            }
            token_ptr = &tokens_[token_ptr->parent];
          }
#else
          for (i = parser_.toknext - 1; i >= 0; i--)
          {
            token_ptr = &tokens_[i];
            if (token_ptr->start != -1 && token_ptr->end == -1)
            {
              if (token_ptr->type != type)
              {
                return JSMN_ERROR_INVAL;
              }
              parser_.toksuper = -1;
              token_ptr->end = parser_.pos + 1;
              break;
            }
          }
          /* Error if unmatched closing bracket */
          if (i == -1)
          {
            return JSMN_ERROR_INVAL;
          }
          for (; i >= 0; i--)
          {
            token_ptr = &tokens_[i];
            if (token_ptr->start != -1 && token_ptr->end == -1)
            {
              parser_.toksuper = i;
              break;
            }
          }
#endif
          break;
        case '\"':
          start_ = parser_.pos;
          char_parse_result_ = STRING_BEGIN;
          break;
        case '\t' : case '\r' : case '\n' : case ' ':
          char_parse_result_ = WHITESPACE;
          break;
        case ':':
          char_parse_result_ = KEY_END;
          parser_.toksuper = parser_.toknext - 1;
          break;
        case ',':
          char_parse_result_ = VALUE_END;
          if (tokens_ != NULL && parser_.toksuper != -1 &&
            tokens_[parser_.toksuper].type != JSMN_ARRAY &&
            tokens_[parser_.toksuper].type != JSMN_OBJECT)
          {
#ifdef JSMN_PARENT_LINKS
            parser_.toksuper = tokens_[parser_.toksuper].parent;
#else
            for (i = parser_.toknext - 1; i >= 0; i--)
            {
              if (tokens_[i].type == JSMN_ARRAY || tokens_[i].type == JSMN_OBJECT)
              {
                if (tokens_[i].start != -1 && tokens_[i].end == -1)
                {
                  parser_.toksuper = i;
                  break;
                }
              }
            }
#endif
          }
          break;
#ifdef JSMN_STRICT
          /* In strict mode primitives are: numbers and booleans */
        case '-': case '0': case '1' : case '2': case '3' : case '4':
        case '5': case '6': case '7' : case '8': case '9':
        case 't': case 'f': case 'n' :
          /* And they must not be keys of the object */
          if (tokens_ != NULL && parser_.toksuper != -1)
          {
            jsmntok_t *t = &tokens_[parser_.toksuper];
            if (t->type == JSMN_OBJECT ||
              (t->type == JSMN_STRING && t->size != 0))
            {
              return JSMN_ERROR_INVAL;
            }
          }
#else
          /* In non-strict mode every unquoted value is a primitive */
        default:
#endif
          start_ = parser_.pos;
          char_parse_result_ = PRIMATIVE_BEGIN;
          break;

#ifdef JSMN_STRICT
          /* Unexpected char in strict mode */
        default:
          return JSMN_ERROR_INVAL;
#endif
      }
      break;
    case STRING_BEGIN:
    case STRING_BACKSLASH:
    case STRING_CHAR:
      r = parseStringChar(c);
      if (r < 0)
      {
        return r;
      }
      break;
    case PRIMATIVE_BEGIN:
    case PRIMATIVE_CHAR:
      r = parsePrimitiveChar(c);
      if (r < 0)
      {
        return r;
      }
      break;
  }
  parser_.pos++;
  // Serial << "parser_.pos = " << parser_.pos << "\n";
  // Serial << "parser_.toknext = " << parser_.toknext << "\n";
  // Serial << "parser_.toksuper = " << parser_.toksuper << "\n";
  return char_parse_result_;
}

/**
 * Parse JSON string and fill tokens.
 */
int JsmnStream::parseJson(const char *js)
{
  int r;
  size_t len = strlen(js);

  while (parser_.pos < len && js[parser_.pos] != '\0')
  {
    char c;

    c = js[parser_.pos];
    r = parseChar(c);
    if (r < 0)
    {
      return r;
    }
  }
  r = checkParse();
  if (r < 0)
  {
    return r;
  }
  return getTokenCount();
}

size_t JsmnStream::getTokenCount()
{
  return parser_.toknext;
}

int JsmnStream::checkParse()
{
  if (tokens_ != NULL)
  {
    for (int i = parser_.toknext - 1; i >= 0; i--)
    {
      /* Unmatched opened object or array */
      if (tokens_[i].start != -1 && tokens_[i].end == -1)
      {
        return JSMN_ERROR_PART;
      }
    }
    return 0;
  }
  return JSMN_ERROR_NOMEM;
}

void JsmnStream::resetParser()
{
  setup();
}

/**
 * Creates a new parser based over a given buffer with an array of tokens
 * available.
 */
void JsmnStream::setup()
{
  parser_.pos = 0;
  parser_.toknext = 0;
  parser_.toksuper = -1;
  char_parse_result_ = UNKNOWN;
  start_ = 0;
}

/**
 * Allocates a fresh unused token from the token pull.
 */
JsmnStream::jsmntok_t* JsmnStream::allocToken()
{
  jsmntok_t *tok;
  if (parser_.toknext >= token_count_max_)
  {
    return NULL;
  }
  tok = &tokens_[parser_.toknext++];
  tok->start = tok->end = -1;
  tok->size = 0;
#ifdef JSMN_PARENT_LINKS
  tok->parent = -1;
#endif
  return tok;
}

/**
 * Fills token type and boundaries.
 */
void JsmnStream::fillToken(jsmntok_t *token_ptr, jsmntype_t type, int start, int end)
{
  token_ptr->type = type;
  token_ptr->start = start;
  token_ptr->end = end;
  token_ptr->size = 0;
}

/**
 * Fills next available token with JSON primitive.
 */
int JsmnStream::parsePrimitiveChar(const char c)
{
  switch (c)
  {
#ifndef JSMN_STRICT
    /* In strict mode primitive must be followed by "," or "}" or "]" */
    case ':':
#endif
    case '\t' : case '\r' : case '\n' : case ' ' :
    case ','  : case ']'  : case '}' :
      goto found;
    default:
      char_parse_result_ = PRIMATIVE_CHAR;
      break;
  }
  if (c < 32 || c >= 127)
  {
    parser_.pos = start_;
    return JSMN_ERROR_INVAL;
  }
  else
  {
    return char_parse_result_;
  }
  // }
#ifdef JSMN_STRICT
  /* In strict mode primitive must be followed by a comma/object/array */
  parser_.pos = start_;
  return JSMN_ERROR_PART;
#endif

 found:
  // Modified on next call to parseChar
  char_parse_result_ = UNKNOWN;
  if (tokens_ == NULL)
  {
    parser_.pos--;
    return parseChar(c);
  }
  jsmntok_t *token_ptr = allocToken();
  if (token_ptr == NULL)
  {
    parser_.pos = start_;
    return JSMN_ERROR_NOMEM;
  }
  fillToken(token_ptr, JSMN_PRIMITIVE, start_, parser_.pos);
#ifdef JSMN_PARENT_LINKS
  token_ptr->parent = parser_.toksuper;
#endif
  if (parser_.toksuper != -1 && tokens_ != NULL)
  {
    tokens_[parser_.toksuper].size++;
  }
  parser_.pos--;
  return parseChar(c);
}

/**
 * Fills next token with JSON string.
 */
int JsmnStream::parseStringChar(const char c)
{
  switch (char_parse_result_)
  {
    case STRING_BEGIN:
    case STRING_CHAR:
      if (c == '\\')
      {
        char_parse_result_ = STRING_BACKSLASH;
        break;
      }
      /* Quote: end of string */
      else if (c == '\"')
      {
        char_parse_result_ = STRING_END;
        if (tokens_ == NULL)
        {
          break;
        }
        jsmntok_t *token_ptr = allocToken();
        if (token_ptr == NULL)
        {
          parser_.pos = start_;
          return JSMN_ERROR_NOMEM;
        }
        fillToken(token_ptr, JSMN_STRING, start_+1, parser_.pos);
#ifdef JSMN_PARENT_LINKS
        token_ptr->parent = parser_.toksuper;
#endif
        if (parser_.toksuper != -1 && tokens_ != NULL)
        {
          tokens_[parser_.toksuper].size++;
        }
      }
      else
      {
        char_parse_result_ = STRING_CHAR;
      }
      break;
    case STRING_BACKSLASH:
      switch (c)
      {
        case '\"': case '/' : case '\\' : case 'b' :
        case 'f' : case 'r' : case 'n'  : case 't' :
          char_parse_result_ = STRING_CHAR;
          break;
        default:
          parser_.pos = start_;
          return JSMN_ERROR_INVAL;
      }
    case UNKNOWN:
    case OBJECT_BEGIN:
    case OBJECT_END:
    case ARRAY_BEGIN:
    case ARRAY_END:
    case STRING_END:
    case WHITESPACE:
    case KEY_END:
    case VALUE_END:
    case PRIMATIVE_BEGIN:
    case PRIMATIVE_CHAR:
      break;
  }
  return char_parse_result_;
}
