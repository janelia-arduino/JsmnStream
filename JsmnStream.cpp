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
  switch (char_parse_state_)
  {
    case PARSING_ROOT:
      switch (c)
      {
        case '{': case '[':
          count_++;
          if (tokens_ == NULL)
          {
            break;
          }
          token = allocToken();
          if (token == NULL)
          {
            return JSMN_ERROR_NOMEM;
          }
          if (parser_.toksuper != -1)
          {
            tokens_[parser_.toksuper].size++;
#ifdef JSMN_PARENT_LINKS
            token->parent = parser_.toksuper;
#endif
          }
          token->type = (c == '{' ? JSMN_OBJECT : JSMN_ARRAY);
          token->start = parser_.pos;
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
          token = &tokens_[parser_.toknext - 1];
          for (;;)
          {
            if (token->start != -1 && token->end == -1)
            {
              if (token->type != type)
              {
                return JSMN_ERROR_INVAL;
              }
              token->end = parser_.pos + 1;
              parser_.toksuper = token->parent;
              break;
            }
            if (token->parent == -1)
            {
              break;
            }
            token = &tokens_[token->parent];
          }
#else
          for (i = parser_.toknext - 1; i >= 0; i--)
          {
            token = &tokens_[i];
            if (token->start != -1 && token->end == -1)
            {
              if (token->type != type)
              {
                return JSMN_ERROR_INVAL;
              }
              parser_.toksuper = -1;
              token->end = parser_.pos + 1;
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
            token = &tokens_[i];
            if (token->start != -1 && token->end == -1)
            {
              parser_.toksuper = i;
              break;
            }
          }
#endif
          break;
        case '\"':
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
          r = parsePrimitive(js,len);
          if (r < 0)
          {
            return r;
          }
          count_++;
          if (parser_.toksuper != -1 && tokens_ != NULL)
          {
            tokens_[parser_.toksuper].size++;
          }
          break;

#ifdef JSMN_STRICT
          /* Unexpected char in strict mode */
        default:
          return JSMN_ERROR_INVAL;
#endif
      }
      break;
    case PARSING_STRING:
          // r = parseString(js,len);
          // if (r < 0)
          // {
          //   return r;
          // }
      break;
    // case PARSED_STRING:
    //   count_++;
    //   if (parser_.toksuper != -1 && tokens_ != NULL)
    //   {
    //     tokens_[parser_.toksuper].size++;
    //   }
    //   char_parse_state_ = PARSING_ROOT;
    //   break;
    case PARSING_PRIMATIVE:
      break;
  }
  parser_.pos++;
  return count_;
}

/**
 * Parse JSON string and fill tokens.
 */
int JsmnStream::parseJson(const char *js)
{
  int r;
  int i;
  jsmntok_t *token;
  count_ = parser_.toknext;
  size_t len = strlen(js);

  while (parser_.pos < len && js[parser_.pos] != '\0')
  {
    char c;
    jsmntype_t type;

    c = js[parser_.pos];
    r = parseChar(c);
    if (r < 0)
    {
      return r;
    }
  }

  if (tokens_ != NULL)
  {
    for (i = parser_.toknext - 1; i >= 0; i--)
    {
      /* Unmatched opened object or array */
      if (tokens_[i].start != -1 && tokens_[i].end == -1)
      {
        return JSMN_ERROR_PART;
      }
    }
  }

  return count_;
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
void JsmnStream::fillToken(jsmntok_t *token, jsmntype_t type, int start, int end)
{
  token->type = type;
  token->start = start;
  token->end = end;
  token->size = 0;
}

/**
 * Fills next available token with JSON primitive.
 */
int JsmnStream::parsePrimitive(const char *js, size_t len)
{
  jsmntok_t *token;
  int start;

  start = parser_.pos;

  for (; parser_.pos < len && js[parser_.pos] != '\0'; parser_.pos++)
  {
    switch (js[parser_.pos])
    {
#ifndef JSMN_STRICT
      /* In strict mode primitive must be followed by "," or "}" or "]" */
      case ':':
#endif
      case '\t' : case '\r' : case '\n' : case ' ' :
      case ','  : case ']'  : case '}' :
        goto found;
    }
    if (js[parser_.pos] < 32 || js[parser_.pos] >= 127)
    {
      parser_.pos = start;
      return JSMN_ERROR_INVAL;
    }
  }
#ifdef JSMN_STRICT
  /* In strict mode primitive must be followed by a comma/object/array */
  parser_.pos = start;
  return JSMN_ERROR_PART;
#endif

 found:
  if (tokens_ == NULL)
  {
    parser_.pos--;
    return 0;
  }
  token = allocToken();
  if (token == NULL)
  {
    parser_.pos = start;
    return JSMN_ERROR_NOMEM;
  }
  fillToken(token, JSMN_PRIMITIVE, start, parser_.pos);
#ifdef JSMN_PARENT_LINKS
  token->parent = parser_.toksuper;
#endif
  parser_.pos--;
  return 0;
}

/**
 * Fills next token with JSON string.
 */
// int JsmnStream::parseString(const char *js, size_t len)
int JsmnStream::parseStringChar(const char c)
{
  jsmntok_t *token;

  int start = parser_.pos;

  parser_.pos++;

  /* Skip starting quote */
  for (; parser_.pos < len && js[parser_.pos] != '\0'; parser_.pos++)
  {
    char c = js[parser_.pos];

    /* Quote: end of string */
    if (c == '\"')
    {
      if (tokens_ == NULL)
      {
        return 0;
      }
      token = allocToken();
      if (token == NULL)
      {
        parser_.pos = start;
        return JSMN_ERROR_NOMEM;
      }
      fillToken(token, JSMN_STRING, start+1, parser_.pos);
#ifdef JSMN_PARENT_LINKS
      token->parent = parser_.toksuper;
#endif
      return 0;
    }

    /* Backslash: Quoted symbol expected */
    if (c == '\\' && parser_.pos + 1 < len)
    {
      int i;
      parser_.pos++;
      switch (js[parser_.pos])
      {
        /* Allowed escaped symbols */
        case '\"': case '/' : case '\\' : case 'b' :
        case 'f' : case 'r' : case 'n'  : case 't' :
          break;
          /* Allows escaped symbol \uXXXX */
        case 'u':
          parser_.pos++;
          for(i = 0; i < 4 && parser_.pos < len && js[parser_.pos] != '\0'; i++)
          {
            /* If it isn't a hex character we have an error */
            if(!((js[parser_.pos] >= 48 && js[parser_.pos] <= 57) || /* 0-9 */
                 (js[parser_.pos] >= 65 && js[parser_.pos] <= 70) || /* A-F */
                 (js[parser_.pos] >= 97 && js[parser_.pos] <= 102))) /* a-f */
            {
              parser_.pos = start;
              return JSMN_ERROR_INVAL;
            }
            parser_.pos++;
          }
          parser_.pos--;
          break;
          /* Unexpected symbol */
        default:
          parser_.pos = start;
          return JSMN_ERROR_INVAL;
      }
    }
  }
  parser_.pos = start;
  return JSMN_ERROR_PART;
}
