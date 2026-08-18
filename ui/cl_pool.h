/*
 * cl_pool.h
 *
 * The backdrop gradient of node 1:3, as a full-size alpha map built at
 * start-up from the quarter-scale asset in icons/.  See cl_pool.c for why it
 * is done this way rather than with shadows or a zoomed image.
 */

#ifndef CL_POOL_H
#define CL_POOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "cl_icons.h"
#include "cl_theme.h"

/** The full-size map.  Not valid until cl_pool_build() has run. */
extern lv_image_dsc_t cl_pool;

/** Expand the quarter-scale asset into it.  Called by cl_screen_create(). */
void cl_pool_build(void);

#ifdef __cplusplus
}
#endif

#endif /* CL_POOL_H */
