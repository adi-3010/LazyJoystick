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
- Automatic re-connection after disconnects
- Automatic connection to a specified vendor and product ID

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

## Usage
For the first time you connect a particular controller, you will have to find it's event file. With KDE, you can find the correct event ID by simply going to the game controller page in the settings. To build from source, you will first need to install libevdev, if you haven't already

```bash
# Fedora
sudo dnf install libevdev-devl

# Ubuntu/Debian
sudo apt install libevdev-dev
```

Build main.c either with the provided VS Code configs or with the command below:

```bash
gcc -I/usr/include/libevdev-1.0 main.c -o lazyjoystick -levdev -lm
```

On running for the first time, you have two options.

**Option 1:** 
- Create a config.ini file in the same directory as the lazyjoystick binary and copy the below configuration with **your own controller's product and vendor IDs**
```
[Device]
vendor_id = 0x045E
product_id = 0x02FD
```
- Now run the lazyjoystick binary
```bash
./lazyjoystick
```
- Any device with the given product and vendor ID will automatically be grabbed on connection.

**Option 2:** 
- Find your controller's event number and run the following command. 
```bash
# Replace eventX with your gamepad's event ID
./lazyjoystick /dev/input/eventX
``` 
- This will automatically create a config.ini file with the selected controller's vendor and product ID. You can simply run lazyjoystick and connect the controller from the next time.

**WARNING: Entering any other device's product and vendor ID or event ID will cause it to be grabbed. This means that the device will be unusable as long as the program is running**

## Default Bindings
By default, the following bindings apply
- Left Stick - Mouse pointer
- Right Stick - Scroll wheel (horizontal disabled)
- Left bumper - Left click
- Right bumper - Right click
- Y - Middle click
Configuration functionality will be added in the future through the config.ini file. 

## License
This project is licensed under the GNU Public License v3(GPLv3)

## Refrences & Documentation
- [libevdev Documentation](https://www.freedesktop.org/software/libevdev/doc/latest/)
- [Input Subsystem API Documentation](https://docs.kernel.org/driver-api/input.html)
- [Userspace Input API Documentation](https://docs.kernel.org/input/input_uapi.html)
- [uinput Documentation](https://docs.kernel.org/input/uinput.html)
- [Event Code Documentation](https://docs.kernel.org/input/event-codes.html)
- [user-space API guide](https://docs.kernel.org/userspace-api/index.html)
- [raylib CheatSheet](https://www.raylib.com/cheatsheet/cheatsheet.html)
- [raylib Wiki](https://github.com/raysan5/raylib/wiki)

