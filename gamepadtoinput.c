#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <stdint.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
#include <linux/uinput.h>
#include <math.h>

/**
 * TODO: Toggle horizontal scrolling via a key combination
 * TODO: Implement mouse movement and clicking
 */




#define TICK_RATE_MS 16.67f       // 60 FPS Target Frame Time
#define CENTER_VAL 32768.0f       // The physical center of the stick axes
#define MAX_DEV 32768.0f          // Maximum deviation from center (32768 -> 65536 or 0)
#define DEADZONE 0.02f            // deadzone to prevent ghost scrolling
#define SCROLL_SPEED_MOD 0.5f    // Sensitivity multiplier. Tweak this value to change scroll speed!
#define STICK_DIRECTION_VERT 1 // Give this a value of -1 to invert the stick direction
#define STICK_DIRECTION_HORIZ 1 // Give this a value of -1 to invert the stick direction

#define CONSTRAIN(x, lower, upper) (((x)<(lower))?(lower):(x>upper)?(upper):(x))

// 120 units represents exactly ONE traditional scroll wheel notch click.
#define WHEEL_NOTCH_VALUE 120.0f
// Maximum scroll speed: How many high-res units to scroll per second at full stick tilt.
// 840.0f units/sec = 7 full mechanical notches per second. Increasing this number
// also increases the overall scrolling speed, as it is the number that is scaled
#define MAX_SCROLL_SPEED_HI_RES 840.0f 

bool horizScrollState = true; // true is on, false is off

typedef struct {
    int vertical;
    int horizontal;
}Cartesian;

typedef struct {
    float vertical;
    float horizontal;
}Cartestian_flt;

typedef struct {
    float hiRes;
    float legacy;
}accumScroll;

typedef struct {
    float x;
    float y;
}accumMouse;

typedef union {
    uint16_t buttonMap;
    struct {
        uint16_t a:1;
        uint16_t b:1;
        uint16_t x:1;
        uint16_t y:1;
        uint16_t start:1;
        uint16_t select:1;
        uint16_t home:1;
        uint16_t up:1;
        uint16_t down:1;
        uint16_t left:1;
        uint16_t right:1;
        uint16_t thumbL:1;
        uint16_t thumbR:1;
        uint16_t bumperR:1;
        uint16_t bumperL:1;
        uint16_t unused:1;
    };
}buttonState_t;


/**
 *  Button Mapping - Cosmic Byte Stellaris
    axisLX -  ABS_X
    axisLY - ABS_Y
    axisRX - ABS_Z
    axisRY - ABS_RZ
    triggerL - ABS_BRAKE
    triggerR - ABS_GAS
    hatUp - ABS_HAT0Y -1
    hatDown - ABS_HAT0Y 1
    hatLeft - ABS_HAT0X -1
    hatRight - ABS_HAT0X 1
    btA - BTN_SOUTH
    btX - BTN_NORTH
    btY - BTN_WEST
    btB - BTN_EAST
    thumbL - BTN_THUMBL
    thumbR - BTN_THUMBR
    bumperL - BTN_TL
    bumperR - BTL_TR
    select - BTN_BACK
    start - BTN_START
    home - KEY_HOMEPAGE
 */
int updateStates(struct libevdev *hw_dev, Cartesian *left, Cartesian *right, struct input_event *ev, buttonState_t *btn){
    // Flush and read all outstanding kernel events since the last tick
    int update = 0;
    int rc;
    while ((rc = libevdev_next_event(hw_dev, LIBEVDEV_READ_FLAG_NORMAL, ev)) == LIBEVDEV_READ_STATUS_SUCCESS) {
        if (ev->type == EV_ABS) {
            update = 1;
            if(ev->code == ABS_RZ) right->vertical = ev->value;
            else if(ev->code == ABS_Z) right->horizontal = ev->value;
            else if(ev->code == ABS_X) left->horizontal = ev->value;
            else if(ev->code == ABS_Y) left->vertical = ev->value;
            else if(ev->code == ABS_HAT0Y){ // we don't want to clear the values of the hats for every abs
                btn->up = (ev->value == -1);
                btn->down = (ev->value == 1);
            }
            else if(ev->code == ABS_HAT0X && ev->value == -1) {
                btn->left = (ev->value == -1);
                btn->right = (ev->value == 1);
            }
        }
        if(ev->type == EV_KEY){
            update = 1;
            bool pressed = (ev->value != 0);// apparently value can have values 0, 1 and 2(which apparently means repeat)
            if(ev->code == BTN_SOUTH) btn->a = pressed;
            else if(ev->code == BTN_NORTH) btn->x = pressed;
            else if(ev->code == BTN_WEST) btn->y = pressed;
            else if(ev->code == BTN_EAST) btn->b = pressed;
            else if(ev->code == KEY_HOMEPAGE) btn->home = pressed;
            else if(ev->code == BTN_START) btn->start = pressed;
            else if(ev->code == BTN_BACK) btn->select = pressed;
            else if(ev->code == BTN_THUMBL) btn->thumbL = pressed;
            else if(ev->code == BTN_THUMBR) btn->thumbR = pressed;
            else if(ev->code == BTN_TL) btn->bumperL = pressed;
            else if(ev->code == BTN_TR) btn->bumperR = pressed;
        }
    }
    if (rc == LIBEVDEV_READ_STATUS_SYNC) {
        while (rc == LIBEVDEV_READ_STATUS_SYNC) rc = libevdev_next_event(hw_dev, LIBEVDEV_READ_FLAG_SYNC, ev);
    }
    return update;
}

