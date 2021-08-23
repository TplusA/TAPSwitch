/*
 * Copyright (C) 2017, 2018, 2020, 2021  T+A elektroakustik GmbH & Co. KG
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

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <doctest.h>

#include <glib.h>

#include "audiopath.hh"
#include "audiopathswitch.hh"
#include "appliance.hh"

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

template <typename T>
std::unique_ptr<T> mk_proxy(const char *dest, const char *obj_path);

template <>
std::unique_ptr<AudioPath::Player::PType>
mk_proxy(const char *dest, const char *obj_path)
{
    REQUIRE(dest != nullptr);
    REQUIRE(dest[0] != '\0');
    REQUIRE(dest[1] == '\0');

    return std::unique_ptr<AudioPath::Player::PType>(
                new AudioPath::Player::PType(aupath_player_proxy(dest[0])));
}

template<>
Proxy<_tdbusaupathPlayer>::~Proxy()
{
    CHECK(proxy_ != nullptr);
    proxy_ = nullptr;
}

template <>
std::unique_ptr<AudioPath::Source::PType>
mk_proxy(const char *dest, const char *obj_path)
{
    REQUIRE(dest != nullptr);
    REQUIRE(dest[0] != '\0');
    REQUIRE(dest[1] == '\0');

    return std::unique_ptr<AudioPath::Source::PType>(
                new AudioPath::Source::PType(aupath_source_proxy(dest[0])));
}

template<>
Proxy<_tdbusaupathSource>::~Proxy()
{
    CHECK(proxy_ != nullptr);
    proxy_ = nullptr;
}

}

TEST_SUITE_BEGIN("Audio path switching");

class Fixture
{
  protected:
    std::unique_ptr<MockMessages::Mock> mock_messages;
    std::unique_ptr<MockAudiopathDBus::Mock> mock_audiopath_dbus;

    std::unique_ptr<AudioPath::Paths> paths;
    std::unique_ptr<AudioPath::Switch> pswitch;

  public:
    explicit Fixture():
        mock_messages(std::make_unique<MockMessages::Mock>()),
        mock_audiopath_dbus(std::make_unique<MockAudiopathDBus::Mock>()),
        paths(std::make_unique<AudioPath::Paths>()),
        pswitch(std::make_unique<AudioPath::Switch>())
    {
        MockMessages::singleton = mock_messages.get();
        MockAudiopathDBus::singleton = mock_audiopath_dbus.get();

        paths->add_source(AudioPath::Source(
                "srcA1", "Source A", "pl1",
                DBus::mk_proxy<AudioPath::Source::PType>("A", "/dbus/sourceA")));
        paths->add_source(AudioPath::Source(
                "srcB1", "Source B", "pl1",
                DBus::mk_proxy<AudioPath::Source::PType>("B", "/dbus/sourceB")));
        paths->add_source(AudioPath::Source(
                "srcC2", "Source C", "pl2",
                DBus::mk_proxy<AudioPath::Source::PType>("C", "/dbus/sourceC")));
        paths->add_source(AudioPath::Source(
                "srcD-", "Source D", "player_does_not_exist",
                DBus::mk_proxy<AudioPath::Source::PType>("D", "/dbus/sourceD")));
        paths->add_source(AudioPath::Source(
                "srcE3", "Source E", "pl3",
                DBus::mk_proxy<AudioPath::Source::PType>("E", "/dbus/sourceE")));

        paths->add_player(AudioPath::Player(
                "pl1", "Player 1",
                DBus::mk_proxy<AudioPath::Player::PType>("1", "/dbus/player1")));
        paths->add_player(AudioPath::Player(
                "pl2", "Player 2",
                DBus::mk_proxy<AudioPath::Player::PType>("2", "/dbus/player2")));
        paths->add_player(AudioPath::Player(
                "pl3", "Player 3",
                DBus::mk_proxy<AudioPath::Player::PType>("3", "/dbus/player3")));
        paths->add_player(AudioPath::Player(
                "pl-", "Unused player",
                DBus::mk_proxy<AudioPath::Player::PType>("-", "/dbus/player-")));

        mock_messages->ignore_messages_above(MESSAGE_LEVEL_DIAG);
    }

    virtual ~Fixture()
    {
        try
        {
            mock_messages->done();
            mock_audiopath_dbus->done();
        }
        catch(...)
        {
            /* no throwing from dtors */
        }

        MockMessages::singleton = nullptr;
        MockAudiopathDBus::singleton = nullptr;
    }
};

