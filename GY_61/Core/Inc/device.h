/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
#ifndef ZEPHYR_INCLUDE_DEVICE_H_
#define ZEPHYR_INCLUDE_DEVICE_H_
 
#include <init.h>
#include <linker/sections.h>
#include <sys/device_mmio.h>
#include <sys/util.h>
 
#ifdef __cplusplus
extern "C" {
#endif
 
typedef int16_t device_handle_t;
 
#define DEVICE_HANDLE_SEP INT16_MIN
 
#define DEVICE_HANDLE_ENDS INT16_MAX
 
#define DEVICE_HANDLE_NULL 0
 
#define Z_DEVICE_MAX_NAME_LEN   48
 
#define DEVICE_NAME_GET(name) _CONCAT(__device_, name)
 
#define SYS_DEVICE_DEFINE(drv_name, init_fn, level, prio)               \
        __DEPRECATED_MACRO SYS_INIT(init_fn, level, prio)
 
/* Node paths can exceed the maximum size supported by device_get_binding() in user mode,
 * so synthesize a unique dev_name from the devicetree node.
 *
 * The ordinal used in this name can be mapped to the path by
 * examining zephyr/include/generated/device_extern.h header. If the
 * format of this conversion changes, gen_defines should be updated to
 * match it.
 */
#define Z_DEVICE_DT_DEV_NAME(node_id) _CONCAT(dts_ord_, DT_DEP_ORD(node_id))
 
/* Synthesize a unique name for the device state associated with
 * dev_name.
 */
#define Z_DEVICE_STATE_NAME(dev_name) _CONCAT(__devstate_, dev_name)
 
#define Z_DEVICE_STATE_DEFINE(node_id, dev_name)                        \
        static struct device_state Z_DEVICE_STATE_NAME(dev_name)        \
        __attribute__((__section__(".z_devstate")));
 
#define DEVICE_DEFINE(dev_name, drv_name, init_fn, pm_device,           \
                      data_ptr, cfg_ptr, level, prio, api_ptr)          \
        Z_DEVICE_STATE_DEFINE(DT_INVALID_NODE, dev_name) \
        Z_DEVICE_DEFINE(DT_INVALID_NODE, dev_name, drv_name, init_fn,   \
                        pm_device,                                      \
                        data_ptr, cfg_ptr, level, prio, api_ptr,        \
                        &Z_DEVICE_STATE_NAME(dev_name))
 
#define DEVICE_DT_NAME(node_id) \
        DT_PROP_OR(node_id, label, DT_NODE_FULL_NAME(node_id))
 
#define DEVICE_DT_DEFINE(node_id, init_fn, pm_device,                   \
                         data_ptr, cfg_ptr, level, prio,                \
                         api_ptr, ...)                                  \
        Z_DEVICE_STATE_DEFINE(node_id, Z_DEVICE_DT_DEV_NAME(node_id)) \
        Z_DEVICE_DEFINE(node_id, Z_DEVICE_DT_DEV_NAME(node_id),         \
                        DEVICE_DT_NAME(node_id), init_fn,               \
                        pm_device,                                      \
                        data_ptr, cfg_ptr, level, prio,                 \
                        api_ptr,                                        \
                        &Z_DEVICE_STATE_NAME(Z_DEVICE_DT_DEV_NAME(node_id)),    \
                        __VA_ARGS__)
 
#define DEVICE_DT_INST_DEFINE(inst, ...) \
        DEVICE_DT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)
 
#define DEVICE_DT_NAME_GET(node_id) DEVICE_NAME_GET(Z_DEVICE_DT_DEV_NAME(node_id))
 
#define DEVICE_DT_GET(node_id) (&DEVICE_DT_NAME_GET(node_id))
 
#define DEVICE_DT_INST_GET(inst) DEVICE_DT_GET(DT_DRV_INST(inst))
 
#define DEVICE_DT_GET_ANY(compat)                                           \
        COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(compat),                      \
                    (DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(compat))), \
                    (NULL))
 
#define DEVICE_DT_GET_ONE(compat)                                           \
        COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(compat),                      \
                    (DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(compat))), \
                    (ZERO_OR_COMPILE_ERROR(0)))
 
#define DEVICE_DT_GET_OR_NULL(node_id)                                  \
        COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay),                  \
                    (DEVICE_DT_GET(node_id)), (NULL))
 
#define DEVICE_GET(name) (&DEVICE_NAME_GET(name))
 
#define DEVICE_DECLARE(name) static const struct device DEVICE_NAME_GET(name)
 
struct device_state {
        unsigned int init_res : 8;
 
        bool initialized : 1;
};
 
struct pm_device;
 