Cartestian_flt calcDeflection(Cartesian stickData){
    Cartestian_flt result;
    // Calculate deflections. Using linear scaling and mapping direct percentages
    result.vertical = (((float)stickData.vertical) - CENTER_VAL)/MAX_DEV;
    result.horizontal = (((float)stickData.horizontal) - CENTER_VAL)/MAX_DEV;
    // constrain to [-1.0f, 1.0f]
    result.vertical = CONSTRAIN(result.vertical, -1.0f, 1.0f);
    result.horizontal = CONSTRAIN(result.horizontal, -1.0f, 1.0f);
    // Deadzone limits
    if(fabsf(result.vertical) < DEADZONE){
        result.vertical = 0.0f;
    }
    if(fabsf(result.horizontal) < DEADZONE){
        result.horizontal = 0.0f;
    }
    return result;
}

int sendScroll(int ui_fd, accumScroll *vertical, accumScroll *horizontal){
    // If any one command needs to be sent, make sendPacket true
    int sendPacket = 0;
    // High-Resolution First
    // Vertical
    if(abs((int)vertical->hiRes)>=1){ // if there is at least 1 
        struct input_event hiRes_ev = {
            .type = EV_REL,
            .code = REL_WHEEL_HI_RES,
            .value = (int)vertical->hiRes
        };
        write(ui_fd, &hiRes_ev, sizeof(hiRes_ev));
        vertical->hiRes -= (int)vertical->hiRes;
        sendPacket = 1;
    }
    // Horizontal
    if(abs((int)horizontal->hiRes)>=1){
        struct input_event hiRes_ev = {
            .type = EV_REL,
            .code = REL_HWHEEL_HI_RES,
            .value = (int)horizontal->hiRes
        };
        write(ui_fd, &hiRes_ev, sizeof(hiRes_ev));
        horizontal->hiRes -= (int)horizontal->hiRes;
        sendPacket = 1;
    }

    // Legacy
    // Vertical
    if(abs((int)vertical->legacy)>=WHEEL_NOTCH_VALUE){ // if there is at least 1 
        int notch = (vertical->legacy > 0)?1:-1;
        struct input_event legacy_ev = {
            .type = EV_REL,
            .code = REL_WHEEL,
            .value = notch
        };
        write(ui_fd, &legacy_ev, sizeof(legacy_ev));
        vertical->legacy -= notch*WHEEL_NOTCH_VALUE;
        sendPacket = 1;
    }
    // Horizontal
    if(abs((int)horizontal->legacy)>=WHEEL_NOTCH_VALUE){
        int notch = (horizontal->legacy>0)?1:-1;
        struct input_event legacy_ev = {
            .type = EV_REL,
            .code = REL_HWHEEL,
            .value = notch
        };
        write(ui_fd, &legacy_ev, sizeof(legacy_ev));
        horizontal->legacy -= notch*WHEEL_NOTCH_VALUE;
        sendPacket = 1;
    }
    return sendPacket;
}

