/*
 * Copyright (C) 2017, 2020  T+A elektroakustik GmbH & Co. KG
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

#include "audiopath.hh"

/*!
 * \addtogroup audiopath_tests Unit tests
 * \ingroup audiopath
 *
 * DBus handlers unit tests.
 */
/*!@{*/

TEST_SUITE_BEGIN("Audio path");

struct _tdbusaupathPlayer
{
  private:
    std::string dummy_;

  public:
    _tdbusaupathPlayer(std::string &&dummy): dummy_(std::move(dummy)) {}
    const std::string &const_string() const { return dummy_; }
};

struct _tdbusaupathSource
{
  private:
    std::string dummy_;

  public:
    _tdbusaupathSource(std::string &&dummy): dummy_(std::move(dummy)) {}
    const std::string &const_string() const { return dummy_; }
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

template <typename T>
std::unique_ptr<T> mk_proxy(const char *dest, const char *obj_path);

template <>
std::unique_ptr<AudioPath::Player::PType>
mk_proxy(const char *dest, const char *obj_path)
{
    std::string temp(dest);

    temp.append(":");
    temp.append(obj_path);

    /* use raw \c new to find memory leaks */
    auto *raw_proxy = new _tdbusaupathPlayer(std::move(temp));
    REQUIRE(raw_proxy != nullptr);

    return std::make_unique<AudioPath::Player::PType>(raw_proxy);
}

template<>
Proxy<_tdbusaupathPlayer>::~Proxy()
{
    CHECK(proxy_ != nullptr);
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
    REQUIRE(raw_proxy != nullptr);

    return std::make_unique<AudioPath::Source::PType>(raw_proxy);
}

template<>
Proxy<_tdbusaupathSource>::~Proxy()
{
    CHECK(proxy_ != nullptr);
    delete proxy_;
}

}

/*!\test
 * Look up audio source right after initialization.
 */
TEST_CASE("Lookup source in empty paths returns nullptr")
{
    AudioPath::Paths paths;
    CHECK(paths.lookup_source("src") == nullptr);
}

/*!\test
 * Look up audio source while only a single player has registered.
 */
TEST_CASE("Lookup nonexistent source in paths with player of same name returns nullptr")
{
    AudioPath::Paths paths;

    CHECK(static_cast<int>(paths.add_player(
            AudioPath::Player("foo", "Foo player",
                              DBus::mk_proxy<AudioPath::Player::PType>("dbus.player",
                                                                       "/dbus/player")))) ==
          static_cast<int>(AudioPath::Paths::AddResult::NEW_COMPONENT));
    CHECK(paths.lookup_source("foo") == nullptr);
}

/*!\test
 * Look up audio path right after initialization.
 */
TEST_CASE("Lookup path in empty paths returns nullptr")
{
    AudioPath::Paths paths;

    const auto ap(paths.lookup_path("src"));

    CHECK(ap.first == nullptr);
    CHECK(ap.second == nullptr);
}

static void expect_no_paths(const AudioPath::Paths &paths,
                            AudioPath::Paths::ForEach mode = AudioPath::Paths::ForEach::COMPLETE_PATHS)
{
    paths.for_each([] (const AudioPath::Paths::Path &p)
                   {
                       FAIL("Unexpected callback invocation");
                   },
                   mode);
}

/*!\test
 * Enumerating audio paths after initialization does not enumerate anything.
 */
TEST_CASE("Path enumeration in empty paths")
{
    AudioPath::Paths paths;
    expect_no_paths(paths, AudioPath::Paths::ForEach::ANY);
}

/*!\test
 * Look up audio path for nonexistent source while another audio path is
 * availiable.
 *
 * This test is supposed to show that there is no fallback mechanism or
 * hardcoded lookup.
 */
