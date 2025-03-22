# TeamSpeak 3 SDK Go Wrapper Examples

This directory contains examples demonstrating how to use the TeamSpeak 3 SDK Go Wrapper.

## Basic Example

The `main.go` file contains a basic example that demonstrates:

- Initializing the TeamSpeak 3 Client SDK
- Creating a server connection handler
- Setting up callbacks for various events
- Connecting to a TeamSpeak 3 server
- Sending and receiving messages

## Running the Example

To run the example, make sure you have a TeamSpeak 3 server running, then:

```bash
cd example
go run main.go
```

## Customizing the Example

You can modify the server connection parameters in the `main.go` file to connect to your own TeamSpeak 3 server:

```go
// Replace these values with your server details
serverAddress := "localhost"
serverPort := uint16(9987)
nickname := "GoSDKClient"
```

## Notes

- The example uses the TeamSpeak 3 Client SDK, which must be properly installed on your system.
- Make sure the TeamSpeak 3 Client SDK libraries are in your system's library path or in the same directory as the executable.
- The example creates a new identity each time it runs. For persistent identities, you can save and reuse the identity string.