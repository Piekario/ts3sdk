// Package ts3sdk provides Go bindings for the TeamSpeak 3 Client SDK.
package ts3sdk

/*
#include <stdlib.h>
#include <teamspeak/clientlib.h>
#include <teamspeak/public_definitions.h>
#include <teamspeak/public_errors.h>
*/
import "C"
import (
	"sync"
)

// Callback types for TeamSpeak 3 events
type (
	// ConnectStatusChangeCallback is called when the connection status changes
	ConnectStatusChangeCallback func(serverConnectionHandlerID ConnectionHandlerID, newStatus ConnectStatus, errorNumber Error)

	// ServerProtocolVersionCallback is called when the server protocol version is received
	ServerProtocolVersionCallback func(serverConnectionHandlerID ConnectionHandlerID, protocolVersion int)

	// NewChannelCallback is called when a new channel is discovered
	NewChannelCallback func(serverConnectionHandlerID ConnectionHandlerID, channelID ChannelID, channelParentID ChannelID)

	// NewChannelCreatedCallback is called when a new channel is created
	NewChannelCreatedCallback func(serverConnectionHandlerID ConnectionHandlerID, channelID ChannelID, channelParentID ChannelID, invokerID ClientID, invokerName string, invokerUniqueIdentifier string)

	// DelChannelCallback is called when a channel is deleted
	DelChannelCallback func(serverConnectionHandlerID ConnectionHandlerID, channelID ChannelID, invokerID ClientID, invokerName string, invokerUniqueIdentifier string)

	// ChannelMoveCallback is called when a channel is moved
	ChannelMoveCallback func(serverConnectionHandlerID ConnectionHandlerID, channelID ChannelID, newChannelParentID ChannelID, invokerID ClientID, invokerName string, invokerUniqueIdentifier string)

	// UpdateChannelCallback is called when channel data is updated
	UpdateChannelCallback func(serverConnectionHandlerID ConnectionHandlerID, channelID ChannelID)

	// UpdateChannelEditedCallback is called when a channel is edited
	UpdateChannelEditedCallback func(serverConnectionHandlerID ConnectionHandlerID, channelID ChannelID, invokerID ClientID, invokerName string, invokerUniqueIdentifier string)

	// UpdateClientCallback is called when client data is updated
	UpdateClientCallback func(serverConnectionHandlerID ConnectionHandlerID, clientID ClientID, invokerID ClientID, invokerName string, invokerUniqueIdentifier string)

	// ClientMoveCallback is called when a client moves to another channel
	ClientMoveCallback func(serverConnectionHandlerID ConnectionHandlerID, clientID ClientID, oldChannelID ChannelID, newChannelID ChannelID, visibility int, moveMessage string)

	// ClientMoveSubscriptionCallback is called when a client subscription changes
	ClientMoveSubscriptionCallback func(serverConnectionHandlerID ConnectionHandlerID, clientID ClientID, oldChannelID ChannelID, newChannelID ChannelID, visibility int)

	// ClientMoveTimeoutCallback is called when a client times out
	ClientMoveTimeoutCallback func(serverConnectionHandlerID ConnectionHandlerID, clientID ClientID, oldChannelID ChannelID, newChannelID ChannelID, visibility int, timeoutMessage string)

	// TalkStatusChangeCallback is called when a client's talk status changes
	TalkStatusChangeCallback func(serverConnectionHandlerID ConnectionHandlerID, status int, isReceivedWhisper int, clientID ClientID)

	// TextMessageCallback is called when a text message is received
	TextMessageCallback func(serverConnectionHandlerID ConnectionHandlerID, targetMode int, toID uint64, fromID ClientID, fromName string, fromUniqueIdentifier string, message string)
)

// Callbacks holds all the callback functions
type Callbacks struct {
	ConnectStatusChange    ConnectStatusChangeCallback
	ServerProtocolVersion  ServerProtocolVersionCallback
	NewChannel             NewChannelCallback
	NewChannelCreated      NewChannelCreatedCallback
	DelChannel             DelChannelCallback
	ChannelMove            ChannelMoveCallback
	UpdateChannel          UpdateChannelCallback
	UpdateChannelEdited    UpdateChannelEditedCallback
	UpdateClient           UpdateClientCallback
	ClientMove             ClientMoveCallback
	ClientMoveSubscription ClientMoveSubscriptionCallback
	ClientMoveTimeout      ClientMoveTimeoutCallback
	TalkStatusChange       TalkStatusChangeCallback
	TextMessage            TextMessageCallback
}

// Global callbacks instance and mutex to protect it
var (
	callbacks      Callbacks
	callbacksMutex sync.RWMutex
)

