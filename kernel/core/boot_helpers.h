#ifndef CORTEXOS_BOOT_HELPERS_H
#define CORTEXOS_BOOT_HELPERS_H

struct virtio_pci_scan_result;

void boot_status(const char *msg);
void boot_failed(const char *msg);
void boot_info(const char *msg);
void boot_delay(void);
void print_banner(void);
void report_virtio_devices(const struct virtio_pci_scan_result *result);

#endif
