/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/design/designer_window.h"

#include "el/elements.h"

namespace el {
namespace design {

DesignerWindow::DesignerWindow() {
  set_text("Empty Designer");

  LoadData(
      "LayoutBox\n"
      "	distribution gravity\n"
      "	size available\n"
      "	LayoutBox\n"
      "		gravity top bottom\n"
      "		distribution available\n"
      "		distribution-position top left\n"
      "		size available\n"
      "		axis y\n"
      "		LayoutBox\n"
      "			distribution available\n"
      "			Button\n"
      "				text Test\n"
      "				id test\n"
      "		TextBox\n"
      "			placeholder @search\n"
      "			type search\n"
      "			id element_list_search_box\n"
      "		ListBox\n"
      "			id element_list_box\n"
      "	LayoutBox\n"
      "		distribution available\n"
      "		TextBox\n"
      "			id source_text_box\n"
      "			multiline 1\n"
      "			gravity all\n"
      "		LayoutBox: axis: y, distribution: available, position: left\n"
      "			ScrollContainer\n"
      "				id build_container\n"
      "				gravity all\n"
      "			LabelContainer: text: \"Adapt to container\"\n"
      "				CheckBox: id: \"constrained\"\n");

  element_list_box_ = GetElementById<ListBox>(TBIDC("element_list_box"));
  assert(!!element_list_box_);
  element_list_box_->set_source(nullptr);

  source_text_box_ = GetElementById<TextBox>(TBIDC("source_text_box"));
  assert(!!source_text_box_);
  event_handler_.Listen(EventType::kChanged, source_text_box_,
                        [this](const Event& ev) {
                          RefreshContent();
                          return false;  // Just watching.
                        });

  build_container_ = GetElementById<Element>(TBIDC("build_container"));
  assert(!!build_container_);
  build_content_root_ = build_container_->content_root();
}

DesignerWindow::~DesignerWindow() { CloseContent(); }

void DesignerWindow::Show(Element* root_element) {
  set_rect({100, 50, 900, 600});

  auto root = root_element->parent_root();
  root->AddChild(this);
}

void DesignerWindow::BindContent(Element* bind_element) {
  CloseContent();
  bind_element_ = bind_element;

  // TODO(benvanik): serialize element tree.
  source_text_box_->set_text("Label: text: foo");

  RefreshContent();
}

void DesignerWindow::CloseContent() {
  bind_element_.reset();
  RefreshContent();
}

void DesignerWindow::RefreshContent() {
  auto build_content_root_ = build_container_->content_root();

  // Clear existing content.
  build_content_root_->DeleteAllChildren();

  // Bail if no content specified.
  if (!bind_element_) {
    set_text("Empty Designer");
    element_list_box_->set_source(nullptr);
    source_text_box_->set_text("");
    return;
  }
  auto bind_element = bind_element_.get();
  set_text_format("Designer: %s", bind_element_.get()->GetClassName());

  // Load content views.
  auto source_text = source_text_box_->text();
  auto loaded = build_content_root_->LoadData(source_text);
  if (loaded) {
    // Good!
  } else {
    set_text_format("INVALID");
    // TODO(benvanik): error display/etc.
  }

  // Setup other UI.
  PopulateElementListBox();

  // Focus source again.
  source_text_box_->set_focus(FocusReason::kUnknown);
}

void DesignerWindow::PopulateElementListBox() {
  element_list_source_.clear();

  // TODO(benvanik): walk tree.
  element_list_source_.push_back(std::make_unique<ElementItem>(nullptr, "foo"));

  element_list_box_->set_source(&element_list_source_);
}

}  // namespace design
}  // namespace el
