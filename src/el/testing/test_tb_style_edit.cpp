/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/elements/text_box.h"
#include "el/testing/testing.h"

#ifdef EL_UNIT_TESTING

using namespace el;
using namespace el::elements;
using namespace el::text;

EL_TEST_GROUP(tb_text_box) {
  TextBox* edit;
  TextView* sedit;

  // == Setup & helpers =====================================================

  EL_TEST(Setup) {
    EL_VERIFY(edit = new TextBox());
    edit->set_multiline(true);
    sedit = edit->text_view();

    /** Set a size so the layout code will be called and we can do some layout
     * tests. */
    edit->set_rect({0, 0, 1000, 1000});

    /** Force windows style line breaks so testing is the same on all platforms.
     */
    sedit->set_windows_style_break(true);
  }
  EL_TEST(Cleanup) { delete edit; }

  // == Tests ===============================================================

  EL_TEST(settext_singleline) {
    edit->set_multiline(false);
    edit->set_text("One\nTwo", CaretPosition::kEnd);
    EL_VERIFY_STR(edit->text(), "One");
  }

  EL_TEST(settext_multiline) {
    // Both unix and windows line endings should be ok.
    edit->set_text("One\nTwo", CaretPosition::kEnd);
    EL_VERIFY_STR(edit->text(), "One\nTwo");
    edit->set_text("One\r\nTwo", CaretPosition::kEnd);
    EL_VERIFY_STR(edit->text(), "One\r\nTwo");
  }

  EL_TEST(settext_singleline_malformed_utf8) {
    // Should not detect these sequences as having new line character

    edit->set_text("\xC0\x8A", CaretPosition::kEnd);
    EL_VERIFY(sedit->blocks.CountLinks() == 1);

    edit->set_text("\xE0\x80\x8A", CaretPosition::kEnd);
    EL_VERIFY(sedit->blocks.CountLinks() == 1);

    edit->set_text("\xF0\x80\x80\x8A", CaretPosition::kEnd);
    EL_VERIFY(sedit->blocks.CountLinks() == 1);

    edit->set_text("\xF8\x80\x80\x80\x8A", CaretPosition::kEnd);
    EL_VERIFY(sedit->blocks.CountLinks() == 1);

    edit->set_text("\xFC\x80\x80\x80\x80\x8A", CaretPosition::kEnd);
    EL_VERIFY(sedit->blocks.CountLinks() == 1);

    // Should detect the new line character

    edit->set_text("\xF0\nHello", CaretPosition::kEnd);
    EL_VERIFY(sedit->blocks.CountLinks() == 2);
  }

  EL_TEST(settext_undo_stack_ins) {
    // 1 character insertions in sequence should be merged to word boundary.
    sedit->InsertText("O");
    sedit->InsertText("N");
    sedit->InsertText("E");
    sedit->InsertText(" ");
    sedit->InsertText("T");
    sedit->InsertText("W");
    sedit->InsertText("O");
    EL_VERIFY_STR(edit->text(), "ONE TWO");

    sedit->Undo();
    EL_VERIFY_STR(edit->text(), "ONE ");
    sedit->Undo();
    EL_VERIFY_STR(edit->text(), "");

    sedit->Redo();
    EL_VERIFY_STR(edit->text(), "ONE ");
    sedit->Redo();
    EL_VERIFY_STR(edit->text(), "ONE TWO");

    // 1 character insertions that are multi byte utf8 should also merge.
    // Note: this will also replace "TWO" since redoing keeps the redoed part
    // selected.
    sedit->InsertText("L");
    sedit->InsertText("Ö");
    sedit->InsertText("K");
    EL_VERIFY_STR(edit->text(), "ONE LÖK");
    sedit->Undo();
    EL_VERIFY_STR(edit->text(), "ONE ");
  }

  EL_TEST(settext_undo_stack_ins_scattered) {
    sedit->InsertText("AAA");
    sedit->caret.set_global_offset(2);
    sedit->InsertText(".");
    sedit->caret.set_global_offset(1);
    sedit->InsertText(".");
    EL_VERIFY_STR(edit->text(), "A.A.A");

    sedit->Undo();
    EL_VERIFY_STR(edit->text(), "AA.A");
    sedit->Undo();
    EL_VERIFY_STR(edit->text(), "AAA");

    sedit->Redo();
    EL_VERIFY_STR(edit->text(), "AA.A");
    sedit->Redo();
    EL_VERIFY_STR(edit->text(), "A.A.A");
  }

  EL_TEST(settext_undo_stack_ins_multiline) {
    sedit->InsertText("ONE\nTWO");
    EL_VERIFY_STR(edit->text(), "ONE\nTWO");

    sedit->Undo();
    EL_VERIFY_STR(edit->text(), "");
    sedit->Redo();
    EL_VERIFY_STR(edit->text(), "ONE\nTWO");
  }

  EL_TEST(settext_undo_stack_del) {
    sedit->InsertText("ONE TWO");
    sedit->selection.Select(3, 7);
    sedit->selection.RemoveContent();
    EL_VERIFY_STR(edit->text(), "ONE");

    sedit->Undo();
    EL_VERIFY_STR(edit->text(), "ONE TWO");
    sedit->Redo();
    EL_VERIFY_STR(edit->text(), "ONE");
  }

  EL_TEST(settext_undo_stack_ins_linebreak_1) {
    sedit->InsertText("ONETWO");
    sedit->caret.set_global_offset(3);
    sedit->InsertText("\n");
    EL_VERIFY_STR(edit->text(), "ONE\nTWO");

    sedit->Undo();
    EL_VERIFY_STR(edit->text(), "ONETWO");
    sedit->Redo();
    EL_VERIFY_STR(edit->text(), "ONE\nTWO");
  }

  EL_TEST(settext_undo_stack_ins_linebreak_2) {
    // Inserting a linebreak at the end when we don't end
    // the line with a linebreak character must generate
    // one extra linebreak.
    sedit->InsertBreak();
    EL_VERIFY_STR(edit->text(), "\r\n\r\n");

    sedit->InsertBreak();
    EL_VERIFY_STR(edit->text(), "\r\n\r\n\r\n");

    sedit->Undo();
    EL_VERIFY_STR(edit->text(), "\r\n\r\n");
    sedit->Undo();
    EL_VERIFY_STR(edit->text(), "");
  }

  EL_TEST(settext_undo_stack_ins_linebreak_3) {
    sedit->set_windows_style_break(false);

    sedit->InsertBreak();
    EL_VERIFY_STR(edit->text(), "\n\n");

    sedit->InsertBreak();
    EL_VERIFY_STR(edit->text(), "\n\n\n");

    sedit->Undo();
    EL_VERIFY_STR(edit->text(), "\n\n");
    sedit->Undo();
    EL_VERIFY_STR(edit->text(), "");
  }

  EL_TEST(settext_undo_stack_ins_linebreak_4) {
    sedit->InsertText("ONE");
    sedit->InsertBreak();
    EL_VERIFY_STR(edit->text(), "ONE\r\n\r\n");

    sedit->Undo();
    EL_VERIFY_STR(edit->text(), "ONE");

    sedit->Redo();
    EL_VERIFY_STR(edit->text(), "ONE\r\n\r\n");
  }

  EL_TEST(settext_undo_stack_bugfix1) {
    // Make sure we use the test dummy font (ID 0), so we're not dependant on
    // the available fonts & font backend in this test.
    FontDescription fd;
    const int font_size = 48;
    fd.set_size(font_size);
    edit->set_font_description(fd);

    sedit->InsertBreak();
    sedit->InsertText("A");
    EL_VERIFY_STR(edit->text(), "\r\nA\r\n");
    EL_VERIFY(sedit->GetContentHeight() == font_size * 2);

    sedit->KeyDown(0, SpecialKey::kBackspace, ModifierKeys::kNone);
    EL_VERIFY_STR(edit->text(), "\r\n\r\n");
    EL_VERIFY(sedit->GetContentHeight() == font_size * 2);

    sedit->Undo();
    EL_VERIFY_STR(edit->text(), "\r\nA\r\n");
    EL_VERIFY(sedit->GetContentHeight() == font_size * 2);

    sedit->Redo();
    EL_VERIFY_STR(edit->text(), "\r\n\r\n");
    EL_VERIFY(sedit->GetContentHeight() == font_size * 2);
  }

  EL_TEST(settext_insert_linebreaks_move) {
    sedit->InsertText("Foo\n");
    sedit->InsertText("Foo\n");
    sedit->InsertText("Foo\n");
    EL_VERIFY_STR(edit->text(), "Foo\nFoo\nFoo\n");
  }

  EL_TEST(multiline_overflow_1) {
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
    EL_VERIFY(sedit->GetContentHeight() == font_size * 2);
  }
}

#endif  // EL_UNIT_TESTING
