#include "boot_helpers.h"
#include "console.h"
#include "virtio_pci.h"

void boot_status(const char *msg)
{
    console_print_color("[  OK  ] ", VGA_ATTR(VGA_GREEN, VGA_BLACK));
    console_print_color(msg, VGA_DEFAULT_ATTR);
    console_print("\n");
}

void boot_failed(const char *msg)
{
    console_print_color("[FAILED] ", VGA_ATTR(VGA_RED, VGA_BLACK));
    console_print_color(msg, VGA_DEFAULT_ATTR);
    console_print("\n");
}

void boot_info(const char *msg)
{
    console_print_color("[ INFO ] ", VGA_ATTR(VGA_CYAN, VGA_BLACK));
    console_print_color(msg, VGA_DEFAULT_ATTR);
    console_print("\n");
}

void boot_delay(void)
{
    volatile int i;
    for (i = 0; i < 3000000; i++)
        ;
}

void print_banner(void)
{
    console_print_color("\n", VGA_DEFAULT_ATTR);
    console_print_color("========================================\n", VGA_ATTR(VGA_CYAN, VGA_BLACK));
    console_print_color(" _ _ _ _ ____ \n", VGA_ATTR(VGA_CYAN, VGA_BLACK));
    console_print_color(" | \\ | | ___ _ __| | | |/ ___| \n", VGA_ATTR(VGA_CYAN, VGA_BLACK));
    console_print_color(" | \\| |/ _ \\| '__| | | | | _ \\ \n", VGA_ATTR(VGA_CYAN, VGA_BLACK));
    console_print_color(" | |\\ | (_) | | | |_| | |_| | \n", VGA_ATTR(VGA_CYAN, VGA_BLACK));
    console_print_color(" |_| \\_|\\___/|_| \\___/ \\____| \n", VGA_ATTR(VGA_CYAN, VGA_BLACK));
    console_print_color("========================================\n", VGA_ATTR(VGA_CYAN, VGA_BLACK));
    console_print_color(" Open Source Operating System\n", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
    console_print("\n");
    boot_info("Booting CortexOS v3 x86_64...\n");
    boot_delay();
}

void report_virtio_devices(const struct virtio_pci_scan_result *result)
{
    char digits[11];
    unsigned int value;
    unsigned int length = 0;

    if (!result)
        return;
    value = result->count;
    do {
        digits[length++] = (char)('0' + (value % 10));
        value /= 10;
    } while (value != 0 && length < sizeof(digits));

    console_print("VirtIO PCI devices detected: ");
    while (length > 0)
        console_putchar(digits[--length]);
    console_print("\n");
}