TEST_CASE("Lookup path for nonexistent source returns nullptr")
{
    AudioPath::Paths paths;

    CHECK(static_cast<int>(paths.add_player(
            AudioPath::Player("p1", "Test player",
                              DBus::mk_proxy<AudioPath::Player::PType>("dbus.player",
                                                                       "/dbus/player")))) ==
          static_cast<int>(AudioPath::Paths::AddResult::NEW_COMPONENT));
    CHECK(static_cast<int>(paths.add_source(
            AudioPath::Source("s1", "Test source", "p1",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.source",
                                                                       "/dbus/source")))) ==
          static_cast<int>(AudioPath::Paths::AddResult::NEW_PATH));

    const auto ap(paths.lookup_path("src"));

    CHECK(ap.first == nullptr);
    CHECK(ap.second == nullptr);

    CHECK(paths.lookup_source("s1") != nullptr);
    CHECK(paths.lookup_source("src") == nullptr);
}

/*!\test
 * Look up partial audio path where a required has not registered yet.
 */
TEST_CASE("Lookup source without player returns only source")
{
    AudioPath::Paths paths;

    CHECK(static_cast<int>(paths.add_source(
            AudioPath::Source("src", "Test source", "p1",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.source",
                                                                       "/dbus/source")))) ==
          static_cast<int>(AudioPath::Paths::AddResult::NEW_COMPONENT));

    const auto ap(paths.lookup_path("src"));

    REQUIRE(ap.first != nullptr);
    CHECK(ap.first->id_ == "src");
    CHECK(ap.second == nullptr);
}

/*!\test
 * Looking up audio path works if a player registers after the source has
 * registered which requires the player.
 */
TEST_CASE("Add source then player")
{
    AudioPath::Paths paths;

    CHECK(static_cast<int>(paths.add_source(
            AudioPath::Source("s1", "Test source", "p1",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.source",
                                                                       "/dbus/source")))) ==
          static_cast<int>(AudioPath::Paths::AddResult::NEW_COMPONENT));

    expect_no_paths(paths);
    expect_no_paths(paths, AudioPath::Paths::ForEach::UNCONNECTED_PLAYERS);

    int called = 0;
    paths.for_each([&called] (const AudioPath::Paths::Path &p)
                   {
                       ++called;
                       REQUIRE(p.first != nullptr);
                       CHECK(p.second == nullptr);
                       CHECK(p.first->id_ == "s1");
                   },
                   AudioPath::Paths::ForEach::UNCONNECTED_SOURCES);
    CHECK(called == 1);

    called = 0;
    paths.for_each([&called] (const AudioPath::Paths::Path &p)
                   {
                       ++called;
                       REQUIRE(p.first != nullptr);
                       CHECK(p.second == nullptr);
                       CHECK(p.first->id_ == "s1");
                   },
                   AudioPath::Paths::ForEach::INCOMPLETE_PATHS);
    CHECK(called == 1);

    CHECK(static_cast<int>(paths.add_player(
            AudioPath::Player("p1", "Test player",
                              DBus::mk_proxy<AudioPath::Player::PType>("dbus.player",
                                                                       "/dbus/player")))) ==
          static_cast<int>(AudioPath::Paths::AddResult::NEW_PATH));

    const auto ap(paths.lookup_path("s1"));

    REQUIRE(ap.first != nullptr);
    CHECK(ap.first->id_ == "s1");
    CHECK(ap.first->name_ == "Test source");
    CHECK(ap.first->get_dbus_proxy().get()->const_string() == "dbus.source:/dbus/source");
    CHECK(ap.first->player_id_ == "p1");

    REQUIRE(ap.second != nullptr);
    CHECK(ap.second->id_ == "p1");
    CHECK(ap.second->name_ == "Test player");
    CHECK(ap.second->get_dbus_proxy().get()->const_string() == "dbus.player:/dbus/player");

    called = 0;
    paths.for_each([&ap, &called] (const AudioPath::Paths::Path &p)
                   {
                       ++called;
                       CHECK(p.first == ap.first);
                       CHECK(p.second == ap.second);
                   });
    CHECK(called == 1);

    expect_no_paths(paths, AudioPath::Paths::ForEach::INCOMPLETE_PATHS);
    expect_no_paths(paths, AudioPath::Paths::ForEach::UNCONNECTED_SOURCES);
    expect_no_paths(paths, AudioPath::Paths::ForEach::UNCONNECTED_PLAYERS);
}

