/*
 * (C) Copyright 2013 Kurento (http://kurento.org/)
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */
#ifndef __KMS_ELEMENT_H__
#define __KMS_ELEMENT_H__

#include <gst/gst.h>
#include "kmselementpadtype.h"

G_BEGIN_DECLS
/* #defines don't like whitespacey bits */
#define KMS_TYPE_ELEMENT \
  (kms_element_get_type())
#define KMS_ELEMENT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),KMS_TYPE_ELEMENT,KmsElement))
#define KMS_ELEMENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),KMS_TYPE_ELEMENT,KmsElementClass))
#define KMS_IS_ELEMENT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),KMS_TYPE_ELEMENT))
#define KMS_IS_ELEMENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),KMS_TYPE_ELEMENT))
#define KMS_ELEMENT_CAST(obj) ((KmsElement*)(obj))
#define KMS_ELEMENT_GET_CLASS(obj) (       \
  G_TYPE_INSTANCE_GET_CLASS (              \
    (obj),                                 \
    KMS_TYPE_ELEMENT,                      \
    KmsElementClass                        \
  )                                        \
)

const gchar *kms_element_pad_type_str (KmsElementPadType type);

typedef struct _KmsElement KmsElement;
typedef struct _KmsElementClass KmsElementClass;
typedef struct _KmsElementPrivate KmsElementPrivate;

#define KMS_ELEMENT_LOCK(elem) \
  (g_rec_mutex_lock (&KMS_ELEMENT_CAST ((elem))->mutex))
#define KMS_ELEMENT_UNLOCK(elem) \
  (g_rec_mutex_unlock (&KMS_ELEMENT_CAST ((elem))->mutex))

struct _KmsElement
{
  GstBin parent;

  GRecMutex mutex;

  /*< private > */
  KmsElementPrivate *priv;
};

struct _KmsElementClass
{
  GstBinClass parent_class;

  /* signals */
  void (*agnosticbin_added) (KmsElementClass * self);

  /* actions */
  gchar * (*request_new_srcpad) (KmsElement *self, KmsElementPadType type, const gchar *desc);
  gboolean (*release_requested_srcpad) (KmsElement *self, const gchar *pad_name);

  /* protected methods */
  gboolean (*sink_query) (KmsElement *self, GstPad * pad, GstQuery *query);
};

GType kms_element_get_type (void);

/* Private methods */
GstElement * kms_element_get_audio_agnosticbin (KmsElement * self);
GstElement * kms_element_get_video_agnosticbin (KmsElement * self);

#define kms_element_connect_sink_target(self, target, type)   \
  kms_element_connect_sink_target_full (self, target, type, NULL)

GstPad *kms_element_connect_sink_target_full (KmsElement *self, GstPad * target,
    KmsElementPadType type, const gchar *description);
void kms_element_remove_sink (KmsElement *self, GstPad * pad);

#define kms_element_remove_sink_by_type(self, type) \
  kms_element_remove_sink_by_type_full (self, type, NULL)

void kms_element_remove_sink_by_type_full (KmsElement *self,
    KmsElementPadType type, const gchar *description);

KmsElementPadType kms_element_get_pad_type (KmsElement * self, GstPad * pad);

G_END_DECLS
#endif /* __KMS_ELEMENT_H__ */
