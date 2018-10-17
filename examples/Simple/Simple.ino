#include <Arduino.h>
#include <Streaming.h>
#include <JsmnStream.h>

/*
 * A small example of jsmn parsing when JSON structure is known and number of
 * tokens is predictable.
 */

const long BAUD = 115200;
const long LOOP_DELAY = 2000;

const char *JSON_STRING =
  "{\"user\": \"johndoe\", \"admin\": false, \"uid\": 1000,\n  "
  "\"groups\": [\"users\", \"wheel\", \"audio\", \"video\"]}";

static int jsoneq(const char *json, JsmnStream::jsmntok_t *tok, const char *s)
{
  if (tok->type == JsmnStream::JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
    strncmp(json + tok->start, s, tok->end - tok->start) == 0)
  {
    return 0;
  }
  return -1;
}

void setup()
{
  Serial.begin(BAUD);
}

void loop()
{
  delay(LOOP_DELAY);

  JsmnStream::jsmntok_t t[128]; /* We expect no more than 128 tokens */
  JsmnStream jsmn_stream(t);

  Serial << "JSON_STRING = " << "\n";
  Serial << JSON_STRING << "\n";

  int r = jsmn_stream.parseJson(JSON_STRING);
  if (r < 0)
  {
    Serial << "Failed to parse JSON: " << r << "\n";
    return;
  }

  int parse_result;
  int len = strlen(JSON_STRING);
  char c;
  for (int index=0; index < len; ++index)
  {
    c = JSON_STRING[index];
    parse_result = jsmn_stream.parseChar(c);
    if (parse_result < 0)
    {
      Serial << "Failed to parse JSON: " << parse_result << "\n";
      return;
    }
    else
    {
      switch (parse_result)
      {
        case JsmnStream::UNKNOWN:
          Serial  << c << " UNKNOWN\n";
          break;
        case JsmnStream::OBJECT_BEGIN:
          Serial  << c << " OBJECT_BEGIN\n";
          break;
        case JsmnStream::OBJECT_END:
          Serial  << c << " OBJECT_END\n";
          break;
        case JsmnStream::ARRAY_BEGIN:
          Serial  << c << " ARRAY_BEGIN\n";
          break;
        case JsmnStream::ARRAY_END:
          Serial  << c << " ARRAY_END\n";
          break;
        case JsmnStream::STRING_BEGIN:
          Serial  << c << " STRING_BEGIN\n";
          break;
        case JsmnStream::STRING_END:
          Serial  << c << " STRING_END\n";
          break;
        case JsmnStream::STRING_BACKSLASH:
          Serial  << c << " STRING_BACKSLASH\n";
          break;
        case JsmnStream::STRING_CHAR:
          Serial  << c << " STRING_CHAR\n";
          break;
        case JsmnStream::WHITESPACE:
          Serial  << c << " WHITESPACE\n";
          break;
        case JsmnStream::KEY_END:
          Serial  << c << " KEY_END\n";
          break;
        case JsmnStream::VALUE_END:
          Serial  << c << " VALUE_END\n";
          break;
        case JsmnStream::PRIMATIVE_BEGIN:
          Serial  << c << " PRIMATIVE_BEGIN\n";
          break;
        case JsmnStream::PRIMATIVE_CHAR:
          Serial  << c << " PRIMATIVE_CHAR\n";
          break;
      }
    }
  }
  parse_result = jsmn_stream.checkParse();
  if (parse_result < 0)
  {
    Serial << "Failed to parse JSON: " << parse_result << "\n";
    return;
  }

  int token_count = jsmn_stream.getTokenCount();
  /* Assume the top-level element is an object */
  if (token_count < 1 || t[0].type != JsmnStream::JSMN_OBJECT)
  {
    Serial << "Object expected\n";
    return;
  }

  /* Loop over all keys of the root object */
  for (int i = 1; i < token_count; ++i)
  {
    if (jsoneq(JSON_STRING, &t[i], "user") == 0)
    {
      size_t str_len = t[i+1].end-t[i+1].start;
      char str[str_len+1];
      memcpy(str,JSON_STRING + t[i+1].start,str_len);
      str[str_len] = 0;
      Serial << "- User: " << str << "\n";
      i++;
    }
    else if (jsoneq(JSON_STRING, &t[i], "admin") == 0)
    {
      size_t str_len = t[i+1].end-t[i+1].start;
      char str[str_len+1];
      memcpy(str,JSON_STRING + t[i+1].start,str_len);
      str[str_len] = 0;
      Serial << "- Admin: " << str << "\n";
      /* We may additionally check if the value is either "true" or "false" */
      i++;
    }
    else if (jsoneq(JSON_STRING, &t[i], "uid") == 0)
    {
      size_t str_len = t[i+1].end-t[i+1].start;
      char str[str_len+1];
      memcpy(str,JSON_STRING + t[i+1].start,str_len);
      str[str_len] = 0;
      Serial << "- UID: " << str << "\n";
      /* We may want to do strtol() here to get numeric value */
      i++;
    }
    else if (jsoneq(JSON_STRING, &t[i], "groups") == 0)
    {
      int j;
      // printf("- Groups:\n");
      Serial << "- Groups:\n";
      if (t[i+1].type != JsmnStream::JSMN_ARRAY)
      {
        continue; /* We expect groups to be an array of strings */
      }
      for (j = 0; j < t[i+1].size; j++)
      {
        JsmnStream::jsmntok_t *g = &t[i+j+2];
        size_t str_len = g->end-g->start;
        char str[str_len+1];
        memcpy(str,JSON_STRING + g->start,str_len);
        str[str_len] = 0;
        Serial << "  * " << str << "\n";
      }
      i += t[i+1].size + 1;
    }
    else
    {
      size_t str_len = t[i].end-t[i].start;
      char str[str_len+1];
      memcpy(str,JSON_STRING + t[i].start,str_len);
      str[str_len] = 0;
      Serial << "Unexpected key: " << str << "\n";
    }
  }
}
