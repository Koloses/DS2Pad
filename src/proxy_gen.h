#define PX_COUNT 367
static const char *const px_names[PX_COUNT] = {
    "AIL_debug_printf",
    "AIL_sprintf",
    "DLSClose",
    "DLSCompactMemory",
    "DLSGetInfo",
    "DLSLoadFile",
    "DLSLoadMemFile",
    "DLSMSSOpen",
    "DLSSetAttribute",
    "DLSUnloadAll",
    "DLSUnloadFile",
    "RIB_alloc_provider_handle",
    "RIB_enumerate_interface",
    "RIB_error",
    "RIB_find_file_provider",
    "RIB_free_provider_handle",
    "RIB_free_provider_library",
    "RIB_load_provider_library",
    "RIB_register_interface",
    "RIB_request_interface",
    "RIB_request_interface_entry",
    "RIB_type_string",
    "RIB_unregister_interface",
    "_AIL_3D_distance_factor@4",
    "_AIL_3D_doppler_factor@4",
    "_AIL_3D_orientation@28",
    "_AIL_3D_position@16",
    "_AIL_3D_provider_attribute@12",
    "_AIL_3D_rolloff_factor@4",
    "_AIL_3D_room_type@4",
    "_AIL_3D_sample_attribute@12",
    "_AIL_3D_sample_cone@16",
    "_AIL_3D_sample_distances@12",
    "_AIL_3D_sample_effects_level@4",
    "_AIL_3D_sample_exclusion@4",
    "_AIL_3D_sample_length@4",
    "_AIL_3D_sample_loop_count@4",
    "_AIL_3D_sample_obstruction@4",
    "_AIL_3D_sample_occlusion@4",
    "_AIL_3D_sample_offset@4",
    "_AIL_3D_sample_playback_rate@4",
    "_AIL_3D_sample_status@4",
    "_AIL_3D_sample_volume@4",
    "_AIL_3D_speaker_type@4",
    "_AIL_3D_user_data@8",
    "_AIL_3D_velocity@16",
    "_AIL_DLS_close@8",
    "_AIL_DLS_compact@4",
    "_AIL_DLS_get_info@12",
    "_AIL_DLS_get_reverb_levels@12",
    "_AIL_DLS_load_file@12",
    "_AIL_DLS_load_memory@12",
    "_AIL_DLS_open@28",
    "_AIL_DLS_set_reverb_levels@12",
    "_AIL_DLS_unload@8",
    "_AIL_HWND@0",
    "_AIL_MIDI_handle_reacquire@4",
    "_AIL_MIDI_handle_release@4",
    "_AIL_MIDI_to_XMI@20",
    "_AIL_MMX_available@0",
    "_AIL_WAV_file_write@20",
    "_AIL_WAV_info@8",
    "_AIL_XMIDI_master_volume@4",
    "_AIL_active_3D_sample_count@4",
    "_AIL_active_sample_count@4",
    "_AIL_active_sequence_count@4",
    "_AIL_allocate_3D_sample_handle@4",
    "_AIL_allocate_file_sample@12",
    "_AIL_allocate_sample_handle@4",
    "_AIL_allocate_sequence_handle@4",
    "_AIL_auto_service_stream@8",
    "_AIL_auto_update_3D_position@8",
    "_AIL_background@0",
    "_AIL_branch_index@8",
    "_AIL_channel_notes@8",
    "_AIL_close_3D_listener@4",
    "_AIL_close_3D_object@4",
    "_AIL_close_3D_provider@4",
    "_AIL_close_XMIDI_driver@4",
    "_AIL_close_digital_driver@4",
    "_AIL_close_filter@4",
    "_AIL_close_input@4",
    "_AIL_close_stream@4",
    "_AIL_compress_ADPCM@12",
    "_AIL_compress_ASI@20",
    "_AIL_compress_DLS@20",
    "_AIL_controller_value@12",
    "_AIL_create_wave_synthesizer@16",
    "_AIL_decompress_ADPCM@12",
    "_AIL_decompress_ASI@24",
    "_AIL_delay@4",
    "_AIL_destroy_wave_synthesizer@4",
    "_AIL_digital_CPU_percent@4",
    "_AIL_digital_configuration@16",
    "_AIL_digital_handle_reacquire@4",
    "_AIL_digital_handle_release@4",
    "_AIL_digital_latency@4",
    "_AIL_digital_master_reverb@16",
    "_AIL_digital_master_reverb_levels@12",
    "_AIL_digital_master_volume_level@4",
    "_AIL_end_3D_sample@4",
    "_AIL_end_sample@4",
    "_AIL_end_sequence@4",
    "_AIL_enumerate_3D_provider_attributes@12",
    "_AIL_enumerate_3D_providers@12",
    "_AIL_enumerate_3D_sample_attributes@12",
    "_AIL_enumerate_filter_attributes@12",
    "_AIL_enumerate_filter_sample_attributes@12",
    "_AIL_enumerate_filters@12",
    "_AIL_extract_DLS@28",
    "_AIL_file_error@0",
    "_AIL_file_read@8",
    "_AIL_file_size@4",
    "_AIL_file_type@8",
    "_AIL_file_write@12",
    "_AIL_filter_DLS_attribute@12",
    "_AIL_filter_DLS_with_XMI@24",
    "_AIL_filter_attribute@12",
    "_AIL_filter_sample_attribute@12",
    "_AIL_filter_stream_attribute@12",
    "_AIL_find_3D_provider@8",
    "_AIL_find_DLS@24",
    "_AIL_find_filter@8",
    "_AIL_get_DirectSound_info@12",
    "_AIL_get_input_info@4",
    "_AIL_get_preference@4",
    "_AIL_get_timer_highest_delay@0",
    "_AIL_init_sample@4",
    "_AIL_init_sequence@12",
    "_AIL_last_error@0",
    "_AIL_list_DLS@20",
    "_AIL_list_MIDI@20",
    "_AIL_load_sample_buffer@16",
    "_AIL_lock@0",
    "_AIL_lock_channel@4",
    "_AIL_lock_mutex@0",
    "_AIL_map_sequence_channel@12",
    "_AIL_mem_alloc_lock@4",
    "_AIL_mem_free_lock@4",
    "_AIL_mem_use_free@4",
    "_AIL_mem_use_malloc@4",
    "_AIL_merge_DLS_with_XMI@16",
    "_AIL_midiOutClose@4",
    "_AIL_midiOutOpen@12",
    "_AIL_minimum_sample_buffer_size@12",
    "_AIL_ms_count@0",
    "_AIL_open_3D_listener@4",
    "_AIL_open_3D_object@4",
    "_AIL_open_3D_provider@4",
    "_AIL_open_XMIDI_driver@4",
    "_AIL_open_digital_driver@16",
    "_AIL_open_filter@8",
    "_AIL_open_input@4",
    "_AIL_open_stream@12",
    "_AIL_pause_stream@8",
    "_AIL_primary_digital_driver@4",
    "_AIL_process_digital_audio@24",
    "_AIL_quick_copy@4",
    "_AIL_quick_halt@4",
    "_AIL_quick_handles@12",
    "_AIL_quick_load@4",
    "_AIL_quick_load_and_play@12",
    "_AIL_quick_load_mem@8",
    "_AIL_quick_ms_length@4",
    "_AIL_quick_ms_position@4",
    "_AIL_quick_play@8",
    "_AIL_quick_set_low_pass_cut_off@8",
    "_AIL_quick_set_ms_position@8",
    "_AIL_quick_set_reverb_levels@12",
    "_AIL_quick_set_speed@8",
    "_AIL_quick_set_volume@12",
    "_AIL_quick_shutdown@0",
    "_AIL_quick_startup@20",
    "_AIL_quick_status@4",
    "_AIL_quick_type@4",
    "_AIL_quick_unload@4",
    "_AIL_redbook_close@4",
    "_AIL_redbook_eject@4",
    "_AIL_redbook_id@4",
    "_AIL_redbook_open@4",
    "_AIL_redbook_open_drive@4",
    "_AIL_redbook_pause@4",
    "_AIL_redbook_play@12",
    "_AIL_redbook_position@4",
    "_AIL_redbook_resume@4",
    "_AIL_redbook_retract@4",
    "_AIL_redbook_set_volume_level@8",
    "_AIL_redbook_status@4",
    "_AIL_redbook_stop@4",
    "_AIL_redbook_track@4",
    "_AIL_redbook_track_info@16",
    "_AIL_redbook_tracks@4",
    "_AIL_redbook_volume_level@4",
    "_AIL_register_3D_EOS_callback@8",
    "_AIL_register_EOB_callback@8",
    "_AIL_register_EOF_callback@8",
    "_AIL_register_EOS_callback@8",
    "_AIL_register_ICA_array@8",
    "_AIL_register_SOB_callback@8",
    "_AIL_register_beat_callback@8",
    "_AIL_register_event_callback@8",
    "_AIL_register_prefix_callback@8",
    "_AIL_register_sequence_callback@8",
    "_AIL_register_stream_callback@8",
    "_AIL_register_timbre_callback@8",
    "_AIL_register_timer@4",
    "_AIL_register_trigger_callback@8",
    "_AIL_release_3D_sample_handle@4",
    "_AIL_release_all_timers@0",
    "_AIL_release_channel@8",
    "_AIL_release_sample_handle@4",
    "_AIL_release_sequence_handle@4",
    "_AIL_release_timer_handle@4",
    "_AIL_request_EOB_ASI_reset@8",
    "_AIL_resume_3D_sample@4",
    "_AIL_resume_sample@4",
    "_AIL_resume_sequence@4",
    "_AIL_sample_buffer_info@20",
    "_AIL_sample_buffer_ready@4",
    "_AIL_sample_granularity@4",
    "_AIL_sample_loop_count@4",
    "_AIL_sample_low_pass_cut_off@4",
    "_AIL_sample_ms_position@12",
    "_AIL_sample_playback_rate@4",
    "_AIL_sample_position@4",
    "_AIL_sample_reverb_levels@12",
    "_AIL_sample_status@4",
    "_AIL_sample_user_data@8",
    "_AIL_sample_volume_levels@12",
    "_AIL_sample_volume_pan@12",
    "_AIL_send_channel_voice_message@20",
    "_AIL_send_sysex_message@8",
    "_AIL_sequence_loop_count@4",
    "_AIL_sequence_ms_position@12",
    "_AIL_sequence_position@12",
    "_AIL_sequence_status@4",
    "_AIL_sequence_tempo@4",
    "_AIL_sequence_user_data@8",
    "_AIL_sequence_volume@4",
    "_AIL_serve@0",
    "_AIL_service_stream@8",
    "_AIL_set_3D_distance_factor@8",
    "_AIL_set_3D_doppler_factor@8",
    "_AIL_set_3D_orientation@28",
    "_AIL_set_3D_position@16",
    "_AIL_set_3D_provider_preference@12",
    "_AIL_set_3D_rolloff_factor@8",
    "_AIL_set_3D_room_type@8",
    "_AIL_set_3D_sample_cone@16",
    "_AIL_set_3D_sample_distances@12",
    "_AIL_set_3D_sample_effects_level@8",
    "_AIL_set_3D_sample_exclusion@8",
    "_AIL_set_3D_sample_file@8",
    "_AIL_set_3D_sample_info@8",
    "_AIL_set_3D_sample_loop_block@12",
    "_AIL_set_3D_sample_loop_count@8",
    "_AIL_set_3D_sample_obstruction@8",
    "_AIL_set_3D_sample_occlusion@8",
    "_AIL_set_3D_sample_offset@8",
    "_AIL_set_3D_sample_playback_rate@8",
    "_AIL_set_3D_sample_preference@12",
    "_AIL_set_3D_sample_volume@8",
    "_AIL_set_3D_speaker_type@8",
    "_AIL_set_3D_user_data@12",
    "_AIL_set_3D_velocity@20",
    "_AIL_set_3D_velocity_vector@16",
    "_AIL_set_DLS_processor@12",
    "_AIL_set_DirectSound_HWND@8",
    "_AIL_set_XMIDI_master_volume@8",
    "_AIL_set_digital_driver_processor@12",
    "_AIL_set_digital_master_reverb@16",
    "_AIL_set_digital_master_reverb_levels@12",
    "_AIL_set_digital_master_room_type@8",
    "_AIL_set_digital_master_volume_level@8",
    "_AIL_set_error@4",
    "_AIL_set_file_async_callbacks@20",
    "_AIL_set_file_callbacks@16",
    "_AIL_set_filter_DLS_preference@12",
    "_AIL_set_filter_preference@12",
    "_AIL_set_filter_sample_preference@12",
    "_AIL_set_filter_stream_preference@12",
    "_AIL_set_input_state@8",
    "_AIL_set_named_sample_file@20",
    "_AIL_set_preference@8",
    "_AIL_set_redist_directory@4",
    "_AIL_set_sample_address@12",
    "_AIL_set_sample_adpcm_block_size@8",
    "_AIL_set_sample_file@12",
    "_AIL_set_sample_info@8",
    "_AIL_set_sample_loop_block@12",
    "_AIL_set_sample_loop_count@8",
    "_AIL_set_sample_low_pass_cut_off@8",
    "_AIL_set_sample_ms_position@8",
    "_AIL_set_sample_playback_rate@8",
    "_AIL_set_sample_position@8",
    "_AIL_set_sample_processor@12",
    "_AIL_set_sample_reverb_levels@12",
    "_AIL_set_sample_type@12",
    "_AIL_set_sample_user_data@12",
    "_AIL_set_sample_volume_levels@12",
    "_AIL_set_sample_volume_pan@12",
    "_AIL_set_sequence_loop_count@8",
    "_AIL_set_sequence_ms_position@8",
    "_AIL_set_sequence_tempo@12",
    "_AIL_set_sequence_user_data@12",
    "_AIL_set_sequence_volume@12",
    "_AIL_set_stream_loop_block@12",
    "_AIL_set_stream_loop_count@8",
    "_AIL_set_stream_low_pass_cut_off@8",
    "_AIL_set_stream_ms_position@8",
    "_AIL_set_stream_playback_rate@8",
    "_AIL_set_stream_position@8",
    "_AIL_set_stream_processor@12",
    "_AIL_set_stream_reverb_levels@12",
    "_AIL_set_stream_user_data@12",
    "_AIL_set_stream_volume_levels@12",
    "_AIL_set_stream_volume_pan@12",
    "_AIL_set_timer_divisor@8",
    "_AIL_set_timer_frequency@8",
    "_AIL_set_timer_period@8",
    "_AIL_set_timer_user@8",
    "_AIL_shutdown@0",
    "_AIL_size_processed_digital_audio@16",
    "_AIL_start_3D_sample@4",
    "_AIL_start_all_timers@0",
    "_AIL_start_sample@4",
    "_AIL_start_sequence@4",
    "_AIL_start_stream@4",
    "_AIL_start_timer@4",
    "_AIL_startup@0",
    "_AIL_stop_3D_sample@4",
    "_AIL_stop_all_timers@0",
    "_AIL_stop_sample@4",
    "_AIL_stop_sequence@4",
    "_AIL_stop_timer@4",
    "_AIL_stream_info@20",
    "_AIL_stream_loop_count@4",
    "_AIL_stream_low_pass_cut_off@4",
    "_AIL_stream_ms_position@12",
    "_AIL_stream_playback_rate@4",
    "_AIL_stream_position@4",
    "_AIL_stream_reverb_levels@12",
    "_AIL_stream_status@4",
    "_AIL_stream_user_data@8",
    "_AIL_stream_volume_levels@12",
    "_AIL_stream_volume_pan@12",
    "_AIL_true_sequence_channel@8",
    "_AIL_unlock@0",
    "_AIL_unlock_mutex@0",
    "_AIL_update_3D_position@8",
    "_AIL_us_count@0",
    "_AIL_waveOutClose@4",
    "_AIL_waveOutOpen@16",
    "_DLSMSSGetCPU@4",
    "_DllMain@12",
    "_MIX_RIB_MAIN@8",
    "_RIB_enumerate_providers@12",
    "_RIB_find_file_dec_provider@20",
    "_RIB_find_files_provider@20",
    "_RIB_find_provider@12",
    "_RIB_load_application_providers@4",
    "_RIB_load_static_provider_library@8",
    "_RIB_provider_system_data@8",
    "_RIB_provider_user_data@8",
    "_RIB_set_provider_system_data@12",
    "_RIB_set_provider_user_data@12",
    "stream_background",
};
__asm__(".text\n"
  ".globl _px_0\n_px_0: jmp *(_px_tbl+0)\n"
  "_pl_0: pushl $0\n jmp _px_lazy\n"
  ".globl _px_1\n_px_1: jmp *(_px_tbl+4)\n"
  "_pl_1: pushl $1\n jmp _px_lazy\n"
  ".globl _px_2\n_px_2: jmp *(_px_tbl+8)\n"
  "_pl_2: pushl $2\n jmp _px_lazy\n"
  ".globl _px_3\n_px_3: jmp *(_px_tbl+12)\n"
  "_pl_3: pushl $3\n jmp _px_lazy\n"
  ".globl _px_4\n_px_4: jmp *(_px_tbl+16)\n"
  "_pl_4: pushl $4\n jmp _px_lazy\n"
  ".globl _px_5\n_px_5: jmp *(_px_tbl+20)\n"
  "_pl_5: pushl $5\n jmp _px_lazy\n"
  ".globl _px_6\n_px_6: jmp *(_px_tbl+24)\n"
  "_pl_6: pushl $6\n jmp _px_lazy\n"
  ".globl _px_7\n_px_7: jmp *(_px_tbl+28)\n"
  "_pl_7: pushl $7\n jmp _px_lazy\n"
  ".globl _px_8\n_px_8: jmp *(_px_tbl+32)\n"
  "_pl_8: pushl $8\n jmp _px_lazy\n"
  ".globl _px_9\n_px_9: jmp *(_px_tbl+36)\n"
  "_pl_9: pushl $9\n jmp _px_lazy\n"
  ".globl _px_10\n_px_10: jmp *(_px_tbl+40)\n"
  "_pl_10: pushl $10\n jmp _px_lazy\n"
  ".globl _px_11\n_px_11: jmp *(_px_tbl+44)\n"
  "_pl_11: pushl $11\n jmp _px_lazy\n"
  ".globl _px_12\n_px_12: jmp *(_px_tbl+48)\n"
  "_pl_12: pushl $12\n jmp _px_lazy\n"
  ".globl _px_13\n_px_13: jmp *(_px_tbl+52)\n"
  "_pl_13: pushl $13\n jmp _px_lazy\n"
  ".globl _px_14\n_px_14: jmp *(_px_tbl+56)\n"
  "_pl_14: pushl $14\n jmp _px_lazy\n"
  ".globl _px_15\n_px_15: jmp *(_px_tbl+60)\n"
  "_pl_15: pushl $15\n jmp _px_lazy\n"
  ".globl _px_16\n_px_16: jmp *(_px_tbl+64)\n"
  "_pl_16: pushl $16\n jmp _px_lazy\n"
  ".globl _px_17\n_px_17: jmp *(_px_tbl+68)\n"
  "_pl_17: pushl $17\n jmp _px_lazy\n"
  ".globl _px_18\n_px_18: jmp *(_px_tbl+72)\n"
  "_pl_18: pushl $18\n jmp _px_lazy\n"
  ".globl _px_19\n_px_19: jmp *(_px_tbl+76)\n"
  "_pl_19: pushl $19\n jmp _px_lazy\n"
  ".globl _px_20\n_px_20: jmp *(_px_tbl+80)\n"
  "_pl_20: pushl $20\n jmp _px_lazy\n"
  ".globl _px_21\n_px_21: jmp *(_px_tbl+84)\n"
  "_pl_21: pushl $21\n jmp _px_lazy\n"
  ".globl _px_22\n_px_22: jmp *(_px_tbl+88)\n"
  "_pl_22: pushl $22\n jmp _px_lazy\n"
  ".globl _px_23\n_px_23: jmp *(_px_tbl+92)\n"
  "_pl_23: pushl $23\n jmp _px_lazy\n"
  ".globl _px_24\n_px_24: jmp *(_px_tbl+96)\n"
  "_pl_24: pushl $24\n jmp _px_lazy\n"
  ".globl _px_25\n_px_25: jmp *(_px_tbl+100)\n"
  "_pl_25: pushl $25\n jmp _px_lazy\n"
  ".globl _px_26\n_px_26: jmp *(_px_tbl+104)\n"
  "_pl_26: pushl $26\n jmp _px_lazy\n"
  ".globl _px_27\n_px_27: jmp *(_px_tbl+108)\n"
  "_pl_27: pushl $27\n jmp _px_lazy\n"
  ".globl _px_28\n_px_28: jmp *(_px_tbl+112)\n"
  "_pl_28: pushl $28\n jmp _px_lazy\n"
  ".globl _px_29\n_px_29: jmp *(_px_tbl+116)\n"
  "_pl_29: pushl $29\n jmp _px_lazy\n"
  ".globl _px_30\n_px_30: jmp *(_px_tbl+120)\n"
  "_pl_30: pushl $30\n jmp _px_lazy\n"
  ".globl _px_31\n_px_31: jmp *(_px_tbl+124)\n"
  "_pl_31: pushl $31\n jmp _px_lazy\n"
  ".globl _px_32\n_px_32: jmp *(_px_tbl+128)\n"
  "_pl_32: pushl $32\n jmp _px_lazy\n"
  ".globl _px_33\n_px_33: jmp *(_px_tbl+132)\n"
  "_pl_33: pushl $33\n jmp _px_lazy\n"
  ".globl _px_34\n_px_34: jmp *(_px_tbl+136)\n"
  "_pl_34: pushl $34\n jmp _px_lazy\n"
  ".globl _px_35\n_px_35: jmp *(_px_tbl+140)\n"
  "_pl_35: pushl $35\n jmp _px_lazy\n"
  ".globl _px_36\n_px_36: jmp *(_px_tbl+144)\n"
  "_pl_36: pushl $36\n jmp _px_lazy\n"
  ".globl _px_37\n_px_37: jmp *(_px_tbl+148)\n"
  "_pl_37: pushl $37\n jmp _px_lazy\n"
  ".globl _px_38\n_px_38: jmp *(_px_tbl+152)\n"
  "_pl_38: pushl $38\n jmp _px_lazy\n"
  ".globl _px_39\n_px_39: jmp *(_px_tbl+156)\n"
  "_pl_39: pushl $39\n jmp _px_lazy\n"
  ".globl _px_40\n_px_40: jmp *(_px_tbl+160)\n"
  "_pl_40: pushl $40\n jmp _px_lazy\n"
  ".globl _px_41\n_px_41: jmp *(_px_tbl+164)\n"
  "_pl_41: pushl $41\n jmp _px_lazy\n"
  ".globl _px_42\n_px_42: jmp *(_px_tbl+168)\n"
  "_pl_42: pushl $42\n jmp _px_lazy\n"
  ".globl _px_43\n_px_43: jmp *(_px_tbl+172)\n"
  "_pl_43: pushl $43\n jmp _px_lazy\n"
  ".globl _px_44\n_px_44: jmp *(_px_tbl+176)\n"
  "_pl_44: pushl $44\n jmp _px_lazy\n"
  ".globl _px_45\n_px_45: jmp *(_px_tbl+180)\n"
  "_pl_45: pushl $45\n jmp _px_lazy\n"
  ".globl _px_46\n_px_46: jmp *(_px_tbl+184)\n"
  "_pl_46: pushl $46\n jmp _px_lazy\n"
  ".globl _px_47\n_px_47: jmp *(_px_tbl+188)\n"
  "_pl_47: pushl $47\n jmp _px_lazy\n"
  ".globl _px_48\n_px_48: jmp *(_px_tbl+192)\n"
  "_pl_48: pushl $48\n jmp _px_lazy\n"
  ".globl _px_49\n_px_49: jmp *(_px_tbl+196)\n"
  "_pl_49: pushl $49\n jmp _px_lazy\n"
  ".globl _px_50\n_px_50: jmp *(_px_tbl+200)\n"
  "_pl_50: pushl $50\n jmp _px_lazy\n"
  ".globl _px_51\n_px_51: jmp *(_px_tbl+204)\n"
  "_pl_51: pushl $51\n jmp _px_lazy\n"
  ".globl _px_52\n_px_52: jmp *(_px_tbl+208)\n"
  "_pl_52: pushl $52\n jmp _px_lazy\n"
  ".globl _px_53\n_px_53: jmp *(_px_tbl+212)\n"
  "_pl_53: pushl $53\n jmp _px_lazy\n"
  ".globl _px_54\n_px_54: jmp *(_px_tbl+216)\n"
  "_pl_54: pushl $54\n jmp _px_lazy\n"
  ".globl _px_55\n_px_55: jmp *(_px_tbl+220)\n"
  "_pl_55: pushl $55\n jmp _px_lazy\n"
  ".globl _px_56\n_px_56: jmp *(_px_tbl+224)\n"
  "_pl_56: pushl $56\n jmp _px_lazy\n"
  ".globl _px_57\n_px_57: jmp *(_px_tbl+228)\n"
  "_pl_57: pushl $57\n jmp _px_lazy\n"
  ".globl _px_58\n_px_58: jmp *(_px_tbl+232)\n"
  "_pl_58: pushl $58\n jmp _px_lazy\n"
  ".globl _px_59\n_px_59: jmp *(_px_tbl+236)\n"
  "_pl_59: pushl $59\n jmp _px_lazy\n"
  ".globl _px_60\n_px_60: jmp *(_px_tbl+240)\n"
  "_pl_60: pushl $60\n jmp _px_lazy\n"
  ".globl _px_61\n_px_61: jmp *(_px_tbl+244)\n"
  "_pl_61: pushl $61\n jmp _px_lazy\n"
  ".globl _px_62\n_px_62: jmp *(_px_tbl+248)\n"
  "_pl_62: pushl $62\n jmp _px_lazy\n"
  ".globl _px_63\n_px_63: jmp *(_px_tbl+252)\n"
  "_pl_63: pushl $63\n jmp _px_lazy\n"
  ".globl _px_64\n_px_64: jmp *(_px_tbl+256)\n"
  "_pl_64: pushl $64\n jmp _px_lazy\n"
  ".globl _px_65\n_px_65: jmp *(_px_tbl+260)\n"
  "_pl_65: pushl $65\n jmp _px_lazy\n"
  ".globl _px_66\n_px_66: jmp *(_px_tbl+264)\n"
  "_pl_66: pushl $66\n jmp _px_lazy\n"
  ".globl _px_67\n_px_67: jmp *(_px_tbl+268)\n"
  "_pl_67: pushl $67\n jmp _px_lazy\n"
  ".globl _px_68\n_px_68: jmp *(_px_tbl+272)\n"
  "_pl_68: pushl $68\n jmp _px_lazy\n"
  ".globl _px_69\n_px_69: jmp *(_px_tbl+276)\n"
  "_pl_69: pushl $69\n jmp _px_lazy\n"
  ".globl _px_70\n_px_70: jmp *(_px_tbl+280)\n"
  "_pl_70: pushl $70\n jmp _px_lazy\n"
  ".globl _px_71\n_px_71: jmp *(_px_tbl+284)\n"
  "_pl_71: pushl $71\n jmp _px_lazy\n"
  ".globl _px_72\n_px_72: jmp *(_px_tbl+288)\n"
  "_pl_72: pushl $72\n jmp _px_lazy\n"
  ".globl _px_73\n_px_73: jmp *(_px_tbl+292)\n"
  "_pl_73: pushl $73\n jmp _px_lazy\n"
  ".globl _px_74\n_px_74: jmp *(_px_tbl+296)\n"
  "_pl_74: pushl $74\n jmp _px_lazy\n"
  ".globl _px_75\n_px_75: jmp *(_px_tbl+300)\n"
  "_pl_75: pushl $75\n jmp _px_lazy\n"
  ".globl _px_76\n_px_76: jmp *(_px_tbl+304)\n"
  "_pl_76: pushl $76\n jmp _px_lazy\n"
  ".globl _px_77\n_px_77: jmp *(_px_tbl+308)\n"
  "_pl_77: pushl $77\n jmp _px_lazy\n"
  ".globl _px_78\n_px_78: jmp *(_px_tbl+312)\n"
  "_pl_78: pushl $78\n jmp _px_lazy\n"
  ".globl _px_79\n_px_79: jmp *(_px_tbl+316)\n"
  "_pl_79: pushl $79\n jmp _px_lazy\n"
  ".globl _px_80\n_px_80: jmp *(_px_tbl+320)\n"
  "_pl_80: pushl $80\n jmp _px_lazy\n"
  ".globl _px_81\n_px_81: jmp *(_px_tbl+324)\n"
  "_pl_81: pushl $81\n jmp _px_lazy\n"
  ".globl _px_82\n_px_82: jmp *(_px_tbl+328)\n"
  "_pl_82: pushl $82\n jmp _px_lazy\n"
  ".globl _px_83\n_px_83: jmp *(_px_tbl+332)\n"
  "_pl_83: pushl $83\n jmp _px_lazy\n"
  ".globl _px_84\n_px_84: jmp *(_px_tbl+336)\n"
  "_pl_84: pushl $84\n jmp _px_lazy\n"
  ".globl _px_85\n_px_85: jmp *(_px_tbl+340)\n"
  "_pl_85: pushl $85\n jmp _px_lazy\n"
  ".globl _px_86\n_px_86: jmp *(_px_tbl+344)\n"
  "_pl_86: pushl $86\n jmp _px_lazy\n"
  ".globl _px_87\n_px_87: jmp *(_px_tbl+348)\n"
  "_pl_87: pushl $87\n jmp _px_lazy\n"
  ".globl _px_88\n_px_88: jmp *(_px_tbl+352)\n"
  "_pl_88: pushl $88\n jmp _px_lazy\n"
  ".globl _px_89\n_px_89: jmp *(_px_tbl+356)\n"
  "_pl_89: pushl $89\n jmp _px_lazy\n"
  ".globl _px_90\n_px_90: jmp *(_px_tbl+360)\n"
  "_pl_90: pushl $90\n jmp _px_lazy\n"
  ".globl _px_91\n_px_91: jmp *(_px_tbl+364)\n"
  "_pl_91: pushl $91\n jmp _px_lazy\n"
  ".globl _px_92\n_px_92: jmp *(_px_tbl+368)\n"
  "_pl_92: pushl $92\n jmp _px_lazy\n"
  ".globl _px_93\n_px_93: jmp *(_px_tbl+372)\n"
  "_pl_93: pushl $93\n jmp _px_lazy\n"
  ".globl _px_94\n_px_94: jmp *(_px_tbl+376)\n"
  "_pl_94: pushl $94\n jmp _px_lazy\n"
  ".globl _px_95\n_px_95: jmp *(_px_tbl+380)\n"
  "_pl_95: pushl $95\n jmp _px_lazy\n"
  ".globl _px_96\n_px_96: jmp *(_px_tbl+384)\n"
  "_pl_96: pushl $96\n jmp _px_lazy\n"
  ".globl _px_97\n_px_97: jmp *(_px_tbl+388)\n"
  "_pl_97: pushl $97\n jmp _px_lazy\n"
  ".globl _px_98\n_px_98: jmp *(_px_tbl+392)\n"
  "_pl_98: pushl $98\n jmp _px_lazy\n"
  ".globl _px_99\n_px_99: jmp *(_px_tbl+396)\n"
  "_pl_99: pushl $99\n jmp _px_lazy\n"
  ".globl _px_100\n_px_100: jmp *(_px_tbl+400)\n"
  "_pl_100: pushl $100\n jmp _px_lazy\n"
  ".globl _px_101\n_px_101: jmp *(_px_tbl+404)\n"
  "_pl_101: pushl $101\n jmp _px_lazy\n"
  ".globl _px_102\n_px_102: jmp *(_px_tbl+408)\n"
  "_pl_102: pushl $102\n jmp _px_lazy\n"
  ".globl _px_103\n_px_103: jmp *(_px_tbl+412)\n"
  "_pl_103: pushl $103\n jmp _px_lazy\n"
  ".globl _px_104\n_px_104: jmp *(_px_tbl+416)\n"
  "_pl_104: pushl $104\n jmp _px_lazy\n"
  ".globl _px_105\n_px_105: jmp *(_px_tbl+420)\n"
  "_pl_105: pushl $105\n jmp _px_lazy\n"
  ".globl _px_106\n_px_106: jmp *(_px_tbl+424)\n"
  "_pl_106: pushl $106\n jmp _px_lazy\n"
  ".globl _px_107\n_px_107: jmp *(_px_tbl+428)\n"
  "_pl_107: pushl $107\n jmp _px_lazy\n"
  ".globl _px_108\n_px_108: jmp *(_px_tbl+432)\n"
  "_pl_108: pushl $108\n jmp _px_lazy\n"
  ".globl _px_109\n_px_109: jmp *(_px_tbl+436)\n"
  "_pl_109: pushl $109\n jmp _px_lazy\n"
  ".globl _px_110\n_px_110: jmp *(_px_tbl+440)\n"
  "_pl_110: pushl $110\n jmp _px_lazy\n"
  ".globl _px_111\n_px_111: jmp *(_px_tbl+444)\n"
  "_pl_111: pushl $111\n jmp _px_lazy\n"
  ".globl _px_112\n_px_112: jmp *(_px_tbl+448)\n"
  "_pl_112: pushl $112\n jmp _px_lazy\n"
  ".globl _px_113\n_px_113: jmp *(_px_tbl+452)\n"
  "_pl_113: pushl $113\n jmp _px_lazy\n"
  ".globl _px_114\n_px_114: jmp *(_px_tbl+456)\n"
  "_pl_114: pushl $114\n jmp _px_lazy\n"
  ".globl _px_115\n_px_115: jmp *(_px_tbl+460)\n"
  "_pl_115: pushl $115\n jmp _px_lazy\n"
  ".globl _px_116\n_px_116: jmp *(_px_tbl+464)\n"
  "_pl_116: pushl $116\n jmp _px_lazy\n"
  ".globl _px_117\n_px_117: jmp *(_px_tbl+468)\n"
  "_pl_117: pushl $117\n jmp _px_lazy\n"
  ".globl _px_118\n_px_118: jmp *(_px_tbl+472)\n"
  "_pl_118: pushl $118\n jmp _px_lazy\n"
  ".globl _px_119\n_px_119: jmp *(_px_tbl+476)\n"
  "_pl_119: pushl $119\n jmp _px_lazy\n"
  ".globl _px_120\n_px_120: jmp *(_px_tbl+480)\n"
  "_pl_120: pushl $120\n jmp _px_lazy\n"
  ".globl _px_121\n_px_121: jmp *(_px_tbl+484)\n"
  "_pl_121: pushl $121\n jmp _px_lazy\n"
  ".globl _px_122\n_px_122: jmp *(_px_tbl+488)\n"
  "_pl_122: pushl $122\n jmp _px_lazy\n"
  ".globl _px_123\n_px_123: jmp *(_px_tbl+492)\n"
  "_pl_123: pushl $123\n jmp _px_lazy\n"
  ".globl _px_124\n_px_124: jmp *(_px_tbl+496)\n"
  "_pl_124: pushl $124\n jmp _px_lazy\n"
  ".globl _px_125\n_px_125: jmp *(_px_tbl+500)\n"
  "_pl_125: pushl $125\n jmp _px_lazy\n"
  ".globl _px_126\n_px_126: jmp *(_px_tbl+504)\n"
  "_pl_126: pushl $126\n jmp _px_lazy\n"
  ".globl _px_127\n_px_127: jmp *(_px_tbl+508)\n"
  "_pl_127: pushl $127\n jmp _px_lazy\n"
  ".globl _px_128\n_px_128: jmp *(_px_tbl+512)\n"
  "_pl_128: pushl $128\n jmp _px_lazy\n"
  ".globl _px_129\n_px_129: jmp *(_px_tbl+516)\n"
  "_pl_129: pushl $129\n jmp _px_lazy\n"
  ".globl _px_130\n_px_130: jmp *(_px_tbl+520)\n"
  "_pl_130: pushl $130\n jmp _px_lazy\n"
  ".globl _px_131\n_px_131: jmp *(_px_tbl+524)\n"
  "_pl_131: pushl $131\n jmp _px_lazy\n"
  ".globl _px_132\n_px_132: jmp *(_px_tbl+528)\n"
  "_pl_132: pushl $132\n jmp _px_lazy\n"
  ".globl _px_133\n_px_133: jmp *(_px_tbl+532)\n"
  "_pl_133: pushl $133\n jmp _px_lazy\n"
  ".globl _px_134\n_px_134: jmp *(_px_tbl+536)\n"
  "_pl_134: pushl $134\n jmp _px_lazy\n"
  ".globl _px_135\n_px_135: jmp *(_px_tbl+540)\n"
  "_pl_135: pushl $135\n jmp _px_lazy\n"
  ".globl _px_136\n_px_136: jmp *(_px_tbl+544)\n"
  "_pl_136: pushl $136\n jmp _px_lazy\n"
  ".globl _px_137\n_px_137: jmp *(_px_tbl+548)\n"
  "_pl_137: pushl $137\n jmp _px_lazy\n"
  ".globl _px_138\n_px_138: jmp *(_px_tbl+552)\n"
  "_pl_138: pushl $138\n jmp _px_lazy\n"
  ".globl _px_139\n_px_139: jmp *(_px_tbl+556)\n"
  "_pl_139: pushl $139\n jmp _px_lazy\n"
  ".globl _px_140\n_px_140: jmp *(_px_tbl+560)\n"
  "_pl_140: pushl $140\n jmp _px_lazy\n"
  ".globl _px_141\n_px_141: jmp *(_px_tbl+564)\n"
  "_pl_141: pushl $141\n jmp _px_lazy\n"
  ".globl _px_142\n_px_142: jmp *(_px_tbl+568)\n"
  "_pl_142: pushl $142\n jmp _px_lazy\n"
  ".globl _px_143\n_px_143: jmp *(_px_tbl+572)\n"
  "_pl_143: pushl $143\n jmp _px_lazy\n"
  ".globl _px_144\n_px_144: jmp *(_px_tbl+576)\n"
  "_pl_144: pushl $144\n jmp _px_lazy\n"
  ".globl _px_145\n_px_145: jmp *(_px_tbl+580)\n"
  "_pl_145: pushl $145\n jmp _px_lazy\n"
  ".globl _px_146\n_px_146: jmp *(_px_tbl+584)\n"
  "_pl_146: pushl $146\n jmp _px_lazy\n"
  ".globl _px_147\n_px_147: jmp *(_px_tbl+588)\n"
  "_pl_147: pushl $147\n jmp _px_lazy\n"
  ".globl _px_148\n_px_148: jmp *(_px_tbl+592)\n"
  "_pl_148: pushl $148\n jmp _px_lazy\n"
  ".globl _px_149\n_px_149: jmp *(_px_tbl+596)\n"
  "_pl_149: pushl $149\n jmp _px_lazy\n"
  ".globl _px_150\n_px_150: jmp *(_px_tbl+600)\n"
  "_pl_150: pushl $150\n jmp _px_lazy\n"
  ".globl _px_151\n_px_151: jmp *(_px_tbl+604)\n"
  "_pl_151: pushl $151\n jmp _px_lazy\n"
  ".globl _px_152\n_px_152: jmp *(_px_tbl+608)\n"
  "_pl_152: pushl $152\n jmp _px_lazy\n"
  ".globl _px_153\n_px_153: jmp *(_px_tbl+612)\n"
  "_pl_153: pushl $153\n jmp _px_lazy\n"
  ".globl _px_154\n_px_154: jmp *(_px_tbl+616)\n"
  "_pl_154: pushl $154\n jmp _px_lazy\n"
  ".globl _px_155\n_px_155: jmp *(_px_tbl+620)\n"
  "_pl_155: pushl $155\n jmp _px_lazy\n"
  ".globl _px_156\n_px_156: jmp *(_px_tbl+624)\n"
  "_pl_156: pushl $156\n jmp _px_lazy\n"
  ".globl _px_157\n_px_157: jmp *(_px_tbl+628)\n"
  "_pl_157: pushl $157\n jmp _px_lazy\n"
  ".globl _px_158\n_px_158: jmp *(_px_tbl+632)\n"
  "_pl_158: pushl $158\n jmp _px_lazy\n"
  ".globl _px_159\n_px_159: jmp *(_px_tbl+636)\n"
  "_pl_159: pushl $159\n jmp _px_lazy\n"
  ".globl _px_160\n_px_160: jmp *(_px_tbl+640)\n"
  "_pl_160: pushl $160\n jmp _px_lazy\n"
  ".globl _px_161\n_px_161: jmp *(_px_tbl+644)\n"
  "_pl_161: pushl $161\n jmp _px_lazy\n"
  ".globl _px_162\n_px_162: jmp *(_px_tbl+648)\n"
  "_pl_162: pushl $162\n jmp _px_lazy\n"
  ".globl _px_163\n_px_163: jmp *(_px_tbl+652)\n"
  "_pl_163: pushl $163\n jmp _px_lazy\n"
  ".globl _px_164\n_px_164: jmp *(_px_tbl+656)\n"
  "_pl_164: pushl $164\n jmp _px_lazy\n"
  ".globl _px_165\n_px_165: jmp *(_px_tbl+660)\n"
  "_pl_165: pushl $165\n jmp _px_lazy\n"
  ".globl _px_166\n_px_166: jmp *(_px_tbl+664)\n"
  "_pl_166: pushl $166\n jmp _px_lazy\n"
  ".globl _px_167\n_px_167: jmp *(_px_tbl+668)\n"
  "_pl_167: pushl $167\n jmp _px_lazy\n"
  ".globl _px_168\n_px_168: jmp *(_px_tbl+672)\n"
  "_pl_168: pushl $168\n jmp _px_lazy\n"
  ".globl _px_169\n_px_169: jmp *(_px_tbl+676)\n"
  "_pl_169: pushl $169\n jmp _px_lazy\n"
  ".globl _px_170\n_px_170: jmp *(_px_tbl+680)\n"
  "_pl_170: pushl $170\n jmp _px_lazy\n"
  ".globl _px_171\n_px_171: jmp *(_px_tbl+684)\n"
  "_pl_171: pushl $171\n jmp _px_lazy\n"
  ".globl _px_172\n_px_172: jmp *(_px_tbl+688)\n"
  "_pl_172: pushl $172\n jmp _px_lazy\n"
  ".globl _px_173\n_px_173: jmp *(_px_tbl+692)\n"
  "_pl_173: pushl $173\n jmp _px_lazy\n"
  ".globl _px_174\n_px_174: jmp *(_px_tbl+696)\n"
  "_pl_174: pushl $174\n jmp _px_lazy\n"
  ".globl _px_175\n_px_175: jmp *(_px_tbl+700)\n"
  "_pl_175: pushl $175\n jmp _px_lazy\n"
  ".globl _px_176\n_px_176: jmp *(_px_tbl+704)\n"
  "_pl_176: pushl $176\n jmp _px_lazy\n"
  ".globl _px_177\n_px_177: jmp *(_px_tbl+708)\n"
  "_pl_177: pushl $177\n jmp _px_lazy\n"
  ".globl _px_178\n_px_178: jmp *(_px_tbl+712)\n"
  "_pl_178: pushl $178\n jmp _px_lazy\n"
  ".globl _px_179\n_px_179: jmp *(_px_tbl+716)\n"
  "_pl_179: pushl $179\n jmp _px_lazy\n"
  ".globl _px_180\n_px_180: jmp *(_px_tbl+720)\n"
  "_pl_180: pushl $180\n jmp _px_lazy\n"
  ".globl _px_181\n_px_181: jmp *(_px_tbl+724)\n"
  "_pl_181: pushl $181\n jmp _px_lazy\n"
  ".globl _px_182\n_px_182: jmp *(_px_tbl+728)\n"
  "_pl_182: pushl $182\n jmp _px_lazy\n"
  ".globl _px_183\n_px_183: jmp *(_px_tbl+732)\n"
  "_pl_183: pushl $183\n jmp _px_lazy\n"
  ".globl _px_184\n_px_184: jmp *(_px_tbl+736)\n"
  "_pl_184: pushl $184\n jmp _px_lazy\n"
  ".globl _px_185\n_px_185: jmp *(_px_tbl+740)\n"
  "_pl_185: pushl $185\n jmp _px_lazy\n"
  ".globl _px_186\n_px_186: jmp *(_px_tbl+744)\n"
  "_pl_186: pushl $186\n jmp _px_lazy\n"
  ".globl _px_187\n_px_187: jmp *(_px_tbl+748)\n"
  "_pl_187: pushl $187\n jmp _px_lazy\n"
  ".globl _px_188\n_px_188: jmp *(_px_tbl+752)\n"
  "_pl_188: pushl $188\n jmp _px_lazy\n"
  ".globl _px_189\n_px_189: jmp *(_px_tbl+756)\n"
  "_pl_189: pushl $189\n jmp _px_lazy\n"
  ".globl _px_190\n_px_190: jmp *(_px_tbl+760)\n"
  "_pl_190: pushl $190\n jmp _px_lazy\n"
  ".globl _px_191\n_px_191: jmp *(_px_tbl+764)\n"
  "_pl_191: pushl $191\n jmp _px_lazy\n"
  ".globl _px_192\n_px_192: jmp *(_px_tbl+768)\n"
  "_pl_192: pushl $192\n jmp _px_lazy\n"
  ".globl _px_193\n_px_193: jmp *(_px_tbl+772)\n"
  "_pl_193: pushl $193\n jmp _px_lazy\n"
  ".globl _px_194\n_px_194: jmp *(_px_tbl+776)\n"
  "_pl_194: pushl $194\n jmp _px_lazy\n"
  ".globl _px_195\n_px_195: jmp *(_px_tbl+780)\n"
  "_pl_195: pushl $195\n jmp _px_lazy\n"
  ".globl _px_196\n_px_196: jmp *(_px_tbl+784)\n"
  "_pl_196: pushl $196\n jmp _px_lazy\n"
  ".globl _px_197\n_px_197: jmp *(_px_tbl+788)\n"
  "_pl_197: pushl $197\n jmp _px_lazy\n"
  ".globl _px_198\n_px_198: jmp *(_px_tbl+792)\n"
  "_pl_198: pushl $198\n jmp _px_lazy\n"
  ".globl _px_199\n_px_199: jmp *(_px_tbl+796)\n"
  "_pl_199: pushl $199\n jmp _px_lazy\n"
  ".globl _px_200\n_px_200: jmp *(_px_tbl+800)\n"
  "_pl_200: pushl $200\n jmp _px_lazy\n"
  ".globl _px_201\n_px_201: jmp *(_px_tbl+804)\n"
  "_pl_201: pushl $201\n jmp _px_lazy\n"
  ".globl _px_202\n_px_202: jmp *(_px_tbl+808)\n"
  "_pl_202: pushl $202\n jmp _px_lazy\n"
  ".globl _px_203\n_px_203: jmp *(_px_tbl+812)\n"
  "_pl_203: pushl $203\n jmp _px_lazy\n"
  ".globl _px_204\n_px_204: jmp *(_px_tbl+816)\n"
  "_pl_204: pushl $204\n jmp _px_lazy\n"
  ".globl _px_205\n_px_205: jmp *(_px_tbl+820)\n"
  "_pl_205: pushl $205\n jmp _px_lazy\n"
  ".globl _px_206\n_px_206: jmp *(_px_tbl+824)\n"
  "_pl_206: pushl $206\n jmp _px_lazy\n"
  ".globl _px_207\n_px_207: jmp *(_px_tbl+828)\n"
  "_pl_207: pushl $207\n jmp _px_lazy\n"
  ".globl _px_208\n_px_208: jmp *(_px_tbl+832)\n"
  "_pl_208: pushl $208\n jmp _px_lazy\n"
  ".globl _px_209\n_px_209: jmp *(_px_tbl+836)\n"
  "_pl_209: pushl $209\n jmp _px_lazy\n"
  ".globl _px_210\n_px_210: jmp *(_px_tbl+840)\n"
  "_pl_210: pushl $210\n jmp _px_lazy\n"
  ".globl _px_211\n_px_211: jmp *(_px_tbl+844)\n"
  "_pl_211: pushl $211\n jmp _px_lazy\n"
  ".globl _px_212\n_px_212: jmp *(_px_tbl+848)\n"
  "_pl_212: pushl $212\n jmp _px_lazy\n"
  ".globl _px_213\n_px_213: jmp *(_px_tbl+852)\n"
  "_pl_213: pushl $213\n jmp _px_lazy\n"
  ".globl _px_214\n_px_214: jmp *(_px_tbl+856)\n"
  "_pl_214: pushl $214\n jmp _px_lazy\n"
  ".globl _px_215\n_px_215: jmp *(_px_tbl+860)\n"
  "_pl_215: pushl $215\n jmp _px_lazy\n"
  ".globl _px_216\n_px_216: jmp *(_px_tbl+864)\n"
  "_pl_216: pushl $216\n jmp _px_lazy\n"
  ".globl _px_217\n_px_217: jmp *(_px_tbl+868)\n"
  "_pl_217: pushl $217\n jmp _px_lazy\n"
  ".globl _px_218\n_px_218: jmp *(_px_tbl+872)\n"
  "_pl_218: pushl $218\n jmp _px_lazy\n"
  ".globl _px_219\n_px_219: jmp *(_px_tbl+876)\n"
  "_pl_219: pushl $219\n jmp _px_lazy\n"
  ".globl _px_220\n_px_220: jmp *(_px_tbl+880)\n"
  "_pl_220: pushl $220\n jmp _px_lazy\n"
  ".globl _px_221\n_px_221: jmp *(_px_tbl+884)\n"
  "_pl_221: pushl $221\n jmp _px_lazy\n"
  ".globl _px_222\n_px_222: jmp *(_px_tbl+888)\n"
  "_pl_222: pushl $222\n jmp _px_lazy\n"
  ".globl _px_223\n_px_223: jmp *(_px_tbl+892)\n"
  "_pl_223: pushl $223\n jmp _px_lazy\n"
  ".globl _px_224\n_px_224: jmp *(_px_tbl+896)\n"
  "_pl_224: pushl $224\n jmp _px_lazy\n"
  ".globl _px_225\n_px_225: jmp *(_px_tbl+900)\n"
  "_pl_225: pushl $225\n jmp _px_lazy\n"
  ".globl _px_226\n_px_226: jmp *(_px_tbl+904)\n"
  "_pl_226: pushl $226\n jmp _px_lazy\n"
  ".globl _px_227\n_px_227: jmp *(_px_tbl+908)\n"
  "_pl_227: pushl $227\n jmp _px_lazy\n"
  ".globl _px_228\n_px_228: jmp *(_px_tbl+912)\n"
  "_pl_228: pushl $228\n jmp _px_lazy\n"
  ".globl _px_229\n_px_229: jmp *(_px_tbl+916)\n"
  "_pl_229: pushl $229\n jmp _px_lazy\n"
  ".globl _px_230\n_px_230: jmp *(_px_tbl+920)\n"
  "_pl_230: pushl $230\n jmp _px_lazy\n"
  ".globl _px_231\n_px_231: jmp *(_px_tbl+924)\n"
  "_pl_231: pushl $231\n jmp _px_lazy\n"
  ".globl _px_232\n_px_232: jmp *(_px_tbl+928)\n"
  "_pl_232: pushl $232\n jmp _px_lazy\n"
  ".globl _px_233\n_px_233: jmp *(_px_tbl+932)\n"
  "_pl_233: pushl $233\n jmp _px_lazy\n"
  ".globl _px_234\n_px_234: jmp *(_px_tbl+936)\n"
  "_pl_234: pushl $234\n jmp _px_lazy\n"
  ".globl _px_235\n_px_235: jmp *(_px_tbl+940)\n"
  "_pl_235: pushl $235\n jmp _px_lazy\n"
  ".globl _px_236\n_px_236: jmp *(_px_tbl+944)\n"
  "_pl_236: pushl $236\n jmp _px_lazy\n"
  ".globl _px_237\n_px_237: jmp *(_px_tbl+948)\n"
  "_pl_237: pushl $237\n jmp _px_lazy\n"
  ".globl _px_238\n_px_238: jmp *(_px_tbl+952)\n"
  "_pl_238: pushl $238\n jmp _px_lazy\n"
  ".globl _px_239\n_px_239: jmp *(_px_tbl+956)\n"
  "_pl_239: pushl $239\n jmp _px_lazy\n"
  ".globl _px_240\n_px_240: jmp *(_px_tbl+960)\n"
  "_pl_240: pushl $240\n jmp _px_lazy\n"
  ".globl _px_241\n_px_241: jmp *(_px_tbl+964)\n"
  "_pl_241: pushl $241\n jmp _px_lazy\n"
  ".globl _px_242\n_px_242: jmp *(_px_tbl+968)\n"
  "_pl_242: pushl $242\n jmp _px_lazy\n"
  ".globl _px_243\n_px_243: jmp *(_px_tbl+972)\n"
  "_pl_243: pushl $243\n jmp _px_lazy\n"
  ".globl _px_244\n_px_244: jmp *(_px_tbl+976)\n"
  "_pl_244: pushl $244\n jmp _px_lazy\n"
  ".globl _px_245\n_px_245: jmp *(_px_tbl+980)\n"
  "_pl_245: pushl $245\n jmp _px_lazy\n"
  ".globl _px_246\n_px_246: jmp *(_px_tbl+984)\n"
  "_pl_246: pushl $246\n jmp _px_lazy\n"
  ".globl _px_247\n_px_247: jmp *(_px_tbl+988)\n"
  "_pl_247: pushl $247\n jmp _px_lazy\n"
  ".globl _px_248\n_px_248: jmp *(_px_tbl+992)\n"
  "_pl_248: pushl $248\n jmp _px_lazy\n"
  ".globl _px_249\n_px_249: jmp *(_px_tbl+996)\n"
  "_pl_249: pushl $249\n jmp _px_lazy\n"
  ".globl _px_250\n_px_250: jmp *(_px_tbl+1000)\n"
  "_pl_250: pushl $250\n jmp _px_lazy\n"
  ".globl _px_251\n_px_251: jmp *(_px_tbl+1004)\n"
  "_pl_251: pushl $251\n jmp _px_lazy\n"
  ".globl _px_252\n_px_252: jmp *(_px_tbl+1008)\n"
  "_pl_252: pushl $252\n jmp _px_lazy\n"
  ".globl _px_253\n_px_253: jmp *(_px_tbl+1012)\n"
  "_pl_253: pushl $253\n jmp _px_lazy\n"
  ".globl _px_254\n_px_254: jmp *(_px_tbl+1016)\n"
  "_pl_254: pushl $254\n jmp _px_lazy\n"
  ".globl _px_255\n_px_255: jmp *(_px_tbl+1020)\n"
  "_pl_255: pushl $255\n jmp _px_lazy\n"
  ".globl _px_256\n_px_256: jmp *(_px_tbl+1024)\n"
  "_pl_256: pushl $256\n jmp _px_lazy\n"
  ".globl _px_257\n_px_257: jmp *(_px_tbl+1028)\n"
  "_pl_257: pushl $257\n jmp _px_lazy\n"
  ".globl _px_258\n_px_258: jmp *(_px_tbl+1032)\n"
  "_pl_258: pushl $258\n jmp _px_lazy\n"
  ".globl _px_259\n_px_259: jmp *(_px_tbl+1036)\n"
  "_pl_259: pushl $259\n jmp _px_lazy\n"
  ".globl _px_260\n_px_260: jmp *(_px_tbl+1040)\n"
  "_pl_260: pushl $260\n jmp _px_lazy\n"
  ".globl _px_261\n_px_261: jmp *(_px_tbl+1044)\n"
  "_pl_261: pushl $261\n jmp _px_lazy\n"
  ".globl _px_262\n_px_262: jmp *(_px_tbl+1048)\n"
  "_pl_262: pushl $262\n jmp _px_lazy\n"
  ".globl _px_263\n_px_263: jmp *(_px_tbl+1052)\n"
  "_pl_263: pushl $263\n jmp _px_lazy\n"
  ".globl _px_264\n_px_264: jmp *(_px_tbl+1056)\n"
  "_pl_264: pushl $264\n jmp _px_lazy\n"
  ".globl _px_265\n_px_265: jmp *(_px_tbl+1060)\n"
  "_pl_265: pushl $265\n jmp _px_lazy\n"
  ".globl _px_266\n_px_266: jmp *(_px_tbl+1064)\n"
  "_pl_266: pushl $266\n jmp _px_lazy\n"
  ".globl _px_267\n_px_267: jmp *(_px_tbl+1068)\n"
  "_pl_267: pushl $267\n jmp _px_lazy\n"
  ".globl _px_268\n_px_268: jmp *(_px_tbl+1072)\n"
  "_pl_268: pushl $268\n jmp _px_lazy\n"
  ".globl _px_269\n_px_269: jmp *(_px_tbl+1076)\n"
  "_pl_269: pushl $269\n jmp _px_lazy\n"
  ".globl _px_270\n_px_270: jmp *(_px_tbl+1080)\n"
  "_pl_270: pushl $270\n jmp _px_lazy\n"
  ".globl _px_271\n_px_271: jmp *(_px_tbl+1084)\n"
  "_pl_271: pushl $271\n jmp _px_lazy\n"
  ".globl _px_272\n_px_272: jmp *(_px_tbl+1088)\n"
  "_pl_272: pushl $272\n jmp _px_lazy\n"
  ".globl _px_273\n_px_273: jmp *(_px_tbl+1092)\n"
  "_pl_273: pushl $273\n jmp _px_lazy\n"
  ".globl _px_274\n_px_274: jmp *(_px_tbl+1096)\n"
  "_pl_274: pushl $274\n jmp _px_lazy\n"
  ".globl _px_275\n_px_275: jmp *(_px_tbl+1100)\n"
  "_pl_275: pushl $275\n jmp _px_lazy\n"
  ".globl _px_276\n_px_276: jmp *(_px_tbl+1104)\n"
  "_pl_276: pushl $276\n jmp _px_lazy\n"
  ".globl _px_277\n_px_277: jmp *(_px_tbl+1108)\n"
  "_pl_277: pushl $277\n jmp _px_lazy\n"
  ".globl _px_278\n_px_278: jmp *(_px_tbl+1112)\n"
  "_pl_278: pushl $278\n jmp _px_lazy\n"
  ".globl _px_279\n_px_279: jmp *(_px_tbl+1116)\n"
  "_pl_279: pushl $279\n jmp _px_lazy\n"
  ".globl _px_280\n_px_280: jmp *(_px_tbl+1120)\n"
  "_pl_280: pushl $280\n jmp _px_lazy\n"
  ".globl _px_281\n_px_281: jmp *(_px_tbl+1124)\n"
  "_pl_281: pushl $281\n jmp _px_lazy\n"
  ".globl _px_282\n_px_282: jmp *(_px_tbl+1128)\n"
  "_pl_282: pushl $282\n jmp _px_lazy\n"
  ".globl _px_283\n_px_283: jmp *(_px_tbl+1132)\n"
  "_pl_283: pushl $283\n jmp _px_lazy\n"
  ".globl _px_284\n_px_284: jmp *(_px_tbl+1136)\n"
  "_pl_284: pushl $284\n jmp _px_lazy\n"
  ".globl _px_285\n_px_285: jmp *(_px_tbl+1140)\n"
  "_pl_285: pushl $285\n jmp _px_lazy\n"
  ".globl _px_286\n_px_286: jmp *(_px_tbl+1144)\n"
  "_pl_286: pushl $286\n jmp _px_lazy\n"
  ".globl _px_287\n_px_287: jmp *(_px_tbl+1148)\n"
  "_pl_287: pushl $287\n jmp _px_lazy\n"
  ".globl _px_288\n_px_288: jmp *(_px_tbl+1152)\n"
  "_pl_288: pushl $288\n jmp _px_lazy\n"
  ".globl _px_289\n_px_289: jmp *(_px_tbl+1156)\n"
  "_pl_289: pushl $289\n jmp _px_lazy\n"
  ".globl _px_290\n_px_290: jmp *(_px_tbl+1160)\n"
  "_pl_290: pushl $290\n jmp _px_lazy\n"
  ".globl _px_291\n_px_291: jmp *(_px_tbl+1164)\n"
  "_pl_291: pushl $291\n jmp _px_lazy\n"
  ".globl _px_292\n_px_292: jmp *(_px_tbl+1168)\n"
  "_pl_292: pushl $292\n jmp _px_lazy\n"
  ".globl _px_293\n_px_293: jmp *(_px_tbl+1172)\n"
  "_pl_293: pushl $293\n jmp _px_lazy\n"
  ".globl _px_294\n_px_294: jmp *(_px_tbl+1176)\n"
  "_pl_294: pushl $294\n jmp _px_lazy\n"
  ".globl _px_295\n_px_295: jmp *(_px_tbl+1180)\n"
  "_pl_295: pushl $295\n jmp _px_lazy\n"
  ".globl _px_296\n_px_296: jmp *(_px_tbl+1184)\n"
  "_pl_296: pushl $296\n jmp _px_lazy\n"
  ".globl _px_297\n_px_297: jmp *(_px_tbl+1188)\n"
  "_pl_297: pushl $297\n jmp _px_lazy\n"
  ".globl _px_298\n_px_298: jmp *(_px_tbl+1192)\n"
  "_pl_298: pushl $298\n jmp _px_lazy\n"
  ".globl _px_299\n_px_299: jmp *(_px_tbl+1196)\n"
  "_pl_299: pushl $299\n jmp _px_lazy\n"
  ".globl _px_300\n_px_300: jmp *(_px_tbl+1200)\n"
  "_pl_300: pushl $300\n jmp _px_lazy\n"
  ".globl _px_301\n_px_301: jmp *(_px_tbl+1204)\n"
  "_pl_301: pushl $301\n jmp _px_lazy\n"
  ".globl _px_302\n_px_302: jmp *(_px_tbl+1208)\n"
  "_pl_302: pushl $302\n jmp _px_lazy\n"
  ".globl _px_303\n_px_303: jmp *(_px_tbl+1212)\n"
  "_pl_303: pushl $303\n jmp _px_lazy\n"
  ".globl _px_304\n_px_304: jmp *(_px_tbl+1216)\n"
  "_pl_304: pushl $304\n jmp _px_lazy\n"
  ".globl _px_305\n_px_305: jmp *(_px_tbl+1220)\n"
  "_pl_305: pushl $305\n jmp _px_lazy\n"
  ".globl _px_306\n_px_306: jmp *(_px_tbl+1224)\n"
  "_pl_306: pushl $306\n jmp _px_lazy\n"
  ".globl _px_307\n_px_307: jmp *(_px_tbl+1228)\n"
  "_pl_307: pushl $307\n jmp _px_lazy\n"
  ".globl _px_308\n_px_308: jmp *(_px_tbl+1232)\n"
  "_pl_308: pushl $308\n jmp _px_lazy\n"
  ".globl _px_309\n_px_309: jmp *(_px_tbl+1236)\n"
  "_pl_309: pushl $309\n jmp _px_lazy\n"
  ".globl _px_310\n_px_310: jmp *(_px_tbl+1240)\n"
  "_pl_310: pushl $310\n jmp _px_lazy\n"
  ".globl _px_311\n_px_311: jmp *(_px_tbl+1244)\n"
  "_pl_311: pushl $311\n jmp _px_lazy\n"
  ".globl _px_312\n_px_312: jmp *(_px_tbl+1248)\n"
  "_pl_312: pushl $312\n jmp _px_lazy\n"
  ".globl _px_313\n_px_313: jmp *(_px_tbl+1252)\n"
  "_pl_313: pushl $313\n jmp _px_lazy\n"
  ".globl _px_314\n_px_314: jmp *(_px_tbl+1256)\n"
  "_pl_314: pushl $314\n jmp _px_lazy\n"
  ".globl _px_315\n_px_315: jmp *(_px_tbl+1260)\n"
  "_pl_315: pushl $315\n jmp _px_lazy\n"
  ".globl _px_316\n_px_316: jmp *(_px_tbl+1264)\n"
  "_pl_316: pushl $316\n jmp _px_lazy\n"
  ".globl _px_317\n_px_317: jmp *(_px_tbl+1268)\n"
  "_pl_317: pushl $317\n jmp _px_lazy\n"
  ".globl _px_318\n_px_318: jmp *(_px_tbl+1272)\n"
  "_pl_318: pushl $318\n jmp _px_lazy\n"
  ".globl _px_319\n_px_319: jmp *(_px_tbl+1276)\n"
  "_pl_319: pushl $319\n jmp _px_lazy\n"
  ".globl _px_320\n_px_320: jmp *(_px_tbl+1280)\n"
  "_pl_320: pushl $320\n jmp _px_lazy\n"
  ".globl _px_321\n_px_321: jmp *(_px_tbl+1284)\n"
  "_pl_321: pushl $321\n jmp _px_lazy\n"
  ".globl _px_322\n_px_322: jmp *(_px_tbl+1288)\n"
  "_pl_322: pushl $322\n jmp _px_lazy\n"
  ".globl _px_323\n_px_323: jmp *(_px_tbl+1292)\n"
  "_pl_323: pushl $323\n jmp _px_lazy\n"
  ".globl _px_324\n_px_324: jmp *(_px_tbl+1296)\n"
  "_pl_324: pushl $324\n jmp _px_lazy\n"
  ".globl _px_325\n_px_325: jmp *(_px_tbl+1300)\n"
  "_pl_325: pushl $325\n jmp _px_lazy\n"
  ".globl _px_326\n_px_326: jmp *(_px_tbl+1304)\n"
  "_pl_326: pushl $326\n jmp _px_lazy\n"
  ".globl _px_327\n_px_327: jmp *(_px_tbl+1308)\n"
  "_pl_327: pushl $327\n jmp _px_lazy\n"
  ".globl _px_328\n_px_328: jmp *(_px_tbl+1312)\n"
  "_pl_328: pushl $328\n jmp _px_lazy\n"
  ".globl _px_329\n_px_329: jmp *(_px_tbl+1316)\n"
  "_pl_329: pushl $329\n jmp _px_lazy\n"
  ".globl _px_330\n_px_330: jmp *(_px_tbl+1320)\n"
  "_pl_330: pushl $330\n jmp _px_lazy\n"
  ".globl _px_331\n_px_331: jmp *(_px_tbl+1324)\n"
  "_pl_331: pushl $331\n jmp _px_lazy\n"
  ".globl _px_332\n_px_332: jmp *(_px_tbl+1328)\n"
  "_pl_332: pushl $332\n jmp _px_lazy\n"
  ".globl _px_333\n_px_333: jmp *(_px_tbl+1332)\n"
  "_pl_333: pushl $333\n jmp _px_lazy\n"
  ".globl _px_334\n_px_334: jmp *(_px_tbl+1336)\n"
  "_pl_334: pushl $334\n jmp _px_lazy\n"
  ".globl _px_335\n_px_335: jmp *(_px_tbl+1340)\n"
  "_pl_335: pushl $335\n jmp _px_lazy\n"
  ".globl _px_336\n_px_336: jmp *(_px_tbl+1344)\n"
  "_pl_336: pushl $336\n jmp _px_lazy\n"
  ".globl _px_337\n_px_337: jmp *(_px_tbl+1348)\n"
  "_pl_337: pushl $337\n jmp _px_lazy\n"
  ".globl _px_338\n_px_338: jmp *(_px_tbl+1352)\n"
  "_pl_338: pushl $338\n jmp _px_lazy\n"
  ".globl _px_339\n_px_339: jmp *(_px_tbl+1356)\n"
  "_pl_339: pushl $339\n jmp _px_lazy\n"
  ".globl _px_340\n_px_340: jmp *(_px_tbl+1360)\n"
  "_pl_340: pushl $340\n jmp _px_lazy\n"
  ".globl _px_341\n_px_341: jmp *(_px_tbl+1364)\n"
  "_pl_341: pushl $341\n jmp _px_lazy\n"
  ".globl _px_342\n_px_342: jmp *(_px_tbl+1368)\n"
  "_pl_342: pushl $342\n jmp _px_lazy\n"
  ".globl _px_343\n_px_343: jmp *(_px_tbl+1372)\n"
  "_pl_343: pushl $343\n jmp _px_lazy\n"
  ".globl _px_344\n_px_344: jmp *(_px_tbl+1376)\n"
  "_pl_344: pushl $344\n jmp _px_lazy\n"
  ".globl _px_345\n_px_345: jmp *(_px_tbl+1380)\n"
  "_pl_345: pushl $345\n jmp _px_lazy\n"
  ".globl _px_346\n_px_346: jmp *(_px_tbl+1384)\n"
  "_pl_346: pushl $346\n jmp _px_lazy\n"
  ".globl _px_347\n_px_347: jmp *(_px_tbl+1388)\n"
  "_pl_347: pushl $347\n jmp _px_lazy\n"
  ".globl _px_348\n_px_348: jmp *(_px_tbl+1392)\n"
  "_pl_348: pushl $348\n jmp _px_lazy\n"
  ".globl _px_349\n_px_349: jmp *(_px_tbl+1396)\n"
  "_pl_349: pushl $349\n jmp _px_lazy\n"
  ".globl _px_350\n_px_350: jmp *(_px_tbl+1400)\n"
  "_pl_350: pushl $350\n jmp _px_lazy\n"
  ".globl _px_351\n_px_351: jmp *(_px_tbl+1404)\n"
  "_pl_351: pushl $351\n jmp _px_lazy\n"
  ".globl _px_352\n_px_352: jmp *(_px_tbl+1408)\n"
  "_pl_352: pushl $352\n jmp _px_lazy\n"
  ".globl _px_353\n_px_353: jmp *(_px_tbl+1412)\n"
  "_pl_353: pushl $353\n jmp _px_lazy\n"
  ".globl _px_354\n_px_354: jmp *(_px_tbl+1416)\n"
  "_pl_354: pushl $354\n jmp _px_lazy\n"
  ".globl _px_355\n_px_355: jmp *(_px_tbl+1420)\n"
  "_pl_355: pushl $355\n jmp _px_lazy\n"
  ".globl _px_356\n_px_356: jmp *(_px_tbl+1424)\n"
  "_pl_356: pushl $356\n jmp _px_lazy\n"
  ".globl _px_357\n_px_357: jmp *(_px_tbl+1428)\n"
  "_pl_357: pushl $357\n jmp _px_lazy\n"
  ".globl _px_358\n_px_358: jmp *(_px_tbl+1432)\n"
  "_pl_358: pushl $358\n jmp _px_lazy\n"
  ".globl _px_359\n_px_359: jmp *(_px_tbl+1436)\n"
  "_pl_359: pushl $359\n jmp _px_lazy\n"
  ".globl _px_360\n_px_360: jmp *(_px_tbl+1440)\n"
  "_pl_360: pushl $360\n jmp _px_lazy\n"
  ".globl _px_361\n_px_361: jmp *(_px_tbl+1444)\n"
  "_pl_361: pushl $361\n jmp _px_lazy\n"
  ".globl _px_362\n_px_362: jmp *(_px_tbl+1448)\n"
  "_pl_362: pushl $362\n jmp _px_lazy\n"
  ".globl _px_363\n_px_363: jmp *(_px_tbl+1452)\n"
  "_pl_363: pushl $363\n jmp _px_lazy\n"
  ".globl _px_364\n_px_364: jmp *(_px_tbl+1456)\n"
  "_pl_364: pushl $364\n jmp _px_lazy\n"
  ".globl _px_365\n_px_365: jmp *(_px_tbl+1460)\n"
  "_pl_365: pushl $365\n jmp _px_lazy\n"
  ".globl _px_366\n_px_366: jmp *(_px_tbl+1464)\n"
  "_pl_366: pushl $366\n jmp _px_lazy\n"
  "_px_lazy:\n pushal\n pushfl\n pushl 36(%esp)\n call _px_resolve\n addl $4, %esp\n movl %eax, 36(%esp)\n popfl\n popal\n ret\n"
  ".data\n.globl _px_tbl\n_px_tbl:\n"
  " .long _pl_0\n"
  " .long _pl_1\n"
  " .long _pl_2\n"
  " .long _pl_3\n"
  " .long _pl_4\n"
  " .long _pl_5\n"
  " .long _pl_6\n"
  " .long _pl_7\n"
  " .long _pl_8\n"
  " .long _pl_9\n"
  " .long _pl_10\n"
  " .long _pl_11\n"
  " .long _pl_12\n"
  " .long _pl_13\n"
  " .long _pl_14\n"
  " .long _pl_15\n"
  " .long _pl_16\n"
  " .long _pl_17\n"
  " .long _pl_18\n"
  " .long _pl_19\n"
  " .long _pl_20\n"
  " .long _pl_21\n"
  " .long _pl_22\n"
  " .long _pl_23\n"
  " .long _pl_24\n"
  " .long _pl_25\n"
  " .long _pl_26\n"
  " .long _pl_27\n"
  " .long _pl_28\n"
  " .long _pl_29\n"
  " .long _pl_30\n"
  " .long _pl_31\n"
  " .long _pl_32\n"
  " .long _pl_33\n"
  " .long _pl_34\n"
  " .long _pl_35\n"
  " .long _pl_36\n"
  " .long _pl_37\n"
  " .long _pl_38\n"
  " .long _pl_39\n"
  " .long _pl_40\n"
  " .long _pl_41\n"
  " .long _pl_42\n"
  " .long _pl_43\n"
  " .long _pl_44\n"
  " .long _pl_45\n"
  " .long _pl_46\n"
  " .long _pl_47\n"
  " .long _pl_48\n"
  " .long _pl_49\n"
  " .long _pl_50\n"
  " .long _pl_51\n"
  " .long _pl_52\n"
  " .long _pl_53\n"
  " .long _pl_54\n"
  " .long _pl_55\n"
  " .long _pl_56\n"
  " .long _pl_57\n"
  " .long _pl_58\n"
  " .long _pl_59\n"
  " .long _pl_60\n"
  " .long _pl_61\n"
  " .long _pl_62\n"
  " .long _pl_63\n"
  " .long _pl_64\n"
  " .long _pl_65\n"
  " .long _pl_66\n"
  " .long _pl_67\n"
  " .long _pl_68\n"
  " .long _pl_69\n"
  " .long _pl_70\n"
  " .long _pl_71\n"
  " .long _pl_72\n"
  " .long _pl_73\n"
  " .long _pl_74\n"
  " .long _pl_75\n"
  " .long _pl_76\n"
  " .long _pl_77\n"
  " .long _pl_78\n"
  " .long _pl_79\n"
  " .long _pl_80\n"
  " .long _pl_81\n"
  " .long _pl_82\n"
  " .long _pl_83\n"
  " .long _pl_84\n"
  " .long _pl_85\n"
  " .long _pl_86\n"
  " .long _pl_87\n"
  " .long _pl_88\n"
  " .long _pl_89\n"
  " .long _pl_90\n"
  " .long _pl_91\n"
  " .long _pl_92\n"
  " .long _pl_93\n"
  " .long _pl_94\n"
  " .long _pl_95\n"
  " .long _pl_96\n"
  " .long _pl_97\n"
  " .long _pl_98\n"
  " .long _pl_99\n"
  " .long _pl_100\n"
  " .long _pl_101\n"
  " .long _pl_102\n"
  " .long _pl_103\n"
  " .long _pl_104\n"
  " .long _pl_105\n"
  " .long _pl_106\n"
  " .long _pl_107\n"
  " .long _pl_108\n"
  " .long _pl_109\n"
  " .long _pl_110\n"
  " .long _pl_111\n"
  " .long _pl_112\n"
  " .long _pl_113\n"
  " .long _pl_114\n"
  " .long _pl_115\n"
  " .long _pl_116\n"
  " .long _pl_117\n"
  " .long _pl_118\n"
  " .long _pl_119\n"
  " .long _pl_120\n"
  " .long _pl_121\n"
  " .long _pl_122\n"
  " .long _pl_123\n"
  " .long _pl_124\n"
  " .long _pl_125\n"
  " .long _pl_126\n"
  " .long _pl_127\n"
  " .long _pl_128\n"
  " .long _pl_129\n"
  " .long _pl_130\n"
  " .long _pl_131\n"
  " .long _pl_132\n"
  " .long _pl_133\n"
  " .long _pl_134\n"
  " .long _pl_135\n"
  " .long _pl_136\n"
  " .long _pl_137\n"
  " .long _pl_138\n"
  " .long _pl_139\n"
  " .long _pl_140\n"
  " .long _pl_141\n"
  " .long _pl_142\n"
  " .long _pl_143\n"
  " .long _pl_144\n"
  " .long _pl_145\n"
  " .long _pl_146\n"
  " .long _pl_147\n"
  " .long _pl_148\n"
  " .long _pl_149\n"
  " .long _pl_150\n"
  " .long _pl_151\n"
  " .long _pl_152\n"
  " .long _pl_153\n"
  " .long _pl_154\n"
  " .long _pl_155\n"
  " .long _pl_156\n"
  " .long _pl_157\n"
  " .long _pl_158\n"
  " .long _pl_159\n"
  " .long _pl_160\n"
  " .long _pl_161\n"
  " .long _pl_162\n"
  " .long _pl_163\n"
  " .long _pl_164\n"
  " .long _pl_165\n"
  " .long _pl_166\n"
  " .long _pl_167\n"
  " .long _pl_168\n"
  " .long _pl_169\n"
  " .long _pl_170\n"
  " .long _pl_171\n"
  " .long _pl_172\n"
  " .long _pl_173\n"
  " .long _pl_174\n"
  " .long _pl_175\n"
  " .long _pl_176\n"
  " .long _pl_177\n"
  " .long _pl_178\n"
  " .long _pl_179\n"
  " .long _pl_180\n"
  " .long _pl_181\n"
  " .long _pl_182\n"
  " .long _pl_183\n"
  " .long _pl_184\n"
  " .long _pl_185\n"
  " .long _pl_186\n"
  " .long _pl_187\n"
  " .long _pl_188\n"
  " .long _pl_189\n"
  " .long _pl_190\n"
  " .long _pl_191\n"
  " .long _pl_192\n"
  " .long _pl_193\n"
  " .long _pl_194\n"
  " .long _pl_195\n"
  " .long _pl_196\n"
  " .long _pl_197\n"
  " .long _pl_198\n"
  " .long _pl_199\n"
  " .long _pl_200\n"
  " .long _pl_201\n"
  " .long _pl_202\n"
  " .long _pl_203\n"
  " .long _pl_204\n"
  " .long _pl_205\n"
  " .long _pl_206\n"
  " .long _pl_207\n"
  " .long _pl_208\n"
  " .long _pl_209\n"
  " .long _pl_210\n"
  " .long _pl_211\n"
  " .long _pl_212\n"
  " .long _pl_213\n"
  " .long _pl_214\n"
  " .long _pl_215\n"
  " .long _pl_216\n"
  " .long _pl_217\n"
  " .long _pl_218\n"
  " .long _pl_219\n"
  " .long _pl_220\n"
  " .long _pl_221\n"
  " .long _pl_222\n"
  " .long _pl_223\n"
  " .long _pl_224\n"
  " .long _pl_225\n"
  " .long _pl_226\n"
  " .long _pl_227\n"
  " .long _pl_228\n"
  " .long _pl_229\n"
  " .long _pl_230\n"
  " .long _pl_231\n"
  " .long _pl_232\n"
  " .long _pl_233\n"
  " .long _pl_234\n"
  " .long _pl_235\n"
  " .long _pl_236\n"
  " .long _pl_237\n"
  " .long _pl_238\n"
  " .long _pl_239\n"
  " .long _pl_240\n"
  " .long _pl_241\n"
  " .long _pl_242\n"
  " .long _pl_243\n"
  " .long _pl_244\n"
  " .long _pl_245\n"
  " .long _pl_246\n"
  " .long _pl_247\n"
  " .long _pl_248\n"
  " .long _pl_249\n"
  " .long _pl_250\n"
  " .long _pl_251\n"
  " .long _pl_252\n"
  " .long _pl_253\n"
  " .long _pl_254\n"
  " .long _pl_255\n"
  " .long _pl_256\n"
  " .long _pl_257\n"
  " .long _pl_258\n"
  " .long _pl_259\n"
  " .long _pl_260\n"
  " .long _pl_261\n"
  " .long _pl_262\n"
  " .long _pl_263\n"
  " .long _pl_264\n"
  " .long _pl_265\n"
  " .long _pl_266\n"
  " .long _pl_267\n"
  " .long _pl_268\n"
  " .long _pl_269\n"
  " .long _pl_270\n"
  " .long _pl_271\n"
  " .long _pl_272\n"
  " .long _pl_273\n"
  " .long _pl_274\n"
  " .long _pl_275\n"
  " .long _pl_276\n"
  " .long _pl_277\n"
  " .long _pl_278\n"
  " .long _pl_279\n"
  " .long _pl_280\n"
  " .long _pl_281\n"
  " .long _pl_282\n"
  " .long _pl_283\n"
  " .long _pl_284\n"
  " .long _pl_285\n"
  " .long _pl_286\n"
  " .long _pl_287\n"
  " .long _pl_288\n"
  " .long _pl_289\n"
  " .long _pl_290\n"
  " .long _pl_291\n"
  " .long _pl_292\n"
  " .long _pl_293\n"
  " .long _pl_294\n"
  " .long _pl_295\n"
  " .long _pl_296\n"
  " .long _pl_297\n"
  " .long _pl_298\n"
  " .long _pl_299\n"
  " .long _pl_300\n"
  " .long _pl_301\n"
  " .long _pl_302\n"
  " .long _pl_303\n"
  " .long _pl_304\n"
  " .long _pl_305\n"
  " .long _pl_306\n"
  " .long _pl_307\n"
  " .long _pl_308\n"
  " .long _pl_309\n"
  " .long _pl_310\n"
  " .long _pl_311\n"
  " .long _pl_312\n"
  " .long _pl_313\n"
  " .long _pl_314\n"
  " .long _pl_315\n"
  " .long _pl_316\n"
  " .long _pl_317\n"
  " .long _pl_318\n"
  " .long _pl_319\n"
  " .long _pl_320\n"
  " .long _pl_321\n"
  " .long _pl_322\n"
  " .long _pl_323\n"
  " .long _pl_324\n"
  " .long _pl_325\n"
  " .long _pl_326\n"
  " .long _pl_327\n"
  " .long _pl_328\n"
  " .long _pl_329\n"
  " .long _pl_330\n"
  " .long _pl_331\n"
  " .long _pl_332\n"
  " .long _pl_333\n"
  " .long _pl_334\n"
  " .long _pl_335\n"
  " .long _pl_336\n"
  " .long _pl_337\n"
  " .long _pl_338\n"
  " .long _pl_339\n"
  " .long _pl_340\n"
  " .long _pl_341\n"
  " .long _pl_342\n"
  " .long _pl_343\n"
  " .long _pl_344\n"
  " .long _pl_345\n"
  " .long _pl_346\n"
  " .long _pl_347\n"
  " .long _pl_348\n"
  " .long _pl_349\n"
  " .long _pl_350\n"
  " .long _pl_351\n"
  " .long _pl_352\n"
  " .long _pl_353\n"
  " .long _pl_354\n"
  " .long _pl_355\n"
  " .long _pl_356\n"
  " .long _pl_357\n"
  " .long _pl_358\n"
  " .long _pl_359\n"
  " .long _pl_360\n"
  " .long _pl_361\n"
  " .long _pl_362\n"
  " .long _pl_363\n"
  " .long _pl_364\n"
  " .long _pl_365\n"
  " .long _pl_366\n"
  ".text\n");
