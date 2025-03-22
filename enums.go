// Package ts3sdk provides Go bindings for the TeamSpeak 3 Client SDK.
package ts3sdk

/*
#include <teamspeak/public_definitions.h>
#include <teamspeak/public_errors.h>
#include <teamlog/logtypes.h>
*/
import "C"

// Visibility enum values
const (
	EnterVisibility  = int(C.ENTER_VISIBILITY)
	RetainVisibility = int(C.RETAIN_VISIBILITY)
	LeaveVisibility  = int(C.LEAVE_VISIBILITY)
)

// LogTypes enum values
const (
	LogTypeNone         = int(C.LogType_NONE)
	LogTypeFile         = int(C.LogType_FILE)
	LogTypeConsole      = int(C.LogType_CONSOLE)
	LogTypeUserLogging  = int(C.LogType_USERLOGGING)
	LogTypeNoNetLogging = int(C.LogType_NO_NETLOGGING)
	LogTypeDatabase     = int(C.LogType_DATABASE)
	LogTypeSyslog       = int(C.LogType_SYSLOG)
)

// LogLevel enum values
const (
	LogLevelCritical = int(C.LogLevel_CRITICAL)
	LogLevelError    = int(C.LogLevel_ERROR)
	LogLevelWarning  = int(C.LogLevel_WARNING)
	LogLevelDebug    = int(C.LogLevel_DEBUG)
	LogLevelInfo     = int(C.LogLevel_INFO)
	LogLevelDevel    = int(C.LogLevel_DEVEL)
)

// TalkStatus enum values
const (
	StatusNotTalking           = int(C.STATUS_NOT_TALKING)
	StatusTalking              = int(C.STATUS_TALKING)
	StatusTalkingWhileDisabled = int(C.STATUS_TALKING_WHILE_DISABLED)
)

// CodecType enum values
const (
	CodecSpeexNarrowband    = int(C.CODEC_SPEEX_NARROWBAND)
	CodecSpeexWideband      = int(C.CODEC_SPEEX_WIDEBAND)
	CodecSpeexUltrawideband = int(C.CODEC_SPEEX_ULTRAWIDEBAND)
	CodecCeltMono           = int(C.CODEC_CELT_MONO)
	CodecOpusVoice          = int(C.CODEC_OPUS_VOICE)
	CodecOpusMusic          = int(C.CODEC_OPUS_MUSIC)
)

// CodecEncryptionMode enum values
const (
	CodecEncryptionPerChannel = int(C.CODEC_ENCRYPTION_PER_CHANNEL)
	CodecEncryptionForcedOff  = int(C.CODEC_ENCRYPTION_FORCED_OFF)
	CodecEncryptionForcedOn   = int(C.CODEC_ENCRYPTION_FORCED_ON)
)

// TextMessageTargetMode enum values
const (
	TextMessageTargetClient  = int(C.TextMessageTarget_CLIENT)
	TextMessageTargetChannel = int(C.TextMessageTarget_CHANNEL)
	TextMessageTargetServer  = int(C.TextMessageTarget_SERVER)
	TextMessageTargetMax     = int(C.TextMessageTarget_MAX)
)

// MuteInputStatus enum values
const (
	MuteInputNone  = int(C.MUTEINPUT_NONE)
	MuteInputMuted = int(C.MUTEINPUT_MUTED)
)

// MuteOutputStatus enum values
const (
	MuteOutputNone  = int(C.MUTEOUTPUT_NONE)
	MuteOutputMuted = int(C.MUTEOUTPUT_MUTED)
)

// HardwareInputStatus enum values
const (
	HardwareInputDisabled = int(C.HARDWAREINPUT_DISABLED)
	HardwareInputEnabled  = int(C.HARDWAREINPUT_ENABLED)
)

// HardwareOutputStatus enum values
const (
	HardwareOutputDisabled = int(C.HARDWAREOUTPUT_DISABLED)
	HardwareOutputEnabled  = int(C.HARDWAREOUTPUT_ENABLED)
)

// InputDeactivationStatus enum values
const (
	InputActive      = int(C.INPUT_ACTIVE)
	InputDeactivated = int(C.INPUT_DEACTIVATED)
)

// ReasonIdentifier enum values
const (
	ReasonNone                           = int(C.REASON_NONE)
	ReasonMoved                          = int(C.REASON_MOVED)
	ReasonSubscription                   = int(C.REASON_SUBSCRIPTION)
	ReasonLostConnection                 = int(C.REASON_LOST_CONNECTION)
	ReasonKickChannel                    = int(C.REASON_KICK_CHANNEL)
	ReasonKickServer                     = int(C.REASON_KICK_SERVER)
	ReasonKickServerBan                  = int(C.REASON_KICK_SERVER_BAN)
	ReasonServerstop                     = int(C.REASON_SERVERSTOP)
	ReasonClientdisconnect               = int(C.REASON_CLIENTDISCONNECT)
	ReasonChannelupdate                  = int(C.REASON_CHANNELUPDATE)
	ReasonChanneledit                    = int(C.REASON_CHANNELEDIT)
	ReasonClientdisconnectServerShutdown = int(C.REASON_CLIENTDISCONNECT_SERVER_SHUTDOWN)
)

// ChannelProperties enum values
const (
	ChannelName               = int(C.CHANNEL_NAME)
	ChannelTopic              = int(C.CHANNEL_TOPIC)
	ChannelDescription        = int(C.CHANNEL_DESCRIPTION)
	ChannelPassword           = int(C.CHANNEL_PASSWORD)
	ChannelCodec              = int(C.CHANNEL_CODEC)
	ChannelCodecQuality       = int(C.CHANNEL_CODEC_QUALITY)
	ChannelMaxclients         = int(C.CHANNEL_MAXCLIENTS)
	ChannelMaxfamilyclients   = int(C.CHANNEL_MAXFAMILYCLIENTS)
	ChannelOrder              = int(C.CHANNEL_ORDER)
	ChannelFlagPermanent      = int(C.CHANNEL_FLAG_PERMANENT)
	ChannelFlagSemiPermanent  = int(C.CHANNEL_FLAG_SEMI_PERMANENT)
	ChannelFlagDefault        = int(C.CHANNEL_FLAG_DEFAULT)
	ChannelFlagPassword       = int(C.CHANNEL_FLAG_PASSWORD)
	ChannelCodecLatencyFactor = int(C.CHANNEL_CODEC_LATENCY_FACTOR)
	ChannelCodecIsUnencrypted = int(C.CHANNEL_CODEC_IS_UNENCRYPTED)
	ChannelSecuritySalt       = int(C.CHANNEL_SECURITY_SALT)
	ChannelDeleteDelay        = int(C.CHANNEL_DELETE_DELAY)
)
