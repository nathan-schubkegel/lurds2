/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#include "lurds2_jsonstream.h"

#include "lurds2_errors.h"

#define DIAGNOSTIC_JSON_ERROR(message) DIAGNOSTIC_ERROR(message);
#define DIAGNOSTIC_JSON_ERROR2(m1, m2) DIAGNOSTIC_ERROR2((m1), (m2));
#define DIAGNOSTIC_JSON_ERROR3(m1, m2, m3) DIAGNOSTIC_ERROR3((m1), (m2), (m3));
#define DIAGNOSTIC_JSON_ERROR4(m1, m2, m3, m4) DIAGNOSTIC_ERROR4((m1), (m2), (m3), (m4));

typedef struct JsonStreamData {
  char* identifierForDebugMessages;
  char* start;
  char* current;
  char* end;
  JsonStreamTokenType tokenType;
  char* stringBuffer;
  int32_t stringBufferLength;
  int64_t numBuffer;
} JsonStreamData;

JsonStream JsonStream_Parse(const char* jsonData, const char* debugIdentifier)
{
  JsonStreamData* data;
  data = malloc(sizeof(JsonStreamData));
  if (data == 0)
  {
    DIAGNOSTIC_JSON_ERROR("failed to allocate memory for JsonStreamData");
    return 0;
  }
  memset(data, 0, sizeof(JsonStreamData));

  int fileLength = strlen(jsonData);
  data->start = malloc(fileLength + 1);
  if (data->start == 0)
  {
    DIAGNOSTIC_JSON_ERROR("failed to allocate memory for json data");
    free(data);
    return 0;
  }
  
  for (int i = 0; i <= fileLength; i++)
  {
    data->start[i] = jsonData[i];
  }

  data->tokenType = JsonStreamStart;
  data->end = data->start + fileLength;
  data->current = data->start;
  
  // allocate 'identifierForDebugMessages' and copy filename into it
  if (debugIdentifier == 0) debugIdentifier = "unknown";
  int fileNameLength = strlen(debugIdentifier);
  data->identifierForDebugMessages = malloc(fileNameLength + 1);
  if (data->identifierForDebugMessages == 0)
  {
    DIAGNOSTIC_JSON_ERROR("failed to allocate memory for JsonStreamData debugIdentifier");
    free(data->start);
    free(data);
    return 0;
  }
  for (int i = 0; i <= fileNameLength; i++)
  {
    data->identifierForDebugMessages[i] = debugIdentifier[i];
  }

  return data;
}

JsonStream JsonStream_LoadFromResourceFile(const wchar_t * fileName)
{
  JsonStreamData* data;
  data = malloc(sizeof(JsonStreamData));
  if (data == 0)
  {
    DIAGNOSTIC_JSON_ERROR("failed to allocate memory for JsonStreamData");
    return 0;
  }
  memset(data, 0, sizeof(JsonStreamData));

  int fileLength = 0;
  data->start = (char*)ResourceFile_Load(fileName, &fileLength);
  if (data->start == 0)
  {
    free(data);
    return 0;
  }

  data->tokenType = JsonStreamStart;
  data->end = data->start + fileLength;
  data->current = data->start;
  
  // allocate 'identifierForDebugMessages' and copy filename into it
  int fileNameLength = wcslen(fileName);
  data->identifierForDebugMessages = malloc(fileNameLength + 1);
  if (data->identifierForDebugMessages == 0)
  {
    DIAGNOSTIC_JSON_ERROR("failed to allocate memory for JsonStreamData filename");
    free(data->start);
    free(data);
    return 0;
  }
  for (int i = 0; i <= fileNameLength; i++)
  {
    data->identifierForDebugMessages[i] = (char)fileName[i];
  }

  return data;
}

const char* JsonStream_GetDebugIdentifier(JsonStream stream)
{
  JsonStreamData* data = (JsonStreamData*)stream;
  if (data == 0)
  {
    DIAGNOSTIC_JSON_ERROR("invalid null stream arg");
    return 0;
  }
  return data->identifierForDebugMessages;
}

static void ConsumeCommentsCommasAndWhitespace(JsonStreamData* data)
{
  while (data->current < data->end)
  {
    char c = *(data->current);
    
    // advance past whitespace
    if (c == ' ' || c == '\r' || c == '\n' || c == '\t')
    {
      data->current++;
      continue;
    }
    
    // advance past single-line comments
    // (NOTE: data is null terminated beyond 'end' so accessing next character without guard is ok)
    if (c == '/' && *(data->current + 1) == '/')
    {
      while (data->current < data->end && c != '\r' && c != '\n')
      {
        data->current++;
        c = *(data->current);
      }
      continue;
    }
    
    // advance past multi-line comments
    if (c == '/' && *(data->current + 1) == '*')
    {
      while (data->current < data->end && !(c == '*' && *(data->current + 1) == '/'))
      {
        data->current++;
        c = *(data->current);
      }
      
      if (data->current < data->end && c == '*' && *(data->current + 1) == '/')
      {
        data->current += 2;
      }
      
      continue;
    }
    
    // advance past commas
    // (NOTE: I'm deciding I don't care if this means I accept a technically invalid JSON file. This is simple.)
    if (c == ',')
    {
      data->current++;
      continue;
    }
    
    break;
  }
}

