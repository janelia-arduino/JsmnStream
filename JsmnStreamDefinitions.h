// ----------------------------------------------------------------------------
// JsmnStreamDefinitions.h
//
//
// Authors:
// Serge Zaitsev zaitsev.serge@gmail.com
// Peter Polidoro polidorop@janelia.hhmi.org
// ----------------------------------------------------------------------------
#ifndef _JSMN_STREAM_DEFINITIONS_H_
#define _JSMN_STREAM_DEFINITIONS_H_


template<size_t NUM_TOKENS>
JsmnStream::JsmnStream(JsmnStream::jsmntok_t (&tokens)[NUM_TOKENS])
{
  tokens_ = tokens;
  num_tokens_ = NUM_TOKENS;
  setup();
}
#endif /* _JSMN_STREAM_DEFINITIONS_H_ */