int main(int argc, char** argv) {
    if(argc<2){
        printf("Usage: %s /dev/input/eventX", argv[0]);
        return EXIT_FAILURE;
    }

    int hw_fd = open(argv[1], O_RDONLY | O_NONBLOCK);
    if (hw_fd < 0) {
        fprintf(stderr, "Incorrect device ID. Something's wrong with the file descriptor\n");
        return EXIT_FAILURE;
    }

    struct libevdev *hw_dev = NULL;
    if (libevdev_new_from_fd(hw_fd, &hw_dev) < 0) {
        fprintf(stderr, "Couldn't create libevdev struct\n");
        close(hw_fd);
        return EXIT_FAILURE;
    }
    libevdev_grab(hw_dev, LIBEVDEV_GRAB);

    // --- Setup Virtual Mouse Subsystem via uinput ---
    int ui_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (ui_fd < 0) {
        perror("Failed to open /dev/uinput (Are you running with sudo?)");
        libevdev_free(hw_dev);
        close(hw_fd);
        return EXIT_FAILURE;
    }

    struct uinput_setup usetup = {0};
    usetup.id.bustype = BUS_USB; // I want it to be recognised as a USB device obviously
    usetup.id.vendor  = 0x7f10; // Prototype device
    usetup.id.product = 0x6969; // Whatever number you want here. 6969 because nice
    strcpy(usetup.name, "LazyJoystick Virtual Engine");

    // Configure uinput to support relative mouse scroll wheels
    ioctl(ui_fd, UI_SET_EVBIT, EV_REL); 
    ioctl(ui_fd, UI_SET_RELBIT, REL_WHEEL);
    ioctl(ui_fd, UI_SET_RELBIT, REL_WHEEL_HI_RES);
    ioctl(ui_fd, UI_SET_RELBIT, REL_HWHEEL);
    ioctl(ui_fd, UI_SET_RELBIT, REL_HWHEEL_HI_RES);
    ioctl(ui_fd, UI_SET_RELBIT, REL_X); // mouse movement horizontal
    ioctl(ui_fd, UI_SET_RELBIT, REL_Y); // mouse movement vertical

    ioctl(ui_fd, UI_SET_EVBIT, EV_KEY); // Using all buttons
    ioctl(ui_fd, UI_SET_KEYBIT, BTN_LEFT); // Left click
    ioctl(ui_fd, UI_SET_KEYBIT, BTN_RIGHT); // right click
    ioctl(ui_fd, UI_SET_KEYBIT, BTN_MIDDLE); // middle click

    ioctl(ui_fd, UI_DEV_SETUP, &usetup);
    ioctl(ui_fd, UI_DEV_CREATE);
    printf("LazyJoystick active! Update rate is set to 60Hz. Press Ctrl+C to terminate.\n");

    
    buttonState_t buttons;
    buttons.buttonMap = 0;
    Cartesian stickLeft;
    stickLeft.vertical = (int)CENTER_VAL;
    stickLeft.horizontal = (int)CENTER_VAL;
    Cartesian stickRight;
    stickRight.vertical = (int)CENTER_VAL;
    stickRight.horizontal = (int)CENTER_VAL;
    Cartestian_flt defLeft = {.horizontal = 0.0f, .vertical = 0.0f};
    Cartestian_flt defRight = {.horizontal = 0.0f, .vertical = 0.0f};;

    accumScroll scrollVert = {.hiRes = 0.0f, .legacy = 0.0f};
    accumScroll scrollHoriz = {.hiRes = 0.0f, .legacy = 0.0f};
    accumMouse mouse = {.x = 0.0f, .y = 0.0f};

    // Structure timing configurations
    struct timespec frame_time;
    frame_time.tv_sec = 0;
    frame_time.tv_nsec = (long)(TICK_RATE_MS * 1000000L); // 16.67 milliseconds converted to nanoseconds

    float dt = TICK_RATE_MS / 1000.0f; // Delta-time in seconds (0.01667)
    while (1) {
        struct input_event ev;
        int update = updateStates(hw_dev, &stickLeft, &stickRight, &ev, &buttons); // Read stick data
        // Normalise and store deflections (range: [-1.0f, 1.0f])
        defLeft = calcDeflection(stickLeft); 
        defRight = calcDeflection(stickRight);
        
        if(!horizScrollState){ // if the horizontal scroll state is false, make the horizontal deflection 0
            defRight.horizontal = 0.0f;
        }

        // Calculate raw high-res units earned during this frame cycle.
        // The gist of how high-res scrolling works is as follows
        // Any mouse that doesn't have high-res scrolling support will always send a value 
        // that is a multiple of 120
        // Wheels that support high-res scrolling can support values between 0 and 120, 
        // allowing smoother scrolling
        // We invert the sign because pushing stick UP (deflection < 0) should scroll UP 
        // (positive wheel)
        
        Cartestian_flt pointsToAdd;
        pointsToAdd.vertical = -STICK_DIRECTION_VERT*defRight.vertical * MAX_SCROLL_SPEED_HI_RES * dt;
        pointsToAdd.horizontal = STICK_DIRECTION_HORIZ*defRight.horizontal * MAX_SCROLL_SPEED_HI_RES * dt;

        scrollVert.hiRes += pointsToAdd.vertical;
        scrollVert.legacy +=pointsToAdd.vertical;

        scrollHoriz.hiRes +=pointsToAdd.horizontal;
        scrollHoriz.legacy += pointsToAdd.horizontal;

        int sendPacket = 0;

        sendPacket = sendScroll(ui_fd, &scrollVert, &scrollHoriz);

        // Push Synchronization Boundary Packet
        // Without this packet, the commands we have sent won't be processed
        if (sendPacket) {
            struct input_event syn_ev = { .type = EV_SYN, .code = SYN_REPORT, .value = 0 };
            write(ui_fd, &syn_ev, sizeof(syn_ev));
        }

        // High-precision POSIX sleep to regulate target tick speed
        nanosleep(&frame_time, NULL);
    }

    out: // It is probably wise to teleport code here via a goto when something goes wrong
    ioctl(ui_fd, UI_DEV_DESTROY);
    close(ui_fd);
    libevdev_grab(hw_dev, LIBEVDEV_UNGRAB);
    libevdev_free(hw_dev);
    close(hw_fd);
    return 0;
}
