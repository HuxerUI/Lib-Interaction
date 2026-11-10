#pragma once

#include <huxerui/interaction/contextmenu.h>
#include <huxerui/interaction/draggable.h>
#include <huxerui/interaction/dropzone.h>
#include <huxerui/interaction/dragspring.h>
#include <huxerui/interaction/filedropzone.h>
#include <huxerui/interaction/fireworks.h>
#include <huxerui/interaction/highlight.h>
#include <huxerui/interaction/inertialdrag.h>
#include <huxerui/interaction/hover.h>
#include <huxerui/interaction/longpress.h>
#include <huxerui/interaction/multitap.h>
#include <huxerui/interaction/panzoom.h>
#include <huxerui/interaction/pinch.h>
#include <huxerui/interaction/pop.h>
#include <huxerui/interaction/pressable.h>
#include <huxerui/interaction/pulse.h>
#include <huxerui/interaction/reorder.h>
#include <huxerui/interaction/ripple.h>
#include <huxerui/interaction/shake.h>
#include <huxerui/interaction/shimmer.h>
#include <huxerui/interaction/spotlight.h>
#include <huxerui/interaction/swipe.h>

/// @file
/// Interactive animation Views and modifiers for HuxerUI. The widgets compose retained HuxerUI motion primitives
/// (Animated presentation modifiers, MotionController, and NodeExtension paint phases) with interaction events.
/// No installation hook is required. Views impose no intrinsic frame; supply bounded layout through a parent or
/// Frame, and accessibility text through Semantics.
