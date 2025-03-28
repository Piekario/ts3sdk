/*
 * TeamSpeak SDK client sample
 *
 * Copyright (c) TeamSpeak Systems GmbH
 */

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable : 4996)
#include <Windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <teamspeak/public_definitions.h>
#include <teamspeak/public_errors.h>
#include <teamspeak/clientlib.h>

#define DEFAULT_VIRTUAL_SERVER 1
#define NAME_BUFSIZE 1024
#define CHANNEL_PASSWORD_BUFSIZE 1024

#define CHECK_ERROR(x) if((error = x) != ERROR_ok) { goto on_error; }
#define IDENTITY_BUFSIZE 1024

#ifdef _WIN32
#define snprintf sprintf_s
#define SLEEP(x) Sleep(x)
#else
#define SLEEP(x) usleep(x*1000)
#endif

/* This is a global variable to indicate if sound needs to be recorded.
   Normally one would have thread synchronization with locks etc. for 
   thread safety. In the intterest of simplicity, this sample uses the
   most simple way to safely record, using this variable
*/
int recordSound = 0;

/* For voice activation detection demo */
uint64 vadTestscHandlerID;
int vadTestTalkStatus = 0;

/* Enable to use custom encryption */
/* #define USE_CUSTOM_ENCRYPTION
#define CUSTOM_CRYPT_KEY 123 */

/* Uncomment "#define CUSTOM_PASSWORDS" to try custom passwords. 
 * Please note that you have to do the same on the server demo too */
/* #define CUSTOM_PASSWORDS */

/* a struct used for sound recording */
struct WaveHeader {
    /* Riff chunk */
    char riffId[4];  
    unsigned int len;
    char riffType[4];  

    /* Format chunk */
    char fmtId[4];  // 'fmt '
    unsigned int fmtLen;
    unsigned short formatTag;
    unsigned short channels;
    unsigned int samplesPerSec;
    unsigned int avgBytesPerSec;
    unsigned short blockAlign;
    unsigned short bitsPerSample;

    /* Data chunk */
    char dataId[4];  // 'data'
    unsigned int dataLen;
};

/*
 * Callback for connection status change.
 * Connection status switches through the states STATUS_DISCONNECTED, STATUS_CONNECTING, STATUS_CONNECTED and STATUS_CONNECTION_ESTABLISHED.
 *
 * Parameters:
 *   serverConnectionHandlerID - Server connection handler ID
 *   newStatus                 - New connection status, see the enum ConnectStatus in clientlib_publicdefinitions.h
 *   errorNumber               - Error code. Should be zero when connecting or actively disconnection.
 *                               Contains error state when losing connection.
 */
void onConnectStatusChangeEvent(uint64 serverConnectionHandlerID, int newStatus, unsigned int errorNumber) {
    printf("Connect status changed: %llu %d %d\n", (unsigned long long)serverConnectionHandlerID, newStatus, errorNumber);
    /* Failed to connect ? */
    if(newStatus == STATUS_DISCONNECTED && errorNumber == ERROR_failed_connection_initialisation) {
        printf("Looks like there is no server running.\n");
    }
}

/*
 * Callback for current channels being announced to the client after connecting to a server.
 *
 * Parameters:
 *   serverConnectionHandlerID - Server connection handler ID
 *   channelID                 - ID of the announced channel
 *    channelParentID           - ID of the parent channel
 */
void onNewChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 channelParentID) {
    /* Query channel name from channel ID */
    char* name;
    unsigned int error;

    printf("onNewChannelEvent: %llu %llu %llu\n", (unsigned long long)serverConnectionHandlerID, (unsigned long long)channelID, (unsigned long long)channelParentID);
    if((error = ts3client_getChannelVariableAsString(serverConnectionHandlerID, channelID, CHANNEL_NAME, &name)) == ERROR_ok) {
        printf("New channel: %llu %s \n", (unsigned long long)channelID, name);
        ts3client_freeMemory(name);  /* Release dynamically allocated memory only if function succeeded */
    } else {
        char* errormsg;
        if(ts3client_getErrorMessage(error, &errormsg) == ERROR_ok) {
            printf("Error getting channel name in onNewChannelEvent: %s\n", errormsg);
            ts3client_freeMemory(errormsg);
        }
    }
}

/*
 * Callback for just created channels.
 *
 * Parameters:
 *   serverConnectionHandlerID - Server connection handler ID
 *   channelID                 - ID of the announced channel
 *   channelParentID           - ID of the parent channel
 *    invokerID                 - ID of the client who created the channel
 *   invokerName               - Name of the client who created the channel
 */
void onNewChannelCreatedEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 channelParentID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
    char* name;

    /* Query channel name from channel ID */
    if(ts3client_getChannelVariableAsString(serverConnectionHandlerID, channelID, CHANNEL_NAME, &name) != ERROR_ok)
        return;
    printf("New channel created: %s \n", name);
    ts3client_freeMemory(name);  /* Release dynamically allocated memory only if function succeeded */
}

/*
 * Callback when a channel was deleted.
 *
 * Parameters:
 *   serverConnectionHandlerID - Server connection handler ID
 *   channelID                 - ID of the deleted channel
 *   invokerID                 - ID of the client who deleted the channel
 *   invokerName               - Name of the client who deleted the channel
 */
void onDelChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
    printf("Channel ID %llu deleted by %s (%u)\n", (unsigned long long)channelID, invokerName, invokerID);
}

/*
 * Called when a client joins, leaves or moves to another channel.
 *
 * Parameters:
 *   serverConnectionHandlerID - Server connection handler ID
 *   clientID                  - ID of the moved client
 *   oldChannelID              - ID of the old channel left by the client
 *   newChannelID              - ID of the new channel joined by the client
 *   visibility                - Visibility of the moved client. See the enum Visibility in clientlib_publicdefinitions.h
 *                               Values: ENTER_VISIBILITY, RETAIN_VISIBILITY, LEAVE_VISIBILITY
 */
void onClientMoveEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* moveMessage) {
    printf("ClientID %u moves from channel %llu to %llu with message %s\n", clientID, (unsigned long long)oldChannelID, (unsigned long long)newChannelID, moveMessage);
}

/*
 * Callback for other clients in current and subscribed channels being announced to the client.
 *
 * Parameters:
 *   serverConnectionHandlerID - Server connection handler ID
 *   clientID                  - ID of the announced client
 *   oldChannelID              - ID of the subscribed channel where the client left visibility
 *   newChannelID              - ID of the subscribed channel where the client entered visibility
 *   visibility                - Visibility of the announced client. See the enum Visibility in clientlib_publicdefinitions.h
 *                               Values: ENTER_VISIBILITY, RETAIN_VISIBILITY, LEAVE_VISIBILITY
 */
void onClientMoveSubscriptionEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility) {
    char* name;

    /* Query client nickname from ID */
    if(ts3client_getClientVariableAsString(serverConnectionHandlerID, clientID, CLIENT_NICKNAME, &name) != ERROR_ok)
        return;
    printf("New client: %s \n", name);
    ts3client_freeMemory(name);  /* Release dynamically allocated memory only if function succeeded */
}

/*
 * Called when a client drops his connection.
 *
 * Parameters:
 *   serverConnectionHandlerID - Server connection handler ID
 *   clientID                  - ID of the moved client
 *   oldChannelID              - ID of the channel the leaving client was previously member of
 *   newChannelID              - 0, as client is leaving
 *   visibility                - Always LEAVE_VISIBILITY
 *   timeoutMessage            - Optional message giving the reason for the timeout
 */
void onClientMoveTimeoutEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* timeoutMessage) {
    printf("ClientID %u timeouts with message %s\n", clientID, timeoutMessage);
}

