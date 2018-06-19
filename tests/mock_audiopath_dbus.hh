/*
 * Copyright (C) 2017, 2018  T+A elektroakustik GmbH & Co. KG
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

#ifndef MOCK_AUDIOPATH_DBUS_CC
#define MOCK_AUDIOPATH_DBUS_CC

#include "audiopath_dbus.h"
#include "gvariantwrapper.hh"
#include "mock_expectation.hh"

class MockAudiopathDBus
{
  public:
    MockAudiopathDBus(const MockAudiopathDBus &) = delete;
    MockAudiopathDBus &operator=(const MockAudiopathDBus &) = delete;

    class Expectation;
    typedef MockExpectationsTemplate<Expectation> MockExpectations;
    MockExpectations *expectations_;

    explicit MockAudiopathDBus();
    ~MockAudiopathDBus();

    void init();
    void check() const;

    void expect_tdbus_aupath_player_call_activate_sync(gboolean retval, tdbusaupathPlayer *object);
    void expect_tdbus_aupath_player_call_activate_sync(gboolean retval, tdbusaupathPlayer *object, GVariantWrapper &&request_data);
    void expect_tdbus_aupath_player_call_deactivate_sync(gboolean retval, tdbusaupathPlayer *object);
    void expect_tdbus_aupath_player_call_deactivate_sync(gboolean retval, tdbusaupathPlayer *object, GVariantWrapper &&request_data);
    void expect_tdbus_aupath_source_call_selected_on_hold_sync(gboolean retval, tdbusaupathSource *object, const gchar *arg_source_id);
    void expect_tdbus_aupath_source_call_selected_on_hold_sync(gboolean retval, tdbusaupathSource *object, const gchar *arg_source_id, GVariantWrapper &&request_data);
    void expect_tdbus_aupath_source_call_selected_sync(gboolean retval, tdbusaupathSource *object, const gchar *arg_source_id);
    void expect_tdbus_aupath_source_call_selected_sync(gboolean retval, tdbusaupathSource *object, const gchar *arg_source_id, GVariantWrapper &&request_data);
    void expect_tdbus_aupath_source_call_deselected_sync(gboolean retval, tdbusaupathSource *object, const gchar *arg_source_id);
    void expect_tdbus_aupath_source_call_deselected_sync(gboolean retval, tdbusaupathSource *object, const gchar *arg_source_id, GVariantWrapper &&request_data);
};

extern MockAudiopathDBus *mock_audiopath_dbus_singleton;

#endif /* !MOCK_AUDIOPATH_DBUS_CC */
