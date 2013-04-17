#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_FEATURES_H
#include <features.h>
#endif

#include <Eo.h>
#include "ecore_audio_private.h"
#include <sndfile.h>

EAPI Eo_Op ECORE_AUDIO_OBJ_IN_SNDFILE_BASE_ID = EO_NOOP;

#define MY_CLASS ECORE_AUDIO_OBJ_IN_SNDFILE_CLASS
#define MY_CLASS_NAME "ecore_audio_obj_in_sndfile"

struct _Ecore_Audio_Sndfile
{
  SNDFILE *handle;
  SF_INFO sfinfo;
  Ecore_Audio_Vio *vio;
};

typedef struct _Ecore_Audio_Sndfile Ecore_Audio_Sndfile;

static void _read(Eo *eo_obj, void *_pd, va_list *list)
{
  Ecore_Audio_Sndfile *obj = _pd;
  int read;
  void *data = va_arg(*list, void *);
  int len = va_arg(*list, int);
  int *ret = va_arg(*list, int *);

  read = sf_read_float(obj->handle, data, len/4)*4;

  if (ret)
    *ret = read;
}

static void _seek(Eo *eo_obj, void *_pd, va_list *list)
{
  Ecore_Audio_Sndfile *obj = _pd;
  sf_count_t count, pos;

  double offs = va_arg(*list, double);
  int mode = va_arg(*list, int);
  double *ret = va_arg(*list, double *);

  count = offs * obj->sfinfo.samplerate;
  pos = sf_seek(obj->handle, count, mode);

  if (ret)
    *ret = (double)pos / obj->sfinfo.samplerate;
}

static void _source_set(Eo *eo_obj, void *_pd, va_list *list)
{
  Ecore_Audio_Sndfile *obj = _pd;

  Ecore_Audio_Object *ea_obj = eo_data_get(eo_obj, ECORE_AUDIO_OBJ_CLASS);
  Ecore_Audio_Input *in_obj = eo_data_get(eo_obj, ECORE_AUDIO_OBJ_IN_CLASS);

  const char *source = va_arg(*list, const char *);

  if (obj->handle) {
    sf_close(obj->handle);
    obj->handle = NULL;
  }

  eina_stringshare_replace(&ea_obj->source, source);

  if (!ea_obj->source)
    return;

  obj->handle = sf_open(ea_obj->source, SFM_READ, &obj->sfinfo);

  if (!obj->handle) {
    eina_stringshare_del(ea_obj->source);
    ea_obj->source = NULL;
    return;
  }

  in_obj->length = (double)obj->sfinfo.frames / obj->sfinfo.samplerate;

  in_obj->samplerate =  obj->sfinfo.samplerate;
  in_obj->channels =  obj->sfinfo.channels;

  if (obj->sfinfo.format& SF_FORMAT_WAV)
    ea_obj->format = ECORE_AUDIO_FORMAT_WAV;
  else if (obj->sfinfo.format& SF_FORMAT_OGG)
    ea_obj->format = ECORE_AUDIO_FORMAT_OGG;
  else if (obj->sfinfo.format& SF_FORMAT_FLAC)
    ea_obj->format = ECORE_AUDIO_FORMAT_FLAC;
  else
    ea_obj->format = ECORE_AUDIO_FORMAT_AUTO;
}

static void _source_get(Eo *eo_obj, void *_pd EINA_UNUSED, va_list *list)
{
  Ecore_Audio_Object *obj = eo_data_get(eo_obj, ECORE_AUDIO_OBJ_CLASS);

  const char **ret = va_arg(*list, const char **);

  if (ret)
    *ret = obj->source;
}