/*!\test
 * Switch audio source and player.
 */
TEST_CASE_FIXTURE(Fixture, "Switch complete path")
{
    CHECK(pswitch->get_player_id().empty());

    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    /* first activation */
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('A'), "srcA1");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl1");

    /* switch to other source */
    expect<MockAudiopathDBus::SourceDeselectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('A'), "srcA1");
    expect<MockAudiopathDBus::PlayerDeactivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('2'));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('C'), "srcC2");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcC2", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl2");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::DESELECTED_ACTIVE);
    CHECK(pswitch->get_player_id() == "pl2");

    /* switch back to first source */
    expect<MockAudiopathDBus::SourceDeselectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('C'), "srcC2");
    expect<MockAudiopathDBus::PlayerDeactivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('2'));
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('A'), "srcA1");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::DESELECTED_ACTIVE);
    CHECK(pswitch->get_player_id() == "pl1");
}

/*!\test
 * Switch audio source and player, pass extra data on each request.
 */
TEST_CASE_FIXTURE(Fixture, "Switch complete path with request data")
{
    CHECK(pswitch->get_player_id().empty());

    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    /* first activation */
    GVariantDict dict;
    g_variant_dict_init(&dict, nullptr);
    g_variant_dict_insert_value(&dict, "foo", g_variant_new_string("bar"));
    g_variant_dict_insert_value(&dict, "my", g_variant_new_string("data"));
    auto request_data(GVariantWrapper(g_variant_dict_end(&dict)));

    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'), std::move(GVariantWrapper(request_data)));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('A'), "srcA1", GVariantWrapper(request_data));

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id, deselected_result, true, std::move(request_data))) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl1");

    /* switch to other source */
    g_variant_dict_init(&dict, nullptr);
    g_variant_dict_insert_value(&dict, "bar", g_variant_new_string("foo"));
    g_variant_dict_insert_value(&dict, "dadada", g_variant_new_string("dadata"));
    request_data = std::move(GVariantWrapper(g_variant_dict_end(&dict)));

    expect<MockAudiopathDBus::SourceDeselectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('A'), "srcA1", GVariantWrapper(request_data));
    expect<MockAudiopathDBus::PlayerDeactivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'), GVariantWrapper(request_data));
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('2'), GVariantWrapper(request_data));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('C'), "srcC2", GVariantWrapper(request_data));

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcC2", player_id, deselected_result, true, std::move(request_data))) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl2");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::DESELECTED_ACTIVE);
    CHECK(pswitch->get_player_id() == "pl2");

    /* switch back to first source */
    g_variant_dict_init(&dict, nullptr);
    g_variant_dict_insert_value(&dict, "foo", g_variant_new_string("qux"));
    request_data = std::move(GVariantWrapper(g_variant_dict_end(&dict)));

    expect<MockAudiopathDBus::SourceDeselectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('C'), "srcC2", GVariantWrapper(request_data));
    expect<MockAudiopathDBus::PlayerDeactivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('2'), GVariantWrapper(request_data));
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'), GVariantWrapper(request_data));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('A'), "srcA1", GVariantWrapper(request_data));

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id, deselected_result, true, std::move(request_data))) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::DESELECTED_ACTIVE);
    CHECK(pswitch->get_player_id() == "pl1");
}

/*!\test
 * Switch audio source for same player.
 */
TEST_CASE_FIXTURE(Fixture, "Sources for same player")
{
    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    /* first activation */
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('A'), "srcA1");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl1");

    /* switch to other source */
    expect<MockAudiopathDBus::SourceDeselectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('A'), "srcA1");
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('B'), "srcB1");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcB1", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SAME));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::DESELECTED_ACTIVE);
    CHECK(pswitch->get_player_id() == "pl1");

    /* switch back to first source */
    expect<MockAudiopathDBus::SourceDeselectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('B'), "srcB1");
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('A'), "srcA1");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SAME));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::DESELECTED_ACTIVE);
    CHECK(pswitch->get_player_id() == "pl1");
}