// SetClientCallbacks sets the callback functions for TeamSpeak 3 events
func SetClientCallbacks(cb Callbacks) error {
	callbacksMutex.Lock()
	defer callbacksMutex.Unlock()

	callbacks = cb

	// Create C callback struct
	var uiCallbacks C.struct_ClientUIFunctions

	// Set C callbacks to our exported Go functions
	uiCallbacks.onConnectStatusChangeEvent = C.onConnectStatusChangeEvent
	uiCallbacks.onServerProtocolVersionEvent = C.onServerProtocolVersionEvent
	uiCallbacks.onNewChannelEvent = C.onNewChannelEvent
	uiCallbacks.onNewChannelCreatedEvent = C.onNewChannelCreatedEvent
	uiCallbacks.onDelChannelEvent = C.onDelChannelEvent
	uiCallbacks.onChannelMoveEvent = C.onChannelMoveEvent
	uiCallbacks.onUpdateChannelEvent = C.onUpdateChannelEvent
	uiCallbacks.onUpdateChannelEditedEvent = C.onUpdateChannelEditedEvent
	uiCallbacks.onUpdateClientEvent = C.onUpdateClientEvent
	uiCallbacks.onClientMoveEvent = C.onClientMoveEvent
	uiCallbacks.onClientMoveSubscriptionEvent = C.onClientMoveSubscriptionEvent
	uiCallbacks.onClientMoveTimeoutEvent = C.onClientMoveTimeoutEvent
	uiCallbacks.onTalkStatusChangeEvent = C.onTalkStatusChangeEvent
	uiCallbacks.onTextMessageEvent = C.onTextMessageEvent

	// Register callbacks with TeamSpeak 3 SDK
	err := C.ts3client_registerClientUIFunctions(&uiCallbacks, nil)
	if err != C.ERROR_ok {
		return Error(err)
	}

	return nil
}

//export onConnectStatusChangeEvent
func onConnectStatusChangeEvent(serverConnectionHandlerID C.uint64, newStatus C.int, errorNumber C.uint) {
	callbacksMutex.RLock()
	defer callbacksMutex.RUnlock()

	if callbacks.ConnectStatusChange != nil {
		callbacks.ConnectStatusChange(
			ConnectionHandlerID(serverConnectionHandlerID),
			ConnectStatus(newStatus),
			Error(errorNumber),
		)
	}
}

//export onServerProtocolVersionEvent
func onServerProtocolVersionEvent(serverConnectionHandlerID C.uint64, protocolVersion C.int) {
	callbacksMutex.RLock()
	defer callbacksMutex.RUnlock()

	if callbacks.ServerProtocolVersion != nil {
		callbacks.ServerProtocolVersion(
			ConnectionHandlerID(serverConnectionHandlerID),
			int(protocolVersion),
		)
	}
}

//export onNewChannelEvent
func onNewChannelEvent(serverConnectionHandlerID C.uint64, channelID C.uint64, channelParentID C.uint64) {
	callbacksMutex.RLock()
	defer callbacksMutex.RUnlock()

	if callbacks.NewChannel != nil {
		callbacks.NewChannel(
			ConnectionHandlerID(serverConnectionHandlerID),
			ChannelID(channelID),
			ChannelID(channelParentID),
		)
	}
}

//export onNewChannelCreatedEvent
func onNewChannelCreatedEvent(serverConnectionHandlerID C.uint64, channelID C.uint64, channelParentID C.uint64, invokerID C.anyID, invokerName *C.char, invokerUniqueIdentifier *C.char) {
	callbacksMutex.RLock()
	defer callbacksMutex.RUnlock()

	if callbacks.NewChannelCreated != nil {
		callbacks.NewChannelCreated(
			ConnectionHandlerID(serverConnectionHandlerID),
			ChannelID(channelID),
			ChannelID(channelParentID),
			ClientID(invokerID),
			C.GoString(invokerName),
			C.GoString(invokerUniqueIdentifier),
		)
	}
}

//export onDelChannelEvent
func onDelChannelEvent(serverConnectionHandlerID C.uint64, channelID C.uint64, invokerID C.anyID, invokerName *C.char, invokerUniqueIdentifier *C.char) {
	callbacksMutex.RLock()
	defer callbacksMutex.RUnlock()

	if callbacks.DelChannel != nil {
		callbacks.DelChannel(
			ConnectionHandlerID(serverConnectionHandlerID),
			ChannelID(channelID),
			ClientID(invokerID),
			C.GoString(invokerName),
			C.GoString(invokerUniqueIdentifier),
		)
	}
}

//export onChannelMoveEvent
func onChannelMoveEvent(serverConnectionHandlerID C.uint64, channelID C.uint64, newChannelParentID C.uint64, invokerID C.anyID, invokerName *C.char, invokerUniqueIdentifier *C.char) {
	callbacksMutex.RLock()
	defer callbacksMutex.RUnlock()

	if callbacks.ChannelMove != nil {
		callbacks.ChannelMove(
			ConnectionHandlerID(serverConnectionHandlerID),
			ChannelID(channelID),
			ChannelID(newChannelParentID),
			ClientID(invokerID),
			C.GoString(invokerName),
			C.GoString(invokerUniqueIdentifier),
		)
	}
}

