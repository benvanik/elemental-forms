/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb_test.h"
#include "tb/elements/text_box.h"

#ifdef TB_UNIT_TESTING

using namespace tb;
using namespace tb::elements;
using namespace tb::text;

TB_TEST_GROUP(tb_text_box) {
  TextBox* edit;
  TextView* sedit;

  // == Setup & helpers =====================================================

  TB_TEST(Setup) {
    TB_VERIFY(edit = new TextBox());
    edit->set_multiline(true);
    sedit = edit->text_view();

    /** Set a size so the layout code will be called and we can do some layout
     * tests. */
    edit->set_rect({0, 0, 1000, 1000});

    /** Force windows style line breaks so testing is the same on all platforms.
     */
    sedit->set_windows_style_break(true);
  }
  TB_TEST(Cleanup) { delete edit; }

  // == Tests ===============================================================

  TB_TEST(settext_singleline) {
    edit->set_multiline(false);
    edit->set_text("One\nTwo", CaretPosition::kEnd);
    TB_VERIFY_STR(edit->text(), "One");
  }

  TB_TEST(settext_multiline) {
    // Both unix and windows line endings should be ok.
    edit->set_text("One\nTwo", CaretPosition::kEnd);
    TB_VERIFY_STR(edit->text(), "One\nTwo");
    edit->set_text("One\r\nTwo", CaretPosition::kEnd);
    TB_VERIFY_STR(edit->text(), "One\r\nTwo");
  }

  TB_TEST(settext_singleline_malformed_utf8) {
    // Should not detect these sequences as having new line character

    edit->set_text("\xC0\x8A", CaretPosition::kEnd);
    TB_VERIFY(sedit->blocks.CountLinks() == 1);

    edit->set_text("\xE0\x80\x8A", CaretPosition::kEnd);
    TB_VERIFY(sedit->blocks.CountLinks() == 1);

    edit->set_text("\xF0\x80\x80\x8A", CaretPosition::kEnd);
    TB_VERIFY(sedit->blocks.CountLinks() == 1);

    edit->set_text("\xF8\x80\x80\x80\x8A", CaretPosition::kEnd);
    TB_VERIFY(sedit->blocks.CountLinks() == 1);

    edit->set_text("\xFC\x80\x80\x80\x80\x8A", CaretPosition::kEnd);
    TB_VERIFY(sedit->blocks.CountLinks() == 1);

    // Should detect the new line character

    edit->set_text("\xF0\nHello", CaretPosition::kEnd);
    TB_VERIFY(sedit->blocks.CountLinks() == 2);
  }

  TB_TEST(settext_undo_stack_ins) {
    // 1 character insertions in sequence should be merged to word boundary.
    sedit->InsertText("O");
    sedit->InsertText("N");
    sedit->InsertText("E");
    sedit->InsertText(" ");
    sedit->InsertText("T");
    sedit->InsertText("W");
    sedit->InsertText("O");
    TB_VERIFY_STR(edit->text(), "ONE TWO");

    sedit->Undo();
    TB_VERIFY_STR(edit->text(), "ONE ");
    sedit->Undo();
    TB_VERIFY_STR(edit->text(), "");

    sedit->Redo();
    TB_VERIFY_STR(edit->text(), "ONE ");
    sedit->Redo();
    TB_VERIFY_STR(edit->text(), "ONE TWO");

    // 1 character insertions that are multi byte utf8 should also merge.
    // Note: this will also replace "TWO" since redoing keeps the redoed part
    // selected.
    sedit->InsertText("L");
    sedit->InsertText("Ö");
    sedit->InsertText("K");
    TB_VERIFY_STR(edit->text(), "ONE LÖK");
    sedit->Undo();
    TB_VERIFY_STR(edit->text(), "ONE ");
  }

  TB_TEST(settext_undo_stack_ins_scattered) {
    sedit->InsertText("AAA");
    sedit->caret.set_global_offset(2);
    sedit->InsertText(".");
    sedit->caret.set_global_offset(1);
    sedit->InsertText(".");
    TB_VERIFY_STR(edit->text(), "A.A.A");

    sedit->Undo();
    TB_VERIFY_STR(edit->text(), "AA.A");
    sedit->Undo();
    TB_VERIFY_STR(edit->text(), "AAA");

    sedit->Redo();
    TB_VERIFY_STR(edit->text(), "AA.A");
    sedit->Redo();
    TB_VERIFY_STR(edit->text(), "A.A.A");
  }

  TB_TEST(settext_undo_stack_ins_multiline) {
    sedit->InsertText("ONE\nTWO");
    TB_VERIFY_STR(edit->text(), "ONE\nTWO");

    sedit->Undo();
    TB_VERIFY_STR(edit->text(), "");
    sedit->Redo();
    TB_VERIFY_STR(edit->text(), "ONE\nTWO");
  }

  TB_TEST(settext_undo_stack_del) {
    sedit->InsertText("ONE TWO");
    sedit->selection.Select(3, 7);
    sedit->selection.RemoveContent();
    TB_VERIFY_STR(edit->text(), "ONE");

    sedit->Undo();
    TB_VERIFY_STR(edit->text(), "ONE TWO");
    sedit->Redo();
    TB_VERIFY_STR(edit->text(), "ONE");
  }

  TB_TEST(settext_undo_stack_ins_linebreak_1) {
    sedit->InsertText("ONETWO");
    sedit->caret.set_global_offset(3);
    sedit->InsertText("\n");
    TB_VERIFY_STR(edit->text(), "ONE\nTWO");

    sedit->Undo();
    TB_VERIFY_STR(edit->text(), "ONETWO");
    sedit->Redo();
    TB_VERIFY_STR(edit->text(), "ONE\nTWO");
  }

  TB_TEST(settext_undo_stack_ins_linebreak_2) {
    // Inserting a linebreak at the end when we don't end
    // the line with a linebreak character must generate
    // one extra linebreak.
    sedit->InsertBreak();
    TB_VERIFY_STR(edit->text(), "\r\n\r\n");

    sedit->InsertBreak();
    TB_VERIFY_STR(edit->text(), "\r\n\r\n\r\n");

    sedit->Undo();
    TB_VERIFY_STR(edit->text(), "\r\n\r\n");
    sedit->Undo();
    TB_VERIFY_STR(edit->text(), "");
  }

  TB_TEST(settext_undo_stack_ins_linebreak_3) {
    sedit->set_windows_style_break(false);

    sedit->InsertBreak();
    TB_VERIFY_STR(edit->text(), "\n\n");

    sedit->InsertBreak();
    TB_VERIFY_STR(edit->text(), "\n\n\n");

    sedit->Undo();
    TB_VERIFY_STR(edit->text(), "\n\n");
    sedit->Undo();
    TB_VERIFY_STR(edit->text(), "");
  }

  TB_TEST(settext_undo_stack_ins_linebreak_4) {
    sedit->InsertText("ONE");
    sedit->InsertBreak();
    TB_VERIFY_STR(edit->text(), "ONE\r\n\r\n");

    sedit->Undo();
    TB_VERIFY_STR(edit->text(), "ONE");

    sedit->Redo();
    TB_VERIFY_STR(edit->text(), "ONE\r\n\r\n");
  }

  TB_TEST(settext_undo_stack_bugfix1) {
    // Make sure we use the test dummy font (ID 0), so we're not dependant on
    // the available fonts & font backend in this test.
    FontDescription fd;
    const int font_size = 48;
    fd.set_size(font_size);
    edit->set_font_description(fd);

    sedit->InsertBreak();
    sedit->InsertText("A");
    TB_VERIFY_STR(edit->text(), "\r\nA\r\n");
    TB_VERIFY(sedit->GetContentHeight() == font_size * 2);

    sedit->KeyDown(0, SpecialKey::kBackspace, ModifierKeys::kNone);
    TB_VERIFY_STR(edit->text(), "\r\n\r\n");
    TB_VERIFY(sedit->GetContentHeight() == font_size * 2);

    sedit->Undo();
    TB_VERIFY_STR(edit->text(), "\r\nA\r\n");
    TB_VERIFY(sedit->GetContentHeight() == font_size * 2);

    sedit->Redo();
    TB_VERIFY_STR(edit->text(), "\r\n\r\n");
    TB_VERIFY(sedit->GetContentHeight() == font_size * 2);
  }

  TB_TEST(settext_insert_linebreaks_move) {
    sedit->InsertText("Foo\n");
    sedit->InsertText("Foo\n");
    sedit->InsertText("Foo\n");
    TB_VERIFY_STR(edit->text(), "Foo\nFoo\nFoo\n");
  }

  TB_TEST(multiline_overflow_1) {
    // Make sure we use the test dummy font (ID 0), so we're not dependant on
    // the available fonts & font backend in this test.
    FontDescription fd;
    const int font_size = 48;
    fd.set_size(font_size);
    edit->set_font_description(fd);

    // Test that a long line that overflow but has no allowed break position
    // doesn't wrap.
    edit->set_text(
        "this_is_a_long_line_that_should_not_wrap\n"
        "this_is_a_long_line_that_should_not_wrap",
        CaretPosition::kEnd);
    TB_VERIFY(sedit->GetContentHeight() == font_size * 2);
  }
}

#endif  // TB_UNIT_TESTING
