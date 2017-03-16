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

#include <cppcutter.h>
#include <string>

#include "audiopath.hh"
#include "audiopathswitch.hh"

#include "mock_messages.hh"
#include "mock_audiopath_dbus.hh"

/*!
 * \addtogroup audiopath_tests Unit tests
 * \ingroup audiopath
 *
 * DBus handlers unit tests.
 */
/*!@{*/

static tdbusaupathPlayer *aupath_player_proxy(const char id)
{
    return reinterpret_cast<tdbusaupathPlayer *>(0x61bff800 + id);
}

static tdbusaupathSource *aupath_source_proxy(const char id)
{
    return reinterpret_cast<tdbusaupathSource *>(0xc0154b00 + id);
}

namespace DBus
{

template <>
std::unique_ptr<AudioPath::Player::PType>
mk_proxy(const char *dest, const char *obj_path)
{
    cppcut_assert_not_null(dest);
    cppcut_assert_not_equal('\0', dest[0]);
    cppcut_assert_equal('\0', dest[1]);

    return std::unique_ptr<AudioPath::Player::PType>(
                new AudioPath::Player::PType(aupath_player_proxy(dest[0])));
}

template<>
Proxy<_tdbusaupathPlayer>::~Proxy()
{
    cppcut_assert_not_null(proxy_);
    proxy_ = nullptr;
}

template <>
std::unique_ptr<AudioPath::Source::PType>
mk_proxy(const char *dest, const char *obj_path)
{
    cppcut_assert_not_null(dest);
    cppcut_assert_not_equal('\0', dest[0]);
    cppcut_assert_equal('\0', dest[1]);

    return std::unique_ptr<AudioPath::Source::PType>(
                new AudioPath::Source::PType(aupath_source_proxy(dest[0])));
}

template<>
Proxy<_tdbusaupathSource>::~Proxy()
{
    cppcut_assert_not_null(proxy_);
    proxy_ = nullptr;
}

}

namespace audiopath_switch_tests
{

static MockMessages *mock_messages = nullptr;
static MockAudiopathDBus *mock_audiopath_dbus = nullptr;

static std::unique_ptr<AudioPath::Paths> paths;
static std::unique_ptr<AudioPath::Switch> pswitch;

void cut_setup()
{
    mock_messages = new MockMessages;
    cppcut_assert_not_null(mock_messages);
    mock_messages->init();
    mock_messages_singleton = mock_messages;

    mock_audiopath_dbus = new MockAudiopathDBus;
    cppcut_assert_not_null(mock_audiopath_dbus);
    mock_audiopath_dbus->init();
    mock_audiopath_dbus_singleton = mock_audiopath_dbus;

    paths.reset(new AudioPath::Paths);
    pswitch.reset(new AudioPath::Switch);

    paths->add_source(std::move(
        AudioPath::Source("srcA1", "Source A", "pl1",
                          DBus::mk_proxy<AudioPath::Source::PType>("A", "/dbus/sourceA"))));
    paths->add_source(std::move(
        AudioPath::Source("srcB1", "Source B", "pl1",
                          DBus::mk_proxy<AudioPath::Source::PType>("B", "/dbus/sourceB"))));
    paths->add_source(std::move(
        AudioPath::Source("srcC2", "Source C", "pl2",
                          DBus::mk_proxy<AudioPath::Source::PType>("C", "/dbus/sourceC"))));
    paths->add_source(std::move(
        AudioPath::Source("srcD-", "Source D", "player_does_not_exist",
                          DBus::mk_proxy<AudioPath::Source::PType>("D", "/dbus/sourceD"))));
    paths->add_source(std::move(
        AudioPath::Source("srcE3", "Source E", "pl3",
                          DBus::mk_proxy<AudioPath::Source::PType>("E", "/dbus/sourceE"))));

    paths->add_player(std::move(
        AudioPath::Player("pl1", "Player 1",
                          DBus::mk_proxy<AudioPath::Player::PType>("1", "/dbus/player1"))));
    paths->add_player(std::move(
        AudioPath::Player("pl2", "Player 2",
                          DBus::mk_proxy<AudioPath::Player::PType>("2", "/dbus/player2"))));
    paths->add_player(std::move(
        AudioPath::Player("pl3", "Player 3",
                          DBus::mk_proxy<AudioPath::Player::PType>("3", "/dbus/player3"))));
    paths->add_player(std::move(
        AudioPath::Player("pl-", "Unused player",
                          DBus::mk_proxy<AudioPath::Player::PType>("-", "/dbus/player-"))));

    mock_messages->ignore_messages_above(MESSAGE_LEVEL_DIAG);
}

void cut_teardown()
{
    paths.reset(nullptr);

    mock_messages->check();
    mock_audiopath_dbus->check();

    mock_messages_singleton = nullptr;
    mock_audiopath_dbus_singleton = nullptr;

    delete mock_messages;
    delete mock_audiopath_dbus;

    mock_messages = nullptr;
    mock_audiopath_dbus = nullptr;
}

/*!\test
 * Switch audio source and player.
 */
void test_switch_complete_path()
{
    cut_assert_true(pswitch->get_player_id().empty());

    const std::string *player_id;

    /* first activation */
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_activate_sync(true, aupath_player_proxy('1'));
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(true, aupath_source_proxy('A'), "srcA1");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl1", player_id->c_str());
    cppcut_assert_equal("pl1", pswitch->get_player_id().c_str());

    /* switch to other source */
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_deselected_sync(true, aupath_source_proxy('A'), "srcA1");
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_deactivate_sync(true, aupath_player_proxy('1'));
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_activate_sync(true, aupath_player_proxy('2'));
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(true, aupath_source_proxy('C'), "srcC2");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcC2", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl2", player_id->c_str());
    cppcut_assert_equal("pl2", pswitch->get_player_id().c_str());

