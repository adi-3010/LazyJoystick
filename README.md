# LazyJoystick
LazyJoystick is a linux user-space translation driver which converts commands from any Xbox-like controller into keyboard or mouse commands. This is meant to allow easy navigation of the computer using the controller, mainly for the convenience of scrolling and navigating through the desktop environment. Some application where you may be interested in using this driver are
- Scrolling through PDFs
- Using a controller as a wireless input device during presentations
- Navigating basic tasks from the couch

## Purpose
While all of the above are possible using other solutions such as Steam Input or KDE's in-built translator(which is sadly, not configurable), this driver is built with the aim of being completely customisable and meant for the singular purpose of convenient usage. The prospect of being able to use my PC from the comforts of the couch is the actual primary motivation behind this project.

## Implemented Features
- High resolution scrolling with analog inputs (sticks and triggers). For now only scrolling with right stick works
- Ability to use analog inputs as a mouse pointer

## Planned Features
The features that have been planned for the implementation of the driver along with their priority(lower number - higher priority) are as follows
- A UI front-end to configure controller inputs - 2
- Ability to set customisable response curves for the analog inputs - 2
- Ability to map buttons on the controller to keyboard or mouse keys - 1
- Ability to bind shortcut keys - 3
- Ability to bind macros - 4
- Add a functionality similar to Logitech's G-Shift - 3

## Working
The driver makes use of libevdev to acquire/grab the inputs from the controller which are then sent to a processor which makes use of a uinput module which creates a virtual keyboard/mouse device to perform the needed functions. 

## Documentation
- [libevdev Documentation](https://www.freedesktop.org/software/libevdev/doc/latest/)
- [Input Subsystem API Documentation](https://docs.kernel.org/driver-api/input.html)
- [Userspace Input API Documentation](https://docs.kernel.org/input/input_uapi.html)
- [uinput Documentation](https://docs.kernel.org/input/uinput.html)
- [Event Code Documentation](https://docs.kernel.org/input/event-codes.html)
- [user-space API guide](https://docs.kernel.org/userspace-api/index.html)
- [raylib CheatSheet](https://www.raylib.com/cheatsheet/cheatsheet.html)
- [raylib Wiki](https://github.com/raysan5/raylib/wiki)

