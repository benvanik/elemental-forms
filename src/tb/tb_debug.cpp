/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#include "tb_debug.h"

#include <cstdio>

#include "tb_core.h"
#include "tb_font_renderer.h"
#include "tb_tempbuffer.h"
#include "tb_text_box.h"
#include "tb_widgets_reader.h"
#include "tb_window.h"

namespace tb {

#ifdef TB_RUNTIME_DEBUG_INFO

TBDebugInfo g_tb_debug;

TBDebugInfo::TBDebugInfo() = default;

/** Window showing runtime debug settings. */
class DebugSettingsWindow : public Window, public WidgetListener {
 public:
  TextBox* output;

  TBOBJECT_SUBCLASS(DebugSettingsWindow, Window);

  DebugSettingsWindow(TBWidget* root) {
    SetText("Debug settings");
    g_widgets_reader->LoadData(
        this,
        "Layout: axis: y, distribution: available, position: left\n"
        "	Layout: id: 'container', axis: y, size: available\n"
        "	Label: text: 'Event output:'\n"
        "	TextBox: id: 'output', gravity: all, multiline: 1, wrap: "
        "0\n"
        "		lp: pref-height: 100dp");

    AddCheckbox(TBDebugInfo::Setting::kLayoutBounds, "Layout bounds");
    AddCheckbox(TBDebugInfo::Setting::kLayoutClipping, "Layout clipping");
    AddCheckbox(TBDebugInfo::Setting::kLayoutSizing, "Layout size calculation");
    AddCheckbox(TBDebugInfo::Setting::kDrawRenderBatches, "Render batches");
    AddCheckbox(TBDebugInfo::Setting::kDrawSkinBitmapFragments,
                "Render skin bitmap fragments");
    AddCheckbox(TBDebugInfo::Setting::kDrawFontBitmapFragments,
                "Render font bitmap fragments");

    output = GetWidgetByIDAndType<TextBox>(TBIDC("output"));

    Rect bounds(0, 0, root->GetRect().w, root->GetRect().h);
    SetRect(GetResizeToFitContentRect().CenterIn(bounds).MoveIn(bounds).Clip(
        bounds));

    root->AddChild(this);

    WidgetListener::AddGlobalListener(this);
  }

  ~DebugSettingsWindow() { WidgetListener::RemoveGlobalListener(this); }

  void AddCheckbox(TBDebugInfo::Setting setting, const char* str) {
    CheckBox* check = new CheckBox();
    check->SetValue(g_tb_debug.settings[int(setting)]);
    check->data.SetInt(int(setting));
    check->SetID(TBIDC("check"));

    LabelContainer* label = new LabelContainer();
    label->SetText(str);
    label->GetContentRoot()->AddChild(check, WidgetZ::kBottom);

    GetWidgetByID(TBIDC("container"))->AddChild(label);
  }

  virtual bool OnEvent(const TBWidgetEvent& ev) {
    if (ev.type == EventType::kClick && ev.target->GetID() == TBIDC("check")) {
      // Update setting and invalidate
      g_tb_debug.settings[ev.target->data.GetInt()] = ev.target->GetValue();
      GetParentRoot()->Invalidate();
      return true;
    }
    return Window::OnEvent(ev);
  }

  virtual void OnPaint(const PaintProps& paint_props) {
    // Draw stuff to the right of the debug window
    g_renderer->Translate(GetRect().w, 0);

    // Draw skin bitmap fragments
    if (TB_DEBUG_SETTING(Setting::kDrawSkinBitmapFragments)) g_tb_skin->Debug();

    // Draw font glyph fragments (the font of the hovered widget)
    if (TB_DEBUG_SETTING(Setting::kDrawFontBitmapFragments)) {
      TBWidget* widget = TBWidget::hovered_widget ? TBWidget::hovered_widget
                                                  : TBWidget::focused_widget;
      g_font_manager->GetFontFace(
                          widget ? widget->GetCalculatedFontDescription()
                                 : g_font_manager->GetDefaultFontDescription())
          ->Debug();
    }

    g_renderer->Translate(-GetRect().w, 0);
  }

  std::string GetIDString(const TBID& id) {
    std::string str;
#ifdef TB_RUNTIME_DEBUG_INFO
    str.append("\"");
    str.append(id.debug_string);
    str.append("\"");
#else
    str.SetFormatted("%u", (uint32_t)id);
#endif
    return str;
  }

  // WidgetListener
  virtual bool OnWidgetInvokeEvent(TBWidget* widget, const TBWidgetEvent& ev) {
    // Skip these events for now
    if (ev.IsPointerEvent()) return false;

    // Always ignore activity in this window (or we might get endless recursion)
    if (Window* window = widget->GetParentWindow())
      if (TBSafeCast<DebugSettingsWindow>(window)) return false;

    TBTempBuffer buf;
    buf.AppendString(GetEventTypeStr(ev.type));
    buf.AppendString(" (");
    buf.AppendString(widget->GetClassName());
    buf.AppendString(")");

    buf.AppendString(" id: ");
    buf.AppendString(GetIDString(ev.target->GetID()));

    if (ev.ref_id) {
      buf.AppendString(", ref_id: ");
      buf.AppendString(GetIDString(ev.ref_id));
    }

    if (ev.type == EventType::kChanged) {
      auto text = ev.target->GetText();
      if (text.size() > 24) {
        text.erase(20);
        text.append("...");
      }
      auto extra = tb::format_string(", value: %.2f (\"%s\")",
                                     ev.target->GetValueDouble(), text.c_str());
      buf.AppendString(extra);
    }
    buf.AppendString("\n");

    // Append the line to the output textfield
    StyleEdit* se = output->GetStyleEdit();
    se->selection.SelectNothing();
    se->AppendText(buf.GetData(), std::string::npos, true);
    se->ScrollIfNeeded(false, true);

    // Remove lines from the top if we exceed the height limit.
    const int height_limit = 2000;
    int current_height = se->GetContentHeight();
    if (current_height > height_limit) {
      se->caret.Place(Point(0, current_height - height_limit));
      se->selection.SelectToCaret(se->blocks.GetFirst(), 0);
      se->Delete();
    }
    return false;
  }

  const char* GetEventTypeStr(EventType type) const {
    switch (type) {
      case EventType::kClick:
        return "CLICK";
      case EventType::kLongClick:
        return "LONG_CLICK";
      case EventType::kPointerDown:
        return "POINTER_DOWN";
      case EventType::kPointerUp:
        return "POINTER_UP";
      case EventType::kPointerMove:
        return "POINTER_MOVE";
      case EventType::kWheel:
        return "WHEEL";
      case EventType::kChanged:
        return "CHANGED";
      case EventType::kKeyDown:
        return "KEY_DOWN";
      case EventType::kKeyUp:
        return "KEY_UP";
      case EventType::kShortcut:
        return "SHORT_CUT";
      case EventType::kContextMenu:
        return "CONTEXT_MENU";
      default:
        return "[UNKNOWN]";
    };
  }
};

void ShowDebugInfoSettingsWindow(TBWidget* root) {
  new DebugSettingsWindow(root);
}

#endif  // TB_RUNTIME_DEBUG_INFO

}  // namespace tb
