/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

// This file contains defines for the default configuration of Elemental.
// You may change these here, but to make upgrades easier it's better to create
// a copy of this file in a include path that is searched before Elemental
// during build (F.ex the solution directory for Visual Studio).

#ifndef EL_CONFIG_H_
#define EL_CONFIG_H_

// Enables for some handy runtime debugging, enabled by modifying the various
// settings in DebugInfo::get()-> A settings window can be shown by calling
// ShowDebugInfoSettingsWindow.
#define EL_RUNTIME_DEBUG_INFO

#ifdef CHECKED
// Enables compilation of unit tests.
#define EL_UNIT_TESTING
#endif

/** Enable if the focus state should automatically be set on edit fields even
        when using the pointer. It is normally set only while moving focus by
   keyboard. */
//#define EL_ALWAYS_SHOW_EDIT_FOCUS

/** Enable to support TBBF fonts (Turbo Badger Bitmap Fonts) */
#define EL_FONT_RENDERER_TBBF

/** Enable to support truetype fonts using freetype. */
//#define EL_FONT_RENDERER_FREETYPE

/** Enable to support truetype fonts using stb_truetype.h
   (http://nothings.org/).
        It's a *very unsafe* font library. Use only with fonts distributed with
   your
        app, that you know work! Freetype generates much prettier glyphs (using
        hinting) but is a lot larger. This implementation is kept here as
   alternative
        as long as it compiles. */
//#define EL_FONT_RENDERER_STB

#endif  // EL_CONFIG_H_