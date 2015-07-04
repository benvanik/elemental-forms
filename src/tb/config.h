/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */
//
// This file contains defines for the default configuration of Turbo Badger.
// You may change these here, but to make upgrades easier it's better to create
// a copy of this file in a include path that is searched before Turbo Badger
// during build (F.ex the solution directory for Visual Studio).

#ifndef TB_CONFIG_H_
#define TB_CONFIG_H_

// Enables for some handy runtime debugging, enabled by modifying the various
// settings in DebugInfo::get()-> A settings window can be shown by calling
// ShowDebugInfoSettingsWindow.
#define TB_RUNTIME_DEBUG_INFO

#ifndef NDEBUG
// Enables compilation of unit tests.
#define TB_UNIT_TESTING
#endif

/** Enable if the focus state should automatically be set on edit fields even
        when using the pointer. It is normally set only while moving focus by
   keyboard. */
//#define TB_ALWAYS_SHOW_EDIT_FOCUS

/** Enable to support TBBF fonts (Turbo Badger Bitmap Fonts) */
#define TB_FONT_RENDERER_TBBF

/** Enable to support truetype fonts using freetype. */
//#define TB_FONT_RENDERER_FREETYPE

/** Enable to support truetype fonts using stb_truetype.h
   (http://nothings.org/).
        It's a *very unsafe* font library. Use only with fonts distributed with
   your
        app, that you know work! Freetype generates much prettier glyphs (using
        hinting) but is a lot larger. This implementation is kept here as
   alternative
        as long as it compiles. */
//#define TB_FONT_RENDERER_STB

/** Enable to support image loading using stb_image.c (http://nothings.org/).
        It's a *very unsafe* image library. Use only with images distributed
   with
        your app, that you know work! */
#define TB_IMAGE_LOADER_STB

/** The width of the font glyph cache. Must be a power of two. */
#define TB_GLYPH_CACHE_WIDTH 512

/** The height of the font glyph cache. Must be a power of two. */
#define TB_GLYPH_CACHE_HEIGHT 512

#endif  // TB_CONFIG_H_
