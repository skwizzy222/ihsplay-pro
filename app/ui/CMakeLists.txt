target_sources(ihsplay PRIVATE
        app_ui.c
        app_ui_font.c
        launcher.c
        assets/hydra_icon.c
        hosts/hosts_fragment.c
        hosts/add_host_fragment.c
        gamepads/gamepads_fragment.c
        settings/settings.c
        settings/basic.c
        settings/widgets.c)

add_subdirectory(common)
add_subdirectory(connection)
add_subdirectory(session)
add_subdirectory(support)