/*!\test
 * Switch to same source twice does not cause D-Bus traffic.
 */
TEST_CASE_FIXTURE(Fixture, "Switching to same source is NOP")
{
    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    /* first activation */
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('A'), "srcA1");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl1");
    mock_audiopath_dbus->done();

    /* second activation */
    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_UNCHANGED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl1");
}

/*!\test
 * Attempting to switch to nonexistent source does not shutdown current source.
 */
TEST_CASE_FIXTURE(Fixture, "Switching to nonexistent source keeps current source")
{
    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    /* first activation */
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('2'));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('C'), "srcC2");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcC2", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl2");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl2");

    /* failed activation */
    expect<MockMessages::MsgError>(mock_messages, 0, LOG_NOTICE,
            "AUDIO SOURCE SWITCH: Unknown audio source srcD2", false);
    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcD2", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::ERROR_SOURCE_UNKNOWN));

    CHECK(player_id == nullptr);
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl2");
    mock_audiopath_dbus->done();

    /* other activation */
    expect<MockAudiopathDBus::SourceDeselectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('C'), "srcC2");
    expect<MockAudiopathDBus::PlayerDeactivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('2'));
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('A'), "srcA1");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::DESELECTED_ACTIVE);
    CHECK(pswitch->get_player_id() == "pl1");
}

/*!\test
 * Attempting to switch to source with no registered player shuts down path.
 */
TEST_CASE_FIXTURE(Fixture, "Switching to existent source without player keeps current source")
{
    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    /* first activation */
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('2'));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('C'), "srcC2");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcC2", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl2");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl2");

    /* failed activation */
    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcD-", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::ERROR_PLAYER_UNKNOWN));

    CHECK(player_id == nullptr);
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl2");
    mock_audiopath_dbus->done();

    /* other activation */
    expect<MockAudiopathDBus::SourceDeselectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('C'), "srcC2");
    expect<MockAudiopathDBus::PlayerDeactivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('2'));
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('A'), "srcA1");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::DESELECTED_ACTIVE);
    CHECK(pswitch->get_player_id() == "pl1");
}

/*!\test
 * Audio path is killed if source selection fails. Player remains activate.
 */
TEST_CASE_FIXTURE(Fixture, "Switch to failing source for same player kills audio path and keeps player active")
{
    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    /* first activation */
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('B'), "srcB1");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcB1", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl1");

    /* second activation fails */
    expect<MockAudiopathDBus::SourceDeselectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('B'), "srcB1");
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, false, aupath_source_proxy('A'), "srcA1");
    expect<MockMessages::MsgError>(mock_messages, 0, LOG_EMERG,
            "Select source: Got g-io-error-quark error: Mock source A selection failure",
            false);
    expect<MockMessages::MsgError>(mock_messages, 0, LOG_ERR,
            "AUDIO SOURCE SWITCH: Selecting audio source srcA1 failed", false);

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::ERROR_SOURCE_FAILED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::DESELECTED_ACTIVE);
    CHECK(pswitch->get_player_id() == "pl1");
    mock_audiopath_dbus->done();
    mock_messages->done();

    /* prove that expected player was still active */
    expect<MockAudiopathDBus::PlayerDeactivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('3'));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('E'), "srcE3");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcE3", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl3");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl3");
}

/*!\test
 * Audio path is killed if source selection fails. Player remains activate.
 */