/*
 * This event is called when a client starts or stops talking.
 *
 * Parameters:
 *   serverConnectionHandlerID - Server connection handler ID
 *   status                    - 1 if client starts talking, 0 if client stops talking
 *   isReceivedWhisper         - 1 if this event was caused by whispering, 0 if caused by normal talking
 *   clientID                  - ID of the client who announced the talk status change
 */
void onTalkStatusChangeEvent(uint64 serverConnectionHandlerID, int status, int isReceivedWhisper, anyID clientID) {
    char* name;

    /* If we are currently in configure-microphone mode, remember the status and use it later in the loop in configureMicrophone() */
    if(serverConnectionHandlerID == vadTestscHandlerID) {
        vadTestTalkStatus = status;
        return;
    }

    /* Query client nickname from ID */
    if(ts3client_getClientVariableAsString(serverConnectionHandlerID, clientID, CLIENT_NICKNAME, &name) != ERROR_ok)
        return;
    if(status == STATUS_TALKING) {
        printf("Client \"%s\" starts talking.\n", name);
    } else {
        printf("Client \"%s\" stops talking.\n", name);
    }
    ts3client_freeMemory(name);  /* Release dynamically allocated memory only if function succeeded */
}

/*
 * This event is called when another client starts whispering to own client. Own client can decide to accept or deny
 * receiving the whisper by adding the sending client to the whisper allow list. If not added, whispering is blocked.
 *
 * Parameters:
 *   serverConnectionHandlerID - Server connection handler ID
 *   clientID                  - ID of the whispering client
 */
void onIgnoredWhisperEvent(uint64 serverConnectionHandlerID, anyID clientID) {
    unsigned int error;

    /* Add sending client to whisper allow list so own client will hear the voice data.
     * It is sufficient to add a clientID only once, not everytime this event is called. However it won't
     * hurt to add the same clientID to the allow list repeatedly, but is is not necessary. */
    if((error = ts3client_allowWhispersFrom(serverConnectionHandlerID, clientID)) != ERROR_ok) {
        printf("Error setting client on whisper allow list: %u\n", error);
    }
    printf("Added client %d to whisper allow list\n", clientID);
}

void onServerErrorEvent(uint64 serverConnectionHandlerID, const char* errorMessage, unsigned int error, const char* returnCode, const char* extraMessage) {
    printf("Error for server %llu: %s (%u) %s\n", (unsigned long long)serverConnectionHandlerID, errorMessage, error, extraMessage);
}

/*
 * Callback for user-defined logging.
 *
 * Parameter:
 *   logMessage        - Log message text
 *   logLevel          - Severity of log message
 *   logChannel        - Custom text to categorize the message channel
 *   logID             - Virtual server ID giving the virtual server source of the log event
 *   logTime           - String with the date and time the log entry occured
 *   completeLogString - Verbose log message including all previous parameters for convinience
 */
void onUserLoggingMessageEvent(const char* logMessage, int logLevel, const char* logChannel, uint64 logID, const char* logTime, const char* completeLogString) {
    /* Your custom error display here... */
    /* printf("LOG: %s\n", completeLogString); */
    if(logLevel == LogLevel_CRITICAL) {
        exit(1);  /* Your custom handling of critical errors */
    }
}

/*
 * Callback allowing to apply custom encryption to outgoing packets.
 * Using this function is optional. Do not implement if you want to handle the TeamSpeak 3 SDK encryption.
 *
 * Parameters:
 *   dataToSend - Pointer to an array with the outgoing data to be encrypted
 *   sizeOfData - Pointer to an integer value containing the size of the data array
 *
 * Apply your custom encryption to the data array. If the encrypted data is smaller than sizeOfData, write
 * your encrypted data into the existing memory of dataToSend. If your encrypted data is larger, you need
 * to allocate memory and redirect the pointer dataToSend. You need to take care of freeing your own allocated
 * memory yourself. The memory allocated by the SDK, to which dataToSend is originally pointing to, must not
 * be freed.
 * 
 */
void onCustomPacketEncryptEvent(char** dataToSend, unsigned int* sizeOfData) {
#ifdef USE_CUSTOM_ENCRYPTION
    unsigned int i;
    for(i = 0; i < *sizeOfData; i++) {
        (*dataToSend)[i] ^= CUSTOM_CRYPT_KEY;
    }
#endif
}

/*
 * Callback allowing to apply custom encryption to incoming packets
 * Using this function is optional. Do not implement if you want to handle the TeamSpeak 3 SDK encryption.
 *
 * Parameters:
 *   dataToSend - Pointer to an array with the received data to be decrypted
 *   sizeOfData - Pointer to an integer value containing the size of the data array
 *
 * Apply your custom decryption to the data array. If the decrypted data is smaller than dataReceivedSize, write
 * your decrypted data into the existing memory of dataReceived. If your decrypted data is larger, you need
 * to allocate memory and redirect the pointer dataReceived. You need to take care of freeing your own allocated
 * memory yourself. The memory allocated by the SDK, to which dataReceived is originally pointing to, must not
 * be freed.
 */
void onCustomPacketDecryptEvent(char** dataReceived, unsigned int* dataReceivedSize) {
#ifdef USE_CUSTOM_ENCRYPTION
    unsigned int i;
    for(i = 0; i < *dataReceivedSize; i++) {
        (*dataReceived)[i] ^= CUSTOM_CRYPT_KEY;
    }
#endif
}

/*
 * Callback allowing access to voice data after it has been mixed by TeamSpeak 3
 * This event can be used to alter/add to the voice data being played by TeamSpeak 3.
 * But here we use it to record the voice data to a wave file.
 *
 * Parameters:
 *   serverConnectionHandlerID - Server connection handler ID
 *   samples - Pointer to a buffer containg 16 bit voice data samples at 48000 Hz. Channels are interleaved.
 *   sampleCount - The number of samples 1 channel of sample data contains. 
 *   channels - The number of channels the sample data contains.
 *   channelSpeakerArray - A bitmask of the speakers for each channel.
 *   channelFillMask - A bitmask of channels that actually have valid data.
 *
 * -The size of the data "samples" points to is: sizeof(short)*sampleCount*channels
 * -channelSpeakerArray uses SPEAKER_ defined in public_definitions.h
 * -In the interrest of optimizations, a channel only contains data, if there is sound data for it. For example:
 * in 5.1 or 7.1 we (almost) never have data for the subwoofer. Teamspeak then leaves the data in this channel
 * undefined. This is more efficient for mixing.
 * This implementation will record sound to a 2 channel (stereo) wave file. This sample assumes there is only
 * 1 connection to a server
 * Hint: Normally you would want to defer the writing to an other thread because this callback is very time sensitive
 */
