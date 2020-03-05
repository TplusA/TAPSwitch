/*
 * Copyright (C) 2017, 2018, 2020  T+A elektroakustik GmbH & Co. KG
 *
 * This file is part of TAPSwitch.
 *
 * TAPSwitch is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 3 as
 * published by the Free Software Foundation.
 *
 * TAPSwitch is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with TAPSwitch.  If not, see <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <doctest.h>

#include "mock_audiopath_dbus.hh"

MockAudiopathDBus::Mock *MockAudiopathDBus::singleton = nullptr;


gboolean tdbus_aupath_player_call_activate_sync(tdbusaupathPlayer *proxy, GVariant *arg_request_data, GCancellable *cancellable, GError **error)
{
    return
        MockAudiopathDBus::singleton->check_next<MockAudiopathDBus::PlayerActivateSync>(
            proxy, arg_request_data, cancellable, error);
}

gboolean tdbus_aupath_player_call_deactivate_sync(tdbusaupathPlayer *proxy, GVariant *arg_request_data, GCancellable *cancellable, GError **error)
{
    return
        MockAudiopathDBus::singleton->check_next<MockAudiopathDBus::PlayerDeactivateSync>(
            proxy, arg_request_data, cancellable, error);
}

gboolean tdbus_aupath_source_call_selected_on_hold_sync(tdbusaupathSource *proxy, const gchar *arg_source_id, GVariant *arg_request_data, GCancellable *cancellable, GError **error)
{
    return
        MockAudiopathDBus::singleton->check_next<MockAudiopathDBus::SourceSelectedOnHoldSync>(
            proxy, arg_source_id, arg_request_data, cancellable, error);
}

gboolean tdbus_aupath_source_call_selected_sync(tdbusaupathSource *proxy, const gchar *arg_source_id, GVariant *arg_request_data, GCancellable *cancellable, GError **error)
{
    return
        MockAudiopathDBus::singleton->check_next<MockAudiopathDBus::SourceSelectedSync>(
            proxy, arg_source_id, arg_request_data, cancellable, error);
}

gboolean tdbus_aupath_source_call_deselected_sync(tdbusaupathSource *proxy, const gchar *arg_source_id, GVariant *arg_request_data, GCancellable *cancellable, GError **error)
{
    return
        MockAudiopathDBus::singleton->check_next<MockAudiopathDBus::SourceDeselectedSync>(
            proxy, arg_source_id, arg_request_data, cancellable, error);
}
