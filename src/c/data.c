/*
 * Copyright (c) 2018
 * IoTech Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "edgex-rest.h"
#include "rest.h"
#include "data.h"
#include "errorlist.h"
#include "config.h"
#include "edgex-time.h"
#include "device.h"
#include "transform.h"

JSON_Value *edgex_data_generate_event
(
  const char *device_name,
  const edgex_cmdinfo *commandinfo,
  edgex_device_commandresult *values,
  bool doTransforms
)
{
  uint64_t timenow = edgex_device_millitime ();
  JSON_Value *jevent = json_value_init_object ();
  JSON_Object *jobj = json_value_get_object (jevent);
  JSON_Value *arrval = json_value_init_array ();
  JSON_Array *jrdgs = json_value_get_array (arrval);
  for (uint32_t i = 0; i < commandinfo->nreqs; i++)
  {
    if (doTransforms)
    {
      edgex_transform_outgoing (&values[i], commandinfo->pvals[i], commandinfo->maps[i]);
    }
    char *reading = edgex_value_tostring (&values[i], commandinfo->pvals[i]->floatAsBinary);
    const char *assertion = commandinfo->pvals[i]->assertion;
    if (assertion && *assertion && strcmp (reading, assertion))
    {
       free (reading);
       json_value_free (jevent);
       return NULL;
    }

    JSON_Value *rval = json_value_init_object ();
    JSON_Object *robj = json_value_get_object (rval);

    json_object_set_string (robj, "name", commandinfo->reqs[i].resname);
    json_object_set_string (robj, "value", reading);
    if (values[i].origin)
    {
      json_object_set_number (robj, "origin", values[i].origin);
    }
    json_array_append_value (jrdgs, rval);
    free (reading);
  }

  json_object_set_string (jobj, "device", device_name);
  json_object_set_number (jobj, "origin", timenow);
  json_object_set_value (jobj, "readings", arrval);
  return jevent;
}

void edgex_data_client_add_event
(
  iot_logger_t *lc,
  edgex_service_endpoints *endpoints,
  JSON_Value *eventval,
  edgex_error *err
)
{
  edgex_ctx ctx;
  char url[URL_BUF_SIZE];
  char *json;

  memset (&ctx, 0, sizeof (edgex_ctx));
  snprintf
  (
    url,
    URL_BUF_SIZE - 1,
    "http://%s:%u/api/v1/event",
    endpoints->data.host,
    endpoints->data.port
  );

  json = json_serialize_to_string (eventval);
  edgex_http_post (lc, &ctx, url, json, edgex_http_write_cb, err);

  free (ctx.buff);
  free (json);
}

edgex_valuedescriptor *edgex_data_client_add_valuedescriptor
(
  iot_logger_t *lc,
  edgex_service_endpoints *endpoints,
  const char *name,
  uint64_t origin,
  const char *min,
  const char *max,
  const char *type,
  const char *uomLabel,
  const char *defaultValue,
  const char *formatting,
  const char *description,
  const char *mediaType,
  const char *floatEncoding,
  edgex_error *err
)
{
  edgex_valuedescriptor *result = malloc (sizeof (edgex_valuedescriptor));
  edgex_ctx ctx;
  char url[URL_BUF_SIZE];
  char *json;

  memset (result, 0, sizeof (edgex_valuedescriptor));
  memset (&ctx, 0, sizeof (edgex_ctx));
  snprintf
  (
    url,
    URL_BUF_SIZE - 1,
    "http://%s:%u/api/v1/valuedescriptor",
    endpoints->data.host,
    endpoints->data.port
  );
  result->origin = origin;
  result->name = strdup (name);
  result->min = strdup (min);
  result->max = strdup (max);
  result->type = strdup (type);
  result->uomLabel = strdup (uomLabel);
  result->defaultValue = strdup (defaultValue);
  result->formatting = strdup (formatting);
  result->description = strdup (description);
  result->mediaType = strdup (mediaType);
  result->floatEncoding = strdup (floatEncoding);
  json = edgex_valuedescriptor_write (result);
  edgex_http_post (lc, &ctx, url, json, edgex_http_write_cb, err);
  result->id = ctx.buff;
  free (json);

  return result;
}

bool edgex_data_client_ping
(
  iot_logger_t *lc,
  edgex_service_endpoints *endpoints,
  edgex_error *err
)
{
  edgex_ctx ctx;
  char url[URL_BUF_SIZE];

  memset (&ctx, 0, sizeof (edgex_ctx));
  snprintf
  (
    url,
    URL_BUF_SIZE - 1,
    "http://%s:%u/api/v1/ping",
    endpoints->data.host,
    endpoints->data.port
  );

  edgex_http_get (lc, &ctx, url, NULL, err);
  return (err->code == 0);
}
