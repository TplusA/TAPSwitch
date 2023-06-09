/*
 * Copyright (C) 2017, 2018, 2020  T+A elektroakustik GmbH & Co. KG
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

#ifndef AUDIOPATH_HH
#define AUDIOPATH_HH

#include <string>
#include <map>
#include <memory>
#include <functional>

#include "dbus_proxy_wrapper.hh"

struct _tdbusaupathPlayer;
struct _tdbusaupathSource;

/*!
 * \addtogroup audiopath Audio path management
 */
/*!@{*/

namespace AudioPath
{

class Player
{
  public:
    using PType = DBus::Proxy<struct _tdbusaupathPlayer>;

    const std::string id_;
    const std::string name_;

  private:
    std::unique_ptr<PType> dbus_proxy_;

  public:
    Player(const Player &) = delete;
    Player(Player &&) = default;
    Player &operator=(const Player &) = delete;

    explicit Player(const char *id, const char *name,
                    std::unique_ptr<PType> dbus_proxy):
        id_(id),
        name_(name),
        dbus_proxy_(std::move(dbus_proxy))
    {}

    const PType &get_dbus_proxy() const { return *(dbus_proxy_.get()); }
    void take_proxy_from(Player &p) { dbus_proxy_ = std::move(p.dbus_proxy_); }
};

class Source
{
  public:
    using PType = DBus::Proxy<struct _tdbusaupathSource>;

    const std::string id_;
    const std::string name_;
    const std::string player_id_;

  private:
    std::unique_ptr<PType> dbus_proxy_;

  public:
    Source(const Source &) = delete;
    Source(Source &&) = default;
    Source &operator=(const Source &) = delete;

    explicit Source(const char *id, const char *name, const char *player_id,
                    std::unique_ptr<PType> dbus_proxy):
        id_(id),
        name_(name),
        player_id_(player_id),
        dbus_proxy_(std::move(dbus_proxy))
    {}

    const PType &get_dbus_proxy() const { return *(dbus_proxy_.get()); }
    void take_proxy_from(Source &s) { dbus_proxy_ = std::move(s.dbus_proxy_); }
};

template <typename T>
struct AddItemTraits;

class Paths
{
  public:
    using Path = std::pair<const Source *, const Player *>;

    enum class AddResult
    {
        NEW_COMPONENT,
        UPDATED_COMPONENT,
        NEW_PATH,
        UPDATED_PATH,
    };

  private:
    std::map<const std::string, Player> players_;
    std::map<const std::string, Source> sources_;

    friend struct AddItemTraits<Player>;
    friend struct AddItemTraits<Source>;

  public:
    Paths(const Paths &) = delete;
    Paths &operator=(const Paths &) = delete;

    explicit Paths() {}

    AddResult add_player(Player &&player);
    AddResult add_source(Source &&source);

    const Player *lookup_player(const std::string &player_id) const;
    const Source *lookup_source(const std::string &source_id) const;
    std::pair<const Source *, const Player *> lookup_path(const std::string &source_id) const;

    const Player *lookup_player(const char *player_id) const
    {
        return lookup_player(std::string(player_id));
    }

    const Source *lookup_source(const char *source_id) const
    {
        return lookup_source(std::string(source_id));
    }

    Path lookup_path(const char *source_id) const
    {
        return lookup_path(std::string(source_id));
    }

    enum class ForEach
    {
        ANY,
        COMPLETE_PATHS,
        INCOMPLETE_PATHS,
        UNCONNECTED_SOURCES,
        UNCONNECTED_PLAYERS,
    };

    void for_each(const std::function<void(const AudioPath::Paths::Path &)> &apply,
                  ForEach mode = ForEach::COMPLETE_PATHS) const;

  private:
    template <typename T>
    const T &add_item(T &&item, bool &inserted);
};

}

/*!@}*/

#endif /* !AUDIOPATH_HH */
