#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include "gstagnosticbin.h"

#define PLUGIN_NAME "agnosticbin"

#define INPUT_TEE "input_tee"
#define DECODED_TEE "decoded_tee"

#define OLD_EVENT_FUNC_DATA "old_event_func"
#define START_STOP_EVENT_FUNC_DATA "start_stop_event_func"
#define AGNOSTIC_BIN_DATA "agnostic_bin"
#define DECODEBIN_VALVE_DATA "decodebin_valve"
#define QUEUE_DATA "queue"

#define START_STOP_EVENT_NAME "event/start-stop"
#define START "start"

#define RAW_VIDEO_CAPS "video/x-raw, "\
  "format={ I420, YV12, YUY2, UYVY, AYUV, RGBx, BGRx, xRGB, xBGR, RGBA, BGRA, "\
      "ARGB, ABGR, RGB, BGR, Y41B, Y42B, YVYU, Y444, v210, v216, NV12, NV21, "\
      "GRAY8, GRAY16_BE, GRAY16_LE, v308, RGB16, BGR16, RGB15, BGR15, UYVP, "\
      "A420, RGB8P, YUV9, YVU9, IYU1, ARGB64, AYUV64, r210, I420_10LE, "\
      "I420_10BE, I422_10LE, I422_10BE }, "\
  "width=[1, 2147483647], "\
  "height=[1, 2147483647], "\
  "framerate=[ 0/1, 2147483647/1];"

#define RAW_AUDIO_CAPS "audio/x-raw, "\
  "format={ S8, U8, S16LE, S16BE, U16LE, U16BE, S24_32LE, S24_32BE, U24_32LE, "\
      "U24_32BE, S32LE, S32BE, U32LE, U32BE, S24LE, S24BE, U24LE, U24BE, "\
      "S20LE, S20BE, U20LE, U20BE, S18LE, S18BE, U18LE, U18BE, F32LE, F32BE, "\
      "F64LE, F64BE }, "\
  "rate=[1, 2147483647], "\
  "channels=[1, 2147483647], "\
  "layout=interleaved;"

#define RAW_CAPS RAW_AUDIO_CAPS RAW_VIDEO_CAPS

static GstStaticCaps static_raw_caps = GST_STATIC_CAPS (RAW_CAPS);

#define AUDIO_CAPS RAW_AUDIO_CAPS \
  "audio/x-sbc;" \
  "audio/x-mulaw;" \
  "audio/x-flac;" \
  "audio/x-alaw;" \
  "audio/x-speex;" \
  "audio/x-ac3;" \
  "audio/x-alac;" \
  "audio/mpeg,mpegversion=1,layer=2;" \
  "audio/x-nellymoser;" \
  "audio/x-gst_ff-sonic;" \
  "audio/x-gst_ff-sonicls;" \
  "audio/x-wma,wmaversion=1;" \
  "audio/x-wma,wmaversion=2;" \
  "audio/x-dpcm,layout=roq;" \
  "audio/x-adpcm,layout=adx;" \
  "audio/x-adpcm,layout=g726;" \
  "audio/x-adpcm,layout=quicktime;" \
  "audio/x-adpcm,layout=dvi;" \
  "audio/x-adpcm,layout=microsoft;" \
  "audio/x-adpcm,layout=swf;" \
  "audio/x-adpcm,layout=yamaha;" \
  "audio/mpeg,mpegversion=4;" \
  "audio/mpeg,mpegversion=1,layer=3;" \
  "audio/x-celt;" \
  "audio/mpeg,mpegversion=[2, 4];" \
  "audio/x-vorbis;" \
  "audio/x-opus;" \
  "audio/AMR,rate=[8000, 16000],channels=1;" \
  "audio/x-gsm;"

