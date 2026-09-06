#pragma once

#include "stream_manager.h"
#include "stream_media.h"

#include "array_list.h"

typedef enum stream_manager_state_t {
    STREAM_MANAGER_STATE_IDLE,
    STREAM_MANAGER_STATE_CONNECTING,
    STREAM_MANAGER_STATE_STREAMING,
    STREAM_MANAGER_STATE_DISCONNECTING,
} stream_manager_state_t;

struct stream_manager_t {
    app_t *app;
    array_list_t *listeners;

    stream_manager_state_t state;

    stream_media_session_t *media;
    IHS_Session *session;
    SDL_TimerID back_timer;
    int back_counter;
    bool overlay_opened;
    bool requested_disconnect;

    /* No-video watchdog → silent reconnect without leaving session UI */
    SDL_TimerID watchdog_timer;
    uint32_t last_video_frame_ms;
    bool watchdog_reconnect;
    int watchdog_attempts;
    IHS_HostInfo reconnect_host;
    IHS_StreamInterface reconnect_interface;
    bool has_reconnect_target;

    int viewport_width, viewport_height;
    int capture_width, capture_height;
    int overlay_height;
};
