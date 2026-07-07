#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
#include <linux/uinput.h>

#define TICK_RATE_MS 16.67f       // 60 FPS Target Frame Time
#define CENTER_VAL 32768.0f       // The physical center of your RZ Axis
#define MAX_DEV 32768.0f          // Maximum deviation from center (32768 -> 65536 or 0)
#define DEADZONE 0.02f            // 8% deadzone to prevent ghost scrolling
#define SCROLL_SPEED_MOD 0.5f    // Sensitivity multiplier. Tweak this value to change scroll speed!

// 120 units represents exactly ONE traditional scroll wheel notch click.
#define WHEEL_NOTCH_VALUE 120.0f
// Maximum scroll speed: How many high-res units to scroll per second at full stick tilt.
// 720.0f units/sec = 6 full mechanical notches per second.
#define MAX_SCROLL_SPEED_HI_RES 720.0f 

int main(int argc, char** argv) {
    if(argc<2){
        printf("Usage: %s /dev/input/eventX", argv[0]);
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
    ioctl(ui_fd, UI_DEV_SETUP, &usetup);
    ioctl(ui_fd, UI_DEV_CREATE);

    printf("LazyJoystick active! Update rate is set to 60Hz. Press Ctrl+C to terminate.\n");

    int current_rz = (int)CENTER_VAL;


    float hires_accumulator = 0.0f;
    float legacy_accumulator = 0.0f;

    // Structure timing configurations
    struct timespec frame_time;
    frame_time.tv_sec = 0;
    frame_time.tv_nsec = (long)(TICK_RATE_MS * 1000000L); // 16.67 milliseconds converted to nanoseconds

    float dt = TICK_RATE_MS / 1000.0f; // Delta-time in seconds (0.01667)
    while (1) {
        struct input_event ev;
        int rc;

        // Flush and read all outstanding kernel events since the last tick
        while ((rc = libevdev_next_event(hw_dev, LIBEVDEV_READ_FLAG_NORMAL, &ev)) == LIBEVDEV_READ_STATUS_SUCCESS) {
            if (ev.type == EV_ABS && ev.code == ABS_RZ) {
                current_rz = ev.value;
            }
        }
        if (rc == LIBEVDEV_READ_STATUS_SYNC) {
            while (rc == LIBEVDEV_READ_STATUS_SYNC) rc = libevdev_next_event(hw_dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
        }

        // Linear Scaling
        // Calculate raw deflection direction and normalize it down to [-1.0, 1.0]
        float deflection = ((float)current_rz - CENTER_VAL) / MAX_DEV;

        // Clamp normalization bounds safely
        if (deflection > 1.0f) deflection = 1.0f;
        if (deflection < -1.0f) deflection = -1.0f;

        // Apply Deadzone Filter
        if (deflection > -DEADZONE && deflection < DEADZONE) {
            deflection = 0.0f;
        }

        // Calculate raw high-res units earned during this frame cycle.
        // We invert the sign because pushing stick UP (deflection < 0) should scroll UP (positive wheel)
        float earned_units = -deflection * MAX_SCROLL_SPEED_HI_RES * dt;

        hires_accumulator += earned_units;
        legacy_accumulator += earned_units;

        int send_packet = 0;

        // Emit High-Resolution Scroll Steps
        // We can emit integer chunks of high-resolution scroll steps immediately
        if (abs((int)hires_accumulator) >= 1) {
            int steps_to_send = (int)hires_accumulator;
            
            struct input_event hires_ev = {
                .type = EV_REL,
                .code = REL_WHEEL_HI_RES,
                .value = steps_to_send
            };
            write(ui_fd, &hires_ev, sizeof(hires_ev));
            
            hires_accumulator -= steps_to_send; // Keep the fractional remainder
            send_packet = 1;
        }

        // Emit Legacy Compatibility Clicks
        // If the legacy accumulator crosses the 120-unit notch threshold, we fire a legacy click
        if (legacy_accumulator >= WHEEL_NOTCH_VALUE) {
            struct input_event legacy_ev = {
                .type = EV_REL,
                .code = REL_WHEEL,
                .value = 1 // Scroll Up
            };
            write(ui_fd, &legacy_ev, sizeof(legacy_ev));
            
            legacy_accumulator -= WHEEL_NOTCH_VALUE;
            send_packet = 1;
        } 
        else if (legacy_accumulator <= -WHEEL_NOTCH_VALUE) {
            struct input_event legacy_ev = {
                .type = EV_REL,
                .code = REL_WHEEL,
                .value = -1 // Scroll Down
            };
            write(ui_fd, &legacy_ev, sizeof(legacy_ev));
            
            legacy_accumulator += WHEEL_NOTCH_VALUE;
            send_packet = 1;
        }

        // Push Synchronization Boundary Packet
        if (send_packet) {
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