#ifdef CONFIG_HAS_DYNAMIC_DEVICE_HANDLES
#define Z_DEVICE_HANDLES_CONST
#else
#define Z_DEVICE_HANDLES_CONST const
#endif
 
struct device {
        const char *name;
        const void *config;
        const void *api;
        struct device_state * const state;
        void * const data;
        Z_DEVICE_HANDLES_CONST device_handle_t * const handles;
 
#ifdef CONFIG_PM_DEVICE
        struct pm_device * const pm;
#endif
};
 
static inline device_handle_t
device_handle_get(const struct device *dev)
{
        device_handle_t ret = DEVICE_HANDLE_NULL;
        extern const struct device __device_start[];
 
        /* TODO: If/when devices can be constructed that are not part of the
         * fixed sequence we'll need another solution.
         */
        if (dev != NULL) {
                ret = 1 + (device_handle_t)(dev - __device_start);
        }
 
        return ret;
}
 
static inline const struct device *
device_from_handle(device_handle_t dev_handle)
{
        extern const struct device __device_start[];
        extern const struct device __device_end[];
        const struct device *dev = NULL;
        size_t numdev = __device_end - __device_start;
 
        if ((dev_handle > 0) && ((size_t)dev_handle <= numdev)) {
                dev = &__device_start[dev_handle - 1];
        }
 
        return dev;
}
 
typedef int (*device_visitor_callback_t)(const struct device *dev, void *context);
 
static inline const device_handle_t *
device_required_handles_get(const struct device *dev,
                            size_t *count)
{
        const device_handle_t *rv = dev->handles;
 
        if (rv != NULL) {
                size_t i = 0;
 
                while ((rv[i] != DEVICE_HANDLE_ENDS)
                       && (rv[i] != DEVICE_HANDLE_SEP)) {
                        ++i;
                }
                *count = i;
        }
 
        return rv;
}
 
static inline const device_handle_t *
device_supported_handles_get(const struct device *dev,
                             size_t *count)
{
        const device_handle_t *rv = dev->handles;
        size_t region = 0;
        size_t i = 0;
 
        if (rv != NULL) {
                /* Fast forward to supporting devices */
                while (region != 2) {
                        if (*rv == DEVICE_HANDLE_SEP) {
                                region++;
                        }
                        rv++;
                }
                /* Count supporting devices */
                while (rv[i] != DEVICE_HANDLE_ENDS) {
                        ++i;
                }
                *count = i;
        }
 
        return rv;
}
 
int device_required_foreach(const struct device *dev,
                          device_visitor_callback_t visitor_cb,
                          void *context);
 
int device_supported_foreach(const struct device *dev,
                             device_visitor_callback_t visitor_cb,
                             void *context);
 
__syscall const struct device *device_get_binding(const char *name);
 
size_t z_device_get_all_static(const struct device * *devices);
 
bool z_device_is_ready(const struct device *dev);
 
__syscall bool device_is_ready(const struct device *dev);
 
static inline bool z_impl_device_is_ready(const struct device *dev)
{
        return z_device_is_ready(dev);
}
 
__deprecated static inline int z_device_usable_check(const struct device *dev)
{
        return z_device_is_ready(dev) ? 0 : -ENODEV;
}
 
__deprecated static inline int device_usable_check(const struct device *dev)
{
        return device_is_ready(dev) ? 0 : -ENODEV;
}
 
/* Synthesize the name of the object that holds device ordinal and
 * dependency data. If the object doesn't come from a devicetree
 * node, use dev_name.
 */
#define Z_DEVICE_HANDLE_NAME(node_id, dev_name)                         \
        _CONCAT(__devicehdl_,                                           \
                COND_CODE_1(DT_NODE_EXISTS(node_id),                    \
                            (node_id),                                  \
                            (dev_name)))
 
#define Z_DEVICE_EXTRA_HANDLES(...)                             \
        FOR_EACH_NONEMPTY_TERM(IDENTITY, (,), __VA_ARGS__)
 
/*
 * Utility macro to define and initialize the device state.
 *
 * @param node_id Devicetree node id of the device.
 * @param dev_name Device name.
 */
#define Z_DEVICE_STATE_DEFINE(node_id, dev_name)                        \
        static struct device_state Z_DEVICE_STATE_NAME(dev_name)        \
        __attribute__((__section__(".z_devstate")));
 
/* Construct objects that are referenced from struct device. These
 * include power management and dependency handles.
 */
#define Z_DEVICE_DEFINE_PRE(node_id, dev_name, ...)                     \
        Z_DEVICE_DEFINE_HANDLES(node_id, dev_name, __VA_ARGS__)
 
