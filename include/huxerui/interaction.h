#pragma once

#include <huxerui/interaction/hover.h>
#include <huxerui/interaction/pressable.h>
#include <huxerui/interaction/pulse.h>
#include <huxerui/interaction/ripple.h>

/// @file
/// Interactive animation Views and modifiers for HuxerUI. The widgets compose retained HuxerUI motion primitives
/// (Animated presentation modifiers, MotionController, and NodeExtension paint phases) with interaction events.
/// No installation hook is required. Views impose no intrinsic frame; supply bounded layout through a parent or
/// Frame, and accessibility text through Semantics.
