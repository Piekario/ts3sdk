// Example program demonstrating the use of the TeamSpeak 3 Client SDK Go wrapper
package main

import (
	"fmt"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/Piekario/ts3sdk"
)

func main() {
	// Initialize the TeamSpeak 3 Client SDK
	// The first parameter is the path to the client library
	// The second parameter is the resource path
	// The third parameter is the log types
	err := ts3sdk.Initialize("", "", ts3sdk.LogTypeConsole)
	if err != nil {
		fmt.Printf("Error initializing TeamSpeak 3 Client SDK: %s\n", err)
		return
	}
	defer ts3sdk.Shutdown()

	// Get the version of the TeamSpeak 3 Client SDK
	version, err := ts3sdk.GetClientLibVersion()
	if err != nil {
		fmt.Printf("Error getting TeamSpeak 3 Client SDK version: %s\n", err)
		return
	}
	fmt.Printf("TeamSpeak 3 Client SDK version: %s\n", version)

	// Create a server connection handler
	serverConnectionHandlerID, err := ts3sdk.CreateServerConnectionHandler()
	if err != nil {
		fmt.Printf("Error creating server connection handler: %s\n", err)
		return
	}
	defer ts3sdk.DestroyServerConnectionHandler(serverConnectionHandlerID)

	// Set up callbacks
	callbacks := ts3sdk.Callbacks{
		ConnectStatusChange: func(serverConnectionHandlerID ts3sdk.ConnectionHandlerID, newStatus ts3sdk.ConnectStatus, errorNumber ts3sdk.Error) {
			fmt.Printf("Connect status changed: %d, error: %s\n", newStatus, errorNumber)
			if newStatus == ts3sdk.StatusConnectionEstablished {
				fmt.Println("Connection established!")
			}
		},
		TextMessage: func(serverConnectionHandlerID ts3sdk.ConnectionHandlerID, targetMode int, toID uint64, fromID ts3sdk.ClientID, fromName string, fromUniqueIdentifier string, message string) {
			fmt.Printf("Received message from %s (%d): %s\n", fromName, fromID, message)
		},
		TalkStatusChange: func(serverConnectionHandlerID ts3sdk.ConnectionHandlerID, status int, isReceivedWhisper int, clientID ts3sdk.ClientID) {
			if status == ts3sdk.StatusTalking {
				fmt.Printf("Client %d started talking\n", clientID)
			} else if status == ts3sdk.StatusNotTalking {
				fmt.Printf("Client %d stopped talking\n", clientID)
			}
		},
	}

	err = ts3sdk.SetClientCallbacks(callbacks)
	if err != nil {
		fmt.Printf("Error setting callbacks: %s\n", err)
		return
	}

	// Connect to a TeamSpeak 3 server
	// Replace these values with your server details
	serverAddress := "localhost"
	serverPort := uint16(9987)
	nickname := "GoSDKClient"
	identity := "" // Leave empty to generate a new identity

	fmt.Printf("Connecting to %s:%d as %s...\n", serverAddress, serverPort, nickname)

	err = ts3sdk.StartConnection(serverConnectionHandlerID, identity, serverAddress, serverPort, nickname, "", "")
	if err != nil {
		fmt.Printf("Error connecting to server: %s\n", err)
		return
	}

	// Wait for Ctrl+C to exit
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)

	// Main loop
	fmt.Println("Connected to server. Press Ctrl+C to exit.")
	for {
		select {
		case <-sigChan:
			fmt.Println("\nDisconnecting from server...")
			ts3sdk.StopConnection(serverConnectionHandlerID, "Goodbye!")
			return
		default:
			// Check connection status
			status, err := ts3sdk.GetConnectionStatus(serverConnectionHandlerID)
			if err != nil {
				fmt.Printf("Error getting connection status: %s\n", err)
			}

			if status == ts3sdk.StatusDisconnected {
				fmt.Println("Disconnected from server.")
				return
			}

			// Sleep to avoid high CPU usage
			time.Sleep(1 * time.Second)
		}
	}
}