/*!\test
 * Looking up audio path works if a source registers after its required player
 * has registered.
 */
TEST_CASE("Add player then source")
{
    AudioPath::Paths paths;

    CHECK(static_cast<int>(paths.add_player(
            AudioPath::Player("p1", "Test player",
                              DBus::mk_proxy<AudioPath::Player::PType>("dbus.player",
                                                                       "/dbus/player")))) ==
          static_cast<int>(AudioPath::Paths::AddResult::NEW_COMPONENT));

    expect_no_paths(paths);
    expect_no_paths(paths, AudioPath::Paths::ForEach::UNCONNECTED_SOURCES);

    int called = 0;
    paths.for_each([&called] (const AudioPath::Paths::Path &p)
                   {
                       ++called;
                       CHECK(p.first == nullptr);
                       REQUIRE(p.second != nullptr);
                       CHECK(p.second->id_ == "p1");
                   },
                   AudioPath::Paths::ForEach::UNCONNECTED_PLAYERS);
    CHECK(called == 1);

    called = 0;
    paths.for_each([&called] (const AudioPath::Paths::Path &p)
                   {
                       ++called;
                       CHECK(p.first == nullptr);
                       REQUIRE(p.second != nullptr);
                       CHECK(p.second->id_ == "p1");
                   },
                   AudioPath::Paths::ForEach::INCOMPLETE_PATHS);
    CHECK(called == 1);

    CHECK(static_cast<int>(paths.add_source(
            AudioPath::Source("s1", "Test source", "p1",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.source",
                                                                       "/dbus/source")))) ==
          static_cast<int>(AudioPath::Paths::AddResult::NEW_PATH));

    const auto ap(paths.lookup_path("s1"));

    REQUIRE(ap.first != nullptr);
    CHECK(ap.first->id_ == "s1");
    CHECK(ap.first->name_ == "Test source");
    CHECK(ap.first->get_dbus_proxy().get()->const_string() == "dbus.source:/dbus/source");
    CHECK(ap.first->player_id_ == "p1");

    REQUIRE(ap.second != nullptr);
    CHECK(ap.second->id_ == "p1");
    CHECK(ap.second->name_ == "Test player");
    CHECK(ap.second->get_dbus_proxy().get()->const_string() == "dbus.player:/dbus/player");

    called = 0;
    paths.for_each([&ap, &called] (const AudioPath::Paths::Path &p)
                   {
                       ++called;
                       CHECK(p.first == ap.first);
                       CHECK(p.second == ap.second);
                   });
    CHECK(called == 1);

    expect_no_paths(paths, AudioPath::Paths::ForEach::INCOMPLETE_PATHS);
    expect_no_paths(paths, AudioPath::Paths::ForEach::UNCONNECTED_SOURCES);
    expect_no_paths(paths, AudioPath::Paths::ForEach::UNCONNECTED_PLAYERS);
}

/*!\test
 * In case a player restarts and registers (again, from our point of view),
 * then only the D-Bus proxy gets updated.
 */
