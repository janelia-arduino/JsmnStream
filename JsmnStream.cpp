// ----------------------------------------------------------------------------
// JsonStream.cpp
//
//
// Authors:
// Serge Zaitsev zaitsev.serge@gmail.com
// Peter Polidoro polidorop@janelia.hhmi.org
// ----------------------------------------------------------------------------
#include "JsmnStream.h"


/**
 * Parse JSON string one char at a time and fill tokens.
 */
int JsmnStream::parseChar(const char c)
{
  int r;
  jsmntok_t *token_ptr;
  jsmntype_t type;
  int i;
  switch (char_parse_state_)
  {
    case PARSING_ROOT: case PARSED_STRING: case PARSED_PRIMATIVE:
      char_parse_state_ = PARSING_ROOT;
      switch (c)
      {
        case '{': case '[':
          count_++;
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
          token_ptr->type = (c == '{' ? JSMN_OBJECT : JSMN_ARRAY);
          token_ptr->start = parser_.pos;
          parser_.toksuper = parser_.toknext - 1;
          break;
        case '}': case ']':
          if (tokens_ == NULL)
          {
            break;
          }
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
              if (token_ptr->type != type)
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
          backslash_ = false;
          char_parse_state_ = PARSING_STRING;
          break;
        case '\t' : case '\r' : case '\n' : case ' ':
          break;
        case ':':
          parser_.toksuper = parser_.toknext - 1;
          break;
        case ',':
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
          char_parse_state_ = PARSING_PRIMATIVE;
          break;

#ifdef JSMN_STRICT
          /* Unexpected char in strict mode */
        default:
          return JSMN_ERROR_INVAL;
#endif
      }
      break;
    case PARSING_STRING:
      r = parseStringChar(c);
      if (r < 0)
      {
        return r;
      }
      break;
    case PARSING_PRIMATIVE:
      r = parsePrimitiveChar(c);
      if (r < 0)
      {
        return r;
      }
      break;
  }
  parser_.pos++;
  return char_parse_state_;
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
  return count_;
}

size_t JsmnStream::getTokenCount()
{
  return count_;
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

// size_t JsmnStream::getTokenLen(jsmntok_t &token)
// {
//   return token.end-token.start;
// }

/**
 * Creates a new parser based over a given  buffer with an array of tokens
 * available.
 */
void JsmnStream::setup()
{
  parser_.pos = 0;
  parser_.toknext = 0;
  parser_.toksuper = -1;
  count_ = 0;
  char_parse_state_ = PARSING_ROOT;
  start_ = 0;
  backslash_ = false;
}

/**
 * Allocates a fresh unused token from the token pull.
 */
JsmnStream::jsmntok_t* JsmnStream::allocToken()
{
  jsmntok_t *tok;
  if (parser_.toknext >= num_tokens_)
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
  }
  if (c < 32 || c >= 127)
  {
    parser_.pos = start_;
    return JSMN_ERROR_INVAL;
  }
  else
  {
    return 0;
  }
  // }
#ifdef JSMN_STRICT
  /* In strict mode primitive must be followed by a comma/object/array */
  parser_.pos = start_;
  return JSMN_ERROR_PART;
#endif

 found:
  if (tokens_ == NULL)
  {
    parser_.pos--;
    char_parse_state_ = PARSING_ROOT;
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
  count_++;
  if (parser_.toksuper != -1 && tokens_ != NULL)
  {
    tokens_[parser_.toksuper].size++;
  }
  parser_.pos--;
  char_parse_state_ = PARSED_PRIMATIVE;
  return parseChar(c);
}

/**
 * Fills next token with JSON string.
 */
int JsmnStream::parseStringChar(const char c)
{
  if (!backslash_)
  {
    if (c == '\\')
    {
      backslash_ = true;
    }
    /* Quote: end of string */
    else if (c == '\"')
    {
      if (tokens_ == NULL)
      {
        return 0;
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
      count_++;
      if (parser_.toksuper != -1 && tokens_ != NULL)
      {
        tokens_[parser_.toksuper].size++;
      }
      char_parse_state_ = PARSED_STRING;
    }
  }
  else
  {
    backslash_ = false;
    switch (c)
    {
      case '\"': case '/' : case '\\' : case 'b' :
      case 'f' : case 'r' : case 'n'  : case 't' :
        break;
      default:
        parser_.pos = start_;
        return JSMN_ERROR_INVAL;
    }
  }
  return 0;
}
