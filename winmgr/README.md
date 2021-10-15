# Window manager - User Desktop

 This program take ownership of screens and input devices (mouse and keyboard)
 to serve as a graphic compositor to display UI applications.

# Installation

 This program is a simple executable which need to be configured to run on the
 desired system. It will request `lgfx` library in order to work.

# Design

 The compositor run with 2 threads.
 
 One thread is looking for device input.
 It monitors mouse and capture keyboard shortcuts.
 If those inputs are not to be captured it will re-route those to the active 
 window's application.
 It will convert mouse position to the window's origin, and it will send 
 keyboard press events, which are keyboard layout dependends and not generated
 by hardware.
 This is the main UI thread which will need to handle screens redraw.

 A second thread will listen for application messages by lisening to a UPD 
 port. All applications can send requests to the compositor and send back 
 events throught the same socket.

 At startup a third thread is started to load user configuration, but this one
 will die quickly once it's done. It loads in order: cursors, applications,
 fonts and wallpaper. Screens and input devices are loaded prior to
 everything.

# Improvments

 - Small issue with resize windows
 - Timers are not correclty handled
 - No support for multiple screens
 - No screen border limit
 - Big issue while resizing part-maximized windows
 - Command to modify window visibility
 - Menu item should be scrollable
 - Start an application
 - Change windows icon / title
 - Add [Sleep/Stop/Reset button]
 - Open system settings
 - Provide support for dialogs, context-menus and tooltips

# Source files

 - event.c -- Handle the events
 - main.c -- Should do nothing
 - menu.c -- Generic menu handling...
 - paint.c -- Handle all the painting stuff
 - resx.c --
 - users.c --
 - window.c -- Handle window size and position



 