void onEditMixedPlaybackVoiceDataEvent(uint64 serverConnectionHandlerID, short* samples, int sampleCount, int channels, const unsigned int* channelSpeakerArray, unsigned int* channelFillMask){
    #define OUTPUTCHANNELS 2
    static FILE *pfile = NULL;
    static struct WaveHeader header = { {'R','I','F','F'}, 0, {'W','A','V','E'}, {'f','m','t',' '}, 16, 1, 2, 48000, 48000*(16/2)*2, (16/2)*2, 16, {'d','a','t','a'}, 0 };

    int currentSampleMix[OUTPUTCHANNELS]; /*a per channel/sample mix buffer*/
    int channelCount[OUTPUTCHANNELS] = {0,0}; /*how many input channels does the output channel contain */
    
    int currentInChannel;
    int currentOutChannel;
    int currentSample;

    /*for clipping*/
    short shortval;
    int   intval;

    short* outputBuffer;

    int leftChannelMask  = SPEAKER_FRONT_LEFT  | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT  | SPEAKER_FRONT_LEFT_OF_CENTER  | SPEAKER_BACK_CENTER | SPEAKER_SIDE_LEFT  | SPEAKER_TOP_CENTER | SPEAKER_TOP_FRONT_LEFT  | SPEAKER_TOP_FRONT_CENTER | SPEAKER_TOP_BACK_LEFT  | SPEAKER_TOP_BACK_CENTER;
    int rightChannelMask = SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_RIGHT | SPEAKER_FRONT_RIGHT_OF_CENTER | SPEAKER_BACK_CENTER | SPEAKER_SIDE_RIGHT | SPEAKER_TOP_CENTER | SPEAKER_TOP_FRONT_RIGHT | SPEAKER_TOP_FRONT_CENTER | SPEAKER_TOP_BACK_RIGHT | SPEAKER_TOP_BACK_CENTER;
    
    /*detect state changes*/
    if (recordSound && (pfile == NULL)){
        /*start recording*/
        header.len = 0;
        header.dataLen = 0;
        if((pfile = fopen("recordedvoices.wav", "wb")) == NULL) return;
        fwrite(&header, sizeof(struct WaveHeader), 1, pfile);
    } else if (!recordSound && (pfile != NULL)){
        /*stop recording*/
        header.len = sizeof(struct WaveHeader)+header.dataLen - 8;
        fseek (pfile, 0, SEEK_SET);
        fwrite(&header, sizeof(struct WaveHeader), 1, pfile);
        fclose(pfile);
        pfile = NULL;
    }

    /*if there is nothing to do, quit*/
    if (pfile == NULL || sampleCount == 0 || channels == 0) return;

    /* initialize channel mixing */
    currentInChannel = 0;
    /*loop over all possible speakers*/
    for (currentInChannel=0; currentInChannel < channels; ++currentInChannel) {
        /*if the speaker has actual data*/
        if ((*channelFillMask & (1<<currentInChannel)) != 0){
            /*add to the outChannelSpeakerSet*/
            if ((channelSpeakerArray[currentInChannel] & leftChannelMask) != 0) channelCount[0]++;
            if ((channelSpeakerArray[currentInChannel] & rightChannelMask) != 0) channelCount[1]++;
        }
    }

    /*get the outbut buffer*/
    outputBuffer = (short*) malloc( sizeof(short)*sampleCount*2 /*output channels*/);

    /* hint: if channelCount is 0 for all channels, we could write a zero buffer and quit here */

    /*mix the samples*/
    for (currentSample = 0; currentSample < sampleCount; currentSample++){
        currentSampleMix[0] = currentSampleMix[1] = 0;
        
        /*loop over all channels in this frame */
        for(currentInChannel =0; currentInChannel < channels; currentInChannel++){
            if ((channelSpeakerArray[currentInChannel] & leftChannelMask)  != 0) currentSampleMix[0] += samples[ (currentSample*channels)+currentInChannel ];
            if ((channelSpeakerArray[currentInChannel] & rightChannelMask) != 0) currentSampleMix[1] += samples[ (currentSample*channels)+currentInChannel ];
        }
        
        /*collected all samples, store mixed sample */
        for (currentOutChannel = 0; currentOutChannel < OUTPUTCHANNELS; currentOutChannel++){
            if (channelCount[currentOutChannel] == 0){
                outputBuffer[ (currentSample*OUTPUTCHANNELS) + currentOutChannel] = 0;
            } else {
                /*clip*/
                intval = currentSampleMix[currentOutChannel]/channelCount[currentOutChannel];
                if (intval >= SHRT_MAX) shortval = SHRT_MAX;
                else if (intval <= SHRT_MIN) shortval = SHRT_MIN;
                else shortval = (short) intval;
                /*store*/
                outputBuffer[ (currentSample*OUTPUTCHANNELS) + currentOutChannel] = shortval;
            }
        }
    }

    /*write data & update header */
    fwrite(outputBuffer, sampleCount*sizeof(short)*OUTPUTCHANNELS, 1, pfile);
    header.dataLen += sampleCount*sizeof(short)*OUTPUTCHANNELS;

    /*free buffer*/
    free(outputBuffer);
}

#ifdef CUSTOM_PASSWORDS
/*
 * Called to encrypt channel and server passwords
 *
 * In this example, we do not do any encryption. That way we have clear
 * text passwords on the server.
 *
 * Parameters:
 *   serverConnectionHandlerID - Server connection handler ID.
 *   plaintext - the clear text password
 *   encryptedText - pointer to a buffer to store the encrypted password
 *   encryptedTextByteSize - the size of the encryptedText buffer
 */
void onClientPasswordEncrypt(uint64 serverConnectionHandlerID, const char* plaintext, char* encryptedText, int encryptedTextByteSize){
    int pt_len;

    printf("onClientPasswordEncrypt called\n");

    pt_len = strlen(plaintext);
    if (encryptedTextByteSize < pt_len-1) pt_len = encryptedTextByteSize-1;

    memcpy(encryptedText, plaintext, pt_len);
    encryptedText[pt_len]=0;
}
#endif

/*
 * Optional event to further adjust 3D sound. Usually this is not needed, setting 3D position of own and other clients as shown
 * setClient3DPosition is sufficient to configure 3D sound.
 */
void onCustom3dRolloffCalculationClientEvent(uint64 serverConnectionHandlerID, anyID clientID, float distance, float* volume)
{
    printf("onCustom3dRolloffCalculationClientEvent: clientID=%hu distance=%f volume=%f\n", clientID, distance, *volume);

    /* Volume can now be modified further to overwrite the value calculated by the SDK */
    /* *volume *= 10.0f; */
}

/*
 * Print all channels of the given virtual server
 */
void showChannels(uint64 serverConnectionHandlerID) {
    uint64 *ids;
    int i;
    unsigned int error;

    printf("\nList of channels on virtual server %llu:\n", (unsigned long long)serverConnectionHandlerID);
    if((error = ts3client_getChannelList(serverConnectionHandlerID, &ids)) != ERROR_ok) {  /* Get array of channel IDs */
        printf("Error getting channel list: %d\n", error);
        return;
    }
    if(!ids[0]) {
        printf("No channels\n\n");
        ts3client_freeMemory(ids);
        return;
    }
    for(i=0; ids[i]; i++) {
        char* name;
        if((error = ts3client_getChannelVariableAsString(serverConnectionHandlerID, ids[i], CHANNEL_NAME, &name)) != ERROR_ok) {  /* Query channel name */
            printf("Error getting channel name: %d\n", error);
            break;
        }
        printf("%llu - %s\n", (unsigned long long)ids[i], name);
        ts3client_freeMemory(name);
    }
    printf("\n");

    ts3client_freeMemory(ids);  /* Release array */
}

/*
 * Print all clients on the given virtual server in the specified channel
 */
void showChannelClients(uint64 serverConnectionHandlerID, uint64 channelID) {
    anyID* ids;
    anyID ownClientID;
    int i;
    unsigned int error;

    printf("\nList of clients in channel %llu on virtual server %llu:\n", (unsigned long long)channelID, (unsigned long long)serverConnectionHandlerID);
    if((error = ts3client_getChannelClientList(serverConnectionHandlerID, channelID, &ids)) != ERROR_ok) {  /* Get array of client IDs */
        printf("Error getting client list for channel %llu: %d\n", (unsigned long long)channelID, error);
        return;
    }
    if(!ids[0]) {
        printf("No clients\n\n");
        ts3client_freeMemory(ids);
        return;
    }

    /* Get own clientID as we need to call CLIENT_FLAG_TALKING with getClientSelfVariable for own client */
    if((error = ts3client_getClientID(serverConnectionHandlerID, &ownClientID)) != ERROR_ok) {
        printf("Error querying own client ID: %d\n", error);
        return;
    }

    for(i=0; ids[i]; i++) {
        char* name;
        int talkStatus;

        if((error = ts3client_getClientVariableAsString(serverConnectionHandlerID, ids[i], CLIENT_NICKNAME, &name)) != ERROR_ok) {  /* Query client nickname... */
            printf("Error querying client nickname: %d\n", error);
            break;
        }
        
        if(ids[i] == ownClientID) {  /* CLIENT_FLAG_TALKING must be queried with getClientSelfVariable for own client */
            if((error = ts3client_getClientSelfVariableAsInt(serverConnectionHandlerID, CLIENT_FLAG_TALKING, &talkStatus)) != ERROR_ok) {
                printf("Error querying own client talk status: %d\n", error);
                break;
            }
        } else {
            if((error = ts3client_getClientVariableAsInt(serverConnectionHandlerID, ids[i], CLIENT_FLAG_TALKING, &talkStatus)) != ERROR_ok) {
                printf("Error querying client talk status: %d\n", error);
                break;
            }
        }

        printf("%u - %s (%stalking)\n", ids[i], name, (talkStatus == STATUS_TALKING ? "" : "not "));
        ts3client_freeMemory(name);
    }
    printf("\n");

    ts3client_freeMemory(ids);  /* Release array */
}