TEST_CASE("Add player twice updates dbus proxy only")
{
    AudioPath::Paths paths;

    CHECK(static_cast<int>(paths.add_source(
            AudioPath::Source("s1", "Test source", "p1",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.source",
                                                                       "/dbus/source")))) ==
          static_cast<int>(AudioPath::Paths::AddResult::NEW_COMPONENT));

    CHECK(static_cast<int>(paths.add_player(
            AudioPath::Player("p1", "Test player",
                              DBus::mk_proxy<AudioPath::Player::PType>("dbus.player",
                                                                       "/dbus/player")))) ==
          static_cast<int>(AudioPath::Paths::AddResult::NEW_PATH));

    const auto ap_first(paths.lookup_path("s1"));

    REQUIRE(ap_first.first != nullptr);
    REQUIRE(ap_first.second != nullptr);
    CHECK(ap_first.second->id_ == "p1");
    CHECK(ap_first.second->name_ == "Test player");
    CHECK(ap_first.second->get_dbus_proxy().get()->const_string() == "dbus.player:/dbus/player");

    CHECK(static_cast<int>(paths.add_player(
            AudioPath::Player("p1", "Funky Player",
                              DBus::mk_proxy<AudioPath::Player::PType>("dbus.funky",
                                                                       "/dbus/funky")))) ==
          static_cast<int>(AudioPath::Paths::AddResult::UPDATED_PATH));

    const auto ap_second(paths.lookup_path("s1"));

    REQUIRE(ap_second.first != nullptr);
    REQUIRE(ap_second.second != nullptr);
    CHECK(ap_second.second->id_ == "p1");
    CHECK(ap_second.second->name_ == "Test player");
    CHECK(ap_second.second->get_dbus_proxy().get()->const_string() == "dbus.funky:/dbus/funky");
}

/*!\test
 * In case a source restarts and registers (again, from our point of view) as
 * single component, then only the D-Bus proxy gets updated.
 */
TEST_CASE("Add source component twice updates dbus proxy only")
{
    AudioPath::Paths paths;

    CHECK(static_cast<int>(paths.add_source(
            AudioPath::Source("s1", "Test source", "p1",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.source",
                                                                       "/dbus/source")))) ==
          static_cast<int>(AudioPath::Paths::AddResult::NEW_COMPONENT));

    const auto *src_first(paths.lookup_source("s1"));

    REQUIRE(src_first != nullptr);
    CHECK(src_first->id_ == "s1");
    CHECK(src_first->name_ == "Test source");
    CHECK(src_first->get_dbus_proxy().get()->const_string() == "dbus.source:/dbus/source");
    CHECK(src_first->player_id_ == "p1");

    CHECK(static_cast<int>(paths.add_source(
            AudioPath::Source("s1", "Foo Input", "funky",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.foo",
                                                                       "/dbus/foo")))) ==
          static_cast<int>(AudioPath::Paths::AddResult::UPDATED_COMPONENT));

    const auto *src_second(paths.lookup_source("s1"));

    REQUIRE(src_second != nullptr);
    CHECK(src_second->id_ == "s1");
    CHECK(src_second->name_ == "Test source");
    CHECK(src_second->get_dbus_proxy().get()->const_string() == "dbus.foo:/dbus/foo");
    CHECK(src_second->player_id_ == "p1");
}

/*!\test
 * In case a source restarts and registers (again, from our point of view) as
 * part of an existing audio path, then only the D-Bus proxy gets updated.
 */
TEST_CASE("Add source twice into audio path updates dbus proxy only")
{
    AudioPath::Paths paths;

    CHECK(static_cast<int>(paths.add_source(
            AudioPath::Source("s1", "Test source", "p1",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.source",
                                                                       "/dbus/source")))) ==
          static_cast<int>(AudioPath::Paths::AddResult::NEW_COMPONENT));

    CHECK(static_cast<int>(paths.add_player(
            AudioPath::Player("p1", "Test player",
                              DBus::mk_proxy<AudioPath::Player::PType>("dbus.player",
                                                                       "/dbus/player")))) ==
          static_cast<int>(AudioPath::Paths::AddResult::NEW_PATH));

    const auto *src_first(paths.lookup_source("s1"));

    REQUIRE(src_first != nullptr);
    CHECK(src_first->id_ == "s1");
    CHECK(src_first->name_ == "Test source");
    CHECK(src_first->get_dbus_proxy().get()->const_string() == "dbus.source:/dbus/source");
    CHECK(src_first->player_id_ == "p1");

    CHECK(static_cast<int>(paths.add_source(
            AudioPath::Source("s1", "Foo Input", "funky",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.foo",
                                                                       "/dbus/foo")))) ==
          static_cast<int>(AudioPath::Paths::AddResult::UPDATED_PATH));

    const auto *src_second(paths.lookup_source("s1"));

    REQUIRE(src_second != nullptr);
    CHECK(src_second->id_ == "s1");
    CHECK(src_second->name_ == "Test source");
    CHECK(src_second->get_dbus_proxy().get()->const_string() == "dbus.foo:/dbus/foo");
    CHECK(src_second->player_id_ == "p1");
}

