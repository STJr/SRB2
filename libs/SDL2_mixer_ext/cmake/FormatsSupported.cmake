
set(SUMMARY_SUPPORTED_FORMATS "WAV(SFX);VOC(SFX);AIFF(SFX)")
set(SUMMARY_SUPPORTED_FORMATS_PCM "WAV(SFX);VOC(SFX);AIFF(SFX)")
set(SUMMARY_SUPPORTED_FORMATS_MIDI "")
set(SUMMARY_SUPPORTED_FORMATS_TRACKER "")
set(SUMMARY_SUPPORTED_FORMATS_CHIPTUNE "")
set(SUMMARY_SUPPORTED_FORMATS_OTHER "")


macro(appendFormats _in_formats_list)
    set(tail ${SUMMARY_SUPPORTED_FORMATS})

    foreach(FORMATNEW ${_in_formats_list})
        string(STRIP ${FORMATNEW} FORMATNEW_S)
        list(FIND tail "${FORMATNEW_S}" _index)
        if(${_index} EQUAL -1)
            list(APPEND tail ${FORMATNEW_S})
        endif()
    endforeach()

    set(SUMMARY_SUPPORTED_FORMATS ${tail} PARENT_SCOPE)
    unset(tail)

endmacro()


function(appendPcmFormats _in_formats_list)
    appendFormats("${_in_formats_list}")
    set(tail ${SUMMARY_SUPPORTED_FORMATS_PCM})

    foreach(FORMATNEW ${_in_formats_list})
        string(STRIP ${FORMATNEW} FORMATNEW_S)
        list(FIND tail "${FORMATNEW_S}" _index)
        if(${_index} EQUAL -1)
            list(APPEND tail ${FORMATNEW_S})
        endif()
    endforeach()

    set(SUMMARY_SUPPORTED_FORMATS_PCM ${tail} PARENT_SCOPE)
endfunction()

function(appendMidiFormats _in_formats_list)
    appendFormats("${_in_formats_list}")
    set(tail ${SUMMARY_SUPPORTED_FORMATS_MIDI})

    foreach(FORMATNEW ${_in_formats_list})
        string(STRIP ${FORMATNEW} FORMATNEW_S)
        list(FIND tail "${FORMATNEW_S}" _index)
        if(${_index} EQUAL -1)
            list(APPEND tail ${FORMATNEW_S})
        endif()
    endforeach()

    set(SUMMARY_SUPPORTED_FORMATS_MIDI ${tail} PARENT_SCOPE)
endfunction()


function(appendTrackerFormats _in_formats_list)
    appendFormats("${_in_formats_list}")
    set(tail ${SUMMARY_SUPPORTED_FORMATS_TRACKER})

    foreach(FORMATNEW ${_in_formats_list})
        string(STRIP ${FORMATNEW} FORMATNEW_S)
        list(FIND tail "${FORMATNEW_S}" _index)
        if(${_index} EQUAL -1)
            list(APPEND tail ${FORMATNEW_S})
        endif()
    endforeach()

    set(SUMMARY_SUPPORTED_FORMATS_TRACKER ${tail} PARENT_SCOPE)
endfunction()


function(appendChiptuneFormats _in_formats_list)
    appendFormats("${_in_formats_list}")
    set(tail ${SUMMARY_SUPPORTED_FORMATS_CHIPTUNE})

    foreach(FORMATNEW ${_in_formats_list})
        string(STRIP ${FORMATNEW} FORMATNEW_S)
        list(FIND tail "${FORMATNEW_S}" _index)
        if(${_index} EQUAL -1)
            list(APPEND tail ${FORMATNEW_S})
        endif()
    endforeach()

    set(SUMMARY_SUPPORTED_FORMATS_CHIPTUNE ${tail} PARENT_SCOPE)
endfunction()


function(appendOtherFormats _in_formats_list)
    # appendFormats(${_in_formats_list}) // Other things like CMD things shouldn't be listed as "all"
    set(tail ${SUMMARY_SUPPORTED_FORMATS_OTHER})

    foreach(FORMATNEW ${_in_formats_list})
        string(STRIP ${FORMATNEW} FORMATNEW_S)
        list(FIND tail "${FORMATNEW_S}" _index)
        if(${_index} EQUAL -1)
            list(APPEND tail ${FORMATNEW_S})
        endif()
    endforeach()

    set(SUMMARY_SUPPORTED_FORMATS_OTHER ${tail} PARENT_SCOPE)
endfunction()


function(printFormatsList _list)
    set(outputString)
    set(outputString_pre)
    set(counter 0)

    list(SORT ${_list})

    foreach(FORMATNEW ${${_list}})
        if(NOT outputString)
            set(outputString "    ${FORMATNEW}")
            set(outputString_pre "${outputString}")
        else()
            set(outputString_pre "${outputString}, ${FORMATNEW}")
        endif()

        unset(outputString_pre_len)
        string(LENGTH ${outputString_pre} outputString_pre_len)

        if(${outputString_pre_len} LESS 79)
            set(outputString "${outputString_pre}")
        else()
            message("${outputString},")
            set(outputString "    ${FORMATNEW}")
        endif()
    endforeach()

    if(outputString)
        message("${outputString}")
    endif()
    message("-------------------------------------------------------------------------------")
endfunction()


function(printFormats)
    message("===============================================================================")
    message("Enabled audio file formats:")
    message("-------------------------------------------------------------------------------")

    if(SUMMARY_SUPPORTED_FORMATS_PCM)
        list(LENGTH SUMMARY_SUPPORTED_FORMATS_PCM LIST_SIZE)
        message("PCM-based formats (${LIST_SIZE}):")
        printFormatsList(SUMMARY_SUPPORTED_FORMATS_PCM)
    endif()

    if(SUMMARY_SUPPORTED_FORMATS_MIDI)
        list(LENGTH SUMMARY_SUPPORTED_FORMATS_MIDI LIST_SIZE)
        message("MIDI formats (${LIST_SIZE}):")
        printFormatsList(SUMMARY_SUPPORTED_FORMATS_MIDI)
    endif()

    if(SUMMARY_SUPPORTED_FORMATS_TRACKER)
        list(LENGTH SUMMARY_SUPPORTED_FORMATS_TRACKER LIST_SIZE)
        message("Tracker formats (${LIST_SIZE}):")
        printFormatsList(SUMMARY_SUPPORTED_FORMATS_TRACKER)
    endif()

    if(SUMMARY_SUPPORTED_FORMATS_CHIPTUNE)
        list(LENGTH SUMMARY_SUPPORTED_FORMATS_CHIPTUNE LIST_SIZE)
        message("Chiptune formats (${LIST_SIZE}):")
        printFormatsList(SUMMARY_SUPPORTED_FORMATS_CHIPTUNE)
    endif()

    if(SUMMARY_SUPPORTED_FORMATS_OTHER)
        list(LENGTH SUMMARY_SUPPORTED_FORMATS_OTHER LIST_SIZE)
        message("Other music (${LIST_SIZE}):")
        printFormatsList(SUMMARY_SUPPORTED_FORMATS_OTHER)
    endif()

    list(LENGTH SUMMARY_SUPPORTED_FORMATS LIST_SIZE)
    message("All formats (${LIST_SIZE}):")
    printFormatsList(SUMMARY_SUPPORTED_FORMATS)

endfunction()