//export onUpdateChannelEvent
func onUpdateChannelEvent(serverConnectionHandlerID C.uint64, channelID C.uint64) {
	callbacksMutex.RLock()
	defer callbacksMutex.RUnlock()

	if callbacks.UpdateChannel != nil {
		callbacks.UpdateChannel(
			ConnectionHandlerID(serverConnectionHandlerID),
			ChannelID(channelID),
		)
	}
}

//export onUpdateChannelEditedEvent
func onUpdateChannelEditedEvent(serverConnectionHandlerID C.uint64, channelID C.uint64, invokerID C.anyID, invokerName *C.char, invokerUniqueIdentifier *C.char) {
	callbacksMutex.RLock()
	defer callbacksMutex.RUnlock()

	if callbacks.UpdateChannelEdited != nil {
		callbacks.UpdateChannelEdited(
			ConnectionHandlerID(serverConnectionHandlerID),
			ChannelID(channelID),
			ClientID(invokerID),
			C.GoString(invokerName),
			C.GoString(invokerUniqueIdentifier),
		)
	}
}

//export onUpdateClientEvent
func onUpdateClientEvent(serverConnectionHandlerID C.uint64, clientID C.anyID, invokerID C.anyID, invokerName *C.char, invokerUniqueIdentifier *C.char) {
	callbacksMutex.RLock()
	defer callbacksMutex.RUnlock()

	if callbacks.UpdateClient != nil {
		callbacks.UpdateClient(
			ConnectionHandlerID(serverConnectionHandlerID),
			ClientID(clientID),
			ClientID(invokerID),
			C.GoString(invokerName),
			C.GoString(invokerUniqueIdentifier),
		)
	}
}

//export onClientMoveEvent
func onClientMoveEvent(serverConnectionHandlerID C.uint64, clientID C.anyID, oldChannelID C.uint64, newChannelID C.uint64, visibility C.int, moveMessage *C.char) {
	callbacksMutex.RLock()
	defer callbacksMutex.RUnlock()

	if callbacks.ClientMove != nil {
		callbacks.ClientMove(
			ConnectionHandlerID(serverConnectionHandlerID),
			ClientID(clientID),
			ChannelID(oldChannelID),
			ChannelID(newChannelID),
			int(visibility),
			C.GoString(moveMessage),
		)
	}
}

//export onClientMoveSubscriptionEvent
func onClientMoveSubscriptionEvent(serverConnectionHandlerID C.uint64, clientID C.anyID, oldChannelID C.uint64, newChannelID C.uint64, visibility C.int) {
	callbacksMutex.RLock()
	defer callbacksMutex.RUnlock()

	if callbacks.ClientMoveSubscription != nil {
		callbacks.ClientMoveSubscription(
			ConnectionHandlerID(serverConnectionHandlerID),
			ClientID(clientID),
			ChannelID(oldChannelID),
			ChannelID(newChannelID),
			int(visibility),
		)
	}
}

//export onClientMoveTimeoutEvent
func onClientMoveTimeoutEvent(serverConnectionHandlerID C.uint64, clientID C.anyID, oldChannelID C.uint64, newChannelID C.uint64, visibility C.int, timeoutMessage *C.char) {
	callbacksMutex.RLock()
	defer callbacksMutex.RUnlock()

	if callbacks.ClientMoveTimeout != nil {
		callbacks.ClientMoveTimeout(
			ConnectionHandlerID(serverConnectionHandlerID),
			ClientID(clientID),
			ChannelID(oldChannelID),
			ChannelID(newChannelID),
			int(visibility),
			C.GoString(timeoutMessage),
		)
	}
}

//export onTalkStatusChangeEvent
func onTalkStatusChangeEvent(serverConnectionHandlerID C.uint64, status C.int, isReceivedWhisper C.int, clientID C.anyID) {
	callbacksMutex.RLock()
	defer callbacksMutex.RUnlock()

	if callbacks.TalkStatusChange != nil {
		callbacks.TalkStatusChange(
			ConnectionHandlerID(serverConnectionHandlerID),
			int(status),
			int(isReceivedWhisper),
			ClientID(clientID),
		)
	}
}

//export onTextMessageEvent
func onTextMessageEvent(serverConnectionHandlerID C.uint64, targetMode C.anyID, toID C.uint64, fromID C.anyID, fromName *C.char, fromUniqueIdentifier *C.char, message *C.char) {
	callbacksMutex.RLock()
	defer callbacksMutex.RUnlock()

	if callbacks.TextMessage != nil {
		callbacks.TextMessage(
			ConnectionHandlerID(serverConnectionHandlerID),
			int(targetMode),
			uint64(toID),
			ClientID(fromID),
			C.GoString(fromName),
			C.GoString(fromUniqueIdentifier),
			C.GoString(message),
		)
	}
}