TEST_CASE_FIXTURE(Fixture, "Switch to failing source for other player kills audio path and keeps new player active")
{
    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    /* first activation */
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('B'), "srcB1");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcB1", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl1");

    /* second activation fails */
    expect<MockAudiopathDBus::SourceDeselectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('B'), "srcB1");
    expect<MockAudiopathDBus::PlayerDeactivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('2'));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, false, aupath_source_proxy('C'), "srcC2");
    expect<MockMessages::MsgError>(mock_messages, 0, LOG_EMERG,
            "Select source: Got g-io-error-quark error: Mock source C selection failure",
            false);
    expect<MockMessages::MsgError>(mock_messages, 0, LOG_ERR,
            "AUDIO SOURCE SWITCH: Selecting audio source srcC2 failed", false);

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcC2", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::ERROR_SOURCE_FAILED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl2");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::DESELECTED_ACTIVE);
    CHECK(pswitch->get_player_id() == "pl2");
    mock_audiopath_dbus->done();
    mock_messages->done();

    /* prove that expected player was still active */
    expect<MockAudiopathDBus::PlayerDeactivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('2'));
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('3'));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('E'), "srcE3");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcE3", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl3");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl3");
}

/*!\test
 * Audio path is killed completely if player activation fails.
 */
TEST_CASE_FIXTURE(Fixture, "Switch to failing player kills audio path completely")
{
    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    /* first activation */
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('B'), "srcB1");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcB1", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl1");

    /* second activation fails */
    expect<MockAudiopathDBus::SourceDeselectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('B'), "srcB1");
    expect<MockAudiopathDBus::PlayerDeactivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, false, aupath_player_proxy('2'));
    expect<MockMessages::MsgError>(mock_messages, 0, LOG_EMERG,
            "Activate player: Got g-io-error-quark error: Mock player 2 activation failure",
            false);
    expect<MockMessages::MsgError>(mock_messages, 0, LOG_ERR,
            "AUDIO SOURCE SWITCH: Activating player pl2 failed", false);

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcC2", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::ERROR_PLAYER_FAILED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl2");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::DESELECTED_ACTIVE);
    CHECK(pswitch->get_player_id().empty());
    mock_audiopath_dbus->done();
    mock_messages->done();

    /* prove that no player was active */
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('3'));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('E'), "srcE3");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcE3", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl3");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl3");
}

/*!\test
 * The source ID must not be empty.
 */
TEST_CASE_FIXTURE(Fixture, "Switch to nameless source is rejected")
{
    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
            "AUDIO SOURCE SWITCH: Empty audio source ID (Invalid argument)", false);

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::ERROR_SOURCE_UNKNOWN));

    CHECK(player_id == nullptr);
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id().empty());
}

/*!\test
 * Audio path can be taken down completely if requested.
 */
TEST_CASE_FIXTURE(Fixture, "Releasing path deselects source and can deactivate player")
{
    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('A'), "srcA1");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl1");
    mock_audiopath_dbus->done();

    expect<MockAudiopathDBus::SourceDeselectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('A'), "srcA1");
    expect<MockAudiopathDBus::PlayerDeactivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));

    CHECK(static_cast<int>((pswitch->release_path(*paths, true, player_id, deselected_result))) ==
          static_cast<int>(AudioPath::Switch::ReleaseResult::COMPLETE_RELEASE));

    CHECK(player_id == nullptr);
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::DESELECTED_ACTIVE);
    CHECK(pswitch->get_player_id().empty());
}

/*!\test
 * Audio path can be taken down, but keep the player active if requested.
 */
TEST_CASE_FIXTURE(Fixture, "Releasing path deselects source and can keep player active")
{
    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('A'), "srcA1");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcA1", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl1");
    mock_audiopath_dbus->done();

    expect<MockAudiopathDBus::SourceDeselectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('A'), "srcA1");

    CHECK(static_cast<int>(pswitch->release_path(*paths, false, player_id, deselected_result)) ==
          static_cast<int>(AudioPath::Switch::ReleaseResult::SOURCE_DESELECTED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::DESELECTED_ACTIVE);
    CHECK(pswitch->get_player_id() == "pl1");
}

/*!\test
 * Releasing a nonactive audio path is not an error, but it has no effect either.
 */
TEST_CASE_FIXTURE(Fixture, "Releasing nonactive path with player deactivation request is nop")
{
    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    CHECK(static_cast<int>(pswitch->release_path(*paths, false, player_id, deselected_result)) ==
          static_cast<int>(AudioPath::Switch::ReleaseResult::UNCHANGED));

    CHECK(player_id == nullptr);
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id().empty());
}