/* Initial build provides a record that associates the device object
 * with its devicetree ordinal, and provides the dependency ordinals.
 * These are provided as weak definitions (to prevent the reference
 * from being captured when the original object file is compiled), and
 * in a distinct pass1 section (which will be replaced by
 * postprocessing).
 *
 * Before processing in gen_handles.py, the array format is:
 * {
 *     DEVICE_ORDINAL (or DEVICE_HANDLE_NULL if not a devicetree node),
 *     List of devicetree dependency ordinals (if any),
 *     DEVICE_HANDLE_SEP,
 *     List of injected dependency ordinals (if any),
 *     DEVICE_HANDLE_SEP,
 *     List of devicetree supporting ordinals (if any),
 * }
 *
 * After processing in gen_handles.py, the format is updated to:
 * {
 *     List of existing devicetree dependency handles (if any),
 *     DEVICE_HANDLE_SEP,
 *     List of injected dependency ordinals (if any),
 *     DEVICE_HANDLE_SEP,
 *     List of existing devicetree support handles (if any),
 *     DEVICE_HANDLE_NULL
 * }
 *
 * It is also (experimentally) necessary to provide explicit alignment
 * on each object. Otherwise x86-64 builds will introduce padding
 * between objects in the same input section in individual object
 * files, which will be retained in subsequent links both wasting
 * space and resulting in aggregate size changes relative to pass2
 * when all objects will be in the same input section.
 *
 * The build assert will fail if device_handle_t changes size, which
 * means the alignment directives in the linker scripts and in
 * `gen_handles.py` must be updated.
 */
BUILD_ASSERT(sizeof(device_handle_t) == 2, "fix the linker scripts");
#define Z_DEVICE_DEFINE_HANDLES(node_id, dev_name, ...)                 \
        extern Z_DEVICE_HANDLES_CONST device_handle_t                   \
                Z_DEVICE_HANDLE_NAME(node_id, dev_name)[];              \
        Z_DEVICE_HANDLES_CONST device_handle_t                          \
        __aligned(sizeof(device_handle_t))                              \
        __attribute__((__weak__,                                        \
                       __section__(".__device_handles_pass1")))         \
        Z_DEVICE_HANDLE_NAME(node_id, dev_name)[] = {                   \
        COND_CODE_1(DT_NODE_EXISTS(node_id), (                          \
                        DT_DEP_ORD(node_id),                            \
                        DT_REQUIRES_DEP_ORDS(node_id)                   \
                ), (                                                    \
                        DEVICE_HANDLE_NULL,                             \
                ))                                                      \
                        DEVICE_HANDLE_SEP,                              \
                        Z_DEVICE_EXTRA_HANDLES(__VA_ARGS__)             \
                        DEVICE_HANDLE_SEP,                              \
        COND_CODE_1(DT_NODE_EXISTS(node_id),                            \
                        (DT_SUPPORTS_DEP_ORDS(node_id)), ())            \
                };
 
#define Z_DEVICE_DEFINE_INIT(node_id, dev_name)                         \
        .handles = Z_DEVICE_HANDLE_NAME(node_id, dev_name),
 
/* Like DEVICE_DEFINE but takes a node_id AND a dev_name, and trailing
 * dependency handles that come from outside devicetree.
 */
#define Z_DEVICE_DEFINE(node_id, dev_name, drv_name, init_fn, pm_device,\
                        data_ptr, cfg_ptr, level, prio, api_ptr, state_ptr, ...)        \
        Z_DEVICE_DEFINE_PRE(node_id, dev_name, __VA_ARGS__)             \
        COND_CODE_1(DT_NODE_EXISTS(node_id), (), (static))              \
                const Z_DECL_ALIGN(struct device)                       \
                DEVICE_NAME_GET(dev_name) __used                        \
        __attribute__((__section__(".z_device_" #level STRINGIFY(prio)"_"))) = { \
                .name = drv_name,                                       \
                .config = (cfg_ptr),                                    \
                .api = (api_ptr),                                       \
                .state = (state_ptr),                                   \
                .data = (data_ptr),                                     \
                COND_CODE_1(CONFIG_PM_DEVICE, (.pm = pm_device,), ())   \
                Z_DEVICE_DEFINE_INIT(node_id, dev_name)                 \
        };                                                              \
        BUILD_ASSERT(sizeof(Z_STRINGIFY(drv_name)) <= Z_DEVICE_MAX_NAME_LEN, \
                     Z_STRINGIFY(DEVICE_NAME_GET(drv_name)) " too long"); \
        Z_INIT_ENTRY_DEFINE(DEVICE_NAME_GET(dev_name), init_fn,         \
                (&DEVICE_NAME_GET(dev_name)), level, prio)
 
#ifdef __cplusplus
}
#endif
 
/* device_extern is generated based on devicetree nodes */
#include <device_extern.h>
 
#include <syscalls/device.h>
 
#endif /* ZEPHYR_INCLUDE_DEVICE_H_ */