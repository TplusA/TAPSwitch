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

/*!
 * \addtogroup audiopath_tests Unit tests
 * \ingroup audiopath
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

namespace std
{
    template <typename C, typename CT>
    inline std::basic_ostream<C, CT> &
    operator<<(std::basic_ostream<C, CT> &os, const AudioPath::Paths::AddResult r)
    {
        switch(r)
        {
          case AudioPath::Paths::AddResult::NEW_COMPONENT:
            os << "NEW_COMPONENT";
            break;

          case AudioPath::Paths::AddResult::UPDATED_COMPONENT:
            os << "UPDATED_COMPONENT";
            break;

          case AudioPath::Paths::AddResult::NEW_PATH:
            os << "NEW_PATH";
            break;

          case AudioPath::Paths::AddResult::UPDATED_PATH:
            os << "UPDATED_PATH";
            break;
        }

        return os;
    }
}


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

namespace audiopath_tests
{

/*!\test
 * Look up audio source right after initialization.
 */
void test_lookup_source_in_empty_paths_returns_null()
{
    AudioPath::Paths paths;
    cppcut_assert_null(paths.lookup_source("src"));
}

/*!\test
 * Look up audio source while only a single player has registered.
 */
void test_lookup_nonexistent_source_in_paths_with_player_of_same_name_returns_null()
{
    AudioPath::Paths paths;

    cppcut_assert_equal(AudioPath::Paths::AddResult::NEW_COMPONENT,
        paths.add_player(std::move(
            AudioPath::Player("foo", "Foo player",
                              DBus::mk_proxy<AudioPath::Player::PType>("dbus.player",
                                                                       "/dbus/player")))));
    cppcut_assert_null(paths.lookup_source("foo"));
}

/*!\test
 * Look up audio path right after initialization.
 */
void test_lookup_path_in_empty_paths_returns_null()
{
    AudioPath::Paths paths;

    const auto ap(paths.lookup_path("src"));

    cppcut_assert_null(ap.first);
    cppcut_assert_null(ap.second);
}

/*!\test
 * Look up audio path for nonexistent source while another audio path is
 * availiable.
 *
 * This test is supposed to show that there is no fallback mechanism or
 * hardcoded lookup.
 */
void test_lookup_path_for_nonexistent_source_returns_null()
{
    AudioPath::Paths paths;

    cppcut_assert_equal(AudioPath::Paths::AddResult::NEW_COMPONENT,
        paths.add_player(std::move(
            AudioPath::Player("p1", "Test player",
                              DBus::mk_proxy<AudioPath::Player::PType>("dbus.player",
                                                                       "/dbus/player")))));
    cppcut_assert_equal(AudioPath::Paths::AddResult::NEW_PATH,
        paths.add_source(std::move(
            AudioPath::Source("s1", "Test source", "p1",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.source",
                                                                       "/dbus/source")))));

    const auto ap(paths.lookup_path("src"));

    cppcut_assert_null(ap.first);
    cppcut_assert_null(ap.second);

    cppcut_assert_not_null(paths.lookup_source("s1"));
    cppcut_assert_null(paths.lookup_source("src"));
}

/*!\test
 * Look up partial audio path where a required has not registered yet.
 */
void test_lookup_source_without_player_returns_only_source()
{
    AudioPath::Paths paths;

    cppcut_assert_equal(AudioPath::Paths::AddResult::NEW_COMPONENT,
        paths.add_source(std::move(
            AudioPath::Source("src", "Test source", "p1",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.source",
                                                                       "/dbus/source")))));

    const auto ap(paths.lookup_path("src"));

    cppcut_assert_not_null(ap.first);
    cppcut_assert_equal("src", ap.first->id_.c_str());
    cppcut_assert_null(ap.second);
}

/*!\test
 * Looking up audio path works if a source registers after its required player
 * has registered.
 */
void test_add_player_then_source()
{
    AudioPath::Paths paths;

    cppcut_assert_equal(AudioPath::Paths::AddResult::NEW_COMPONENT,
        paths.add_source(std::move(
            AudioPath::Source("s1", "Test source", "p1",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.source",
                                                                       "/dbus/source")))));
    cppcut_assert_equal(AudioPath::Paths::AddResult::NEW_PATH,
        paths.add_player(std::move(
            AudioPath::Player("p1", "Test player",
                              DBus::mk_proxy<AudioPath::Player::PType>("dbus.player",
                                                                       "/dbus/player")))));

    const auto ap(paths.lookup_path("s1"));

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

/*!\test
 * Looking up audio path works if a player registers after the source has
 * registered which requires the player.
 */
void test_add_source_then_player()
{
    AudioPath::Paths paths;

    cppcut_assert_equal(AudioPath::Paths::AddResult::NEW_COMPONENT,
        paths.add_player(std::move(
            AudioPath::Player("p1", "Test player",
                              DBus::mk_proxy<AudioPath::Player::PType>("dbus.player",
                                                                       "/dbus/player")))));
    cppcut_assert_equal(AudioPath::Paths::AddResult::NEW_PATH,
        paths.add_source(std::move(
            AudioPath::Source("s1", "Test source", "p1",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.source",
                                                                       "/dbus/source")))));

    const auto ap(paths.lookup_path("s1"));

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

/*!\test
 * In case a player restarts and registers (again, from our point of view),
 * then only the D-Bus proxy gets updated.
 */