/*!\test
 * Releasing a nonactive audio path is not an error, but it has no effect either.
 */
TEST_CASE_FIXTURE(Fixture, "Releasing nonactive path without player deactivation request is nop")
{
    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    CHECK(static_cast<int>(pswitch->release_path(*paths, true, player_id, deselected_result)) ==
          static_cast<int>(AudioPath::Switch::ReleaseResult::UNCHANGED));

    CHECK(player_id == nullptr);
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id().empty());
}

/*!\test
 * Releasing a defunct audio path with an active player may take down the
 * player if requested.
 */
TEST_CASE_FIXTURE(Fixture, "Releasing nonactive path with active player can deactivate player")
{
    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('2'));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, false, aupath_source_proxy('C'), "srcC2");
    expect<MockMessages::MsgError>(mock_messages, 0, LOG_EMERG,
            "Select source: Got g-io-error-quark error: Mock source C selection failure",
            false);
    expect<MockMessages::MsgError>(mock_messages, 0, LOG_ERR,
            "AUDIO SOURCE SWITCH: Selecting audio source srcC2 failed", false);

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcC2", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::ERROR_SOURCE_FAILED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl2");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl2");
    mock_audiopath_dbus->done();
    mock_messages->done();

    expect<MockAudiopathDBus::PlayerDeactivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('2'));

    CHECK(static_cast<int>(pswitch->release_path(*paths, true, player_id, deselected_result)) ==
          static_cast<int>(AudioPath::Switch::ReleaseResult::PLAYER_DEACTIVATED));

    CHECK(player_id == nullptr);
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id().empty());
}

/*!\test
 * Releasing a defunct audio path with an active player may keep the player
 * active if requested.
 */
TEST_CASE_FIXTURE(Fixture, "Releasing nonactive path with active player can keep player active")
{
    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('2'));
    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, false, aupath_source_proxy('C'), "srcC2");
    expect<MockMessages::MsgError>(mock_messages, 0, LOG_EMERG,
            "Select source: Got g-io-error-quark error: Mock source C selection failure",
            false);
    expect<MockMessages::MsgError>(mock_messages, 0, LOG_ERR,
            "AUDIO SOURCE SWITCH: Selecting audio source srcC2 failed", false);

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcC2", player_id, deselected_result, true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::ERROR_SOURCE_FAILED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl2");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl2");
    mock_audiopath_dbus->done();
    mock_messages->done();

    CHECK(static_cast<int>(pswitch->release_path(*paths, false, player_id, deselected_result)) ==
          static_cast<int>(AudioPath::Switch::ReleaseResult::UNCHANGED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl2");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl2");
}

/*!\test
 * Audio source activation is deferred until appliance tells us it is ready.
 */
TEST_CASE_FIXTURE(Fixture, "Switch path while appliance is not ready")
{
    AudioPath::Appliance appliance;
    CHECK(appliance.set_audio_path_blocked());

    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    /* try to activate */
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::SourceSelectedOnHoldSync>(mock_audiopath_dbus, true, aupath_source_proxy('B'), "srcB1");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcB1", player_id, deselected_result,
                                    appliance.is_audio_path_ready() == true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED_SOURCE_DEFERRED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl1");
    mock_audiopath_dbus->done();
    mock_messages->done();

    /* appliance was blocked, now assume it has told us it is ready */
    CHECK(appliance.set_audio_path_ready());

    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('B'), "srcB1");
    std::string source_id;
    CHECK(static_cast<int>(pswitch->complete_pending_source_activation(*paths, &source_id)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED_SOURCE_DEFERRED));
    CHECK(pswitch->get_player_id() == "pl1");
    CHECK(source_id == "srcB1");
}

/*!\test
 * Audio source activation is deferred, then a different source is selected.
 */
