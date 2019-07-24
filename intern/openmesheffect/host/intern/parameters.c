/*
 * Copyright 2019 Elie Michel
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "util/memory_util.h"

#include "parameters.h"
#include "properties.h"

// OFX PARAMETERS SUITE

// // OfxParamStruct

void init_parameter(OfxParamStruct *param) {
  param->type = PARAM_TYPE_DOUBLE;
  param->name = NULL;
  init_properties(&param->properties);
}

void free_parameter(OfxParamStruct *param) {
  free_properties(&param->properties);
  if (PARAM_TYPE_STRING == param->type) {
    free_array(param->value[0].as_char);
  }
  if (NULL != param->name) {
    free_array(param->name);
  }
}

void parameter_set_type(OfxParamStruct *param, ParamType type) {
  if (param->type == type) {
    return;
  }

  if (PARAM_TYPE_STRING == param->type) {
    free_array(param->value[0].as_char);
    param->value[0].as_char = NULL;
  }

  param->type = type;

  if (PARAM_TYPE_STRING == param->type) {
    param->value[0].as_char = NULL;
    parameter_realloc_string(param, 1);
  }
}

void parameter_realloc_string(OfxParamStruct *param, int size) {
  if (NULL != param->value[0].as_char) {
    free_array(param->value[0].as_char);
  }
  param->value[0].as_char = malloc_array(sizeof(char), size, "parameter string value");
  param->value[0].as_char[0] = '\0';
}

// // OfxParamSetStruct

void deep_copy_parameter(OfxParamStruct *destination, const OfxParamStruct *source) {
  destination->name = source->name;
  if (NULL != destination->name) {
    destination->name = malloc_array(sizeof(char), strlen(source->name), "parameter name");
    strcpy(destination->name, source->name);
  }
  destination->type = source->type;
  destination->value[0] = source->value[0];
  destination->value[1] = source->value[1];
  destination->value[2] = source->value[2];
  destination->value[3] = source->value[3];

  // Strings are dynamically allocated, so deep copy must allocate new data
  if (destination->type == PARAM_TYPE_STRING) {
    int n = strlen(source->value[0].as_char);
    destination->value[0].as_char = NULL;
    parameter_realloc_string(destination, n + 1);
    strcpy(destination->value[0].as_char, source->value[0].as_char);
  }

  deep_copy_property_set(&destination->properties, &source->properties);
}

int find_parameter(OfxParamSetStruct *param_set, const char *param) {
  for (int i = 0 ; i < param_set->num_parameters ; ++i) {
    if (0 == strcmp(param_set->parameters[i]->name, param)) {
      return i;
    }
  }
  return -1;
}

void append_parameters(OfxParamSetStruct *param_set, int count) {
  int old_num_parameters = param_set->num_parameters;
  OfxParamStruct **old_parameters = param_set->parameters;
  param_set->num_parameters += count;
  param_set->parameters = malloc_array(sizeof(OfxParamStruct*), param_set->num_parameters, "parameters");
  for (int i = 0 ; i < param_set->num_parameters ; ++i){
    if (i < old_num_parameters) {
      param_set->parameters[i] = old_parameters[i];
    } else {
      param_set->parameters[i] = malloc_array(sizeof(OfxParamStruct), 1, "parameter");
      init_parameter(param_set->parameters[i]);
    }
  }
  if (NULL != old_parameters) {
    free_array(old_parameters);
  }
}

int ensure_parameter(OfxParamSetStruct *param_set, const char *parameter) {
  int i = find_parameter(param_set, parameter);
  if (i == -1) {
    append_parameters(param_set, 1);
    i = param_set->num_parameters - 1;
    param_set->parameters[i]->name = malloc_array(sizeof(char), strlen(parameter), "parameter name");
    strcpy(param_set->parameters[i]->name, parameter);
  }
  return i;
}

void init_parameter_set(OfxParamSetStruct *param_set) {
  param_set->num_parameters = 0;
  param_set->parameters = NULL;
  param_set->effect_properties = NULL;
}

void free_parameter_set(OfxParamSetStruct *param_set) {
  for (int i = 0 ; i < param_set->num_parameters ; ++i){
    free_parameter(param_set->parameters[i]);
    free_array(param_set->parameters[i]);
  }
  param_set->num_parameters = 0;
  if (NULL != param_set->parameters) {
    free_array(param_set->parameters);
    param_set->parameters = NULL;
  }
}

void deep_copy_parameter_set(OfxParamSetStruct *destination, const OfxParamSetStruct *source) {
  init_parameter_set(destination);
  append_parameters(destination, source->num_parameters);
  for (int i = 0 ; i < destination->num_parameters ; ++i) {
    deep_copy_parameter(destination->parameters[i], source->parameters[i]);
  }
  destination->effect_properties = source->effect_properties;
}

// // Parameter Suite Entry Points

ParamType parse_parameter_type(const char *str) {
  if (0 == strcmp(str, kOfxParamTypeDouble)) {
    return PARAM_TYPE_DOUBLE;
  }
  if (0 == strcmp(str, kOfxParamTypeInteger)) {
    return PARAM_TYPE_INT;
  }
  if (0 == strcmp(str, kOfxParamTypeString)) {
    return PARAM_TYPE_STRING;
  }
  return PARAM_TYPE_UNKNOWN;
}

size_t parameter_type_dimensions(ParamType type) {
  switch (type) {
  case PARAM_TYPE_DOUBLE:
  case PARAM_TYPE_INT:
  case PARAM_TYPE_STRING:
    return 1;
  default:
    return 1;
  }
}

OfxStatus paramDefine(OfxParamSetHandle paramSet,
                      const char *paramType,
                      const char *name,
                      OfxPropertySetHandle *propertySet) {
  int i = find_parameter(paramSet, name);
  if (-1 != i) {
    return kOfxStatErrExists;
  }
  i = ensure_parameter(paramSet, name);
  parameter_set_type(paramSet->parameters[i], parse_parameter_type(paramType));
  if (NULL != propertySet) {
    *propertySet = &paramSet->parameters[i]->properties;
  }
  return kOfxStatOK;
}

OfxStatus paramGetHandle(OfxParamSetHandle paramSet,
                         const char *name,
                         OfxParamHandle *param,
                         OfxPropertySetHandle *propertySet) {
  int i = find_parameter(paramSet, name);
  if (-1 == i) {
    return kOfxStatErrUnknown; // parameter not found
  }
  *param = paramSet->parameters[i];
  if (NULL !=  propertySet) {
    *propertySet = &paramSet->parameters[i]->properties;
  }
  return kOfxStatOK;
}

OfxStatus paramSetGetPropertySet(OfxParamSetHandle paramSet,
                                 OfxPropertySetHandle *propHandle) {
  *propHandle = paramSet->effect_properties;
  return kOfxStatOK;
}

OfxStatus paramGetPropertySet(OfxParamHandle param,
                              OfxPropertySetHandle *propHandle) {
  *propHandle = &param->properties;
  return kOfxStatOK;
}

OfxStatus paramGetValue(OfxParamHandle paramHandle, ...) {
  size_t dimensions = parameter_type_dimensions(paramHandle->type);
  va_list valist;
  va_start(valist, paramHandle);
  for (size_t i = 0 ; i < dimensions ; ++i) {
    switch (paramHandle->type) {
    case PARAM_TYPE_DOUBLE:
      *va_arg(valist, double*) = paramHandle->value[i].as_double;
      break;
    case PARAM_TYPE_INT:
      *va_arg(valist, int*) = paramHandle->value[i].as_int;
      break;
    case PARAM_TYPE_STRING:
    case PARAM_TYPE_UNKNOWN:
      // TODO
      break;
    }
  }
  va_end(valist);
  return kOfxStatOK;
}

OfxStatus paramGetValueAtTime(OfxParamHandle paramHandle, OfxTime time, ...) {
  (void)paramHandle;
  (void)time;
  // TODO
  return kOfxStatErrUnsupported;
}

OfxStatus paramGetDerivative(OfxParamHandle paramHandle, OfxTime time, ...) {
  (void)paramHandle;
  (void)time;
  // TODO
  return kOfxStatErrUnsupported;
}

OfxStatus paramGetIntegral(OfxParamHandle paramHandle, OfxTime time1, OfxTime time2, ...) {
  (void)paramHandle;
  (void)time1;
  (void)time2;
  // TODO
  return kOfxStatErrUnsupported;
}

OfxStatus paramSetValue(OfxParamHandle paramHandle, ...) {
  size_t dimensions = parameter_type_dimensions(paramHandle->type);
  va_list args;
  va_start(args, paramHandle);
  for (size_t i = 0 ; i < dimensions ; ++i) {
    switch (paramHandle->type) {
    case PARAM_TYPE_DOUBLE:
    {
      double d = va_arg(args, double);
      printf("double: %f\n", d);
      paramHandle->value[i].as_double = d;
      break;
    }
    case PARAM_TYPE_INT:
      paramHandle->value[i].as_int = va_arg(args, int);
      break;
    case PARAM_TYPE_STRING:
    case PARAM_TYPE_UNKNOWN:
      // TODO
      break;
    }
  }
  va_end(args);
  return kOfxStatOK;
}

OfxStatus paramSetValueAtTime(OfxParamHandle paramHandle, OfxTime time, ...) {
  (void)paramHandle;
  (void)time;
  // TODO
  return kOfxStatErrUnsupported;
}

OfxStatus paramGetNumKeys(OfxParamHandle paramHandle, unsigned int *numberOfKeys) {
  (void)paramHandle;
  (void)numberOfKeys;
  // TODO
  return kOfxStatErrUnsupported;
}

OfxStatus paramGetKeyTime(OfxParamHandle paramHandle,
                          unsigned int nthKey,
                          OfxTime *time) {
  (void)paramHandle;
  (void)nthKey;
  (void)time;
  // TODO
  return kOfxStatErrUnsupported;
}

OfxStatus paramGetKeyIndex(OfxParamHandle paramHandle,
                           OfxTime time,
                           int direction,
                           int *index) {
  (void)paramHandle;
  (void)time;
  (void)direction;
  (void)index;
  // TODO
  return kOfxStatErrUnsupported;
}

OfxStatus paramDeleteKey(OfxParamHandle paramHandle, OfxTime time) {
  (void)paramHandle;
  (void)time;
  // TODO
  return kOfxStatErrUnsupported;
}

OfxStatus paramDeleteAllKeys(OfxParamHandle paramHandle) {
  (void)paramHandle;
  // TODO
  return kOfxStatErrUnsupported;
}

OfxStatus paramCopy(OfxParamHandle paramTo,
                    OfxParamHandle paramFrom,
                    OfxTime dstOffset,
                    const OfxRangeD *frameRange) {
  (void)paramTo;
  (void)paramFrom;
  (void)dstOffset;
  (void)frameRange;
  // TODO
  return kOfxStatErrUnsupported;
}

OfxStatus paramEditBegin(OfxParamSetHandle paramSet, const char *name) {
  (void)paramSet;
  (void)name;
  // TODO
  return kOfxStatErrUnsupported;
}
 
OfxStatus paramEditEnd(OfxParamSetHandle paramSet) {
  (void)paramSet;
  // TODO
  return kOfxStatErrUnsupported;
}

