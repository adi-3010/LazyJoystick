#define _GNU_SOUeventStatusE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <libevdev/libevdev.h>


/**
 * Helper function to print event details in a human-readable format.
 */
void print_event(struct input_event *ev) {
    // Get human-readable strings for event type and code from libevdev
    const char *type_str = libevdev_event_type_get_name(ev->type);
    const char *code_str = libevdev_event_code_get_name(ev->type, ev->code);

    printf("Event: time %ld.%06ld, type %d (%s), code %d (%s), value %d\n",
           ev->input_event_sec,
           ev->input_event_usec,
           ev->type,
           type_str ? type_str : "?",
           ev->code,
           code_str ? code_str : "?",
           ev->value);
}

int main(int argc, char **argv) {
    if(argc<2){
        printf("Usage: %s /dev/input/eventX", argv[0] );
        return EXIT_FAILURE;
    }
    const char *dev_path = argv[1];

    int hw_fd = open(dev_path, O_RDONLY | O_NONBLOCK);

    struct libevdev *dev = NULL;
    if(libevdev_new_from_fd(hw_fd, &dev)<0){ // you can't control/access anything without the event dev struct
        printf("No struct initialise :(\n");
        return EXIT_FAILURE;
    } 
    printf("Device Name: %s\n", libevdev_get_name(dev));
    printf("Vendor ID: 0x%04x, Product ID: 0x%04x\n", libevdev_get_id_vendor(dev), libevdev_get_id_product(dev));
    
    if(libevdev_grab(dev, LIBEVDEV_GRAB)<0){ // grabs the device so no others can use it
        printf("No grabby grabby :(\n"); 
        return EXIT_FAILURE;
    }

    printf("Device grabbed exclusively! Press Ctrl+C to release and exit.\n\n");

    while (1) {
        struct input_event ev;
        
        int eventStatus = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        
        if (eventStatus == LIBEVDEV_READ_STATUS_SUCCESS) {
            if (ev.type != EV_SYN) {
                print_event(&ev);
            }
        } else if (eventStatus == LIBEVDEV_READ_STATUS_SYNC) {
            printf(">> Re-syncing device state...\n");
            while (eventStatus == LIBEVDEV_READ_STATUS_SYNC) {
                eventStatus = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
            }
            printf(">> Re-sync complete.\n");
        } else {
            if (eventStatus != -EAGAIN) {
                fprintf(stderr, "Error reading event: %s\n", strerror(-eventStatus));
                break;
            }
        }

        usleep(1000); 
    }

    libevdev_grab(dev, LIBEVDEV_UNGRAB);
    libevdev_free(dev);
    close(hw_fd);
    return EXIT_SUCCESS;
}