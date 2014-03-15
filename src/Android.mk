LOCAL_PATH := $(call my-dir)

srb_module_tags := eng user

include $(CLEAR_VARS)

LOCAL_SRC_FILES :=      am_map.c \
                        command.c \
                        comptime.c \
                        console.c \
                        d_clisrv.c \
                        d_main.c \
                        d_net.c \
                        d_netcmd.c \
                        d_netfil.c \
                        dehacked.c \
                        f_finale.c \
                        f_wipe.c \
                        filesrch.c \
                        g_game.c \
                        g_input.c \
                        hu_stuff.c \
                        i_tcp.c \
                        info.c \
                        lzf.c \
                        m_argv.c \
                        m_bbox.c \
                        m_cheat.c \
                        m_fixed.c \
                        m_menu.c \
                        m_misc.c \
                        m_queue.c \
                        m_random.c \
                        md5.c \
                        mserv.c \
                        p_ceilng.c \
                        p_enemy.c \
                        p_fab.c \
                        p_floor.c \
                        p_inter.c \
                        p_lights.c \
                        p_map.c \
                        p_maputl.c \
                        p_mobj.c \
                        p_polyobj.c \
                        p_saveg.c \
                        p_setup.c \
                        p_sight.c \
                        p_spec.c \
                        p_telept.c \
                        p_tick.c \
                        p_user.c \
                        r_bsp.c \
                        r_data.c \
                        r_draw.c \
                        r_main.c \
                        r_plane.c \
                        r_segs.c \
                        r_sky.c \
                        r_splats.c \
                        r_things.c \
                        s_sound.c \
                        screen.c \
                        sounds.c \
                        st_stuff.c \
                        string.c \
                        tables.c \
                        v_video.c \
                        w_wad.c \
                        y_inter.c \
                        z_zone.c \
                        android/i_cdmus.c \
                        android/i_main.c \
                        android/i_net.c \
                        android/i_sound.c \
                        android/i_system.c \
                        android/i_video.c

LOCAL_CFLAGS += -DPLATFORM_ANDROID -DNONX86 -DLINUX -DDEBUGMODE -DNOASM -DNOPIX -DUNIXCOMMON -DNOTERMIOS

LOCAL_MODULE := libsrb2

# we live in an APK, so no prelink for us!
LOCAL_PRELINK_MODULE := false

LOCAL_SHARED_LIBRARIES += liblog libdl

LOCAL_CFLAGS += -Idalvik/libnativehelper/include/nativehelper

include $(BUILD_SHARED_LIBRARY)
