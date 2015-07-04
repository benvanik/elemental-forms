/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_UTIL_TIMER_H_
#define TB_UTIL_TIMER_H_

#include <cstdint>

namespace tb {
namespace util {

// Called when the need to call MessageHandler::ProcessMessages has changed due
// to changes in the message queue.
// fire_time_millis is the new time is needs to be called. It may be 0 which
// means that ProcessMessages should be called asap (but NOT from this call!).
// It may also be kNotSoon which means that ProcessMessages doesn't need to be
// called.
void RescheduleTimer(uint64_t fire_time_millis);

}  // namespace util
}  // namespace tb

#endif  // TB_UTIL_TIMER_H_