#define VIDEO_CAPS RAW_VIDEO_CAPS \
  "video/x-dirac;" \
  "image/png;" \
  "image/jpeg;" \
  "video/x-smoke;" \
  "video/x-asus,asusversion=1;" \
  "video/x-asus,asusversion=2;" \
  "image/bmp;" \
  "video/x-dnxhd;" \
  "video/x-dv;" \
  "video/x-ffv,ffvversion=1;" \
  "video/x-gst_ff-ffvhuff;" \
  "video/x-flash-screen;" \
  "video/x-flash-video,flvversion=1;" \
  "video/x-h261;" \
  "video/x-h263,variant=itu,h263version=h263;" \
  "video/x-h263,variant=itu,h263version=h263p;" \
  "video/x-huffyuv;" \
  "image/jpeg;" \
  "image/jpeg;" \
  "video/mpeg,mpegversion=1;" \
  "video/mpeg,mpegversion=2;" \
  "video/mpeg,mpegversion=4;" \
  "video/x-msmpeg,msmpegversion=41;" \
  "video/x-msmpeg,msmpegversion=42;" \
  "video/x-msmpeg,msmpegversion=43;" \
  "video/x-gst_ff-pam;" \
  "image/pbm;" \
  "video/x-gst_ff-pgm;" \
  "video/x-gst_ff-pgmyuv;" \
  "image/png;" \
  "image/ppm;" \
  "video/x-rle,layout=quicktime;" \
  "video/x-gst_ff-roqvideo;" \
  "video/x-pn-realvideo,rmversion=1;" \
  "video/x-pn-realvideo,rmversion=2;" \
  "video/x-gst_ff-snow;" \
  "video/x-svq,svqversion=1;" \
  "video/x-wmv,wmvversion=1;" \
  "video/x-wmv,wmvversion=2;" \
  "video/x-gst_ff-zmbv;" \
  "video/x-theora;" \
  "video/x-h264;" \
  "video/x-gst_ff-libxvid;" \
  "video/x-h264;" \
  "video/x-xvid;" \
  "video/mpeg,mpegversion=[1, 2];" \
  "video/x-theora;" \
  "video/x-vp8;" \
  "application/x-yuv4mpeg,y4mversion=2;" \

#define AGNOSTIC_CAPS AUDIO_CAPS VIDEO_CAPS

GST_DEBUG_CATEGORY_STATIC (gst_agnostic_bin_debug);
#define GST_CAT_DEFAULT gst_agnostic_bin_debug

#define gst_agnostic_bin_parent_class parent_class
G_DEFINE_TYPE (GstAgnosticBin, gst_agnostic_bin, GST_TYPE_BIN);

typedef void (*GstStartStopFunction) (GstAgnosticBin * agnosticbin,
    GstElement * element, gboolean start);

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0
};

/* the capabilities of the inputs and outputs. */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (AGNOSTIC_CAPS)
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src_%u",
    GST_PAD_SRC,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS (AGNOSTIC_CAPS)
    );

static void
gst_agnostic_bin_valve_start_stop (GstAgnosticBin * agnosticbin,
    GstElement * valve, gboolean start)
{
  g_object_set (valve, "drop", !start, NULL);
}

static void
gst_agnostic_bin_decodebin_start_stop (GstAgnosticBin * agnosticbin,
    GstElement * decodebin, gboolean start)
{
  GstElement *valve =
      g_object_get_data (G_OBJECT (decodebin), DECODEBIN_VALVE_DATA);

  g_object_set (valve, "drop", !start, NULL);
}

static gboolean
gst_agnostic_bin_start_stop_event_handler (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  gboolean ret = TRUE;
  GstPadEventFunction old_func =
      g_object_get_data (G_OBJECT (pad), OLD_EVENT_FUNC_DATA);

  if (GST_EVENT_TYPE (event) == GST_EVENT_CUSTOM_UPSTREAM) {
    const GstStructure *st = gst_event_get_structure (event);

    if (st != NULL
        && g_strcmp0 (gst_structure_get_name (st),
            START_STOP_EVENT_NAME) == 0) {
      gboolean start;

      if (gst_structure_get_boolean (st, START, &start)) {
        GstAgnosticBin *agnosticbin =
            g_object_get_data (G_OBJECT (pad), AGNOSTIC_BIN_DATA);
        GstStartStopFunction callback =
            g_object_get_data (G_OBJECT (pad), START_STOP_EVENT_FUNC_DATA);
        GST_INFO ("Received event: %P", event);

        if (callback != NULL)
          callback (agnosticbin, GST_ELEMENT (parent), start);

        gst_event_unref (event);
        return TRUE;
      }
    }
  }

  if (old_func != NULL)
    ret = old_func (pad, parent, event);
  else
    ret = gst_pad_event_default (pad, parent, event);

  return ret;
}