/*
 * Print all visible clients on the given virtual server
 */
void showClients(uint64 serverConnectionHandlerID) {
    anyID *ids;
    anyID ownClientID;
    int i;
    unsigned int error;

    printf("\nList of all visible clients on virtual server %llu:\n", (unsigned long long)serverConnectionHandlerID);
    if((error = ts3client_getClientList(serverConnectionHandlerID, &ids)) != ERROR_ok) {  /* Get array of client IDs */
        printf("Error getting client list: %d\n", error);
        return;
    }
    if(!ids[0]) {
        printf("No clients\n\n");
        ts3client_freeMemory(ids);
        return;
    }

    /* Get own clientID as we need to call CLIENT_FLAG_TALKING with getClientSelfVariable for own client */
    if((error = ts3client_getClientID(serverConnectionHandlerID, &ownClientID)) != ERROR_ok) {
        printf("Error querying own client ID: %d\n", error);
        return;
    }

    for(i=0; ids[i]; i++) {
        char* name;
        int talkStatus;

        if((error = ts3client_getClientVariableAsString(serverConnectionHandlerID, ids[i], CLIENT_NICKNAME, &name)) != ERROR_ok) {  /* Query client nickname... */
            printf("Error querying client nickname: %d\n", error);
            break;
        }

        if(ids[i] == ownClientID) {  /* CLIENT_FLAG_TALKING must be queried with getClientSelfVariable for own client */
            if((error = ts3client_getClientSelfVariableAsInt(serverConnectionHandlerID, CLIENT_FLAG_TALKING, &talkStatus)) != ERROR_ok) {
                printf("Error querying own client talk status: %d\n", error);
                break;
            }
        } else {
            if((error = ts3client_getClientVariableAsInt(serverConnectionHandlerID, ids[i], CLIENT_FLAG_TALKING, &talkStatus)) != ERROR_ok) {
                printf("Error querying client talk status: %d\n", error);
                break;
            }
        }

        printf("%u - %s (%stalking)\n", ids[i], name, (talkStatus == STATUS_TALKING ? "" : "not "));
        ts3client_freeMemory(name);
    }
    printf("\n");

    ts3client_freeMemory(ids);  /* Release array */
}

void emptyInputBuffer() {
    int c;
    while((c = getchar()) != '\n' && c != EOF);
}

uint64 enterChannelID() {
    uint64 channelID;
    int n;

    printf("\nEnter channel ID: ");
    n = scanf("%llu", (unsigned long long*)&channelID);
    emptyInputBuffer();
    if(n == 0) {
        printf("Invalid input. Please enter a number.\n\n");
        return 0;
    }
    return channelID;
}

void createDefaultChannelName(char *name) {
    static int i = 1;
    sprintf(name, "Channel_%d", i++);
}

void enterName(char *name) {
    char *s;
    printf("\nEnter name: ");
    fgets(name, NAME_BUFSIZE, stdin);
    s = strrchr(name, '\n');
    if(s) {
        *s = '\0';
    }
}

void enterPassword(char *password) {
    char *s;
    printf("\nEnter password: ");
    fgets(password, CHANNEL_PASSWORD_BUFSIZE, stdin);
    s = strrchr(password, '\n');
    if(s) {
        *s = '\0';
    }
}

void createChannel(uint64 serverConnectionHandlerID, const char *name, const char* password) {
    unsigned int error;

    /* Set data of new channel. Use channelID of 0 for creating channels. */
    CHECK_ERROR(ts3client_setChannelVariableAsString(serverConnectionHandlerID, 0, CHANNEL_NAME,                name));
    CHECK_ERROR(ts3client_setChannelVariableAsString(serverConnectionHandlerID, 0, CHANNEL_TOPIC,               "Test channel topic"));
    CHECK_ERROR(ts3client_setChannelVariableAsString(serverConnectionHandlerID, 0, CHANNEL_DESCRIPTION,         "Test channel description"));
    CHECK_ERROR(ts3client_setChannelVariableAsInt   (serverConnectionHandlerID, 0, CHANNEL_FLAG_PERMANENT,      1));
    CHECK_ERROR(ts3client_setChannelVariableAsInt   (serverConnectionHandlerID, 0, CHANNEL_FLAG_SEMI_PERMANENT, 0));
    CHECK_ERROR(ts3client_setChannelVariableAsInt   (serverConnectionHandlerID, 0, CHANNEL_CODEC_QUALITY,       10));

    if(password && *password){
        CHECK_ERROR(ts3client_setChannelVariableAsString(serverConnectionHandlerID, 0, CHANNEL_PASSWORD, password));
    }

    /* Flush changes to server */
    CHECK_ERROR(ts3client_flushChannelCreation(serverConnectionHandlerID, 0, NULL));

    printf("\nCreated channel\n\n");
    return;

on_error:
    printf("\nError creating channel: %d\n\n", error);
}

void deleteChannel(uint64 serverConnectionHandlerID) {
    uint64 channelID;
    unsigned int error;

    /* Query channel ID from user */
    channelID = enterChannelID();

    /* Delete channel */
    if((error = ts3client_requestChannelDelete(serverConnectionHandlerID, channelID, 0, NULL)) == ERROR_ok) {
        printf("Deleted channel %llu\n\n", (unsigned long long)channelID);
    } else {
        char* errormsg;
        if(ts3client_getErrorMessage(error, &errormsg) == ERROR_ok) {
            printf("Error requesting channel delete: %s (%d)\n\n", errormsg, error);
            ts3client_freeMemory(errormsg);
        }
    }
}

void renameChannel(uint64 serverConnectionHandlerID) {
    uint64 channelID;
    unsigned int error;
    char name[NAME_BUFSIZE];

    /* Query channel ID from user */
    channelID = enterChannelID();

    /* Query new channel name from user */
    enterName(name);

    /* Change channel name and flush changes */
    CHECK_ERROR(ts3client_setChannelVariableAsString(serverConnectionHandlerID, channelID, CHANNEL_NAME, name));
    CHECK_ERROR(ts3client_flushChannelUpdates(serverConnectionHandlerID, channelID, NULL));

    printf("Renamed channel %llu\n\n", (unsigned long long)channelID);
    return;

on_error:
    printf("Error renaming channel: %d\n\n", error);
}

