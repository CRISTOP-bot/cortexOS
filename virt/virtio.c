#include "virtio.h"

const char *virtio_device_name(uint16_t device_id)
{
    switch (device_id) {
        case VIRTIO_PCI_LEGACY + VIRTIO_DEVICE_BLOCK:
                return "virtio-block";
                    case VIRTIO_PCI_LEGACY + VIRTIO_DEVICE_CONSOLE:
                            return "virtio-console";
                                case VIRTIO_PCI_LEGACY + VIRTIO_DEVICE_NET:
                                        return "virtio-net";
                                            case VIRTIO_PCI_LEGACY + VIRTIO_DEVICE_ENTROPY:
                                                    return "virtio-rng";
                                                        default:
                                                                return "virtio-unknown";
                                                                    }
                                                                    }

                                                                    /* Legacy queues require the used ring to begin at a 4096-byte boundary. */
                                                                    size_t virtio_queue_bytes(uint16_t entries)
                                                                    {
                                                                        size_t descriptors = sizeof(struct virtq_desc) * entries;
                                                                            size_t available = sizeof(uint16_t) * (3u + entries);
                                                                                size_t used = sizeof(uint16_t) * 2u +
                                                                                                  sizeof(struct virtq_used_elem) * entries;
                                                                                                      size_t total = descriptors + available;
                                                                                                          return (total + 4095u) / 4096u * 4096u + used;
                                                                                                          }

                                                                                                          int virtio_queue_entries_valid(uint16_t entries)
                                                                                                          {
                                                                                                              return entries >= 2u && entries <= 32768u &&
                                                                                                                         (entries & (entries - 1u)) == 0u;
                                                                                                                         }
                                                                                                                         