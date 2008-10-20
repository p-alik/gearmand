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

#ifndef __GEARMAN_RESPONSE_H__
#define __GEARMAN_RESPONSE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Public defines */
gearman_return gearman_response(gearman_server_st *ptr, 
                                gearman_result_st *result);

#ifdef __cplusplus
}
#endif

#endif /* __GEARMAN_RESPONSE_H__ */