void switchChannel(uint64 serverConnectionHandlerID) {
    unsigned int error;
    char password[CHANNEL_PASSWORD_BUFSIZE];
#ifndef CUSTOM_PASSWORDS
    int hasPassword;
#endif

    /* Query channel ID from user */
    uint64 channelID = enterChannelID();

    /* Query own client ID */
    anyID clientID;
    if((error = ts3client_getClientID(serverConnectionHandlerID, &clientID)) != ERROR_ok) {
        printf("Error querying own client ID: %d\n", error);
        return;
    }

#ifndef CUSTOM_PASSWORDS
    /* Using standard password mechanism */

    /* Check if channel has a password set */
    if((error = ts3client_getChannelVariableAsInt(serverConnectionHandlerID, channelID, CHANNEL_FLAG_PASSWORD, &hasPassword)) != ERROR_ok) {
        printf("Failed to get password flag: %d\n", error);
        return;
    }

    /* Get channel password if channel is password protected */
    if(hasPassword) {
        enterPassword(password);
    } else {
        password[0] = '\0';
    }
#else
    /* Using custom password mechanism, always ask user for password */
    enterPassword(password);
#endif

    /* Request moving own client into given channel */
    anyID ids_to_move[2];
    ids_to_move[0] = clientID;
    ids_to_move[1] = 0;
    if((error = ts3client_requestClientMove(serverConnectionHandlerID, ids_to_move, channelID, password, NULL)) != ERROR_ok) {
        printf("Error moving client into channel channel: %d\n", error);
        return;
    }
    printf("Switching into channel %llu\n\n", (unsigned long long)channelID);
}

void setVadMode(uint64 serverConnectionHandlerID)
{
    int vad_mode, n;
    unsigned int error;
    char s[100];
    printf("Vad Mode:\n");
    printf("0 - likelihood based (default)\n");
    printf("1 - power based\n");
    printf("2 - likelihood and power based\n\n");
    printf("The Vad Level is tied to modes 2 and 3 - the modes using power.\nn");
    printf("\nEnter VAD mode: ");
    n = scanf("%d", &vad_mode);
    emptyInputBuffer();
    if (n == 0) {
        printf("Invalid input. Please enter a number in the range 0 - 2.\n\n");
        return;
    }

    /* Adjust "vad_mode" preprocessor value */
    snprintf(s, 100, "%d", vad_mode);
    if ((error = ts3client_setPreProcessorConfigValue(serverConnectionHandlerID, "vad_mode", s)) != ERROR_ok) {
        printf("Error setting VAD mode: %d\n", error);
        return;
    }
    printf("\nSet VAD mode to %s.\n\n", s);
}

void toggleVAD(uint64 serverConnectionHandlerID) {
    static short b = 0;
    unsigned int error;

    /* Adjust "vad" preprocessor value */
    if((error = ts3client_setPreProcessorConfigValue(serverConnectionHandlerID, "vad", b ? "false" : "true")) != ERROR_ok) {
        printf("Error toggling VAD: %d\n", error);
        return;
    }
    b = !b;
    printf("\nToggled VAD %s.\n\n", b ? "on" : "off");
}

void setVadLevel(uint64 serverConnectionHandlerID) {
    int vad, n;
    unsigned int error;
    char s[100];

    printf("\nEnter VAD level: ");
    n = scanf("%d", &vad);
    emptyInputBuffer();
    if(n == 0) {
        printf("Invalid input. Please enter a number.\n\n");
        return;
    }

    /* Adjust "voiceactivation_level" preprocessor value */
    snprintf(s, 100, "%d", vad);
    if((error = ts3client_setPreProcessorConfigValue(serverConnectionHandlerID, "voiceactivation_level", s)) != ERROR_ok) {
        printf("Error setting VAD level: %d\n", error);
        return;
    }
    printf("\nSet VAD level to %s.\n\n", s);
}

void toggleDenoise(uint64 serverConnectionHandlerID) {
    static short b = 1;
    unsigned int error;

    /* Adjust "vad" preprocessor value */
    if ((error = ts3client_setPreProcessorConfigValue(serverConnectionHandlerID, "denoise", b ? "false" : "true")) != ERROR_ok) {
        printf("Error toggling Denoiser: %d\n", error);
        return;
    }
    b = !b;
    printf("\nToggled Denoiser %s.\n\n", b ? "on" : "off");
}

void setDenoiserLevel(uint64 serverConnectionHandlerID) {
    int denoiser_level, n;
    unsigned int error;
    char s[100];

    printf("\nEnter Denoiser level: ");
    n = scanf("%d", &denoiser_level);
    emptyInputBuffer();
    if (n == 0) {
        printf("Invalid input. Please enter a number.\n\n");
        return;
    }

    /* Adjust "voiceactivation_level" preprocessor value */
    snprintf(s, 100, "%d", denoiser_level);
    if ((error = ts3client_setPreProcessorConfigValue(serverConnectionHandlerID, "denoiser_level", s)) != ERROR_ok) {
        printf("Error setting Denoiser level: %d\n", error);
        return;
    }
    printf("\nSet Denoiser level to %s.\n\n", s);
}

void toggleTypingSuppression(uint64 serverConnectionHandlerID) {
    static short b = 0;
    unsigned int error;

    /* Adjust "vad" preprocessor value */
    if ((error = ts3client_setPreProcessorConfigValue(serverConnectionHandlerID, "typing_suppression", b ? "false" : "true")) != ERROR_ok) {
        printf("Error toggling typing suppression: %d\n", error);
        return;
    }
    b = !b;
    printf("\nToggled typing suppression %s.\n\n", b ? "on" : "off");
}

void toggleAec(uint64 serverConnectionHandlerID)
{
    static short b = 0;
    unsigned int error;

    /* Adjust "aec" preprocessor value */
    if ((error = ts3client_setPreProcessorConfigValue(serverConnectionHandlerID, "aec", b ? "false" : "true")) != ERROR_ok) {
        printf("Error toggling echo cancellation: %d\n", error);
        return;
    }
    b = !b;
    printf("\nToggled echo cancellation %s.\n\n", b ? "on" : "off");
}

void toggleAgc(uint64 serverConnectionHandlerID) {
    static short b = 1;
    unsigned int error;

    /* Adjust "agc" preprocessor value */
    if ((error = ts3client_setPlaybackConfigValue(serverConnectionHandlerID, "agc", b ? "false" : "true")) != ERROR_ok) {
        printf("Error toggling Automatic Gain Control (AGC): %d\n", error);
        return;
    }
    b = !b;
    printf("\nToggled Automatic Gain Control (AGC) %s.\n\n", b ? "on" : "off");
}

void toggleEchoReductionDucking(uint64 serverConnectionHandlerID) {
    int vad, n;
    unsigned int error;
    char s[100];

    printf("\nEnter Ducking level (dB): ");
    n = scanf("%d", &vad);
    emptyInputBuffer();
    if (n == 0) {
        printf("Invalid input. Please enter a number.\n\n");
        return;
    }

    /* Adjust "echo_reduction_ducking" preprocessor value */
    snprintf(s, 100, "%d", vad);
    if ((error = ts3client_setPlaybackConfigValue(serverConnectionHandlerID, "echo_reduction_ducking", s)) != ERROR_ok) {
        printf("Error setting echo_reduction_ducking level: %d\n", error);
        return;
    }
    printf("\nSet echo_reduction_ducking level to %s.\n\n", s);
}

void requestWhisperList(uint64 serverConnectionHandlerID) {
    int n;
    anyID clientID;
    uint64 targetID;
    unsigned int error;
    uint64 targetChannels[2];

    printf("\nEnter ID of the client whose whisper list should be modified (0 for own client): ");
    n = scanf("%hu", &clientID);
    emptyInputBuffer();
    if(n == 0) {
        printf("Invalid input. Please enter a number.\n\n");
        return;
    }

    printf("\nEnter target channel ID: ");
    n = scanf("%llu", (unsigned long long*)&targetID);
    emptyInputBuffer();
    if(n == 0) {
        printf("Invalid input. Please enter a number.\n\n");
        return;
    }

    targetChannels[0] = targetID;
    targetChannels[1] = 0;

    if((error = ts3client_requestClientSetWhisperList(serverConnectionHandlerID, clientID, targetChannels, NULL, NULL)) != ERROR_ok) {
        char* errormsg;
        if(ts3client_getErrorMessage(error, &errormsg) == ERROR_ok) {
            printf("Error requesting whisperlist: %s\n", errormsg);
            ts3client_freeMemory(errormsg);
        }
        return;
    }
    printf("Whisper list requested for client %d in channel %llu\n", clientID, (unsigned long long)targetID);
}

