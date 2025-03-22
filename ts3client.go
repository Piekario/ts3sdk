// Package ts3sdk provides Go bindings for the TeamSpeak 3 Client SDK.
//
// This package uses cgo to call the native TeamSpeak 3 Client SDK functions.
// It provides a more Go-friendly API for interacting with TeamSpeak 3 servers.
package ts3sdk

/*
#cgo windows LDFLAGS: -L${SRCDIR}/ts_sdk_3.3.1/lib/windows -lts3client_win64
#cgo windows CFLAGS: -I${SRCDIR}/ts_sdk_3.3.1/include

#include <stdlib.h>
#include <teamspeak/clientlib.h>
#include <teamspeak/public_definitions.h>
#include <teamspeak/public_errors.h>
#include <teamlog/logtypes.h>
*/
import "C"
import (
	"fmt"
	"runtime"
	"unsafe"
)

// Error represents a TeamSpeak 3 error code
type Error int

// Error implements the error interface
func (e Error) Error() string {
	if e == 0 {
		return "ok"
	}

	var errorMsg *C.char
	C.ts3client_getErrorMessage(C.uint(e), &errorMsg)
	defer C.ts3client_freeMemory(unsafe.Pointer(errorMsg))
	return fmt.Sprintf("TeamSpeak error %d: %s", int(e), C.GoString(errorMsg))
}

// Common error codes
const (
	ErrorOK                  = Error(C.ERROR_ok)
	ErrorUndefined           = Error(C.ERROR_undefined)
	ErrorNotImplemented      = Error(C.ERROR_not_implemented)
	ErrorOkNoUpdate          = Error(C.ERROR_ok_no_update)
	ErrorDontNotify          = Error(C.ERROR_dont_notify)
	ErrorLibTimeLimitReached = Error(C.ERROR_lib_time_limit_reached)
	ErrorOutOfMemory         = Error(C.ERROR_out_of_memory)
)

// ConnectStatus represents the connection status
type ConnectStatus int

// Connection status constants
const (
	StatusDisconnected           ConnectStatus = ConnectStatus(C.STATUS_DISCONNECTED)
	StatusConnecting             ConnectStatus = ConnectStatus(C.STATUS_CONNECTING)
	StatusConnected              ConnectStatus = ConnectStatus(C.STATUS_CONNECTED)
	StatusConnectionEstablishing ConnectStatus = ConnectStatus(C.STATUS_CONNECTION_ESTABLISHING)
	StatusConnectionEstablished  ConnectStatus = ConnectStatus(C.STATUS_CONNECTION_ESTABLISHED)
)

// ConnectionHandlerID represents a server connection handler ID
type ConnectionHandlerID uint64

// ClientID represents a client ID
type ClientID uint16

// ChannelID represents a channel ID
type ChannelID uint64

// Initialize initializes the TeamSpeak 3 Client SDK
func Initialize(clientLibPath, resourcePath string, logTypes int) error {
	cClientLibPath := C.CString(clientLibPath)
	defer C.free(unsafe.Pointer(cClientLibPath))

	cResourcePath := C.CString(resourcePath)
	defer C.free(unsafe.Pointer(cResourcePath))

	err := C.ts3client_initClientLib(C.int(logTypes), cClientLibPath, cResourcePath)
	if err != C.ERROR_ok {
		return Error(err)
	}

	// Set up finalizer to ensure we clean up
	runtime.SetFinalizer(&struct{}{}, func(_ interface{}) {
		C.ts3client_destroyClientLib()
	})

	return nil
}

// Shutdown shuts down the TeamSpeak 3 Client SDK
func Shutdown() error {
	err := C.ts3client_destroyClientLib()
	if err != C.ERROR_ok {
		return Error(err)
	}
	return nil
}

// CreateServerConnectionHandler creates a new server connection handler
func CreateServerConnectionHandler() (ConnectionHandlerID, error) {
	var serverConnectionHandlerID C.uint64
	err := C.ts3client_spawnNewServerConnectionHandler(0, &serverConnectionHandlerID)
	if err != C.ERROR_ok {
		return 0, Error(err)
	}
	return ConnectionHandlerID(serverConnectionHandlerID), nil
}

// DestroyServerConnectionHandler destroys a server connection handler
func DestroyServerConnectionHandler(serverConnectionHandlerID ConnectionHandlerID) error {
	err := C.ts3client_destroyServerConnectionHandler(C.uint64(serverConnectionHandlerID))
	if err != C.ERROR_ok {
		return Error(err)
	}
	return nil
}

// GetClientLibVersion returns the version of the TeamSpeak 3 Client SDK
func GetClientLibVersion() (string, error) {
	var version *C.char
	err := C.ts3client_getClientLibVersion(&version)
	if err != C.ERROR_ok {
		return "", Error(err)
	}
	defer C.ts3client_freeMemory(unsafe.Pointer(version))
	return C.GoString(version), nil
}

