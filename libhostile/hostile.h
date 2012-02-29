/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  libhostile
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if defined(HAVE_LIBHOSTILE) && HAVE_LIBHOSTILE
void set_recv_close(bool arg, int frequency, int not_until_arg);

void set_send_close(bool arg, int frequency, int not_until_arg);
#else

#define set_recv_close(__arg, __frequency, __not_until_arg)
#define set_send_close(__arg, __frequency, __not_until_arg)

#endif

#ifdef __cplusplus
}
#endif
