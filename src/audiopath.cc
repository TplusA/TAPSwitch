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

#include <algorithm>

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
const T &AudioPath::Paths::add_item(T &&item, bool &inserted)
{
    using Traits = AudioPath::AddItemTraits<T>;

    std::string key(item.id_);

    auto &map(Traits::get_map(*this));
    auto it(map.find(key));

    if(it == map.end())
    {
        inserted = true;
        const auto result(map.emplace(std::move(key), std::move(item)));
        return result.first->second;
    }
    else
    {
        inserted = false;
        it->second.take_proxy_from(item);
        return it->second;
    }
}

AudioPath::Paths::AddResult
AudioPath::Paths::add_player(AudioPath::Player &&player)
{
    bool inserted;
    const auto &p(add_item(std::move(player), inserted));
    const bool have_path = std::find_if(sources_.begin(), sources_.end(),
                                        [&p] (const decltype(Paths::sources_)::value_type &src)
                                        {
                                            return src.second.player_id_ == p.id_;
                                        }) != sources_.end();


    if(!inserted)
        return have_path ? AddResult::UPDATED_PATH : AddResult::UPDATED_COMPONENT;
    else
        return have_path ? AddResult::NEW_PATH : AddResult::NEW_COMPONENT;
}

AudioPath::Paths::AddResult
AudioPath::Paths::add_source(AudioPath::Source &&source)
{
    bool inserted;
    const auto &s(add_item(std::move(source), inserted));
    const bool have_path(players_.find(s.player_id_) != players_.end());

    if(!inserted)
        return have_path ? AddResult::UPDATED_PATH : AddResult::UPDATED_COMPONENT;
    else
        return have_path ? AddResult::NEW_PATH : AddResult::NEW_COMPONENT;
}

const AudioPath::Player *
AudioPath::Paths::lookup_player(const std::string &player_id) const
{
    auto player(players_.find(player_id));
    return (player != players_.end()) ? &player->second : nullptr;
}

const AudioPath::Source *
AudioPath::Paths::lookup_source(const std::string &source_id) const
{
    auto source(sources_.find(source_id));
    return (source != sources_.end()) ? &source->second : nullptr;
}

AudioPath::Paths::Path
AudioPath::Paths::lookup_path(const std::string &source_id) const
{
    const auto *source(lookup_source(source_id));

    if(source == nullptr)
        return std::make_pair(nullptr, nullptr);

    auto player(players_.find(source->player_id_));

    if(player == players_.end())
        return std::make_pair(source, nullptr);
    else
        return std::make_pair(source, &player->second);
}

void AudioPath::Paths::for_each(const std::function<void(const AudioPath::Paths::Path &)> &apply,
                                AudioPath::Paths::ForEach mode) const
{
    AudioPath::Paths::Path temp;

    if(mode != ForEach::UNCONNECTED_PLAYERS)
    {
        for(const auto &s : sources_)
        {
            const auto p(players_.find(s.second.player_id_));

            if(p != players_.end())
            {
                switch(mode)
                {
                  case ForEach::ANY:
                  case ForEach::COMPLETE_PATHS:
                    temp.first = &s.second;
                    temp.second = &p->second;
                    apply(temp);
                    break;

                  case ForEach::INCOMPLETE_PATHS:
                  case ForEach::UNCONNECTED_SOURCES:
                  case ForEach::UNCONNECTED_PLAYERS:
                    break;
                }
            }
            else
            {
                switch(mode)
                {
                  case ForEach::ANY:
                  case ForEach::INCOMPLETE_PATHS:
                  case ForEach::UNCONNECTED_SOURCES:
                    temp.first = &s.second;
                    temp.second = nullptr;
                    apply(temp);
                    break;

                  case ForEach::COMPLETE_PATHS:
                  case ForEach::UNCONNECTED_PLAYERS:
                    break;
                }
            }
        }
    }

    switch(mode)
    {
      case ForEach::ANY:
      case ForEach::INCOMPLETE_PATHS:
      case ForEach::UNCONNECTED_PLAYERS:
        for(const auto &p : players_)
        {
            if(std::find_if(sources_.begin(), sources_.end(),
                            [&p] (const decltype(Paths::sources_)::value_type &src)
                            {
                                return src.second.player_id_ == p.second.id_;
                            }) == sources_.end())
            {
                temp.first = nullptr;
                temp.second = &p.second;
                apply(temp);
            }
        }

        break;

      case ForEach::COMPLETE_PATHS:
      case ForEach::UNCONNECTED_SOURCES:
        break;
    }
}
