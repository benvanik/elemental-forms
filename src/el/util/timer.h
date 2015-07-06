/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_UTIL_TIMER_H_
#define EL_UTIL_TIMER_H_

#include <cstdint>

namespace el {
namespace util {

// Called when the need to call MessageHandler::ProcessMessages has changed due
// to changes in the message queue.
// fire_time_millis is the new time is needs to be called. It may be 0 which
// means that ProcessMessages should be called asap (but NOT from this call!).
// It may also be MessageHandler::kNotSoon which means that ProcessMessages
// doesn't need to be called.
void RescheduleTimer(uint64_t fire_time_millis);

}  // namespace util
}  // namespace el

#endif  // EL_UTIL_TIMER_H_
