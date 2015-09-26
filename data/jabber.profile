[Profile]
DisplayName = Jabber
IconName = general_jabber
Manager = gabble
Protocol = jabber
Priority = -20
VCardDefault = 1
VCardField = X-JABBER
Capabilities = chat-p2p, chat-room, chat-room-list, , , contact-search, registration-ui, supports-avatars, supports-alias, supports-roster
DefaultAccountDomain = jabber.org
Default-fallback-conference-server = conference.jabber.org
SupportedPresences = away,extended-away,hidden,do-not-disturb
SupportsPresenceMessage = true
ConfigurationUI = jabber-plugin

[KeepAlive]
ParamName = keepalive-interval
Value-WLAN_INFRA = 120
Value-GPRS = 600

[Presence available]
Name = pres_bd_jabber_online
IconName = general_presence_online
Type = 2

[Presence dnd]
Name = pres_bd_jabber_do_not_disturb
IconName = general_presence_busy
Type = 6

[Presence hidden]
Name = pres_bd_jabber_invisible
IconName = general_presence_invisible
Type = 5

[Presence offline]
Name = pres_bd_jabber_offline
IconName = general_presence_offline
Type = 1

[Action chat]
Name = addr_bd_cont_starter_im_service_chat
IconName = general_sms
VCardFields = X-JABBER
prop-org.freedesktop.Telepathy.Channel.ChannelType-s = org.freedesktop.Telepathy.Channel.Type.Text

[Action call]
Name = addr_bd_cont_starter_im_service_call
IconName = general_call
VCardFields = X-JABBER
prop-org.freedesktop.Telepathy.Channel.ChannelType-s = org.freedesktop.Telepathy.Channel.Type.StreamedMedia