static void
gst_agnostic_bin_set_start_stop_event_handler (GstAgnosticBin * agnosticbin,
    GstElement * element, const char *pad_name, GstStartStopFunction callback)
{
  GstPad *pad = gst_element_get_static_pad (element, pad_name);

  if (pad == NULL)
    return;

  g_object_set_data (G_OBJECT (pad), OLD_EVENT_FUNC_DATA, pad->eventfunc);
  g_object_set_data (G_OBJECT (pad), START_STOP_EVENT_FUNC_DATA, callback);
  g_object_set_data (G_OBJECT (pad), AGNOSTIC_BIN_DATA, agnosticbin);
  gst_pad_set_event_function (pad, gst_agnostic_bin_start_stop_event_handler);
  g_object_unref (pad);
}

static void
gst_agnostic_bin_send_start_stop_event (GstPad * pad, gboolean start)
{
  GstStructure *data =
      gst_structure_new (START_STOP_EVENT_NAME, START, G_TYPE_BOOLEAN, start,
      NULL);
  GstEvent *event = gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM, data);

  if (GST_PAD_DIRECTION (pad) == GST_PAD_SRC) {
    gst_pad_send_event (pad, event);
  } else {
    GstPad *peer = gst_pad_get_peer (pad);

    if (peer != NULL && GST_PAD_DIRECTION (peer) == GST_PAD_SRC)
      gst_pad_send_event (peer, event);
    else
      gst_event_unref (event);

    if (peer != NULL)
      g_object_unref (peer);
  }
}

static void
gst_agnostic_bin_unlink_from_tee (GstElement * element, const gchar * pad_name)
{
  GstPad *sink = gst_element_get_static_pad (element, pad_name);
  GstPad *tee_src = gst_pad_get_peer (sink);
  GstElement *tee;

  g_object_unref (sink);

  if (tee_src != NULL) {
    tee = gst_pad_get_parent_element (tee_src);

    if (tee != NULL) {
      gint n_pads = 0;
      GstPad *tee_sink = gst_element_get_static_pad (tee, "sink");

      GST_PAD_STREAM_LOCK (tee_sink);
      gst_element_unlink (tee, element);
      gst_element_release_request_pad (tee, tee_src);

      g_object_get (tee, "num-src-pads", &n_pads, NULL);
      if (n_pads == 0)
        gst_agnostic_bin_send_start_stop_event (tee_sink, FALSE);

      GST_PAD_STREAM_UNLOCK (tee_sink);
      g_object_unref (tee_sink);
    }

    g_object_unref (tee_src);
  }
}

static void
gst_agnostic_bin_link_to_tee (GstElement * tee, GstElement * element,
    const gchar * sink_name)
{
  GstPad *tee_src, *tee_sink = gst_element_get_static_pad (tee, "sink");

  gst_agnostic_bin_unlink_from_tee (element, sink_name);
  GST_PAD_STREAM_LOCK (tee_sink);
  tee_src = gst_element_get_request_pad (tee, "src_%u");
  if (tee_src != NULL) {
    if (!gst_element_link_pads (tee, GST_OBJECT_NAME (tee_src), element,
            sink_name)) {
      gst_element_release_request_pad (tee, tee_src);
    } else {
      GstEvent *event = gst_event_new_reconfigure ();

      gst_pad_send_event (tee_src, event);
      gst_agnostic_bin_send_start_stop_event (tee_sink, TRUE);
    }

    g_object_unref (tee_src);
  }

  GST_PAD_STREAM_UNLOCK (tee_sink);
  g_object_unref (tee_sink);
}

static void
gst_agnostic_bin_disconnect_srcpad (GstAgnosticBin * agnosticbin,
    GstPad * srcpad)
{
  GstElement *queue = g_object_get_data (G_OBJECT (srcpad), QUEUE_DATA);

  gst_agnostic_bin_unlink_from_tee (queue, "sink");
}

