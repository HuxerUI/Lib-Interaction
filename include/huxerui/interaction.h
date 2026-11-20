#pragma once

// pipeline combinators
#include <huxerui/interaction/pipe.h>
// press
#include <huxerui/interaction/press/contextmenu.h>
#include <huxerui/interaction/press/copyable.h>
#include <huxerui/interaction/press/holdrepeat.h>
#include <huxerui/interaction/press/longpress.h>
#include <huxerui/interaction/press/multiclick.h>
#include <huxerui/interaction/press/pressable.h>
#include <huxerui/interaction/press/ripple.h>
#include <huxerui/interaction/press/surface.h>
// hover
#include <huxerui/interaction/hover/highlight.h>
#include <huxerui/interaction/hover/hoverlift.h>
#include <huxerui/interaction/hover/hovershadow.h>
#include <huxerui/interaction/hover/magnetic.h>
#include <huxerui/interaction/hover/spotlight.h>
// gesture
#include <huxerui/interaction/gesture/compareslider.h>
#include <huxerui/interaction/gesture/dismissible.h>
#include <huxerui/interaction/gesture/dragspring.h>
#include <huxerui/interaction/gesture/inertialdrag.h>
#include <huxerui/interaction/gesture/marqueeselect.h>
#include <huxerui/interaction/gesture/pressdraghandle.h>
#include <huxerui/interaction/gesture/radialdial.h>
#include <huxerui/interaction/gesture/resizehandle.h>
#include <huxerui/interaction/gesture/scrubvalue.h>
#include <huxerui/interaction/gesture/snapdrag.h>
#include <huxerui/interaction/gesture/swipe.h>
#include <huxerui/interaction/gesture/swipedeck.h>
#include <huxerui/interaction/gesture/transform.h>
// reorder
#include <huxerui/interaction/reorder/reorder.h>
#include <huxerui/interaction/reorder/reordergrid.h>
#include <huxerui/interaction/reorder/reorderhandle.h>
// drag and drop
#include <huxerui/interaction/dragdrop/draggable.h>
#include <huxerui/interaction/dragdrop/dropzone.h>
#include <huxerui/interaction/dragdrop/filedropzone.h>
// overlay
#include <huxerui/interaction/overlay/hovercard.h>
#include <huxerui/interaction/overlay/popover.h>
// scroll
#include <huxerui/interaction/scroll/nestedscroll.h>
#include <huxerui/interaction/scroll/parallax.h>
#include <huxerui/interaction/scroll/scrolledgefade.h>
#include <huxerui/interaction/scroll/scrollprogress.h>
#include <huxerui/interaction/scroll/scrollreveal.h>
#include <huxerui/interaction/scroll/scrollspy.h>
#include <huxerui/interaction/scroll/scrolltotop.h>
#include <huxerui/interaction/scroll/stickyheader.h>
// accessibility
#include <huxerui/interaction/accessibility/backhandler.h>
#include <huxerui/interaction/accessibility/keyboardshortcut.h>
#include <huxerui/interaction/accessibility/rovingfocus.h>
#include <huxerui/interaction/accessibility/semanticactions.h>
#include <huxerui/interaction/accessibility/shortcutmap.h>
// visual (non-input motion & value-driven presentation)
#include <huxerui/interaction/visual/appear.h>
#include <huxerui/interaction/visual/expandable.h>
#include <huxerui/interaction/visual/fireworks.h>
#include <huxerui/interaction/visual/pop.h>
#include <huxerui/interaction/visual/pulse.h>
#include <huxerui/interaction/visual/revealontap.h>
#include <huxerui/interaction/visual/rollingnumber.h>
#include <huxerui/interaction/visual/shake.h>
#include <huxerui/interaction/visual/sharedmorph.h>
#include <huxerui/interaction/visual/shimmer.h>

/// @file
/// Interactive primitives, controllers, retained feedback modifiers, and visual motion effects for HuxerUI.
/// Headless builders (InteractiveSurface, Hoverable, DropZone) and controllers (SwipeController, PanZoomController,
/// HoverCardController) separate interaction state from presentation; styled presets and pipeable ViewAdaptors build
/// ergonomically on top. Views impose no intrinsic frame; supply bounded layout through a parent or Frame.
