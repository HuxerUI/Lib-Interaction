#pragma once

// press
#include <huxerui/interaction/press/longpress.h>
#include <huxerui/interaction/press/multiclick.h>
#include <huxerui/interaction/press/multitap.h>
#include <huxerui/interaction/press/pressable.h>
#include <huxerui/interaction/press/ripple.h>
#include <huxerui/interaction/press/secondaryclick.h>
// hover
#include <huxerui/interaction/hover/highlight.h>
#include <huxerui/interaction/hover/hover.h>
#include <huxerui/interaction/hover/hovercard.h>
#include <huxerui/interaction/hover/spotlight.h>
// motion
#include <huxerui/interaction/motion/fireworks.h>
#include <huxerui/interaction/motion/pop.h>
#include <huxerui/interaction/motion/pulse.h>
#include <huxerui/interaction/motion/revealontap.h>
#include <huxerui/interaction/motion/rollingnumber.h>
#include <huxerui/interaction/motion/shake.h>
#include <huxerui/interaction/motion/shimmer.h>
// gesture
#include <huxerui/interaction/gesture/dismissible.h>
#include <huxerui/interaction/gesture/dragspring.h>
#include <huxerui/interaction/gesture/inertialdrag.h>
#include <huxerui/interaction/gesture/panzoom.h>
#include <huxerui/interaction/gesture/pinch.h>
#include <huxerui/interaction/gesture/pressdraghandle.h>
#include <huxerui/interaction/gesture/resizehandle.h>
#include <huxerui/interaction/gesture/swipe.h>
// reorder
#include <huxerui/interaction/reorder/reorder.h>
#include <huxerui/interaction/reorder/reordergrid.h>
// drag and drop
#include <huxerui/interaction/dragdrop/draggable.h>
#include <huxerui/interaction/dragdrop/dropzone.h>
#include <huxerui/interaction/dragdrop/filedropzone.h>
// scroll
#include <huxerui/interaction/scroll/scrolledgefade.h>
#include <huxerui/interaction/scroll/scrollprogress.h>
// overlay
#include <huxerui/interaction/overlay/contextmenu.h>
// accessibility
#include <huxerui/interaction/accessibility/keyboardshortcut.h>
#include <huxerui/interaction/accessibility/semanticactions.h>

/// @file
/// Interactive animation Views and modifiers for HuxerUI. The widgets compose retained HuxerUI motion primitives
/// (Animated presentation modifiers, MotionController, and NodeExtension paint phases) with interaction events.
/// No installation hook is required. Views impose no intrinsic frame; supply bounded layout through a parent or
/// Frame, and accessibility text through Semantics.
