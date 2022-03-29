#include <interfaces/cps_io.h>

int main() {
    // Initialize a new cps
    int err = cps_create("test.rtxc");
    if (err) {
        printf("Error in codeplug initialization!\n");
        return -1;
    }
    // Write data
    // Close it
    // Re-open it
    err = cps_open("test.rtxc");
    if (err) {
        printf("Error in codeplug reading!\n");
        return -1;
    }
    // Read data and compare
}