void requestClearWhisperList(uint64 serverConnectionHandlerID) {
    int n;
    anyID clientID;
    unsigned int error;

    printf("\nEnter ID of the client whose whisper list should be cleared (0 for own client): ");
    n = scanf("%hu", &clientID);
    emptyInputBuffer();
    if(n == 0) {
        printf("Invalid input. Please enter a number.\n\n");
        return;
    }

    if((error = ts3client_requestClientSetWhisperList(serverConnectionHandlerID, clientID, NULL, NULL, NULL)) != ERROR_ok) {
        char* errormsg;
        if(ts3client_getErrorMessage(error, &errormsg) == ERROR_ok) {
            printf("Error clearing whisperlist: %s\n", errormsg);
            ts3client_freeMemory(errormsg);
        }
        return;
    }
    printf("Whisper list cleared for client %u\n", clientID);
}

void setClient3DPosition(uint64 serverConnectionHandlerID)
{
    int i, n;
    anyID clientID;
    float pos[3];
    TS3_VECTOR position = { 0 };
    char* txt[] = { "x", "y", "z" };

    /* Query ID of client whose 3D position we want to change */
    printf("\nEnter ID of the client whose 3D position should be changed (0 for own client): ");
    n = scanf("%hu", &clientID);
    emptyInputBuffer();
    if(n == 0)
    {
        printf("Invalid input. Please enter a number.\n\n");
        return;
    }

    /* Query 3D position */
    for(i = 0; i < 3; ++i)
    {
        printf("Enter %s coordinate: ", txt[i]);
        n = scanf("%f", &pos[i]);
        emptyInputBuffer();
        if(n == 0)
        {
            printf("Invalid input. Please enter a number.\n\n");
            return;
        }
    }
    position.x = pos[0];
    position.y = pos[1];
    position.z = pos[2];

    if(clientID == 0)
    {
        /* Own client */
        TS3_VECTOR zero = { 0 };
        unsigned int error = ts3client_systemset3DListenerAttributes(serverConnectionHandlerID, &position, &zero, &zero);
        if(error != ERROR_ok)
        {
            printf("Failed to set 3D position for own client: %d\n", error);
            return;
        }
        printf("Set 3D position for own client to %f, %f, %f\n", position.x, position.y, position.z);
    }
    else
    {
        /* Other client */
        unsigned int error = ts3client_channelset3DAttributes(serverConnectionHandlerID, clientID, &position);
        if(error != ERROR_ok)
        {
            printf("Failed to set 3D position for other client: %d\n", error);
            return;
        }
        printf("Set 3D position for client %hu to %f, %f, %f\n", clientID, position.x, position.y, position.z);
    }
}

int initLocalTestMode() {
    unsigned int error;

    /* spawn a new server connection handler used as local loopback device */
    if((error = ts3client_spawnNewServerConnectionHandler(0, &vadTestscHandlerID)) != ERROR_ok) {
        printf("Error spawning server connection handler: %d\n", error);
        vadTestscHandlerID = 0;
        return 1;
    }

    /* Open default capture device for the new server connection handler */
    if((error = ts3client_openCaptureDevice(vadTestscHandlerID, "", "")) != ERROR_ok) {
        printf("Error opening capture device: %d\n", error);
        return 1;
    }

    /* Open default playback device for the new server connection handler */
    if((error = ts3client_openPlaybackDevice(vadTestscHandlerID, "", "")) != ERROR_ok) {
        printf("Error opening playback device: %d\n", error);
        return 1;
    }

    /* Set the server connection handler as a local loopback device, so we can hear our own voice without connecting to a server. */
    /* The original capture device for the current server is automatically deactivated. */
    if((error = ts3client_setLocalTestMode(vadTestscHandlerID, 1)) != ERROR_ok){
        printf("Error setting local test mode\n");
        return 1;
    }

    return 0;
}

void destroyLocalTestMode(uint64 scHandlerID){
    unsigned int error;

    /* Close playback device */
    if((ts3client_closePlaybackDevice(scHandlerID)) != ERROR_ok){
        printf("Unable to close playback device\n");
    }
    /* Close capture device */
    if((ts3client_closeCaptureDevice(scHandlerID)) != ERROR_ok) {
        printf("Unable to close capture device\n");
    }
    /* Unset local test mode */
    if((error = ts3client_setLocalTestMode(scHandlerID, 0)) != ERROR_ok) {
        printf("Unable to stop local test mode\n");
    }
    /* Destroy schandlerid */
    if((error = ts3client_destroyServerConnectionHandler(scHandlerID)) != ERROR_ok){
        printf("Unable to destroy scHandler\n");
    }

    /* After closing the local loopback device, reactivate the microphone on our current server connection. */
    if((error = ts3client_activateCaptureDevice(DEFAULT_VIRTUAL_SERVER)) != ERROR_ok){
        printf("unable to reactivate capture device\n");
    }
}

void printVadLevel() {
    unsigned int error;
    float result;

    if((error = ts3client_getPreProcessorInfoValueFloat(vadTestscHandlerID, "decibel_last_period", &result)) != ERROR_ok) {
        printf("Error getting vad level\n");
    }
    printf("%.2f - %s", result, (vadTestTalkStatus == STATUS_TALKING ? "talking" : "not talking"));
    printf("\n");
}

