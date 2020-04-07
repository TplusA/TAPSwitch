# T+A Player Switch daemon

## Copyright and contact

T+A Player Switch (_TAPSwitch_) is released under the terms of the GNU
General Public License version 2 or (at your option) any later version.
See file <tt>COPYING</tt> for licensing terms of the GNU General Public
License version 2, or <tt>COPYING.GPLv3</tt> for licensing terms of the
GNU General Public License version 3.

Contact:

    T+A elektroakustik GmbH & Co. KG
    Planckstrasse 11
    32052 Herford
    Germany

## Short description

There are multiple audio players in the Streaming Board system that need
exclusive access to the sound device. Depending on the audio source, a specific
audio player must be activated so that the source can be played. Since no
software mixing (thus on-the-fly format conversion) shall be done, the players
need to be told explicitly when they may access the audio device, and when
they must release it.

The _tapswitch_ daemon manages switching between players based on audio source.
It tells the matching player that it may take the audio device, and tells other
players that they may not.

## Communication with other system process

Each **audio player** must register with _tapswitch_ so that it can be informed
when it may take the audio device. An audio player is considered _active_ if it
is allowed to take the audio device, _inactive_ otherwise. If _tapswitch_ tells
an audio player to deactivate via D-Bus method invocation, then the player
_must_ release the audio device before the method invocation returns. If
_tapswitch_ tells an audio player to activate, then it is allowed (but not
required) to use the audio device.

A D-Bus signal is emitted after a player has registered.

Any process may register one or more **audio sources** with _tapswitch_. An
audio source is associated with a player, forming an audio path from the
audio source to the audio player (and thus to the audio device).
Multiple audio sources may be associated with the same player.
Processes can request _tapswitch_ to activate a player by passing the name of
an audio source; then, _tapswitch_ will coordinate players so that the correct
one gets activated. The registered audio source is notified with a D-Bus method
invocation when its player is ready.

Note that as far as _tapswitch_ is concerned, an audio **source** is just a
concept to introduce an extra level of indirection to the system, decoupling
logical audio sources from audio **players**. This way, processes that would
like to activate some audio source do not need to know which audio player is
responsible for the audio source in question. To _tapswitch_, an audio source
is simply a string ID with no further meaning.

## Communication with appliances

Before the Streaming Board can play anything, the appliance the Streaming Board
is built into may have to set up its audio hardware (waking up from sleep
state, switching relays, switch on more appliances, warming up components,
anything like that) before any audible sound can be produced. Therefore, an
audio player must wait for the appliance to become ready, i.e., its activation
must be synchronized to any delays introduced by the appliance.

To achieve this, _tapswitch_ can be informed about the state of the audio
hardware via D-Bus commands. As long as the hardware is not ready, it will
defer the activation of any audio player. Likely, the hardware state methods
will be called by _dcpd_.
