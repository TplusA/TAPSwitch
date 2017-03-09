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

#include "audiopath.hh"

/*
template <typename T>
struct AddItemTraits;
*/

namespace AudioPath
{

template <>
struct AddItemTraits<Player>
{
    using MapType = std::map<const std::string, Player>;
    static MapType &get_map(Paths &paths) { return paths.players_; }
};

template <>
struct AddItemTraits<Source>
{
    using MapType = std::map<const std::string, Source>;
    static MapType &get_map(Paths &paths) { return paths.sources_; }
};

}

template <typename T>
void AudioPath::Paths::add_item(T &&item)
{
    using Traits = AudioPath::AddItemTraits<T>;

    std::string key(item.id_);

    auto &map(Traits::get_map(*this));
    auto it(map.find(key));

    if(it == map.end())
        map.emplace(std::move(key), std::move(item));
    else
        it->second.take_proxy_from(item);
}

void AudioPath::Paths::add_player(AudioPath::Player &&player)
{
    add_item(std::move(player));
}

void AudioPath::Paths::add_source(AudioPath::Source &&source)
{
    add_item(std::move(source));
}

std::pair<const AudioPath::Source *, const AudioPath::Player *>
AudioPath::Paths::lookup(const char *source_id)
{
    auto source(sources_.find(source_id));

    if(source == sources_.end())
        return std::make_pair(nullptr, nullptr);

    auto player(players_.find(source->second.player_id_));

    if(player == players_.end())
        return std::make_pair(&source->second, nullptr);
    else
        return std::make_pair(&source->second, &player->second);
}
