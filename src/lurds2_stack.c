/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#include "lurds2_stack.h"

#include "lurds2_errors.h"

#define DIAGNOSTIC_STACK_ERROR(message) DIAGNOSTIC_ERROR(message);
#define DIAGNOSTIC_STACK_ERROR2(m1, m2) DIAGNOSTIC_ERROR2((m1), (m2));
#define DIAGNOSTIC_STACK_ERROR3(m1, m2, m3) DIAGNOSTIC_ERROR3((m1), (m2), (m3));
#define DIAGNOSTIC_STACK_ERROR4(m1, m2, m3, m4) DIAGNOSTIC_ERROR4((m1), (m2), (m3), (m4));

typedef struct StackData {
  int32_t count; // number of elements in stack
  int32_t capacity; // maximum allowed 'count' before stack must be resized
  int8_t* elements; // allocated memory for elements in stack
  int32_t elementSize; // size of each element
} StackData;

Stack Stack_Create(int32_t elementSize)
{
  if (elementSize < 1)
  {
    DIAGNOSTIC_STACK_ERROR("invalid elementSize arg");
    return 0;
  }

  StackData* data = malloc(sizeof(StackData));
  if (data == 0)
  {
    DIAGNOSTIC_STACK_ERROR("failed to allocate memory for StackData");
    return 0;
  }
  memset(data, 0, sizeof(StackData));
  data->elementSize = elementSize;
  return data;
}

void* Stack_Push(Stack stack)
{
  StackData* data = (StackData*)stack;
  if (data == 0)
  {
    DIAGNOSTIC_STACK_ERROR("invalid null arg");
    return 0;
  }
  
  // grow the stack when needed
  if (data->count == data->capacity)
  {
    int64_t newCapacity64 = (data->capacity + 1) * 2;
    int32_t newCapacity = (int32_t)newCapacity64;
    if (newCapacity64 > newCapacity)
    {
      DIAGNOSTIC_STACK_ERROR("can't grow stack; too large");
      return 0;
    }

    int64_t newSpace64 = newCapacity * data->elementSize;
    int32_t newSpace = (int32_t)newSpace64;
    if (newSpace64 > newSpace)
    {
      DIAGNOSTIC_STACK_ERROR("can't grow stack; too large");
      return 0;
    }
    
    void* newData = realloc(data->elements, newSpace);
    if (newData == 0)
    {
      DIAGNOSTIC_STACK_ERROR("can't grow stack; out of memory");
      return 0;
    }
    data->elements = newData;
    data->capacity = newCapacity;
    int32_t oldSpace = data->elementSize * data->count;
    memset(data->elements + oldSpace, 0, newSpace - oldSpace);
  }
  
  void* result = data->elements + data->count * data->elementSize;
  data->count++;
  return result;
}

void* Stack_Peek(Stack stack)
{
  StackData* data = (StackData*)stack;
  if (data == 0)
  {
    DIAGNOSTIC_STACK_ERROR("invalid null arg");
    return 0;
  }

  if (data->count == 0)
  {
    return 0;
  }
  
  return data->elements + data->elementSize * (data->count - 1);
}

void Stack_Pop(Stack stack)
{
  StackData* data = (StackData*)stack;
  if (data == 0)
  {
    DIAGNOSTIC_STACK_ERROR("invalid null arg");
    return;
  }
  if (data->count == 0)
  {
    DIAGNOSTIC_STACK_ERROR("cannot pop; stack is empty");
  }
  else
  {
    data->count--;
    // TODO: could theoreticall reduce the stack size when it gets small, but meh
  }
}

int32_t Stack_Count(Stack stack)
{
  StackData* data = (StackData*)stack;
  if (data == 0)
  {
    DIAGNOSTIC_STACK_ERROR("invalid null arg");
    return 0;
  }
  return data->count;
}

void Stack_Release(Stack stack)
{
  StackData* data = (StackData*)stack;
  if (data == 0)
  {
    DIAGNOSTIC_STACK_ERROR("invalid null arg");
    return;
  }
  
  if (data->elements)
  {
    free(data->elements);
  }
  free(data);
}