/* Set the microphone voice activation detection level */
void configureMicrophone() {
    unsigned int error;
    int counter = 0;

    /* Enable local loopback device */
    if(initLocalTestMode() != 0) {
        return;
    }

    /* Local loopback device is setup, now enter loop where the user can change the voice activation level while once per second the
     * current volume level is printed. */
    printf("\n**********************************\n");
    printf("Entering configure microphone mode\n");
    printf("[v] - set VAD level\n");
    printf("[q] - quit microphone configuration\n\n");

    for(;;) {
#ifdef _WIN32
        if(_kbhit()) {
            int c = _getche();
#else
            { int c = getc(stdin);  // No kbhit on posix
#endif
            switch(c) {
                case 'v': {
                    int n;
                    float inputVadLevel;
                    char vad[128];

                    printf("Insert value to change voice activations level\n");
                    n = scanf("%f", &inputVadLevel);
                    emptyInputBuffer();
                    if(n == 0) {
                        printf("Invalid input. Please enter a number.\n\n");
                        continue;
                    }
                    sprintf(vad, "%f", inputVadLevel);

                    if((error = ts3client_setPreProcessorConfigValue(vadTestscHandlerID, "voiceactivation_level", vad)) != ERROR_ok) {
                        printf("unable to set vad value\n");
                        continue;
                    }
                    printf("new vad level: %s\n", vad);
                    continue;
                }
                case 'q':
                    destroyLocalTestMode(vadTestscHandlerID);
                    printf("\n**********************************\n");
                    printf("Left configure microphone mode\n\n");
                    vadTestscHandlerID = 0;
                    return;
            }
        }

#ifdef _WIN32  /* On Windows we print this once per second, on Unix only on each Return keyboard input due to lack of easy kbhit replacement on Unix */
        if(++counter > 9)
#endif
        {
            printVadLevel();
            counter = 0;
        }

        SLEEP(100);
    }
}

void toggleRecordSound(uint64 serverConnectionHandlerID){
    unsigned int error;

    if (!recordSound){
        recordSound = 1;
        if((error = ts3client_startVoiceRecording(serverConnectionHandlerID)) != ERROR_ok){
            char* errormsg;
            if(ts3client_getErrorMessage(error, &errormsg) == ERROR_ok) {
                printf("Error notifying server of startVoiceRecording: %s\n", errormsg);
                ts3client_freeMemory(errormsg);
                return;
            }
        }
        printf("Started recording sound to wav\n");
    } else {
        recordSound = 0;
        if((error = ts3client_stopVoiceRecording(serverConnectionHandlerID)) != ERROR_ok){
            char* errormsg;
            if(ts3client_getErrorMessage(error, &errormsg) == ERROR_ok) {
                printf("Error notifying server of stopVoiceRecording: %s\n", errormsg);
                ts3client_freeMemory(errormsg);
                return;
            }
        }
        printf("Stopped recording sound to wav\n");
    }
}

unsigned int printMyConnectionInfo(uint64 serverConnectionHandlerID) {
    anyID my_id;
    unsigned int error = ts3client_getClientID(serverConnectionHandlerID, &my_id);
    if (error != ERROR_ok) {
        return error;
    }
    
    double pingResult;
    if ((error = ts3client_getConnectionVariableAsDouble(serverConnectionHandlerID, my_id, CONNECTION_PING, &pingResult)) == ERROR_ok) {
        double deviationResult;
        if ((error = ts3client_getConnectionVariableAsDouble(serverConnectionHandlerID, my_id, CONNECTION_PING_DEVIATION, &deviationResult)) == ERROR_ok) {
            printf("Ping: %f ms +/- %f\n", pingResult, deviationResult);
        }
    }

    double loss;
    if ((error = ts3client_getConnectionVariableAsDouble(serverConnectionHandlerID, my_id, CONNECTION_SERVER2CLIENT_PACKETLOSS_TOTAL, &loss)) == ERROR_ok) {
        printf("Server to client packet loss (total): %f\n", loss);
    }

    return error;
}

int readIdentity(char* identity) {
    FILE *file;

    if((file = fopen("identity.txt", "r")) == NULL) {
        printf("Could not open file 'identity.txt' for reading.\n");
        return -1;
    }

    fgets(identity, IDENTITY_BUFSIZE, file);
    if(ferror(file) != 0) {
        fclose (file);
        printf("Error reading identity from file 'identity.txt'.\n");
        return -1;
    }
    fclose (file);
    return 0;
}

int writeIdentity(const char* identity) {
    FILE *file;

    if((file = fopen("identity.txt", "w")) == NULL) {
        printf("Could not open file 'identity.txt' for writing.\n");
        return -1;
    }

    fputs(identity, file);
    if(ferror(file) != 0) {
        fclose (file);
        printf("Error writing identity to file 'identity.txt'.\n");
        return -1;
    }
    fclose (file);
    return 0;
}

void showHelp() {
    printf("\n[q] - Disconnect from server\n[h] - Show this help\n[c] - Show channels\n[s] - Switch to specified channel\n");
    printf("[l] - Show all visible clients\n[L] - Show all clients in specific channel\n[n] - Create new channel with generated name\n[N] - Create new channel with custom name\n");
    printf("[d] - Delete channel\n[r] - Rename channel\n[R] - Record sound to wav\n[v] - Toggle Voice Activity Detection / Continuous transmission \n[M] - Set Voice Activity Detection Mode\n[V] - Set Voice Activity Detection level\n");
    printf("[b] - Toggle Denoiser\n[B] - Set Denoiser Level\n[t] - Toggle Typing Suppression\n[e] - Toggle Echo Reduction\n[a] - Toggle Echo Cancellation\n[A] - Toggle AGC\n");
    printf("[w] - Set whisper list\n[W] - Clear whisper list\n[m] - Configure microphone\n[3] - Set 3D position of client\n[i] - Connection info\n\n");
}

char* programPath(char* programInvocation){
    char* path;
    char* end;
    int length;
    char pathsep;

    if (programInvocation == NULL) return strdup("");

#ifdef _WIN32
    pathsep = '\\';
#else
    pathsep = '/';
#endif

    end = strrchr(programInvocation, pathsep);
    if (!end) return strdup("");

    length = (end - programInvocation) + 2;
    path = (char*)malloc(length);
    strncpy(path, programInvocation, length - 1);
    path[length - 1] = '\0';

    return path;
}

struct ConnectInfo {
    char* ip;
    unsigned short port;
};

int main(int argc, char* argv[]) {
    uint64 scHandlerID;
    unsigned int error;
    char* mode;
    char** device;
    char *version;
    char identity[IDENTITY_BUFSIZE];
    short abort = 0;
    char* path;

    /* Check for commandline parameters <ip> <port> */
    struct ConnectInfo connect_info;
    if (argc == 3) {
        size_t sz = strlen(argv[1]) + 1;
        connect_info.ip = (char*)malloc(sz * sizeof(char));
        strcpy(connect_info.ip, argv[1]);
        connect_info.ip[sz - 1] = '\0';
        connect_info.port = atoi(argv[2]);
    } else {
        /* No commandline parameters given, use default ip "localhost" and port 9987 */
        const char* default_ip = "localhost";
        size_t sz = strlen(default_ip) + 1;
        connect_info.ip = (char*)malloc(sz * sizeof(char));
        strcpy(connect_info.ip, default_ip);
        connect_info.port = 9987;
    }

    /* Create struct for callback function pointers */
    struct ClientUIFunctions funcs;

    /* Initialize all callbacks with NULL */
    memset(&funcs, 0, sizeof(struct ClientUIFunctions));

    /* Callback function pointers */
    /* It is sufficient to only assign those callback functions you are using. When adding more callbacks, add those function pointers here. */
    funcs.onConnectStatusChangeEvent        = onConnectStatusChangeEvent;
    funcs.onNewChannelEvent                 = onNewChannelEvent;
    funcs.onNewChannelCreatedEvent          = onNewChannelCreatedEvent;
    funcs.onDelChannelEvent                 = onDelChannelEvent;
    funcs.onClientMoveEvent                 = onClientMoveEvent;
    funcs.onClientMoveSubscriptionEvent     = onClientMoveSubscriptionEvent;
    funcs.onClientMoveTimeoutEvent          = onClientMoveTimeoutEvent;
    funcs.onTalkStatusChangeEvent           = onTalkStatusChangeEvent;
    funcs.onIgnoredWhisperEvent             = onIgnoredWhisperEvent;
    funcs.onServerErrorEvent                = onServerErrorEvent;
    funcs.onUserLoggingMessageEvent         = onUserLoggingMessageEvent;
    funcs.onCustomPacketEncryptEvent        = onCustomPacketEncryptEvent;
    funcs.onCustomPacketDecryptEvent        = onCustomPacketDecryptEvent;
    funcs.onEditMixedPlaybackVoiceDataEvent = onEditMixedPlaybackVoiceDataEvent;
#ifdef CUSTOM_PASSWORDS
    funcs.onClientPasswordEncrypt           = onClientPasswordEncrypt;
#endif
    funcs.onCustom3dRolloffCalculationClientEvent = onCustom3dRolloffCalculationClientEvent;

    /* Initialize client lib with callbacks */
    /* Resource path points to the SDK\bin directory to locate the soundbackends folder when running from Visual Studio. */
    /* If you want to run directly from the SDK\bin directory, use an empty string instead to locate the soundbackends folder in the current directory. */
    path = programPath(argv[0]);
    error = ts3client_initClientLib(&funcs, NULL, LogType_FILE | LogType_CONSOLE | LogType_USERLOGGING, NULL, path);
    free(path);
    path = NULL;
    if (error != ERROR_ok) {
        char* errormsg;
        if(ts3client_getErrorMessage(error, &errormsg) == ERROR_ok) {
            printf("Error initializing clientlib: %s\n", errormsg);
            ts3client_freeMemory(errormsg);
        }
        return 1;
    }

    /* Spawn a new server connection handler using the default port and store the server ID */
    if((error = ts3client_spawnNewServerConnectionHandler(0, &scHandlerID)) != ERROR_ok) {
        printf("Error spawning server connection handler: %d\n", error);
        return 1;
    }

    /* Get default capture mode */
    if((error = ts3client_getDefaultCaptureMode(&mode)) != ERROR_ok) {
        printf("Error getting default capture mode: %d\n", error);
        return 1;
    }
    printf("Default capture mode: %s\n", mode);

    /* Get default capture device */
    if((error = ts3client_getDefaultCaptureDevice(mode, &device)) != ERROR_ok) {
        printf("Error getting default capture device: %d\n", error);
        return 1;
    }
    printf("Default capture device: %s %s\n", device[0], device[1]);

    /* Open default capture device */
    /* Instead of passing mode and device[1], it would also be possible to pass empty strings to open the default device */
    if((error = ts3client_openCaptureDevice(scHandlerID, mode, device[1])) != ERROR_ok) {
        printf("Error opening capture device: %d\n", error);
    }

    /* Adjust "vad" preprocessor value to use vad by default */
    if ((error = ts3client_setPreProcessorConfigValue(scHandlerID, "vad", "true")) != ERROR_ok) {
        printf("Error toggling VAD: %d\n", error);
        return 1;
    }

    /* Adjust "vad_mode" value to use hybrid by default */
    if ((error = ts3client_setPreProcessorConfigValue(scHandlerID, "vad_mode", "2")) != ERROR_ok) {
        printf("Error setting vad_mode value to hybrid: %d\n", error);
        return 1;
    }

    /* Adjust "voiceactivation_level" value */
    if ((error = ts3client_setPreProcessorConfigValue(scHandlerID, "voiceactivation_level", "-20")) != ERROR_ok) {
        printf("Error setting voiceactivation_level: %d\n", error);
        return 1;
    }

    /* Get default playback mode */
    if((error = ts3client_getDefaultPlayBackMode(&mode)) != ERROR_ok) {
        printf("Error getting default playback mode: %d\n", error);
        return 1;
    }
    printf("Default playback mode: %s\n", mode);

    /* Get default playback device */
    if((error = ts3client_getDefaultPlaybackDevice(mode, &device)) != ERROR_ok) {
        printf("Error getting default playback device: %d\n", error);
        return 1;
    }
    printf("Default playback device: %s %s\n", device[0], device[1]);

    /* Open default playback device */
    /* Instead of passing mode and device[1], it would also be possible to pass empty strings to open the default device */
    if((error = ts3client_openPlaybackDevice(scHandlerID, mode, device[1])) != ERROR_ok) {
        printf("Error opening playback device: %d\n", error);
    }

    /* Try reading identity from file, otherwise create new identity */
    if(readIdentity(identity) != 0) {
        char* id;
        if((error = ts3client_createIdentity(&id)) != ERROR_ok) {
            printf("Error creating identity: %d\n", error);
            return 0;
        }
        if(strlen(id) >= IDENTITY_BUFSIZE) {
            printf("Not enough bufsize for identity string\n");
            return 0;
        }
        strcpy(identity, id);
        ts3client_freeMemory(id);
        writeIdentity(identity);
    }
    printf("Using identity: %s\n", identity);

    printf("Connecting to %s:%d\n", connect_info.ip, connect_info.port);
    /* Connect to server on localhost:9987 with nickname "client", no default channel, no default channel password and server password "secret" */
    error = ts3client_startConnection(scHandlerID, identity, connect_info.ip, connect_info.port, "client", NULL, "", "secret");
    free(connect_info.ip);
    connect_info.ip = NULL;
    if (error != ERROR_ok) {
        printf("Error connecting to server: %d\n", error);
        return 1;
    }

    printf("Client lib initialized and running\n");

    /* Query and print client lib version */
    if((error = ts3client_getClientLibVersion(&version)) != ERROR_ok) {
        printf("Failed to get clientlib version: %d\n", error);
        return 1;
    }
    printf("Client lib version: %s\n", version);
    ts3client_freeMemory(version);  /* Release dynamically allocated memory */
    version = NULL;

    SLEEP(300);

    /* Simple commandline interface */
    printf("\nTeamSpeak 3 client commandline interface\n");
    showHelp();

    while(!abort) {
        int c = getc(stdin);
        switch(c) {
            case 'q':
                printf("\nDisconnecting from server...\n");
                abort = 1;
                break;
            case 'h':
                showHelp();
                break;
            case 'c':
                showChannels(DEFAULT_VIRTUAL_SERVER);
                break;
            case 'l':
                showClients(DEFAULT_VIRTUAL_SERVER);
                break;
            case 'L':
            {
                uint64 channelID = enterChannelID();
                if(channelID > 0)
                    showChannelClients(DEFAULT_VIRTUAL_SERVER, channelID);
                break;
            }
            case 'n':
            {
                char name[NAME_BUFSIZE];
                createDefaultChannelName(name);
                createChannel(DEFAULT_VIRTUAL_SERVER, name, NULL);
                break;
            }
            case 'N':
            {
                char name[NAME_BUFSIZE];
                char password[CHANNEL_PASSWORD_BUFSIZE];
                emptyInputBuffer();
                enterName(name);
                enterPassword(password);
                createChannel(DEFAULT_VIRTUAL_SERVER, name, password);
                break;
            }
            case 'd':
                deleteChannel(DEFAULT_VIRTUAL_SERVER);
                break;
            case 'r':
                renameChannel(DEFAULT_VIRTUAL_SERVER);
                break;
            case 'R':
                toggleRecordSound(DEFAULT_VIRTUAL_SERVER);
                break;
            case 's':
                switchChannel(DEFAULT_VIRTUAL_SERVER);
                break;
            case 'v':
                toggleVAD(DEFAULT_VIRTUAL_SERVER);
                break;
            case 'V':
                setVadLevel(DEFAULT_VIRTUAL_SERVER);
                break;
            case 'M':
                setVadMode(DEFAULT_VIRTUAL_SERVER);
                break;
            case 'b':
                toggleDenoise(DEFAULT_VIRTUAL_SERVER);
                break;
            case 'B':
                setDenoiserLevel(DEFAULT_VIRTUAL_SERVER);
                break;
            case 't':
                toggleTypingSuppression(DEFAULT_VIRTUAL_SERVER);
                break;
            case 'e':
                toggleEchoReductionDucking(DEFAULT_VIRTUAL_SERVER);
                break;
            case 'a':
                toggleAec(DEFAULT_VIRTUAL_SERVER);
                break;
            case 'A':
                toggleAgc(DEFAULT_VIRTUAL_SERVER);
                break;
            case 'w':
                requestWhisperList(DEFAULT_VIRTUAL_SERVER);
                break;
            case 'W':
                requestClearWhisperList(DEFAULT_VIRTUAL_SERVER);
                break;
            case 'm':
                configureMicrophone();
                showHelp();  /* Display main menu after leaving configure microphone mode */
                break;
            case '3':
                setClient3DPosition(DEFAULT_VIRTUAL_SERVER);
                break;
            case 'i':
                printMyConnectionInfo(scHandlerID);
                break;
        }

        SLEEP(50);
    }

    /* Disconnect from server */
    if((error = ts3client_stopConnection(scHandlerID, "leaving")) != ERROR_ok) {
        printf("Error stopping connection: %d\n", error);
        return 1;
    }

    SLEEP(200);

    /* Destroy server connection handler */
    if((error = ts3client_destroyServerConnectionHandler(scHandlerID)) != ERROR_ok) {
        printf("Error destroying clientlib: %d\n", error);
        return 1;
    }

    /* Shutdown client lib */
    if((error = ts3client_destroyClientLib()) != ERROR_ok) {
        printf("Failed to destroy clientlib: %d\n", error);
        return 1;
    }

    /* This is a small hack, to close an open recording sound file */
    recordSound = 0;
    onEditMixedPlaybackVoiceDataEvent(DEFAULT_VIRTUAL_SERVER, NULL, 0, 0, NULL, NULL);

    return 0;
}