    /* switch back to first source */
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_deselected_sync(true, aupath_source_proxy('C'), "srcC2");
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_deactivate_sync(true, aupath_player_proxy('2'));
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_activate_sync(true, aupath_player_proxy('1'));
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(true, aupath_source_proxy('A'), "srcA1");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl1", player_id->c_str());
    cppcut_assert_equal("pl1", pswitch->get_player_id().c_str());
}

/*!\test
 * Switch audio source for same player.
 */
void test_sources_for_same_player()
{
    const std::string *player_id;

    /* first activation */
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_activate_sync(true, aupath_player_proxy('1'));
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(true, aupath_source_proxy('A'), "srcA1");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl1", player_id->c_str());
    cppcut_assert_equal("pl1", pswitch->get_player_id().c_str());

    /* switch to other source */
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_deselected_sync(true, aupath_source_proxy('A'), "srcA1");
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(true, aupath_source_proxy('B'), "srcB1");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SAME),
                        static_cast<int>(pswitch->activate_source(*paths, "srcB1", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl1", player_id->c_str());
    cppcut_assert_equal("pl1", pswitch->get_player_id().c_str());

    /* switch back to first source */
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_deselected_sync(true, aupath_source_proxy('B'), "srcB1");
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(true, aupath_source_proxy('A'), "srcA1");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SAME),
                        static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl1", player_id->c_str());
    cppcut_assert_equal("pl1", pswitch->get_player_id().c_str());
}

/*!\test
 * Switch to same source twice does not cause D-Bus traffic.
 */
void test_switching_to_same_source_is_nop()
{
    const std::string *player_id;

    /* first activation */
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_activate_sync(true, aupath_player_proxy('1'));
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(true, aupath_source_proxy('A'), "srcA1");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl1", player_id->c_str());
    cppcut_assert_equal("pl1", pswitch->get_player_id().c_str());
    mock_audiopath_dbus->check();

    /* second activation */
    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::OK_UNCHANGED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl1", player_id->c_str());
    cppcut_assert_equal("pl1", pswitch->get_player_id().c_str());
}

/*!\test
 * Attempting to switch to nonexistent source does not shutdown current source.
 */
