/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_SYSTEM_H
#define TB_SYSTEM_H

#include <string>

#include "tb_core.h"
#include "tb_str.h"

#ifdef TB_RUNTIME_DEBUG_INFO
void TBDebugOut(const char* str);
inline void TBDebugOut(const std::string& str) { TBDebugOut(str.c_str()); }
#define TBDebugPrint(str, ...) \
  { TBDebugOut(tb::format_string(str, __VA_ARGS__)); }
#else
#define TBDebugOut(str) ((void)0)
#define TBDebugPrint(str, ...) ((void)0)
#endif

namespace tb {

// == Platform interface ===================================================

/** TBSystem is porting interface for the underlaying OS. */
class TBSystem {
/* Linux specific timer implementation */
#ifdef TB_SYSTEM_LINUX

 private:
  static int timer_fd;

  /* True if since last reschedule timer has fired. Is false if and only
     there was no expiration since last reschedule */
  static bool timer_state;

 public:
  /** Initialize timer descriptor. Return 0 on success, errno otherwise **/
  static int Init();

  /** Close timer descriptor. Return 0 on success, errno otherwise **/
  static int Shutdown();

  /** Check for timer expiration. Return 0 on success
          (it does NOT imply timer fired; Check timer_state) and errno otherwise
     **/
  static int PollEvents();

#endif /* TB_SYSTEM_LINUX */
 public:
  /** Get the system time in milliseconds since some undefined epoch. */
  static double GetTimeMS();

  /** Called when the need to call TBMessageHandler::ProcessMessages has changed
     due to changes in the
          message queue. fire_time is the new time is needs to be called.
          It may be 0 which means that ProcessMessages should be called asap
     (but NOT from this call!)
          It may also be TB_NOT_SOON which means that ProcessMessages doesn't
     need to be called. */
  static void RescheduleTimer(double fire_time);

  /** Get how many milliseconds it should take after a touch down event should
     generate a long click
          event. */
  static int GetLongClickDelayMS();

  /** Get how many pixels of dragging should start panning scrollable widgets.
   */
  static int GetPanThreshold();

  /** Get how many pixels a typical line is: The length that should be scrolled
     when turning a mouse
          wheel one notch. */
  static int GetPixelsPerLine();

  /** Get Dots Per Inch for the main screen. */
  static int GetDPI();

#ifdef TB_TARGET_LINUX

  /** Check whether timer has fired. For now only
                  does work on Linux. On other platforms it defaults to true **/
  inline static bool GetTimerState() { return timer_state; }

#else
  inline static bool GetTimerState() { return true; }
#endif /* TB_TARGET_LINUX */
};

/** TBClipboard is a porting interface for the clipboard. */
class TBClipboard {
 public:
  /** Empty the contents of the clipboard. */
  static void Empty();

  /** Return true if the clipboard currently contains text. */
  static bool HasText();

  /** Set the text of the clipboard in UTF-8 format. */
  static bool SetText(const std::string& text);

  /** Get the text from the clipboard in UTF-8 format. */
  static std::string GetText();
};

/** TBFile is a porting interface for file access. */
class TBFile {
 public:
  enum TBFileMode { MODE_READ };
  static TBFile* Open(const std::string& filename, TBFileMode mode);

  virtual ~TBFile() {}
  virtual size_t Size() = 0;
  virtual size_t Read(void* buf, size_t elemSize, size_t count) = 0;
};

}  // namespace tb

#endif  // TB_SYSTEM_H
