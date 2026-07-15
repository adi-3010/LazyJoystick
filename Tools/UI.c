#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "raylib.h"
#include "libevdev/libevdev.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define LEFT_DEADZONE 
#define RIGHT_DEADZONE
#define STICK_RADIUS
#define OUTER_RADIUS
#define TRIGGER_HEIGHT
#define TRIGGER_WIDTH
#define BUMPER_HEIGHT
#define BUMPER_WIDTH TRIGGER_WIDTH
#define BUTTON_RADIUS
#define BUTTON_SIZE
#define HAT_SIZE

#define PRESS_COLOR RED
#define IDLE_COLOR SKYBLUE
#define BG_COLOR BLACK
#define TRIG_COLOR GREEN
#define BOUND_COLOR WHITE


// The below struct holds all the data related to the Joystick buttons
typedef struct {
    // Absolute values
    int axisLX; // ABS_X
    int axisLY; // ABS_Y
    int axisRX; // ABS_Z
    int axisRY; // ABS_RZ
    // Absolute but can also be digital absolute
    int triggerL; // ABS_BRAKE
    int triggerR; // ABS_GAS
    // Absoulte but actually digital(0 at idle, 1 or -1 otherwise)
    bool hatUp; // ABS_HAT0Y -1
    bool hatDown; // ABS_HAT0Y 1
    bool hatLeft; // ABS_HAT0X -1
    bool hatRight; // ABS_HAT0X 1

    // Buttons
    bool btA; // BTN_SOUTH
    bool btX; // BTN_NORTH
    bool btY; // BTN_WEST
    bool btB; // BTN_EAST
    bool thumbL; // BTN_THUMBL
    bool thumbR; // BTN_THUMBR
    bool bumperL; // BTN_TL
    bool bumperR; // BTL_TR
    bool select; // BTN_BACK
    bool start; // BTN_START
    bool home; // BTN_HOME

}LazyJoystick;


int main(int argc, char** argv){
    if(argc<2){
        printf("Usage: %s /dev/input/eventX\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Initialise libevdev structs
    int hw_fd = open(argv[1], O_RDONLY | O_NONBLOCK);
    struct libevdev* hw_dev = NULL; // initialise this later with libevdev_new_from_fd
    if(libevdev_new_from_fd(hw_fd, &hw_dev)<0){
        printf("Failed to get device event\n");
        return EXIT_FAILURE;
    }
    if(libevdev_grab(hw_dev, LIBEVDEV_GRAB)<0){ // grab so other apps don't interfere
        printf("Failed to grab device\n");
        return EXIT_FAILURE;
    }

    // Initialise the gamepad state to the default values
    // The default values that I am taking are what I found on my own gamepad
    // use scan.c to find your own gamepad's states and their corresponding values
    // the same is true for the event values (found in the comments of the struct)
    LazyJoystick padState = {
        // at idle, the sticks are at 32768. I prefer not normalising the values 
        .axisLX = 32768,
        .axisLY = 32768,
        .axisRX = 32768,
        .axisRY = 32768,
        // at idle, the triggers are at 0
        .triggerL = 0,
        .triggerR = 0,
        // hats 0 at idle
        .hatDown = 0,
        .hatUp = 0,
        .hatLeft = 0,
        .hatRight = 0,
        // all buttons are false at idle
        .btA = false,
        .btB = false,
        .btX = false,
        .btY = false,
        .bumperL = false,
        .bumperR = false,
        .thumbL = false,
        .thumbR = false,
        .start = false,
        .select = false,
        .home = false
    };

    // Initialise the window of the UI
    SetTargetFPS(60);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "LazyJoystick");

    // GUI from here
    while(!WindowShouldClose()){
        pollGamepad(&hw_dev);

        BeginDrawing();

        // Locations of all UI elements
        Vector2 leftStick = {};
        Vector2 rightStick = {};
        Vector2 faceButtons = {};
        Vector2 centerButton = {};
        Vector2 hats = {};
        Vector2 leftTop = {};
        Vector2 rightTop = {};
        
        // Draw all UI elements
        

        EndDrawing();
    }

    // Cleanup
    out: // goto is a bad idea generally, but it's convenient here
    CloseWindow();
    libevdev_grab(hw_dev, LIBEVDEV_UNGRAB);
    libevdev_free(hw_dev);
    close(hw_fd);
    return EXIT_FAILURE;
}