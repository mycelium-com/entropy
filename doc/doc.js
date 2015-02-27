function ui_on() {
    ui_object.css("opacity", "1");
}

function ui_off() {
    ui_object.css("opacity", "0");
}

var ui_object;
var ui_interval;
var ui_timer_id;
var ui_cnt;
var ui_pulses;
var ui_pause;

function ui_led_handler() {
    ui_timer_id = window.setTimeout(ui_led_handler, ui_interval);

    if (ui_cnt >= 0)
        if (ui_cnt & 1)
            ui_off();
        else
            ui_on();
    if (++ui_cnt >= ui_pulses)
        ui_cnt = -ui_pause;
}

function ui_show_busy(a, finished) {
    if (!finished)
        ui_object.delay(8192 / 115 - $.fx.interval)
                 .animate({ "opacity": 1 }, $.fx.interval)
                 .delay(8192 / 115 - $.fx.interval)
                 .animate({ "opacity": 0 }, {
                     duration: $.fx.interval,
                     done: ui_show_busy
                 });
}

function ui_show_code(n) {
    ui_interval = 16384 / 115;
    ui_pulses = n * 2;
    ui_pause = 8;
    ui_cnt = 0;
    ui_led_handler();
}

function ui_show_usb() {
    ui_interval = 1024;
    ui_pulses = 2;
    ui_pause = 0;
    ui_cnt = 0;
    ui_led_handler();
}

function ui_pulsate(a, finished) {
    if (!finished)
        ui_object.animate({ "opacity": 1 }, 1000)
                 .animate({ "opacity": 0.2 }, {
                     duration: 1000,
                     done: ui_pulsate
        });
}

function ui_stop() {
    if (ui_timer_id) {
        window.clearTimeout(ui_timer_id);
        ui_timer_id = undefined;
    }
    if (ui_object) {
        ui_object.finish();
        ui_off();
    }
}

function ui_handler(ev) {
    ui_stop();
    var svg, tr, layer;
    if (ev.data) {
        // event from inside the SVG
        svg = ev.data;
        tr = svg.closest("tr");
        layer = $(this).find("#leds_on");
    } else {
        // event from the main document
        tr = $(this);
        svg = tr.find("object");
        layer = svg.contents().find("#leds_on");
    }
    if (ev.type != "click" || !layer.is(ui_object)) {
        ui_object = layer;
        var func = {
            ui_busy: ui_show_busy,
            ui_normal: ui_show_usb,
            ui_pulsating: ui_pulsate,
            ui_on: ui_on,
            ui_off: function () {},
        }[svg.parent().attr("class")];
        if (func)
            func();
        else
            ui_show_code(tr.index() + 2);
    } else if (ev.type == "click") {
        ui_object = undefined;
    }
    return true;
}

$(window).on("load", function () {
    // attach event handlers to control LED animation;
    // all SVG sub-documents must have been loaded, as they need their own
    // handlers
    $("table.ui_leds tr, #conf_mode_led, #ui_full_pic")
        .click(ui_handler).hover(ui_handler, ui_stop)   // containing elements
        .find("object").each(function (i) {             // SVG sub-documents
            $(this.contentDocument).click($(this), ui_handler);
        });
});
