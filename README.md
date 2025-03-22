# TeamSpeak 3 SDK Go Wrapper

Go wrapper dla TeamSpeak 3 Client SDK, umożliwiający łatwą integrację funkcjonalności TeamSpeak 3 w aplikacjach Go.

## Wymagania

- Go 1.16 lub nowszy
- TeamSpeak 3 Client SDK 3.3.1 (dołączone do repozytorium)
- System operacyjny: Windows, Linux lub macOS

## Instalacja

```bash
go get github.com/jakub/ts3sdk
```

## Struktura projektu

- `ts3client.go` - Główny plik wrappera zawierający podstawowe funkcje
- `callbacks.go` - Implementacja callbacków TeamSpeak 3 SDK
- `enums.go` - Definicje enumeracji i stałych z SDK
- `example/` - Przykładowe aplikacje demonstracyjne

## Użycie

### Inicjalizacja SDK

```go
package main

import (
    "fmt"
    "github.com/jakub/ts3sdk"
)

func main() {
    // Inicjalizacja SDK
    err := ts3sdk.Initialize("", "", ts3sdk.LogTypeConsole)
    if err != nil {
        fmt.Printf("Błąd inicjalizacji SDK: %s\n", err)
        return
    }
    defer ts3sdk.Shutdown()
    
    // Dalszy kod...
}
```

### Łączenie z serwerem

```go
// Utworzenie handlera połączenia
serverConnectionHandlerID, err := ts3sdk.CreateServerConnectionHandler()
if err != nil {
    fmt.Printf("Błąd tworzenia handlera połączenia: %s\n", err)
    return
}
defer ts3sdk.DestroyServerConnectionHandler(serverConnectionHandlerID)

// Połączenie z serwerem
err = ts3sdk.StartConnection(
    serverConnectionHandlerID,
    "", // identity - puste dla nowej tożsamości
    "localhost", // adres serwera
    9987, // port serwera
    "GoClient", // nazwa klienta
    "", // hasło kanału domyślnego
    "", // hasło serwera
)
if err != nil {
    fmt.Printf("Błąd łączenia z serwerem: %s\n", err)
    return
}
```

### Obsługa callbacków

```go
// Konfiguracja callbacków
callbacks := ts3sdk.Callbacks{
    ConnectStatusChange: func(serverConnectionHandlerID ts3sdk.ConnectionHandlerID, newStatus ts3sdk.ConnectStatus, errorNumber ts3sdk.Error) {
        fmt.Printf("Zmiana statusu połączenia: %d, błąd: %s\n", newStatus, errorNumber)
        if newStatus == ts3sdk.StatusConnectionEstablished {
            fmt.Println("Połączenie ustanowione!")
        }
    },
    TextMessage: func(serverConnectionHandlerID ts3sdk.ConnectionHandlerID, targetMode int, toID uint64, fromID ts3sdk.ClientID, fromName string, fromUniqueIdentifier string, message string) {
        fmt.Printf("Otrzymano wiadomość od %s (%d): %s\n", fromName, fromID, message)
    },
}

err = ts3sdk.SetClientCallbacks(callbacks)
if err != nil {
    fmt.Printf("Błąd ustawiania callbacków: %s\n", err)
    return
}
```

### Wysyłanie wiadomości

```go
// Wysłanie wiadomości do kanału
err = ts3sdk.RequestSendChannelTextMsg(serverConnectionHandlerID, "Cześć wszystkim!", channelID)
if err != nil {
    fmt.Printf("Błąd wysyłania wiadomości: %s\n", err)
}

// Wysłanie prywatnej wiadomości do klienta
err = ts3sdk.RequestSendPrivateTextMsg(serverConnectionHandlerID, "Cześć!", clientID)
if err != nil {
    fmt.Printf("Błąd wysyłania prywatnej wiadomości: %s\n", err)
}
```

## Przykłady

Zobacz katalog `example/` dla pełnych przykładów użycia wrappera.

## Licencja

Ten wrapper jest udostępniany na licencji MIT. Pamiętaj, że samo TeamSpeak 3 SDK podlega własnej licencji TeamSpeak Systems GmbH.

## Uwagi

- Wrapper używa cgo do wywołania natywnych funkcji TeamSpeak 3 SDK.
- Upewnij się, że biblioteki TeamSpeak 3 SDK są dostępne w systemie.
- W przypadku problemów z kompilacją, sprawdź ścieżki do bibliotek w pliku `ts3client.go`.