static void
gst_agnostic_bin_connect_srcpad (GstAgnosticBin * agnosticbin, GstPad * srcpad,
    GstPad * peer, const GstCaps * current_caps)
{
  GstCaps *allowed_caps, *raw_caps;
  GstElement *tee = NULL, *queue;

  GST_DEBUG ("Connecting %P", srcpad);
  if (!GST_IS_GHOST_PAD (srcpad)) {
    GST_DEBUG ("%P is no gp", srcpad);
    return;
  }

  allowed_caps = gst_pad_get_allowed_caps (srcpad);
  if (allowed_caps == NULL) {
    GST_DEBUG ("Allowed caps for %P are NULL. "
        "The pad is not linked, disconnecting", srcpad);
    gst_agnostic_bin_disconnect_srcpad (agnosticbin, srcpad);
    return;
  }

  if (current_caps == NULL) {
    GST_DEBUG ("No current caps, disconnecting %P", srcpad);
    gst_agnostic_bin_disconnect_srcpad (agnosticbin, srcpad);
    return;
  }

  raw_caps = gst_static_caps_get (&static_raw_caps);
  if (gst_caps_can_intersect (current_caps, allowed_caps)) {
    tee = gst_bin_get_by_name (GST_BIN (agnosticbin), INPUT_TEE);
  } else if (gst_caps_can_intersect (raw_caps, allowed_caps)) {
    GST_DEBUG ("Raw caps, looking for a decodebin");
    GST_AGNOSTIC_BIN_LOCK (agnosticbin);
    tee = gst_bin_get_by_name (GST_BIN (agnosticbin), DECODED_TEE);
    GST_AGNOSTIC_BIN_UNLOCK (agnosticbin);
  }
  gst_caps_unref (raw_caps);
  // TODO: Check if reencoding is needed

  queue = g_object_get_data (G_OBJECT (srcpad), QUEUE_DATA);
  if (tee != NULL) {
    gst_agnostic_bin_link_to_tee (tee, queue, "sink");

    g_object_unref (tee);
  } else {
    gst_agnostic_bin_unlink_from_tee (queue, "sink");
  }

  gst_caps_unref (allowed_caps);
}

static void
gst_agnostic_bin_connect_previous_srcpads (GstAgnosticBin * agnosticbin,
    const GstCaps * current_caps)
{
  GValue item = { 0, };
  GstIterator *it;
  gboolean done;

  it = gst_element_iterate_src_pads (GST_ELEMENT (agnosticbin));
  done = FALSE;

  while (!done) {
    switch (gst_iterator_next (it, &item)) {
      case GST_ITERATOR_OK:
      {
        GstPad *srcpad, *peer;

        srcpad = g_value_get_object (&item);
        peer = gst_pad_get_peer (srcpad);

        if (peer != NULL) {
          gst_agnostic_bin_connect_srcpad (agnosticbin, srcpad, peer,
              current_caps);
          g_object_unref (peer);
        }

        g_value_reset (&item);
        break;
      }
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync (it);
        break;
      case GST_ITERATOR_ERROR:
        done = TRUE;
        break;
      case GST_ITERATOR_DONE:
        done = TRUE;
        break;
    }
  }

  g_value_unset (&item);
  gst_iterator_free (it);
}

/* this function handles sink events */
static gboolean
gst_agnostic_bin_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  gboolean ret;
  GstCaps *caps, *old_caps;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
      gst_event_parse_caps (event, &caps);
      old_caps = gst_pad_get_current_caps (pad);
      ret = gst_pad_event_default (pad, parent, event);
      GST_DEBUG ("Received new caps: %P, old was: %P", caps, old_caps);
      if (ret && (old_caps == NULL || !gst_caps_is_equal (old_caps, caps)))
        gst_agnostic_bin_connect_previous_srcpads (GST_AGNOSTIC_BIN (parent),
            caps);
      if (old_caps != NULL)
        gst_caps_unref (old_caps);
      break;
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}

