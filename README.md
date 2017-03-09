# T+A Player Switch daemon

## Copyright and contact

T+A Player Switch (_TAPSwitch_) is released under the terms of the GNU General
Public License version 3 (GPLv3). See file <tt>COPYING</tt> for licensing
terms.

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
need to be told explicitly when Athey may access the audio device, and when
they must release it.

The _tapswitch_ daemon manages switching between players based on audio source.
It tells the matching player that it may take the audio device, and tells other
players that they may not.

## Communication with other system process

Each audio player must register with _tapswitch_ so that it can be informed
when it may take the audio device. An audio player is considered _active_ if it
is allowed to take the audio device, _inactive_ otherwise. If _tapswitch_ tells
an audio player to deactivate via D-Bus method invocation, then the player
_must_ release the audio device before the method invocation returns. If
_tapswitch_ tells an audio player to active, then it is allowed (not required)
to use the audio device.

A D-Bus signal is emitted after a player has registered.

Any process may register one or more audio sources with _tapswitch_. An audio
source is associated with a player, forming an audio path from the source to
the player (and thus to the audio device). Multiple audio sources may be
associated with the same player. Processes can request _tapswitch_ to activate
the player for a given source, and _tapswitch_ will coordinate players so that
the correct one gets activated. The registered source is notified with a D-Bus
method invocation when its player is ready.

Note that an audio source is just a concept used by _tapswitch_ to introduce an
extra level of indirection. This way, processes that would like to activate
some source do not need to know which audio player is responsible for the
source in question. To _tapswitch_, an audio source is simply a string ID with
no further meaning.
