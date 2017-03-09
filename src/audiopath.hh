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

#ifndef AUDIOPATH_HH
#define AUDIOPATH_HH

#include <string>
#include <map>
#include <memory>

#include "dbus_proxy_wrapper.hh"

struct _tdbusaupathPlayer;
struct _tdbusaupathSource;

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
  private:
    std::map<const std::string, Player> players_;
    std::map<const std::string, Source> sources_;

    friend struct AddItemTraits<Player>;
    friend struct AddItemTraits<Source>;

  public:
    Paths(const Paths &) = delete;
    Paths &operator=(const Paths &) = delete;

    explicit Paths() {}

    void add_player(Player &&player);
    void add_source(Source &&source);

    std::pair<const Source *, const Player *> lookup(const char *source_id);

  private:
    template <typename T>
    void add_item(T &&item);
};

};

#endif /* !AUDIOPATH_HH */