static void ConsumeNextJsonToken(JsonStreamData* data)
{
  if (data->tokenType == JsonStreamError || data->tokenType == JsonStreamEnd)
  {
    return;
  }
  
  ConsumeCommentsCommasAndWhitespace(data);
  
  while (data->current < data->end)
  {
    switch (*(data->current))
    {
      case '{':
        data->tokenType = JsonStreamObjectStart;
        data->current++; // move past curly brace
        return;
        
      case '}':
        data->tokenType = JsonStreamObjectEnd;
        data->current++; // move past curly brace
        return;

      case '[':
        data->tokenType = JsonStreamArrayStart;
        data->current++; // move past square brace
        return;
        
      case ']':
        data->tokenType = JsonStreamArrayEnd;
        data->current++; // move past square brace
        return;

      case 't':
      {
        // check that it says "true"
        char * c = data->current;
        if (c[0] == 't' && c[1] == 'r' && c[2] == 'u' && c[3] == 'e')
        {
          data->tokenType = JsonStreamTrue;
          data->current += 4; // move past literal
        }
        else
        {
          DIAGNOSTIC_JSON_ERROR2("invalid JSON content in ", data->identifierForDebugMessages);
          data->tokenType = JsonStreamError;
        }
        return;
      }

      case 'f':
      {
        // check that it says "false"
        char * c = data->current;
        if (c[0] == 'f' && c[1] == 'a' && c[2] == 'l' && c[3] == 's' && c[4] == 'e')
        {
          data->tokenType = JsonStreamFalse;
          data->current += 5; // move past literal
        }
        else
        {
          DIAGNOSTIC_JSON_ERROR2("invalid JSON content in ", data->identifierForDebugMessages);
          data->tokenType = JsonStreamError;
        }
        return;
      }

      case 'n':
      {
        // check that it says "null"
        char * c = data->current;
        if (c[0] == 'n' && c[1] == 'u' && c[2] == 'l' && c[3] == 'l')
        {
          data->tokenType = JsonStreamNull;
          data->current += 4; // move past literal
        }
        else
        {
          DIAGNOSTIC_JSON_ERROR2("invalid JSON content in ", data->identifierForDebugMessages);
          data->tokenType = JsonStreamError;
        }
        return;
      }

      case '"':
      {
        // move past quote
        data->current++;

        data->stringBufferLength = 0;
        int capacity = 0;

        // consume the string content
        int foundEndQuote = 0;
        while (data->current < data->end)
        {
          // increase buffer when full
          if (data->stringBufferLength == capacity)
          {
            capacity = (capacity + 1) * 2;
            char * newContent = realloc(data->stringBuffer, capacity + 1); // plus 1 for null terminator
            if (newContent == 0)
            {
              DIAGNOSTIC_JSON_ERROR2("failed to allocate more memory for string while parsing ", data->identifierForDebugMessages);
              data->tokenType = JsonStreamError;
              return;
            }
            data->stringBuffer = newContent;
            memset(data->stringBuffer + data->stringBufferLength, 0, capacity - data->stringBufferLength + 1); // plus 1 for null terminator
          }

          // consume current character (whatever it is)
          char c = *(data->current);
          data->current++;
          
          if (c == '"')
          {
            foundEndQuote = 1;
            break;
          }

          if (c == '\\')
          {
            // consume character after slash (whatever it is)
            c = *(data->current);
            data->current++;

            switch (c)
            {
              case '"':
              case '\\':
              case '/':
                break;

              case 'b':
                c = '\b';
                break;

              case 'f':
                c = '\f';
                break;

              case 'n':
                c = '\n';
                break;

              case 'r':
                c = '\r';
                break;

              case 't':
                c = '\t';
                break;

              case 'u':
                // TODO: actually support this
                DIAGNOSTIC_JSON_ERROR2("unicode escapes like \u4403 aren't supported by this json parser yet, while parsing ", data->identifierForDebugMessages);
                data->tokenType = JsonStreamError;
                return;
              
              default:
                DIAGNOSTIC_JSON_ERROR2("invalid slash-escaped string data in ", data->identifierForDebugMessages);
                data->tokenType = JsonStreamError;
                return;
            }
          }
          
          data->stringBuffer[data->stringBufferLength] = c;
          data->stringBufferLength++;
        }

        if (foundEndQuote)
        {
          ConsumeCommentsCommasAndWhitespace(data);
          if (*data->current == ':')
          {
            data->current++;
            data->tokenType = JsonStreamPropertyName;
          }
          else
          {
            data->tokenType = JsonStreamString;
          }
        }
        else
        {
          // got to end of string literal without encountering quote
          DIAGNOSTIC_JSON_ERROR2("invalid unterminated string at end of ", data->identifierForDebugMessages);
          data->tokenType = JsonStreamError;
        }
        return;
      }

      case '-':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      {
        // look ahead to see how many number characters are present
        // (NOTE: for now, decimals and exponents are not supported)
        char * end = data->current;
        while (*end == '-' || (*end >= '0' && *end <= '9')) end++;
        if (*end == '.' || *end == 'e' || *end == 'E')
        {
          DIAGNOSTIC_JSON_ERROR2("decimals and exponents are not yet supported by this json parser, while parsing ", data->identifierForDebugMessages);
          data->tokenType = JsonStreamError;
          return;
        }
        
        // capture the number into the string buffer
        // (re)allocate a buffer to hold the parsed string content
        int size = end - data->current;
        char * newContent = realloc(data->stringBuffer, size + 1); // plus 1 for null terminator
        if (newContent == 0)
        {
          DIAGNOSTIC_JSON_ERROR2("failed to allocate memory for number string while parsing ", data->identifierForDebugMessages);
          data->tokenType = JsonStreamError;
          return;
        }
        data->stringBuffer = newContent;
        data->stringBufferLength = size;
        strncpy(data->stringBuffer, data->current, size);
        data->stringBuffer[size] = 0;

        // use strtol to try to interpret the number as an integer
        data->numBuffer = strtol(data->current, 0, 0);
        data->tokenType = JsonStreamNumber;
        data->current = end;
        return;
      }

      default:
      {
        char buffer[100];
        sprintf(buffer, "unrecognized content \"%c\" at character %d of ", *data->current, (int)(data->current - data->start) + 1);
        DIAGNOSTIC_JSON_ERROR3("unrecognized JSON content ", buffer, data->identifierForDebugMessages);
        data->tokenType = JsonStreamError;
        return;
      }
    }
  }
  
  data->tokenType = JsonStreamEnd;
}