void test_add_player_twice_updates_dbus_proxy_only()
{
    AudioPath::Paths paths;

    cppcut_assert_equal(AudioPath::Paths::AddResult::NEW_COMPONENT,
        paths.add_source(std::move(
            AudioPath::Source("s1", "Test source", "p1",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.source",
                                                                       "/dbus/source")))));

    cppcut_assert_equal(AudioPath::Paths::AddResult::NEW_PATH,
        paths.add_player(std::move(
            AudioPath::Player("p1", "Test player",
                              DBus::mk_proxy<AudioPath::Player::PType>("dbus.player",
                                                                       "/dbus/player")))));

    const auto ap_first(paths.lookup_path("s1"));

    cppcut_assert_not_null(ap_first.first);
    cppcut_assert_not_null(ap_first.second);
    cppcut_assert_equal("p1", ap_first.second->id_.c_str());
    cppcut_assert_equal("Test player", ap_first.second->name_.c_str());
    cppcut_assert_equal("dbus.player:/dbus/player",
                        ap_first.second->get_dbus_proxy().get()->const_string());

    cppcut_assert_equal(AudioPath::Paths::AddResult::UPDATED_PATH,
        paths.add_player(std::move(
            AudioPath::Player("p1", "Funky Player",
                              DBus::mk_proxy<AudioPath::Player::PType>("dbus.funky",
                                                                       "/dbus/funky")))));

    const auto ap_second(paths.lookup_path("s1"));

    cppcut_assert_not_null(ap_second.first);
    cppcut_assert_not_null(ap_second.second);
    cppcut_assert_equal("p1", ap_second.second->id_.c_str());
    cppcut_assert_equal("Test player", ap_second.second->name_.c_str());
    cppcut_assert_equal("dbus.funky:/dbus/funky",
                        ap_second.second->get_dbus_proxy().get()->const_string());
}

/*!\test
 * In case a source restarts and registers (again, from our point of view) as
 * single component, then only the D-Bus proxy gets updated.
 */
void test_add_source_component_twice_updates_dbus_proxy_only()
{
    AudioPath::Paths paths;

    cppcut_assert_equal(AudioPath::Paths::AddResult::NEW_COMPONENT,
        paths.add_source(std::move(
            AudioPath::Source("s1", "Test source", "p1",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.source",
                                                                       "/dbus/source")))));

    const auto *src_first(paths.lookup_source("s1"));

    cppcut_assert_not_null(src_first);
    cppcut_assert_equal("s1", src_first->id_.c_str());
    cppcut_assert_equal("Test source", src_first->name_.c_str());
    cppcut_assert_equal("dbus.source:/dbus/source",
                        src_first->get_dbus_proxy().get()->const_string());
    cppcut_assert_equal("p1", src_first->player_id_.c_str());

    cppcut_assert_equal(AudioPath::Paths::AddResult::UPDATED_COMPONENT,
        paths.add_source(std::move(
            AudioPath::Source("s1", "Foo Input", "funky",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.foo",
                                                                       "/dbus/foo")))));

    const auto *src_second(paths.lookup_source("s1"));

    cppcut_assert_not_null(src_second);
    cppcut_assert_equal("s1", src_second->id_.c_str());
    cppcut_assert_equal("Test source", src_second->name_.c_str());
    cppcut_assert_equal("dbus.foo:/dbus/foo",
                        src_second->get_dbus_proxy().get()->const_string());
    cppcut_assert_equal("p1", src_second->player_id_.c_str());
}

/*!\test
 * In case a source restarts and registers (again, from our point of view) as
 * part of an existing audio path, then only the D-Bus proxy gets updated.
 */
void test_add_source_twice_into_audio_path_updates_dbus_proxy_only()
{
    AudioPath::Paths paths;

    cppcut_assert_equal(AudioPath::Paths::AddResult::NEW_COMPONENT,
        paths.add_source(std::move(
            AudioPath::Source("s1", "Test source", "p1",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.source",
                                                                       "/dbus/source")))));

    cppcut_assert_equal(AudioPath::Paths::AddResult::NEW_PATH,
        paths.add_player(std::move(
            AudioPath::Player("p1", "Test player",
                              DBus::mk_proxy<AudioPath::Player::PType>("dbus.player",
                                                                       "/dbus/player")))));

    const auto *src_first(paths.lookup_source("s1"));

    cppcut_assert_not_null(src_first);
    cppcut_assert_equal("s1", src_first->id_.c_str());
    cppcut_assert_equal("Test source", src_first->name_.c_str());
    cppcut_assert_equal("dbus.source:/dbus/source",
                        src_first->get_dbus_proxy().get()->const_string());
    cppcut_assert_equal("p1", src_first->player_id_.c_str());

    cppcut_assert_equal(AudioPath::Paths::AddResult::UPDATED_PATH,
        paths.add_source(std::move(
            AudioPath::Source("s1", "Foo Input", "funky",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.foo",
                                                                       "/dbus/foo")))));

    const auto *src_second(paths.lookup_source("s1"));

    cppcut_assert_not_null(src_second);
    cppcut_assert_equal("s1", src_second->id_.c_str());
    cppcut_assert_equal("Test source", src_second->name_.c_str());
    cppcut_assert_equal("dbus.foo:/dbus/foo",
                        src_second->get_dbus_proxy().get()->const_string());
    cppcut_assert_equal("p1", src_second->player_id_.c_str());
}

}

/*!@}*/
