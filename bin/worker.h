/* - mode: c; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Copyright (C) 2011 Data Differential
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

class Worker
{
public:
  Worker()
  {
    if (gearman_worker_create(&_worker) == NULL)
    {
      std::cerr << "Failed memory allocation while initializing memory." << std::endl;
      abort();
    }
  }

  ~Worker()
  {
    gearman_worker_free(&_worker);
  }

  gearman_worker_st &worker()
  {
    return _worker;
  }

private:
  gearman_worker_st _worker;
};