/*!\test
 * Add multiple sources for same player, then add the player to complete
 * multiple audio paths.
 */
TEST_CASE("Add multiple sources for same player then add player")
{
    AudioPath::Paths paths;

    CHECK(static_cast<int>(paths.add_source(
            AudioPath::Source("s1", "Source 1", "ThePlayer",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.source1",
                                                                       "/dbus/source1")))) ==
          static_cast<int>(AudioPath::Paths::AddResult::NEW_COMPONENT));
    CHECK(static_cast<int>(paths.add_source(
            AudioPath::Source("s2", "Source 2", "ThePlayer",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.source2",
                                                                       "/dbus/source2")))) ==
          static_cast<int>(AudioPath::Paths::AddResult::NEW_COMPONENT));
    CHECK(static_cast<int>(paths.add_source(
            AudioPath::Source("s3", "Source 3", "ThePlayer",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.source3",
                                                                       "/dbus/source3")))) ==
          static_cast<int>(AudioPath::Paths::AddResult::NEW_COMPONENT));
    CHECK(static_cast<int>(paths.add_source(
            AudioPath::Source("s4", "Source 4", "ThePlayer",
                              DBus::mk_proxy<AudioPath::Source::PType>("dbus.source4",
                                                                       "/dbus/source4")))) ==
          static_cast<int>(AudioPath::Paths::AddResult::NEW_COMPONENT));

    expect_no_paths(paths);
    expect_no_paths(paths, AudioPath::Paths::ForEach::UNCONNECTED_PLAYERS);

    int called = 0;
    paths.for_each([&called] (const AudioPath::Paths::Path &p)
                   {
                       ++called;
                       CHECK(p.first != nullptr);
                       CHECK(p.second == nullptr);
                   },
                   AudioPath::Paths::ForEach::UNCONNECTED_SOURCES);
    CHECK(called == 4);

    called = 0;
    paths.for_each([&called] (const AudioPath::Paths::Path &p)
                   {
                       ++called;
                       CHECK(p.first != nullptr);
                       CHECK(p.second == nullptr);
                   },
                   AudioPath::Paths::ForEach::INCOMPLETE_PATHS);
    CHECK(called == 4);

    CHECK(static_cast<int>(paths.add_player(
            AudioPath::Player("ThePlayer", "Our central player",
                              DBus::mk_proxy<AudioPath::Player::PType>("dbus.player",
                                                                       "/dbus/player")))) ==
          static_cast<int>(AudioPath::Paths::AddResult::NEW_PATH));

    std::map<const std::string, bool> seen
    {
        { "s1", false },
        { "s2", false },
        { "s3", false },
        { "s4", false },
    };

    paths.for_each([&seen] (const AudioPath::Paths::Path &p)
                   {
                       CHECK(p.second->id_ == "ThePlayer");
                       CHECK(seen.find(p.first->id_) != seen.end());
                       seen[p.first->id_] = true;
                   });

    CHECK(seen.size() == size_t(4));

    for(const auto &s : seen)
        CHECK(s.second);

    expect_no_paths(paths, AudioPath::Paths::ForEach::INCOMPLETE_PATHS);
    expect_no_paths(paths, AudioPath::Paths::ForEach::UNCONNECTED_SOURCES);
    expect_no_paths(paths, AudioPath::Paths::ForEach::UNCONNECTED_PLAYERS);
}

/*!@}*/
