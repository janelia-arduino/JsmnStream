// ----------------------------------------------------------------------------
// JsmnStreamDefinitions.h
//
//
// Authors:
// Serge Zaitsev zaitsev.serge@gmail.com
// Peter Polidoro peterpolidoro@gmail.com
// ----------------------------------------------------------------------------
#ifndef _JSMN_STREAM_DEFINITIONS_H_
#define _JSMN_STREAM_DEFINITIONS_H_


template<size_t TOKEN_COUNT_MAX>
JsmnStream::JsmnStream(JsmnStream::jsmntok_t (&tokens)[TOKEN_COUNT_MAX])
{
  tokens_ = tokens;
  token_count_max_ = TOKEN_COUNT_MAX;
  setup();
}
#endif /* _JSMN_STREAM_DEFINITIONS_H_ */