TEST_CASE_FIXTURE(Fixture, "Switch path during path switch while appliance is not ready")
{
    AudioPath::Appliance appliance;
    CHECK(appliance.set_audio_path_blocked());

    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    /* try to activate */
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::SourceSelectedOnHoldSync>(mock_audiopath_dbus, true, aupath_source_proxy('B'), "srcB1");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcB1", player_id, deselected_result,
                                    appliance.is_audio_path_ready() == true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED_SOURCE_DEFERRED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl1");
    mock_audiopath_dbus->done();
    mock_messages->done();

    /* activate another source, canceling the deferred source selection */
    expect<MockAudiopathDBus::SourceDeselectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('B'), "srcB1");
    expect<MockAudiopathDBus::PlayerDeactivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('2'));
    expect<MockAudiopathDBus::SourceSelectedOnHoldSync>(mock_audiopath_dbus, true, aupath_source_proxy('C'), "srcC2");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcC2", player_id, deselected_result,
                                    appliance.is_audio_path_ready() == true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED_SOURCE_DEFERRED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl2");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::DESELECTED_PENDING);
    CHECK(pswitch->get_player_id() == "pl2");
    mock_audiopath_dbus->done();
    mock_messages->done();

    /* appliance was blocked, now assume it has told us it is ready */
    CHECK(appliance.set_audio_path_ready());

    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('C'), "srcC2");
    std::string source_id;
    CHECK(static_cast<int>(pswitch->complete_pending_source_activation(*paths, &source_id)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED_SOURCE_DEFERRED));
    CHECK(pswitch->get_player_id() == "pl2");
    CHECK(source_id == "srcC2");
}

/*!\test
 * Deferred audio source activation with request data.
 */
TEST_CASE_FIXTURE(Fixture, "Switch path with request data while appliance is not ready")
{
    AudioPath::Appliance appliance;
    CHECK(appliance.set_audio_path_blocked());

    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    /* try to activate */
    GVariantDict dict;
    g_variant_dict_init(&dict, nullptr);
    g_variant_dict_insert_value(&dict, "cat", g_variant_new_string("meow"));
    g_variant_dict_insert_value(&dict, "dog", g_variant_new_string("woof"));
    g_variant_dict_insert_value(&dict, "frog", g_variant_new_string("ribbit"));
    auto request_data(GVariantWrapper(g_variant_dict_end(&dict)));

    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'), GVariantWrapper(request_data));
    expect<MockAudiopathDBus::SourceSelectedOnHoldSync>(mock_audiopath_dbus, true, aupath_source_proxy('B'), "srcB1", GVariantWrapper(request_data));

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcB1", player_id, deselected_result,
                                            appliance.is_audio_path_ready() == true,
                                            GVariantWrapper(request_data))) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED_SOURCE_DEFERRED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl1");
    mock_audiopath_dbus->done();
    mock_messages->done();

    /* appliance was blocked, now assume it has told us it is ready */
    CHECK(appliance.set_audio_path_ready());

    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('B'), "srcB1", std::move(request_data));
    std::string source_id;
    CHECK(static_cast<int>(pswitch->complete_pending_source_activation(*paths, &source_id)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED_SOURCE_DEFERRED));
    CHECK(pswitch->get_player_id() == "pl1");
    CHECK(source_id == "srcB1");
}

/*!\test
 * New audio source activation while another activation is pending works by
 * replacing the pending activation by a new pending activation.
 *
 * When the appliance is ready, the new pending activation is executed.
 */
