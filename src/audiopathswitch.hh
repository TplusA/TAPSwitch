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

#ifndef AUDIOPATHSWITCH_HH
#define AUDIOPATHSWITCH_HH

#include <string>

#include "gvariantwrapper.hh"

/*!
 * \addtogroup audiopath
 */
/*!@{*/

namespace AudioPath
{

class Paths;

class Switch
{
  public:
    enum class ActivateResult
    {
        ERROR_SOURCE_UNKNOWN,
        ERROR_SOURCE_FAILED,
        ERROR_PLAYER_UNKNOWN,
        ERROR_PLAYER_FAILED,
        OK_UNCHANGED,
        OK_PLAYER_SAME,
        OK_PLAYER_SAME_SOURCE_DEFERRED,
        OK_PLAYER_SWITCHED,
        OK_PLAYER_SWITCHED_SOURCE_DEFERRED,
    };

    enum class ReleaseResult
    {
        SOURCE_DESELECTED,
        PLAYER_DEACTIVATED,
        COMPLETE_RELEASE,
        UNCHANGED,
    };

    struct PendingActivation
    {
      private:
        /*!
         * ID of audio source to select when appliance gets ready.
         *
         * This ID is only non-empty while waiting for the appliance to get
         * ready to produce some audio. It is filled in when trying to activate
         * an audio source at a time the appliance is in some state not yet
         * suitable for playback (sleep mode or something like that).
         */
        std::string source_id_;

        /*!
         * Data passed by caller when requesting an audio source.
         *
         * These data are forwarded to the player on activation and
         * deactivation, and also to the audio source when selection is
         * finished or on hold. We do not look inside the data, we only forward
         * them where needed.
         */
        GVariantWrapper request_data_;

        /*!
         * Stores the result of the first phase of audio path switch.
         *
         * This is the result of the player deactivation/activation part of
         * audio path switching.
         */
        ActivateResult phase_one_result_;

      public:
        PendingActivation(const PendingActivation &) = delete;
        PendingActivation &operator=(const PendingActivation &) = delete;

        explicit PendingActivation():
            phase_one_result_(ActivateResult::ERROR_SOURCE_UNKNOWN)
        {}

        void set(const std::string &source_id, GVariantWrapper &&request_data,
                 ActivateResult result)
        {
            source_id_ = source_id;
            request_data_ = std::move(request_data);
            phase_one_result_ = result;
        }

        GVariantWrapper clear()
        {
            source_id_.clear();
            phase_one_result_ = ActivateResult::ERROR_SOURCE_UNKNOWN;

            auto reqdata(std::move(request_data_));
            return reqdata;
        }

        bool have_pending_activation() const { return !source_id_.empty(); }
        const std::string &get_audio_source_id() const { return source_id_; }
        ActivateResult get_phase_one_result() const { return phase_one_result_; }

        void take_audio_source_id(std::string &dest) { return dest.swap(source_id_); }
    };

  private:
    std::string current_source_id_;
    std::string current_player_id_;

    /*!
     * State while waiting for the appliance to get ready.
     */
    PendingActivation pending_;

  public:
    Switch(const Switch &) = delete;
    Switch &operator=(const Switch &) = delete;

    explicit Switch() {}

    ActivateResult activate_source(const Paths &paths, const char *source_id,
                                   const std::string *&player_id,
                                   bool select_source_now);

    ActivateResult activate_source(const Paths &paths, const char *source_id,
                                   const std::string *&player_id,
                                   bool select_source_now,
                                   GVariantWrapper &&request_data);

    /*!
     * Try to complete a deferred audio path activation.
     *
     * Note that at this point the return values for deferred audio path
     * activation are to be considered equivalent to the values for completed
     * audio path activation. That is, the caller must handle
     * #AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED_SOURCE_DEFERRED
     * the same way as #AudioPath::Switch::ActivateResult::OK_PLAYER_SWITCHED
     * (same for #AudioPath::Switch::ActivateResult::OK_PLAYER_SAME).
     */
    ActivateResult complete_pending_source_activation(const Paths &paths,
                                                      std::string *source_id);

    /*!
     * Cancel deferred audio path activation, if any.
     */
    ActivateResult cancel_pending_source_activation(const Paths &paths,
                                                    std::string &source_id);

    ReleaseResult release_path(const Paths &paths, bool kill_player,
                               const std::string *&player_id);

    ReleaseResult release_path(const Paths &paths, bool kill_player,
                               const std::string *&player_id,
                               GVariantWrapper &&request_data);

    const std::string &get_player_id() const { return current_player_id_; }
};

}

/*!@}*/

#endif /* !AUDIOPATHSWITCH_HH */
