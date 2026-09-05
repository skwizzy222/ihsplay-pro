#include "connection_fragment.h"

#include "backend/host_manager.h"

#include "lvgl/theme.h"
#include "ui/app_ui.h"
#include "ui/common/error_messages.h"
#include "ui/session/session.h"
#include "util/random.h"
#include "pin_fragment.h"
#include "stream_pin_fragment.h"
#include "conn_error_fragment.h"
#include "lvgl/fonts/bootstrap-icons/symbols.h"


typedef struct connection_fragment_t {
    lv_fragment_t base;
    app_t *app;
    IHS_HostInfo host;
    IHS_StreamInterface stream_interface;
    lv_obj_t *content;
    lv_obj_t *title;
    lv_obj_t *cancel_btn;
    bool awaiting_stream_pin;
} connection_fragment_t;

static void conn_ctor(lv_fragment_t *self, void *arg);

static lv_obj_t *conn_create_obj(lv_fragment_t *self, lv_obj_t *container);

static void conn_obj_created(lv_fragment_t *self, lv_obj_t *obj);

static void conn_obj_will_del(lv_fragment_t *self, lv_obj_t *obj);

static void session_started(const IHS_HostInfo *host, const IHS_SessionInfo *info, void *context);

static void session_start_failed(const IHS_HostInfo *host, IHS_StreamingResult result, void *context);

static void authorized(const IHS_HostInfo *host, uint64_t steam_id, void *context);

static void authorization_failed(const IHS_HostInfo *host, IHS_AuthorizationResult result, void *context);

static void open_authorization(connection_fragment_t *fragment, const IHS_HostInfo *info);

static void open_stream_pin(connection_fragment_t *fragment);

static void conn_show_page(connection_fragment_t *fragment, const lv_fragment_class_t *cls, void *data);

static void cancel_clicked(lv_event_t *e);

static bool conn_event_cb(lv_fragment_t *self, int code, void *data);

const lv_fragment_class_t connection_fragment_class = {
        .constructor_cb = conn_ctor,
        .create_obj_cb = conn_create_obj,
        .obj_created_cb = conn_obj_created,
        .obj_will_delete_cb = conn_obj_will_del,
        .event_cb = conn_event_cb,
        .instance_size = sizeof(connection_fragment_t)
};

static const host_manager_listener_t conn_host_listener = {
        .session_started = session_started,
        .session_start_failed = session_start_failed,
        .authorized = authorized,
        .authorization_failed = authorization_failed,
};

static void conn_ctor(lv_fragment_t *self, void *arg) {
    connection_fragment_t *fragment = (connection_fragment_t *) self;
    app_ui_fragment_args_t *args = arg;
    fragment->app = args->app;
    connection_launch_args_t *launch = args->data;
    fragment->host = launch->host;
    fragment->stream_interface = launch->stream_interface;
    fragment->awaiting_stream_pin = false;
    free(launch);
}

static lv_obj_t *conn_create_obj(lv_fragment_t *self, lv_obj_t *container) {
    connection_fragment_t *fragment = (connection_fragment_t *) self;
    lv_obj_t *win = app_lv_win_create(container);
    fragment->title = lv_win_add_title(win, "Подключение");
    fragment->cancel_btn = app_lv_win_add_close_btn(win, fragment->app);
    /* Close must cancel pairing/stream request, not only pop UI. */
    lv_obj_remove_event_cb(fragment->cancel_btn, NULL);
    lv_obj_add_event_cb(fragment->cancel_btn, cancel_clicked, LV_EVENT_CLICKED, fragment);
    fragment->content = lv_win_get_content(win);
    return win;
}

static void conn_obj_created(lv_fragment_t *self, lv_obj_t *obj) {
    (void) obj;
    connection_fragment_t *fragment = (connection_fragment_t *) self;
    host_manager_t *hosts_manager = fragment->app->host_manager;
    host_manager_register_listener(hosts_manager, &conn_host_listener, fragment);
    connection_fragment_set_title(self, "Подключение");
    host_manager_session_request_ex(hosts_manager, &fragment->host, fragment->stream_interface);
}

static void conn_obj_will_del(lv_fragment_t *self, lv_obj_t *obj) {
    (void) obj;
    connection_fragment_t *fragment = (connection_fragment_t *) self;
    host_manager_t *hosts_manager = fragment->app->host_manager;
    host_manager_session_cancel(hosts_manager);
    host_manager_authorization_cancel(hosts_manager);
    host_manager_unregister_listener(hosts_manager, &conn_host_listener);
}