TEST_CASE_FIXTURE(Fixture, "Switch path twice while appliance is not ready")
{
    AudioPath::Appliance appliance;
    CHECK(appliance.set_audio_path_blocked());

    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    /* try to activate the first time */
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::SourceSelectedOnHoldSync>(mock_audiopath_dbus, true, aupath_source_proxy('B'), "srcB1");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcB1", player_id, deselected_result,
                                        appliance.is_audio_path_ready() == true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED_SOURCE_DEFERRED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl1");
    mock_audiopath_dbus->done();
    mock_messages->done();

    /* try to activate the second time */
    expect<MockAudiopathDBus::SourceDeselectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('B'), "srcB1");
    expect<MockAudiopathDBus::PlayerDeactivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('2'));
    expect<MockAudiopathDBus::SourceSelectedOnHoldSync>(mock_audiopath_dbus, true, aupath_source_proxy('C'), "srcC2");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcC2", player_id, deselected_result,
                                        appliance.is_audio_path_ready() == true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED_SOURCE_DEFERRED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl2");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::DESELECTED_PENDING);
    CHECK(pswitch->get_player_id() == "pl2");
    mock_audiopath_dbus->done();
    mock_messages->done();

    /* appliance was blocked, now assume it has told us it is ready */
    CHECK(appliance.set_audio_path_ready());

    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('C'), "srcC2");
    std::string source_id;
    CHECK(static_cast<int>(pswitch->complete_pending_source_activation(*paths, &source_id)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED_SOURCE_DEFERRED));
    CHECK(pswitch->get_player_id() == "pl2");
    CHECK(source_id == "srcC2");
}

/*!\test
 * The current pending activation is not replaced by another activation request
 * of the same path.
 */
TEST_CASE_FIXTURE(Fixture, "Switch to same path twice while appliance is not ready")
{
    AudioPath::Appliance appliance;
    CHECK(appliance.set_audio_path_blocked());

    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    /* try to activate the first time */
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::SourceSelectedOnHoldSync>(mock_audiopath_dbus, true, aupath_source_proxy('B'), "srcB1");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcB1", player_id, deselected_result,
                                        appliance.is_audio_path_ready() == true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED_SOURCE_DEFERRED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl1");
    mock_audiopath_dbus->done();
    mock_messages->done();

    /* try to activate the same audio source again */
    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcB1", player_id, deselected_result,
                                        appliance.is_audio_path_ready() == true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED_SOURCE_DEFERRED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl1");
    mock_audiopath_dbus->done();
    mock_messages->done();

    /* appliance was blocked, now assume it has told us it is ready */
    CHECK(appliance.set_audio_path_ready());

    expect<MockAudiopathDBus::SourceSelectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('B'), "srcB1");
    std::string source_id;
    CHECK(static_cast<int>(pswitch->complete_pending_source_activation(*paths, &source_id)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED_SOURCE_DEFERRED));
    CHECK(pswitch->get_player_id() == "pl1");
    CHECK(source_id == "srcB1");
}

/*!\test
 * The current activation is canceled when the appliance suspends.
 */
TEST_CASE_FIXTURE(Fixture, "Activate audio path while appliance is not ready and then suspends")
{
    AudioPath::Appliance appliance;
    CHECK(appliance.set_up_and_running());
    CHECK(appliance.set_audio_path_blocked());

    const std::string *player_id;
    AudioPath::Switch::DeselectedAudioSourceResult deselected_result;

    /* try to activate */
    expect<MockAudiopathDBus::PlayerActivateSync>(mock_audiopath_dbus, true, aupath_player_proxy('1'));
    expect<MockAudiopathDBus::SourceSelectedOnHoldSync>(mock_audiopath_dbus, true, aupath_source_proxy('B'), "srcB1");

    CHECK(static_cast<int>(pswitch->activate_source(*paths, "srcB1", player_id, deselected_result,
                                        appliance.is_audio_path_ready() == true)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED_SOURCE_DEFERRED));

    REQUIRE(player_id != nullptr);
    CHECK(*player_id == "pl1");
    CHECK(deselected_result == AudioPath::Switch::DeselectedAudioSourceResult::NONE);
    CHECK(pswitch->get_player_id() == "pl1");
    mock_audiopath_dbus->done();
    mock_messages->done();

    /* appliance enters suspend mode */
    CHECK(appliance.set_suspend_mode());
    expect<MockAudiopathDBus::SourceDeselectedSync>(mock_audiopath_dbus, true, aupath_source_proxy('B'), "srcB1");
    std::string source_id;
    CHECK(static_cast<int>(pswitch->cancel_pending_source_activation(*paths, source_id)) ==
          static_cast<int>(AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED));
    CHECK(pswitch->get_player_id() == "pl1");
    CHECK(source_id == "srcB1");
}

/*!@}*/
