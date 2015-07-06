﻿// ================================================================================
// ==      This file is a part of Turbo Badger. (C) 2011-2014, Emil Segerås      ==
// ==                     See turbo_badger.h for more information.               ==
// ================================================================================

Turbo Badger Integration guide
-----------------------------

This is a short guide to integrating Turbo Badger in a game or application.
It basically list what you most probably want to do.
More details is found in the method definitions.

Build configuration
-------------------

There are defines in tb_config.h that control some features & implementation
details. This file may be overridden by your own configuration (see tb_config.h for
more info).

Input
-----

You need to create at least one "root element" for your UI elements. A Element that
act as receiver for input, painting etc, and is parent to your other elements.
Turbo Badger will handle input capture, focus, etc. automatically.

List of input triggers (root is your root Element):

	root->InvokePointer[Down/Up/Move] - Mouse or touch input.
	root->InvokeWheel                 - Mouse wheel input
	root->InvokeKey                   - Keyboard input

Animations, Layout and Painting
-------------------------------

Before painting, it is a good time to run animations, update elements input states
and layout, since all that affect the rendering. This is done by calling:

	AnimationManager::Update()
	root->InvokeProcessStates()
	root->InvokeProcess()

Before painting the root element (and all its children), you need to prepare
the renderer to set up the correct matrix etc.

	Renderer::get()->BeginPaint(window_width, window_height);
	root->InvokePaint(Element::PaintProps())
	Renderer::get()->EndPaint();

Message handling
----------------

MessageHandler::ProcessMessages() needs to be called frequently to process
the message queue.

If you have a game running its loop continuously, you can just call ProcessMessages()
each time.

If you have a event driven application that should only consume CPU when needed
you can call MessageHandler::GetNextMessageFireTime() for scheduleing when to run
ProcessMessage next. Also, util::RescheduleTimer will be called back if the time
it needs to run next is changed.

Rendering
---------

Renderer & Bitmap are the interfaces that implements everything that Turbo Badger
needs for rendering. There is already a OpenGL renderer implementation that can be
used, or you can implement your own (See tb_config.h).

Some apps need to unload all graphics and reload it again, f.ex when the app
is put to sleep in the background and the graphics context is lost.
Turbo Badger implements a renderer listener to handle this in all places that needs
it, so the only thing you need to do is to call:

	Renderer::get()->InvokeContextLost(); // Forget all bitmaps
	...
	Renderer::get()->InvokeContextRestored(); // Reload all bitmaps

Font system
-----------

There is font implementations for freetype and stb_truetype.h, and a bitmap font
system (tbbf - "turbo badger bitmap font") that is very simple to use and supports
any range or characters, glow & shadow. You can choose any of these or add your own.
It possible to use multiple backends, so you can f.ex use tbbf for some bitmap fonts,
and a freetype backend for other fonts.

The implementation API render glyph by glyph, but if it's requested to externalize
the entire string measuring & drawing, support for that shouldn't be hard to add.

If no font implementation is choosen (or if using font ID 0), a test dummy font
is used which renders squares.

System integration
------------------

Here are some additional API's that needs implementations:

ImageLoader
API for loading images. There's a implementation available using STBI. Note: STBI
should only be used to load your well known resources and nothing from the net,
since the library doesn't handle corrupt files safely which might be a security
issue.

TBSystem
Needs to be implemented for timers & some settings.

Clipboard
Needs to be implemented for clipboard support.

File
Needs to be implemented for file loading.