void test_switching_to_nonexistent_source_keeps_current_source()
{
    const std::string *player_id;

    /* first activation */
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_activate_sync(true, aupath_player_proxy('2'));
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(true, aupath_source_proxy('C'), "srcC2");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcC2", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl2", player_id->c_str());
    cppcut_assert_equal("pl2", pswitch->get_player_id().c_str());

    /* failed activation */
    mock_messages->expect_msg_error_formatted(0, LOG_NOTICE,
            "AUDIO SOURCE SWITCH: Unknown audio source srcD2");
    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::ERROR_SOURCE_UNKNOWN),
                        static_cast<int>(pswitch->activate_source(*paths, "srcD2", player_id)));

    cppcut_assert_null(player_id);
    cppcut_assert_equal("pl2", pswitch->get_player_id().c_str());
    mock_audiopath_dbus->check();

    /* other activation */
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_deselected_sync(true, aupath_source_proxy('C'), "srcC2");
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_deactivate_sync(true, aupath_player_proxy('2'));
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_activate_sync(true, aupath_player_proxy('1'));
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(true, aupath_source_proxy('A'), "srcA1");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl1", player_id->c_str());
    cppcut_assert_equal("pl1", pswitch->get_player_id().c_str());
}

/*!\test
 * Attempting to switch to source with no registered player shuts down path.
 */
void test_switching_to_existent_source_without_player_keeps_current_source()
{
    const std::string *player_id;

    /* first activation */
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_activate_sync(true, aupath_player_proxy('2'));
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(true, aupath_source_proxy('C'), "srcC2");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcC2", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl2", player_id->c_str());
    cppcut_assert_equal("pl2", pswitch->get_player_id().c_str());

    /* failed activation */
    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::ERROR_PLAYER_UNKNOWN),
                        static_cast<int>(pswitch->activate_source(*paths, "srcD-", player_id)));

    cppcut_assert_null(player_id);
    cppcut_assert_equal("pl2", pswitch->get_player_id().c_str());
    mock_audiopath_dbus->check();

    /* other activation */
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_deselected_sync(true, aupath_source_proxy('C'), "srcC2");
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_deactivate_sync(true, aupath_player_proxy('2'));
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_activate_sync(true, aupath_player_proxy('1'));
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(true, aupath_source_proxy('A'), "srcA1");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl1", player_id->c_str());
    cppcut_assert_equal("pl1", pswitch->get_player_id().c_str());
}

/*!\test
 * Audio path is killed if source selection fails. Player remains activate.
 */
void test_switch_to_failing_source_for_same_player_kills_audio_path_and_keeps_player_active()
{
    const std::string *player_id;

    /* first activation */
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_activate_sync(true, aupath_player_proxy('1'));
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(true, aupath_source_proxy('B'), "srcB1");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcB1", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl1", player_id->c_str());
    cppcut_assert_equal("pl1", pswitch->get_player_id().c_str());

    /* second activation fails */
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_deselected_sync(true, aupath_source_proxy('B'), "srcB1");
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(false, aupath_source_proxy('A'), "srcA1");
    mock_messages->expect_msg_error_formatted(0, LOG_EMERG,
            "Select source: Got D-Bus error: Mock source A selection failure");
    mock_messages->expect_msg_error_formatted(0, LOG_ERR,
            "AUDIO SOURCE SWITCH: Selecting audio source srcA1 failed");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::ERROR_SOURCE_FAILED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl1", player_id->c_str());
    cppcut_assert_equal("pl1", pswitch->get_player_id().c_str());
    mock_audiopath_dbus->check();
    mock_messages->check();

    /* prove that expected player was still active */
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_deactivate_sync(true, aupath_player_proxy('1'));
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_activate_sync(true, aupath_player_proxy('3'));
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(true, aupath_source_proxy('E'), "srcE3");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcE3", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl3", player_id->c_str());
    cppcut_assert_equal("pl3", pswitch->get_player_id().c_str());
}

/*!\test
 * Audio path is killed if source selection fails. Player remains activate.
 */