static GstPadLinkReturn
gst_agnostic_bin_src_linked (GstPad * pad, GstObject * parent, GstPad * peer)
{
  GstAgnosticBin *agnosticbin = GST_AGNOSTIC_BIN (parent);
  GstCaps *current_caps;

  GST_DEBUG ("%P linked", pad);
  current_caps = gst_pad_get_current_caps (agnosticbin->sinkpad);
  gst_agnostic_bin_connect_srcpad (agnosticbin, pad, peer, current_caps);
  if (current_caps != NULL)
    gst_caps_unref (current_caps);

  if (peer->linkfunc != NULL)
    peer->linkfunc (peer, GST_OBJECT_PARENT (peer), pad);
  return GST_PAD_LINK_OK;
}

static void
gst_agnostic_bin_src_unlinked (GstPad * pad, GstPad * peer,
    GstAgnosticBin * agnosticbin)
{
  GST_DEBUG ("%P unlinked", pad);
  gst_agnostic_bin_disconnect_srcpad (agnosticbin, pad);
}

static GstPad *
gst_agnostic_bin_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps)
{
  GstPad *pad, *target;
  gchar *pad_name;
  GstAgnosticBin *agnosticbin = GST_AGNOSTIC_BIN (element);
  GstElement *queue = gst_element_factory_make ("queue", NULL);

  GST_OBJECT_LOCK (element);
  pad_name = g_strdup_printf ("src_%d", agnosticbin->pad_count++);
  GST_OBJECT_UNLOCK (element);

  gst_bin_add (GST_BIN (agnosticbin), queue);
  gst_element_sync_state_with_parent (queue);
  target = gst_element_get_static_pad (queue, "src");

  pad = gst_ghost_pad_new_from_template (pad_name, target, templ);
  g_object_unref (target);
  g_free (pad_name);
  g_object_set_data_full (G_OBJECT (pad), QUEUE_DATA, g_object_ref (queue),
      g_object_unref);

  gst_pad_set_link_function (pad, gst_agnostic_bin_src_linked);
  g_signal_connect (pad, "unlinked", G_CALLBACK (gst_agnostic_bin_src_unlinked),
      element);

  if (GST_STATE (element) >= GST_STATE_PAUSED
      || GST_STATE_PENDING (element) >= GST_STATE_PAUSED
      || GST_STATE_TARGET (element) >= GST_STATE_PAUSED)
    gst_pad_set_active (pad, TRUE);

  if (gst_element_add_pad (element, pad))
    return pad;

  g_object_unref (pad);

  return NULL;
}

static void
gst_agnostic_bin_release_pad (GstElement * element, GstPad * pad)
{
  GstElement *queue;

  if (GST_STATE (element) >= GST_STATE_PAUSED
      || GST_STATE_PENDING (element) >= GST_STATE_PAUSED)
    gst_pad_set_active (pad, FALSE);

  queue = g_object_get_data (G_OBJECT (pad), QUEUE_DATA);

  gst_bin_remove (GST_BIN (element), queue);
  gst_element_set_state (queue, GST_STATE_PLAYING);

  gst_element_remove_pad (element, pad);
}

static void
gst_agnostic_bin_decodebin_pad_added (GstElement * decodebin, GstPad * pad,
    GstAgnosticBin * agnosticbin)
{
  GstElement *tee;

  GST_AGNOSTIC_BIN_LOCK (agnosticbin);
  tee = gst_bin_get_by_name (GST_BIN (agnosticbin), DECODED_TEE);
  if (tee == NULL) {
    GstElement *valve;
    GstCaps *current_caps;

    tee = gst_element_factory_make ("tee", DECODED_TEE);
    gst_bin_add (GST_BIN (agnosticbin), tee);
    GST_AGNOSTIC_BIN_UNLOCK (agnosticbin);

    gst_element_sync_state_with_parent (tee);
    gst_element_link_pads (decodebin, GST_OBJECT_NAME (pad), tee, "sink");
    valve = g_object_get_data (G_OBJECT (decodebin), DECODEBIN_VALVE_DATA);
    g_object_set (valve, "drop", TRUE, NULL);

    gst_agnostic_bin_set_start_stop_event_handler (agnosticbin, decodebin,
        GST_OBJECT_NAME (pad), gst_agnostic_bin_decodebin_start_stop);

    current_caps = gst_pad_get_current_caps (agnosticbin->sinkpad);
    gst_agnostic_bin_connect_previous_srcpads (agnosticbin, current_caps);
    if (current_caps != NULL)
      gst_caps_unref (current_caps);
  } else {
    GstElement *fakesink;

    GST_AGNOSTIC_BIN_UNLOCK (agnosticbin);
    fakesink = gst_element_factory_make ("fakesink", NULL);
    gst_bin_add (GST_BIN (agnosticbin), fakesink);
    gst_element_sync_state_with_parent (fakesink);
    gst_element_link_pads (decodebin, GST_OBJECT_NAME (pad), fakesink, "sink");
  }
}

