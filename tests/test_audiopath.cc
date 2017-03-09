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

#include "audiopath.hh"

/*!
 * \addtogroup dbus_handlers_tests Unit tests
 * \ingroup dbus_handlers
 *
 * DBus handlers unit tests.
 */
/*!@{*/

struct _tdbusaupathPlayer
{
  private:
    std::string dummy_;

  public:
    _tdbusaupathPlayer(std::string &&dummy): dummy_(std::move(dummy)) {}
    const char *const_string() const { return dummy_.c_str(); }
};

struct _tdbusaupathSource
{
  private:
    std::string dummy_;

  public:
    _tdbusaupathSource(std::string &&dummy): dummy_(std::move(dummy)) {}
    const char *const_string() const { return dummy_.c_str(); }
};


namespace DBus
{

template <>
std::unique_ptr<AudioPath::Player::PType>
mk_proxy(const char *dest, const char *obj_path)
{
    std::string temp(dest);

    temp.append(":");
    temp.append(obj_path);

    /* use raw \c new to find memory leaks */
    auto *raw_proxy = new _tdbusaupathPlayer(std::move(temp));
    cppcut_assert_not_null(raw_proxy);

    return std::unique_ptr<AudioPath::Player::PType>(new AudioPath::Player::PType(raw_proxy));
}

template<>
Proxy<_tdbusaupathPlayer>::~Proxy()
{
    cppcut_assert_not_null(proxy_);
    delete proxy_;
}

template <>
std::unique_ptr<AudioPath::Source::PType>
mk_proxy(const char *dest, const char *obj_path)
{
    std::string temp(dest);

    temp.append(":");
    temp.append(obj_path);

    /* use raw \c new to find memory leaks */
    auto *raw_proxy = new _tdbusaupathSource(std::move(temp));
    cppcut_assert_not_null(raw_proxy);

    return std::unique_ptr<AudioPath::Source::PType>(new AudioPath::Source::PType(raw_proxy));
}

template<>
Proxy<_tdbusaupathSource>::~Proxy()
{
    cppcut_assert_not_null(proxy_);
    delete proxy_;
}

}

namespace dbus_handlers_tests
{

void test_lookup_source_in_empty_paths_returns_null()
{
    AudioPath::Paths paths;

    const auto ap(paths.lookup("src"));

    cppcut_assert_null(ap.first);
    cppcut_assert_null(ap.second);
}

void test_lookup_nonexistent_source_returns_null()
{
    AudioPath::Paths paths;

    paths.add_player(std::move(
        AudioPath::Player("p1", "Test player",
                          DBus::mk_proxy<AudioPath::Player::PType>("dbus.player",
                                                                   "/dbus/player"))));
    paths.add_source(std::move(
        AudioPath::Source("s1", "Test source", "p1",
                          DBus::mk_proxy<AudioPath::Source::PType>("dbus.source",
                                                                   "/dbus/source"))));

    const auto ap(paths.lookup("src"));

    cppcut_assert_null(ap.first);
    cppcut_assert_null(ap.second);
}

void test_lookup_source_without_player_returns_only_source()
{
    AudioPath::Paths paths;

    paths.add_source(std::move(
        AudioPath::Source("src", "Test source", "p1",
                          DBus::mk_proxy<AudioPath::Source::PType>("dbus.source",
                                                                   "/dbus/source"))));

    const auto ap(paths.lookup("src"));

    cppcut_assert_not_null(ap.first);
    cppcut_assert_equal("src", ap.first->id_.c_str());
    cppcut_assert_null(ap.second);
}

void test_add_player_then_source()
{
    AudioPath::Paths paths;

    paths.add_source(std::move(
        AudioPath::Source("s1", "Test source", "p1",
                          DBus::mk_proxy<AudioPath::Source::PType>("dbus.source",
                                                                   "/dbus/source"))));
    paths.add_player(std::move(
        AudioPath::Player("p1", "Test player",
                          DBus::mk_proxy<AudioPath::Player::PType>("dbus.player",
                                                                   "/dbus/player"))));

    const auto ap(paths.lookup("s1"));

    cppcut_assert_not_null(ap.first);
    cppcut_assert_equal("s1", ap.first->id_.c_str());
    cppcut_assert_equal("Test source", ap.first->name_.c_str());
    cppcut_assert_equal("dbus.source:/dbus/source",
                        ap.first->get_dbus_proxy().get()->const_string());
    cppcut_assert_equal("p1", ap.first->player_id_.c_str());

    cppcut_assert_not_null(ap.second);
    cppcut_assert_equal("p1", ap.second->id_.c_str());
    cppcut_assert_equal("Test player", ap.second->name_.c_str());
    cppcut_assert_equal("dbus.player:/dbus/player",
                        ap.second->get_dbus_proxy().get()->const_string());
}

void test_add_source_then_player()
{
    AudioPath::Paths paths;

    paths.add_player(std::move(
        AudioPath::Player("p1", "Test player",
                          DBus::mk_proxy<AudioPath::Player::PType>("dbus.player",
                                                                   "/dbus/player"))));
    paths.add_source(std::move(
        AudioPath::Source("s1", "Test source", "p1",
                          DBus::mk_proxy<AudioPath::Source::PType>("dbus.source",
                                                                   "/dbus/source"))));

    const auto ap(paths.lookup("s1"));

    cppcut_assert_not_null(ap.first);
    cppcut_assert_equal("s1", ap.first->id_.c_str());
    cppcut_assert_equal("Test source", ap.first->name_.c_str());
    cppcut_assert_equal("dbus.source:/dbus/source",
                        ap.first->get_dbus_proxy().get()->const_string());
    cppcut_assert_equal("p1", ap.first->player_id_.c_str());

    cppcut_assert_not_null(ap.second);
    cppcut_assert_equal("p1", ap.second->id_.c_str());
    cppcut_assert_equal("Test player", ap.second->name_.c_str());
    cppcut_assert_equal("dbus.player:/dbus/player",
                        ap.second->get_dbus_proxy().get()->const_string());
}

}

/*!@}*/
