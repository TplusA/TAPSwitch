/*
 * Copyright (C) 2017, 2020  T+A elektroakustik GmbH & Co. KG
 *
 * This file is part of TAPSwitch.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#ifndef DBUS_PROXY_WRAPPER_HH
#define DBUS_PROXY_WRAPPER_HH

#include <memory>

struct _GObject;
struct _GAsyncResult;

namespace DBus
{

/*!
 * GLib D-Bus proxy wrapped into a class for ressource management.
 */
template <typename T>
class Proxy
{
  private:
    T *proxy_;

  public:
    Proxy(const Proxy &) = delete;
    Proxy &operator=(const Proxy &) = delete;

    explicit Proxy(T *proxy):
        proxy_(proxy)
    {}

    ~Proxy();

    const T *get() const { return proxy_; }
    T *get() { return proxy_; }

    T *get_as_nonconst() const { return proxy_; }
};

template <typename T>
void mk_proxy_async(const char *dest, const char *obj_path,
                    std::function<void(std::unique_ptr<T>)> *proxy_done_cb);

template <typename T>
void mk_proxy_done(struct _GObject *source_object, struct _GAsyncResult *res,
                   void *user_data);

}

#endif /* !DBUS_PROXY_WRAPPER_HH */