// StartConnection starts a connection to a TeamSpeak 3 server
func StartConnection(serverConnectionHandlerID ConnectionHandlerID, identity, ip string, port uint16, nickname, defaultChannelPassword, serverPassword string) error {
	cIdentity := C.CString(identity)
	defer C.free(unsafe.Pointer(cIdentity))

	cIP := C.CString(ip)
	defer C.free(unsafe.Pointer(cIP))

	cNickname := C.CString(nickname)
	defer C.free(unsafe.Pointer(cNickname))

	cDefaultChannelPassword := C.CString(defaultChannelPassword)
	defer C.free(unsafe.Pointer(cDefaultChannelPassword))

	cServerPassword := C.CString(serverPassword)
	defer C.free(unsafe.Pointer(cServerPassword))

	err := C.ts3client_startConnection(
		C.uint64(serverConnectionHandlerID),
		cIdentity,
		cIP,
		C.uint(port),
		cNickname,
		nil, // default channel
		cDefaultChannelPassword,
		cServerPassword,
	)
	if err != C.ERROR_ok {
		return Error(err)
	}
	return nil
}

// StopConnection stops a connection to a TeamSpeak 3 server
func StopConnection(serverConnectionHandlerID ConnectionHandlerID, quitMessage string) error {
	cQuitMessage := C.CString(quitMessage)
	defer C.free(unsafe.Pointer(cQuitMessage))

	err := C.ts3client_stopConnection(C.uint64(serverConnectionHandlerID), cQuitMessage)
	if err != C.ERROR_ok {
		return Error(err)
	}
	return nil
}

// GetConnectionStatus returns the connection status
func GetConnectionStatus(serverConnectionHandlerID ConnectionHandlerID) (ConnectStatus, error) {
	var status C.int
	err := C.ts3client_getConnectionStatus(C.uint64(serverConnectionHandlerID), &status)
	if err != C.ERROR_ok {
		return 0, Error(err)
	}
	return ConnectStatus(status), nil
}

// RequestClientMove requests to move a client to another channel
func RequestClientMove(serverConnectionHandlerID ConnectionHandlerID, clientID ClientID, newChannelID ChannelID, password string) error {
	cPassword := C.CString(password)
	defer C.free(unsafe.Pointer(cPassword))

	err := C.ts3client_requestClientMove(
		C.uint64(serverConnectionHandlerID),
		C.anyID(clientID),
		C.uint64(newChannelID),
		cPassword,
		nil,
	)
	if err != C.ERROR_ok {
		return Error(err)
	}
	return nil
}

// RequestSendPrivateTextMsg sends a private text message to a client
func RequestSendPrivateTextMsg(serverConnectionHandlerID ConnectionHandlerID, message string, targetClientID ClientID) error {
	cMessage := C.CString(message)
	defer C.free(unsafe.Pointer(cMessage))

	err := C.ts3client_requestSendPrivateTextMsg(
		C.uint64(serverConnectionHandlerID),
		cMessage,
		C.anyID(targetClientID),
		nil,
	)
	if err != C.ERROR_ok {
		return Error(err)
	}
	return nil
}

// RequestSendChannelTextMsg sends a text message to a channel
func RequestSendChannelTextMsg(serverConnectionHandlerID ConnectionHandlerID, message string, targetChannelID ChannelID) error {
	cMessage := C.CString(message)
	defer C.free(unsafe.Pointer(cMessage))

	err := C.ts3client_requestSendChannelTextMsg(
		C.uint64(serverConnectionHandlerID),
		cMessage,
		C.uint64(targetChannelID),
		nil,
	)
	if err != C.ERROR_ok {
		return Error(err)
	}
	return nil
}

// RequestSendServerTextMsg sends a text message to the server
func RequestSendServerTextMsg(serverConnectionHandlerID ConnectionHandlerID, message string) error {
	cMessage := C.CString(message)
	defer C.free(unsafe.Pointer(cMessage))

	err := C.ts3client_requestSendServerTextMsg(
		C.uint64(serverConnectionHandlerID),
		cMessage,
		nil,
	)
	if err != C.ERROR_ok {
		return Error(err)
	}
	return nil
}

// GetClientID returns the client ID of the local client
func GetClientID(serverConnectionHandlerID ConnectionHandlerID) (ClientID, error) {
	var clientID C.anyID
	err := C.ts3client_getClientID(C.uint64(serverConnectionHandlerID), &clientID)
	if err != C.ERROR_ok {
		return 0, Error(err)
	}
	return ClientID(clientID), nil
}
