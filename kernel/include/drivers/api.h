#ifndef DRIVER_API_H_
#define DRIVER_API_H_

#define DRVLICENSE_GPL "GPL"
#define DRVLICENSE_GPL2 "GPL2"
#define DRVLICENSE_GPL3 "GPL3"

#define DRVLICENSE_BSD0 "BSD0"
#define DRVLICENSE_BSD2 "BSD2"
#define DRVLICENSE_BSD3 "BSD3"
#define DRVLICENSE_BSD4 "BSD4"

#define DRVLICENSE_APACHE "Apache"

#define DRVLICENSE_MIT "MIT"

#define DRVLICENSE_MPL "MPL"

#define DRVLICENSE_CC0 "CC0"

#define DRVLICENSE_DEFAULT DRVLICENSE_GPL3

typedef int (*initcall_t)(void);
typedef void (*exitcall_t)(void);

// trying to replicate linux driver api
// we use linker sections because we cant have dynamic arrays
// at the stage when we need to group the drivers
typedef struct driver {
  const char *name;
  const char *description;
  const char *version;
  const char *license;
  initcall_t init;
  exitcall_t exit;
} driver_t;

#define DRIVER_NAME(n)                                                         \
  static const char __driver_name[] __attribute__((unused)) = (n)

#define DRIVER_DESCRIPTION(n)                                                  \
  static const char __driver_desc[] __attribute__((unused)) = (n)

#define DRIVER_VERSION(n)                                                      \
  static const char __driver_version[] __attribute__((unused)) = (n)

#define DRIVER_LICENSE(n)                                                      \
  static const char __driver_license[] __attribute__((unused)) = (n)

#define driver_init(fn)                                                        \
  static const struct driver __driver_entry_##fn __attribute__((               \
      used, section(".drivers"))) = {.name = __driver_name,                    \
                                     .description = __driver_desc,             \
                                     .version = __driver_version,              \
                                     .license = __driver_license,              \
                                     .init = (fn),                             \
                                     .exit = 0}
#define driver_exit(fn)                                                        \
  static const struct driver __driver_exit_entry_##fn                          \
      __attribute__((used, section(".drivers.exit"))) = {                      \
          .name = __driver_name,                                               \
          .description = __driver_desc,                                        \
          .version = __driver_version,                                         \
          .license = __driver_license,                                         \
          .init = 0,                                                           \
          .exit = (fn),                                                        \
  }

void output_drivers();
void init_drivers();
void cleanup_drivers();

extern driver_t __drivers_start[];
extern driver_t __drivers_end[];

extern driver_t __drivers_exit_start[];
extern driver_t __drivers_exit_end[];

#endif
