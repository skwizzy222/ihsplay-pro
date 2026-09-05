#include "error_messages.h"

const char *authorization_result_str(IHS_AuthorizationResult result) {
    switch (result) {
        case IHS_AuthorizationDenied:
            return "Host denied authorization";
        case IHS_AuthorizationNotLoggedIn:
            return "Steam is not logged in on the host";
        case IHS_AuthorizationOffline:
            return "Host is offline";
        case IHS_AuthorizationBusy:
            return "Host is busy — try again in a moment";
        case IHS_AuthorizationTimedOut:
            return "Authorization timed out";
        case IHS_AuthorizationCanceled:
            return "Authorization cancelled";
        case IHS_AuthorizationFailed:
            return "Authorization failed";
        default:
            return "Unknown authorization error";
    }
}

const char *streaming_result_str(IHS_StreamingResult result) {
    switch (result) {
        case IHS_StreamingUnauthorized:
            return "Device is not authorized — pairing required";
        case IHS_StreamingScreenLocked:
            return "Host screen is locked — unlock it and retry";
        case IHS_StreamingFailed:
            return "Streaming failed on the host";
        case IHS_StreamingBusy:
            return "Host is busy with another stream";
        case IHS_StreamingCanceled:
            return "Connection cancelled";
        case IHS_StreamingDriversNotInstalled:
            return "Streaming drivers are missing on the host";
        case IHS_StreamingDisabled:
            return "Remote Play is disabled in Steam settings";
        case IHS_StreamingBroadcastingActive:
            return "Broadcasting is already active on the host";
        case IHS_StreamingVRActive:
            return "VR session is active on the host";
        case IHS_StreamingPINRequired:
            return "Streaming PIN required";
        case IHS_StreamingTransportUnavailable:
            return "Network transport unavailable";
        case IHS_StreamingInvisible:
            return "Host is not visible for Remote Play";
        case IHS_StreamingGameLaunchFailed:
            return "Host failed to launch the game";
        case IHS_StreamingTimeout:
            return "No response from host — check network / firewall";
        default:
            return "Unknown streaming error";
    }
}