static void
gst_agnostic_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_agnostic_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_agnostic_bin_dispose (GObject * object)
{
  GstAgnosticBin *agnosticbin = GST_AGNOSTIC_BIN (object);

  g_rec_mutex_clear (&agnosticbin->media_mutex);
  G_OBJECT_CLASS (gst_agnostic_bin_parent_class)->dispose (object);
}

static void
gst_agnostic_bin_class_init (GstAgnosticBinClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gstelement_class = GST_ELEMENT_CLASS (klass);

  gobject_class->set_property = gst_agnostic_bin_set_property;
  gobject_class->get_property = gst_agnostic_bin_get_property;
  gobject_class->dispose = gst_agnostic_bin_dispose;

  gst_element_class_set_details_simple (gstelement_class,
      "Agnostic connector",
      "Generic/Bin/Connector",
      "Automatically encodes/decodes media to match sink and source pads caps",
      "José Antonio Santos Cadenas <santoscadenas@kurento.com>, "
      "Miguel París Díaz <mparisdiaz@gmail.com>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));

  gstelement_class->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_agnostic_bin_request_new_pad);
  gstelement_class->release_pad =
      GST_DEBUG_FUNCPTR (gst_agnostic_bin_release_pad);

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "agnosticbin", 0, "agnosticbin");
}

static void
gst_agnostic_bin_init (GstAgnosticBin * agnosticbin)
{
  GstPad *target_sink;
  GstElement *valve, *tee, *decodebin, *queue, *deco_valve;
  GstPadTemplate *templ;

  g_rec_mutex_init (&agnosticbin->media_mutex);

  valve = gst_element_factory_make ("valve", NULL);
  tee = gst_element_factory_make ("tee", INPUT_TEE);
  decodebin = gst_element_factory_make ("decodebin", NULL);
  queue = gst_element_factory_make ("queue", NULL);
  deco_valve = gst_element_factory_make ("valve", NULL);

  g_signal_connect (decodebin, "pad-added",
      G_CALLBACK (gst_agnostic_bin_decodebin_pad_added), agnosticbin);
  g_object_set_data (G_OBJECT (decodebin), DECODEBIN_VALVE_DATA, deco_valve);

  gst_bin_add_many (GST_BIN (agnosticbin), valve, tee, queue, deco_valve,
      decodebin, NULL);
  gst_element_link (valve, tee);
  gst_element_link_many (queue, deco_valve, decodebin, NULL);

  gst_agnostic_bin_set_start_stop_event_handler (agnosticbin, valve, "src",
      gst_agnostic_bin_valve_start_stop);

  target_sink = gst_element_get_static_pad (valve, "sink");
  templ = gst_static_pad_template_get (&sink_factory);

  agnosticbin->pad_count = 0;
  agnosticbin->sinkpad =
      gst_ghost_pad_new_from_template ("sink", target_sink, templ);

  g_object_unref (templ);

  g_object_unref (target_sink);

  gst_pad_set_event_function (agnosticbin->sinkpad,
      GST_DEBUG_FUNCPTR (gst_agnostic_bin_sink_event));
  GST_PAD_SET_PROXY_CAPS (agnosticbin->sinkpad);
  gst_element_add_pad (GST_ELEMENT (agnosticbin), agnosticbin->sinkpad);

  gst_element_link (tee, queue);
}

gboolean
gst_agnostic_bin_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, PLUGIN_NAME, GST_RANK_NONE,
      GST_TYPE_AGNOSTIC_BIN);
}