JsonStreamTokenType JsonStream_MoveNext(JsonStream stream)
{
  JsonStreamData* data = (JsonStreamData*)stream;
  if (data == 0)
  {
    DIAGNOSTIC_JSON_ERROR("invalid null stream arg");
    return 0;
  }
  
  switch (data->tokenType)
  {
    case JsonStreamError:
      DIAGNOSTIC_JSON_ERROR("cannot move next; stream is error'd");
      return 0;

    case JsonStreamEnd:
      DIAGNOSTIC_JSON_ERROR("cannot move next; stream is at end");
      return 0;

    default:
      ConsumeNextJsonToken(data);
      return data->tokenType;
  }
}

JsonStreamTokenType JsonStream_GetTokenType(JsonStream stream)
{
  JsonStreamData* data = (JsonStreamData*)stream;
  if (data == 0)
  {
    DIAGNOSTIC_JSON_ERROR("invalid null stream arg");
    return JsonStreamError;
  }
  
  return data->tokenType;
}

const char* JsonStream_GetString(JsonStream stream, int32_t* length)
{
  JsonStreamData* data = (JsonStreamData*)stream;
  if (data == 0)
  {
    DIAGNOSTIC_JSON_ERROR("invalid null stream arg");
    if (length) *length = 0;
    return 0;
  }
  
  switch (data->tokenType)
  {
    case JsonStreamPropertyName:
    case JsonStreamString:
    case JsonStreamNumber:
      if (length) *length = data->stringBufferLength;
      return data->stringBuffer;

    default:
      if (length) *length = 0;
      return 0;
  }
}

int64_t JsonStream_GetNumberInt(JsonStream stream)
{
  JsonStreamData* data = (JsonStreamData*)stream;
  if (data == 0)
  {
    DIAGNOSTIC_JSON_ERROR("invalid null stream arg");
    return -2000000000;
  }
  
  switch (data->tokenType)
  {
    case JsonStreamNumber:
      return data->numBuffer;

    default:
      return -2000000000;
  }
}

void JsonStream_Release(JsonStream stream)
{
  JsonStreamData* data = (JsonStreamData*)stream;
  if (data == 0)
  {
    DIAGNOSTIC_JSON_ERROR("invalid null stream arg");
    return;
  }
  
  if (data->stringBuffer) free(data->stringBuffer);
  if (data->identifierForDebugMessages) free(data->identifierForDebugMessages);
  free(data->start);
  free(data);
}