void test_switch_to_failing_source_for_other_player_kills_audio_path_and_keeps_new_player_active()
{
    const std::string *player_id;

    /* first activation */
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_activate_sync(true, aupath_player_proxy('1'));
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(true, aupath_source_proxy('B'), "srcB1");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcB1", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl1", player_id->c_str());
    cppcut_assert_equal("pl1", pswitch->get_player_id().c_str());

    /* second activation fails */
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_deselected_sync(true, aupath_source_proxy('B'), "srcB1");
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_deactivate_sync(true, aupath_player_proxy('1'));
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_activate_sync(true, aupath_player_proxy('2'));
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(false, aupath_source_proxy('C'), "srcC2");
    mock_messages->expect_msg_error_formatted(0, LOG_EMERG,
            "Select source: Got D-Bus error: Mock source C selection failure");
    mock_messages->expect_msg_error_formatted(0, LOG_ERR,
            "AUDIO SOURCE SWITCH: Selecting audio source srcC2 failed");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::ERROR_SOURCE_FAILED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcC2", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl2", player_id->c_str());
    cppcut_assert_equal("pl2", pswitch->get_player_id().c_str());
    mock_audiopath_dbus->check();
    mock_messages->check();

    /* prove that expected player was still active */
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_deactivate_sync(true, aupath_player_proxy('2'));
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_activate_sync(true, aupath_player_proxy('3'));
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(true, aupath_source_proxy('E'), "srcE3");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcE3", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl3", player_id->c_str());
    cppcut_assert_equal("pl3", pswitch->get_player_id().c_str());
}

/*!\test
 * Audio path is killed completely if player activation fails.
 */
void test_switch_to_failing_player_kills_audio_path_completely()
{
    const std::string *player_id;

    /* first activation */
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_activate_sync(true, aupath_player_proxy('1'));
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(true, aupath_source_proxy('B'), "srcB1");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcB1", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl1", player_id->c_str());
    cppcut_assert_equal("pl1", pswitch->get_player_id().c_str());

    /* second activation fails */
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_deselected_sync(true, aupath_source_proxy('B'), "srcB1");
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_deactivate_sync(true, aupath_player_proxy('1'));
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_activate_sync(false, aupath_player_proxy('2'));
    mock_messages->expect_msg_error_formatted(0, LOG_EMERG,
            "Activate player: Got D-Bus error: Mock player 2 activation failure");
    mock_messages->expect_msg_error_formatted(0, LOG_ERR,
            "AUDIO SOURCE SWITCH: Activating player pl2 failed");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::ERROR_PLAYER_FAILED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcC2", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl2", player_id->c_str());
    cut_assert_true(pswitch->get_player_id().empty());
    mock_audiopath_dbus->check();
    mock_messages->check();

    /* prove that no player was active */
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_activate_sync(true, aupath_player_proxy('3'));
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(true, aupath_source_proxy('E'), "srcE3");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcE3", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl3", player_id->c_str());
    cppcut_assert_equal("pl3", pswitch->get_player_id().c_str());
}

/*!\test
 * The source ID must not be empty.
 */
void test_switch_to_nameless_source_is_rejected()
{
    const std::string *player_id;

    mock_messages->expect_msg_error_formatted(EINVAL, LOG_ERR,
            "AUDIO SOURCE SWITCH: Empty audio source ID (Invalid argument)");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::ERROR_SOURCE_UNKNOWN),
                        static_cast<int>(pswitch->activate_source(*paths, "", player_id)));

    cppcut_assert_null(player_id);
    cut_assert_true(pswitch->get_player_id().empty());
}

/*!\test
 * Audio path can be taken down completely if requested.
 */
void test_releasing_path_deselects_source_and_can_deactivate_player()
{
    const std::string *player_id;

    mock_audiopath_dbus->expect_tdbus_aupath_player_call_activate_sync(true, aupath_player_proxy('1'));
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(true, aupath_source_proxy('A'), "srcA1");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl1", player_id->c_str());
    cppcut_assert_equal("pl1", pswitch->get_player_id().c_str());
    mock_audiopath_dbus->check();

    mock_audiopath_dbus->expect_tdbus_aupath_source_call_deselected_sync(true, aupath_source_proxy('A'), "srcA1");
    mock_audiopath_dbus->expect_tdbus_aupath_player_call_deactivate_sync(true, aupath_player_proxy('1'));

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ReleaseResult::COMPLETE_RELEASE),
                        static_cast<int>((pswitch->release_path(*paths, true, player_id))));

    cppcut_assert_null(player_id);
    cut_assert_true(pswitch->get_player_id().empty());
}

/*!\test
 * Audio path can be taken down, but keep the player active if requested.
 */
