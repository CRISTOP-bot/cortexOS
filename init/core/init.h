#ifndef CORTEXOS_INIT_H
#define CORTEXOS_INIT_H

#include <stdbool.h>

/* Launch the real OpenRC init as the first user process when installed. */
bool init_start_openrc(void);

#endif /* CORTEXOS_INIT_H */