static void _format_set(Eo *eo_obj, void *_pd, va_list *list)
{
  Ecore_Audio_Sndfile *obj = _pd;
  Ecore_Audio_Object *ea_obj = eo_data_get(eo_obj, ECORE_AUDIO_OBJ_CLASS);

  Ecore_Audio_Format format= va_arg(*list, Ecore_Audio_Format);

  if (ea_obj->source) {
      ERR("Input is already open - cannot change format");
      return;
  }

  switch (format) {
    case ECORE_AUDIO_FORMAT_AUTO:
      obj->sfinfo.format = 0;
    case ECORE_AUDIO_FORMAT_WAV:
      obj->sfinfo.format = SF_FORMAT_WAV|SF_FORMAT_PCM_16;
      break;
    case ECORE_AUDIO_FORMAT_OGG:
      obj->sfinfo.format = SF_FORMAT_OGG|SF_FORMAT_VORBIS;
      break;
    case ECORE_AUDIO_FORMAT_FLAC:
      obj->sfinfo.format = SF_FORMAT_FLAC;
      break;
    default:
      ERR("Format not supported!");
      return;
  }
  ea_obj->format = format;
}

static void _format_get(Eo *eo_obj, void *_pd EINA_UNUSED, va_list *list)
{
  Ecore_Audio_Object *obj = eo_data_get(eo_obj, ECORE_AUDIO_OBJ_CLASS);

  Ecore_Audio_Format *ret = va_arg(*list, Ecore_Audio_Format *);

  if (ret)
    *ret = obj->format;
}

static void _constructor(Eo *eo_obj, void *_pd, va_list *list EINA_UNUSED)
{
  eo_do_super(eo_obj, MY_CLASS, eo_constructor());

}

static void _class_constructor(Eo_Class *klass)
{
  const Eo_Op_Func_Description func_desc[] = {
      /* Virtual functions of parent class implemented in this class */
      EO_OP_FUNC(EO_BASE_ID(EO_BASE_SUB_ID_CONSTRUCTOR), _constructor),
      //EO_OP_FUNC(EO_BASE_ID(EO_BASE_SUB_ID_DESTRUCTOR), _destructor),

      EO_OP_FUNC(ECORE_AUDIO_OBJ_ID(ECORE_AUDIO_OBJ_SUB_ID_SOURCE_SET), _source_set),
      EO_OP_FUNC(ECORE_AUDIO_OBJ_ID(ECORE_AUDIO_OBJ_SUB_ID_SOURCE_GET), _source_get),
      EO_OP_FUNC(ECORE_AUDIO_OBJ_ID(ECORE_AUDIO_OBJ_SUB_ID_FORMAT_SET), _format_set),
      EO_OP_FUNC(ECORE_AUDIO_OBJ_ID(ECORE_AUDIO_OBJ_SUB_ID_FORMAT_GET), _format_get),

      EO_OP_FUNC(ECORE_AUDIO_OBJ_IN_ID(ECORE_AUDIO_OBJ_IN_SUB_ID_SEEK), _seek),
      EO_OP_FUNC(ECORE_AUDIO_OBJ_IN_ID(ECORE_AUDIO_OBJ_IN_SUB_ID_READ_INTERNAL), _read),

      EO_OP_FUNC_SENTINEL
  };

  eo_class_funcs_set(klass, func_desc);
}

static const Eo_Op_Description op_desc[] = {
    EO_OP_DESCRIPTION_SENTINEL
};

static const Eo_Class_Description class_desc = {
    EO_VERSION,
    MY_CLASS_NAME,
    EO_CLASS_TYPE_REGULAR,
    EO_CLASS_DESCRIPTION_OPS(&ECORE_AUDIO_OBJ_IN_SNDFILE_BASE_ID, op_desc, ECORE_AUDIO_OBJ_IN_SNDFILE_SUB_ID_LAST),
    NULL,
    sizeof(Ecore_Audio_Sndfile),
    _class_constructor,
    NULL
};

EO_DEFINE_CLASS(ecore_audio_obj_in_sndfile_class_get, &class_desc, ECORE_AUDIO_OBJ_IN_CLASS, NULL);