void test_releasing_path_deselects_source_and_can_keep_player_active()
{
    const std::string *player_id;

    mock_audiopath_dbus->expect_tdbus_aupath_player_call_activate_sync(true, aupath_player_proxy('1'));
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(true, aupath_source_proxy('A'), "srcA1");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl1", player_id->c_str());
    cppcut_assert_equal("pl1", pswitch->get_player_id().c_str());
    mock_audiopath_dbus->check();

    mock_audiopath_dbus->expect_tdbus_aupath_source_call_deselected_sync(true, aupath_source_proxy('A'), "srcA1");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ReleaseResult::SOURCE_DESELECTED),
                        static_cast<int>(pswitch->release_path(*paths, false, player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl1", player_id->c_str());
    cppcut_assert_equal("pl1", pswitch->get_player_id().c_str());
}

/*!\test
 * Releasing a nonactive audio path is not an error, but it has no effect either.
 */
void test_releasing_nonactive_path_with_player_deactivation_request_is_nop()
{
    const std::string *player_id;

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ReleaseResult::UNCHANGED),
                        static_cast<int>(pswitch->release_path(*paths, false, player_id)));

    cppcut_assert_null(player_id);
    cut_assert_true(pswitch->get_player_id().empty());
}

/*!\test
 * Releasing a nonactive audio path is not an error, but it has no effect either.
 */
void test_releasing_nonactive_path_without_player_deactivation_request_is_nop()
{
    const std::string *player_id;

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ReleaseResult::UNCHANGED),
                        static_cast<int>(pswitch->release_path(*paths, true, player_id)));

    cppcut_assert_null(player_id);
    cut_assert_true(pswitch->get_player_id().empty());
}

/*!\test
 * Releasing a defunct audio path with an active player may take down the
 * player if requested.
 */
void test_releasing_nonactive_path_with_active_player_can_deactivate_player()
{
    const std::string *player_id;

    mock_audiopath_dbus->expect_tdbus_aupath_player_call_activate_sync(true, aupath_player_proxy('2'));
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(false, aupath_source_proxy('C'), "srcC2");
    mock_messages->expect_msg_error_formatted(0, LOG_EMERG,
            "Select source: Got D-Bus error: Mock source C selection failure");
    mock_messages->expect_msg_error_formatted(0, LOG_ERR,
            "AUDIO SOURCE SWITCH: Selecting audio source srcC2 failed");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::ERROR_SOURCE_FAILED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcC2", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl2", player_id->c_str());
    cppcut_assert_equal("pl2", pswitch->get_player_id().c_str());
    mock_audiopath_dbus->check();
    mock_messages->check();

    mock_audiopath_dbus->expect_tdbus_aupath_player_call_deactivate_sync(true, aupath_player_proxy('2'));

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ReleaseResult::PLAYER_DEACTIVATED),
                        static_cast<int>(pswitch->release_path(*paths, true, player_id)));

    cppcut_assert_null(player_id);
    cut_assert_true(pswitch->get_player_id().empty());
}

/*!\test
 * Releasing a defunct audio path with an active player may keep the player
 * active if requested.
 */
void test_releasing_nonactive_path_with_active_player_can_keep_player_active()
{
    const std::string *player_id;

    mock_audiopath_dbus->expect_tdbus_aupath_player_call_activate_sync(true, aupath_player_proxy('2'));
    mock_audiopath_dbus->expect_tdbus_aupath_source_call_selected_sync(false, aupath_source_proxy('C'), "srcC2");
    mock_messages->expect_msg_error_formatted(0, LOG_EMERG,
            "Select source: Got D-Bus error: Mock source C selection failure");
    mock_messages->expect_msg_error_formatted(0, LOG_ERR,
            "AUDIO SOURCE SWITCH: Selecting audio source srcC2 failed");

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ActivateResult::ERROR_SOURCE_FAILED),
                        static_cast<int>(pswitch->activate_source(*paths, "srcC2", player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl2", player_id->c_str());
    cppcut_assert_equal("pl2", pswitch->get_player_id().c_str());
    mock_audiopath_dbus->check();
    mock_messages->check();

    cppcut_assert_equal(static_cast<int>(AudioPath::Switch::ReleaseResult::UNCHANGED),
                        static_cast<int>(pswitch->release_path(*paths, false, player_id)));

    cppcut_assert_not_null(player_id);
    cppcut_assert_equal("pl2", player_id->c_str());
    cppcut_assert_equal("pl2", pswitch->get_player_id().c_str());
}

}

/*!@}*/
