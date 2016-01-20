#JsmnStream

Authors:

    Serge Zaitsev <zaitsev.serge@gmail.com>
    Peter Polidoro <polidorop@janelia.hhmi.org>

License:

    BSD


Reformatting of Serge Zaitsev's jsmn library to parse a JSON string
one char at a time rather than on an entire buffer so it can be
processed more easily from a stream.

[Original jsmn README](./README_ORIGINAL.md)

##jsmn

```c++
jsmn_parser parser;
jsmntok_t tokens[10];

jsmn_init(&parser);

// js - pointer to JSON string
// tokens - an array of tokens available
// 10 - number of tokens available
jsmn_parse(&parser, js, tokens, 10);
```

##JsmnStream

```c++
JsmnStream::jsmntok_t tokens[10];
JsmnStream jsmn_stream(tokens);
// js - pointer to JSON string
// tokens - an array of tokens available
// 10 - number of tokens available
int ret;
char c;
for (int index=0; index < strlen(js); ++index)
{
  c = js[index];
  ret = jsmn_stream.parseChar(c);
}
ret = jsmn_stream.checkParse();
```
