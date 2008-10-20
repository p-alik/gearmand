/* Gearman server and library
 * Copyright (C) 2008 Brian Aker
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __GEARMAN_BEHAVIOR_H__
#define __GEARMAN_BEHAVIOR_H__

#ifdef __cplusplus
extern "C" {
#endif

gearman_return gearman_behavior_set(gearman_st *ptr, gearman_behavior flag, 
                                    uint64_t data);

uint64_t gearman_behavior_get(gearman_st *ptr, gearman_behavior flag);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_BEHAVIOR_H__ */