void connection_fragment_set_title(lv_fragment_t *self, const char *title) {
    connection_fragment_t *fragment = (connection_fragment_t *) self;
    lv_label_set_text(fragment->title, title);
}

void connection_fragment_submit_stream_pin(lv_fragment_t *self, const char *pin) {
    connection_fragment_t *fragment = (connection_fragment_t *) self;
    fragment->awaiting_stream_pin = false;
    connection_fragment_set_title(self, "Подключение");
    host_manager_session_request_with_pin(fragment->app->host_manager, &fragment->host, pin);
}

void connection_fragment_cancel(lv_fragment_t *self) {
    connection_fragment_t *fragment = (connection_fragment_t *) self;
    host_manager_session_cancel(fragment->app->host_manager);
    host_manager_authorization_cancel(fragment->app->host_manager);
    app_ui_pop_top_fragment(fragment->app->ui);
}

static void session_started(const IHS_HostInfo *host, const IHS_SessionInfo *info, void *context) {
    connection_fragment_t *fragment = (connection_fragment_t *) context;
    session_fragment_args_t args = {
            .host = *host,
            .session = *info,
    };
    app_ui_push_fragment(fragment->app->ui, &session_fragment_class, &args);
    app_ui_remove_fragment(fragment->app->ui, (lv_fragment_t *) fragment);
}

static void session_start_failed(const IHS_HostInfo *host, IHS_StreamingResult result, void *context) {
    connection_fragment_t *fragment = (connection_fragment_t *) context;
    if (result == IHS_StreamingUnauthorized) {
        open_authorization(fragment, host);
    } else if (result == IHS_StreamingPINRequired) {
        open_stream_pin(fragment);
    } else if (result == IHS_StreamingCanceled) {
        app_ui_pop_top_fragment(fragment->app->ui);
    } else {
        conn_error_fragment_data data = {
                .message = streaming_result_str(result),
        };
        conn_show_page(fragment, &conn_error_fragment_class, &data);
    }
}

static void authorized(const IHS_HostInfo *host, uint64_t steam_id, void *context) {
    (void) host;
    (void) steam_id;
    connection_fragment_t *fragment = (connection_fragment_t *) context;
    connection_fragment_set_title((lv_fragment_t *) fragment, "Подключение");
    host_manager_t *hosts_manager = fragment->app->host_manager;
    host_manager_session_request_ex(hosts_manager, &fragment->host, fragment->stream_interface);
}

static void authorization_failed(const IHS_HostInfo *host, IHS_AuthorizationResult result, void *context) {
    (void) host;
    connection_fragment_t *fragment = (connection_fragment_t *) context;
    if (result == IHS_AuthorizationCanceled) {
        app_ui_pop_top_fragment(fragment->app->ui);
        return;
    }
    conn_error_fragment_data data = {
            .message = authorization_result_str(result),
    };
    conn_show_page(fragment, &conn_error_fragment_class, &data);
}

static void open_authorization(connection_fragment_t *fragment, const IHS_HostInfo *info) {
    char pin[8];
    random_pin(pin);
    host_manager_authorization_request(fragment->app->host_manager, info, pin);
    conn_show_page(fragment, &pin_fragment_class, pin);
}

static void open_stream_pin(connection_fragment_t *fragment) {
    fragment->awaiting_stream_pin = true;
    conn_show_page(fragment, &stream_pin_fragment_class, NULL);
}

static void cancel_clicked(lv_event_t *e) {
    connection_fragment_t *fragment = lv_event_get_user_data(e);
    connection_fragment_cancel((lv_fragment_t *) fragment);
}

static bool conn_event_cb(lv_fragment_t *self, int code, void *data) {
    (void) data;
    if (code == APP_UI_NAV_BACK) {
        connection_fragment_cancel(self);
        return true;
    }
    return false;
}

static void conn_show_page(connection_fragment_t *fragment, const lv_fragment_class_t *cls, void *data) {
    lv_fragment_t *page = app_ui_create_fragment(fragment->app->ui, cls, data);
    lv_fragment_manager_replace(fragment->base.child_manager, page, &fragment->content);
    lv_obj_set_size(page->obj, LV_PCT(100), LV_PCT(100));
}
