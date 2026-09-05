#include "pin_fragment.h"
#include "ui/app_ui.h"
#include "connection_fragment.h"
#include "backend/host_manager.h"

#include <string.h>

typedef struct pin_fragment_t {
    lv_fragment_t base;
    app_t *app;
    char pin[8];
} pin_fragment_t;

static void pin_ctor(lv_fragment_t *self, void *arg);

static lv_obj_t *pin_create_obj(lv_fragment_t *self, lv_obj_t *container);

static void pin_obj_created(lv_fragment_t *self, lv_obj_t *obj);

static void cancel_clicked(lv_event_t *e);

const lv_fragment_class_t pin_fragment_class = {
        .constructor_cb = pin_ctor,
        .create_obj_cb = pin_create_obj,
        .obj_created_cb = pin_obj_created,
        .instance_size = sizeof(pin_fragment_t)
};

static void pin_ctor(lv_fragment_t *self, void *arg) {
    pin_fragment_t *fragment = (pin_fragment_t *) self;
    app_ui_fragment_args_t *args = arg;
    fragment->app = args->app;
    strncpy(fragment->pin, args->data, sizeof(fragment->pin) - 1);
    fragment->pin[sizeof(fragment->pin) - 1] = '\0';
}

static lv_obj_t *pin_create_obj(lv_fragment_t *self, lv_obj_t *container) {
    pin_fragment_t *fragment = (pin_fragment_t *) self;
    lv_obj_t *obj = lv_obj_create(container);
    lv_obj_set_style_pad_gap(obj, LV_DPX(14), 0);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);

    lv_obj_t *pin_label = lv_label_create(obj);
    lv_obj_set_style_text_font(pin_label, fragment->app->ui->font.huge, 0);
    lv_label_set_text(pin_label, fragment->pin);

    lv_obj_t *hint1 = lv_label_create(obj);
    lv_label_set_text(hint1, "Enter this PIN on your Steam PC to pair this TV");
    lv_obj_set_style_text_align(hint1, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *cancel = lv_btn_create(obj);
    lv_obj_t *cancel_label = lv_label_create(cancel);
    lv_label_set_text(cancel_label, "Cancel pairing");
    lv_obj_center(cancel_label);
    lv_obj_add_event_cb(cancel, cancel_clicked, LV_EVENT_CLICKED, fragment);

    return obj;
}

static void pin_obj_created(lv_fragment_t *self, lv_obj_t *obj) {
    (void) obj;
    lv_fragment_t *parent = lv_fragment_get_parent(self);
    connection_fragment_set_title(parent, "Pairing");
}

static void cancel_clicked(lv_event_t *e) {
    pin_fragment_t *fragment = lv_event_get_user_data(e);
    host_manager_authorization_cancel(fragment->app->host_manager);
    connection_fragment_cancel(lv_fragment_get_parent((lv_fragment_t *) fragment));
}
