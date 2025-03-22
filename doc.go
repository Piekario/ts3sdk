// Package ts3sdk provides Go bindings for the TeamSpeak 3 Client SDK.
//
// This package allows Go applications to interact with TeamSpeak 3 servers
// using the official TeamSpeak 3 Client SDK. It provides a more Go-friendly API
// for connecting to servers, sending and receiving messages, managing channels,
// and handling various TeamSpeak 3 events.
//
// Basic usage example:
//
//  package main
//
//  import (
//      "fmt"
//      "github.com/Piekario/ts3sdk"
//  )
//
//  func main() {
//      // Initialize the SDK
//      err := ts3sdk.Initialize("", "", ts3sdk.LogTypeConsole)
//      if err != nil {
//          fmt.Printf("Error initializing SDK: %s\n", err)
//          return
//      }
//      defer ts3sdk.Shutdown()
//      
//      // Create a connection handler
//      serverConnectionHandlerID, err := ts3sdk.CreateServerConnectionHandler()
//      if err != nil {
//          fmt.Printf("Error creating connection handler: %s\n", err)
//          return
//      }
//      defer ts3sdk.DestroyServerConnectionHandler(serverConnectionHandlerID)
//      
//      // Connect to a server
//      err = ts3sdk.StartConnection(
//          serverConnectionHandlerID,
//          "", // identity - empty for a new identity
//          "localhost", // server address
//          9987, // server port
//          "GoClient", // client nickname
//          "", // default channel password
//          "", // server password
//      )
//      if err != nil {
//          fmt.Printf("Error connecting to server: %s\n", err)
//          return
//      }
//  }
//
// For more examples and detailed usage, see the example directory.
package ts3sdk