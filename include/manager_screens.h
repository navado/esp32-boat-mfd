#pragma once

// Spec 19 D5 - variant application.
//
// Take a validated manager_config::RenderPlan and build LVGL screens,
// one per ScreenPlan in plan.screens[]. Each managed screen is
// registered with ui::register_screen using id "mgr_<plan_id>" so it
// joins the swipe carousel. Widget instances are kept in a per-screen
// static slot so the refresh dispatcher can walk them on each
// ui_refresh tick.
//
// Apply is one-shot in MVP: calling apply() a second time logs
// "already applied" and ignores the new plan. A config-hash change at
// runtime requires a reboot - the screen manager's underlying array
// does not support unregister.

#include <lvgl.h>

#include "manager_config.h"

namespace manager_screens {

// Build one LVGL screen + widget set per plan.screens[] entry.
// Registers each with ui::register_screen as "mgr_<plan.screens[i].id>"
// (or "mgr_<index>" when the screen has no id). Returns true on first
// successful apply; false if a managed screen is already in place or
// the plan has no screens.
bool apply(const manager_config::RenderPlan &plan);

// Number of managed screens currently registered. 0 if !is_applied().
uint8_t managed_count();

// Refresh hook called from the registered Screen.refresh callback.
// No-op if no managed screen is active or the current LVGL screen
// isn't ours.
void refresh();

// True iff a managed screen has been built and is reachable in the
// screen carousel.
bool is_applied();

}  // namespace manager_screens
