/*
 * Copyright (C) 2017  T+A elektroakustik GmbH & Co. KG
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

#include "dbus_handlers.hh"
#include "dbus_handlers.h"
#include "messages.h"

gboolean dbusmethod_aupath_register_player(tdbusaupathManager *object,
                                           GDBusMethodInvocation *invocation,
                                           const gchar *player_id,
                                           const gchar *player_name,
                                           const gchar *path,
                                           gpointer user_data)
{
    BUG("%s(): not implemented yet", __func__);
    return FALSE;
}

gboolean dbusmethod_aupath_register_source(tdbusaupathManager *object,
                                           GDBusMethodInvocation *invocation,
                                           const gchar *source_id,
                                           const gchar *source_name,
                                           const gchar *player_id,
                                           const gchar *path,
                                           gpointer user_data)
{
    BUG("%s(): not implemented yet", __func__);
    return FALSE;
}

gboolean dbusmethod_aupath_request_source(tdbusaupathManager *object,
                                          GDBusMethodInvocation *invocation,
                                          const gchar *source_id,
                                          gpointer user_data)
{
    BUG("%s(): not implemented yet", __func__);
    return FALSE;
}

gboolean dbusmethod_aupath_release_path(tdbusaupathManager *object,
                                        GDBusMethodInvocation *invocation,
                                        gboolean deactivate_player,
                                        gpointer user_data)
{
    BUG("%s(): not implemented yet", __func__);
    return FALSE;
}

gboolean dbusmethod_aupath_get_active_player(tdbusaupathManager *object,
                                             GDBusMethodInvocation *invocation,
                                             const gchar *source_id,
                                             gpointer user_data)
{
    BUG("%s(): not implemented yet", __func__);
    return FALSE;
}
