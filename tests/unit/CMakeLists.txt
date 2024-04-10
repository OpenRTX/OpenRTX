find_package(Threads REQUIRED)

function(add_unit_test test_name test_file)
    add_executable(${test_name} ${test_file})
    add_test(${test_name} ${test_name})
    target_compile_definitions(${test_name} PUBLIC -DPLATFORM_LINUX)
    target_link_libraries(${test_name} PUBLIC
        Core
        Protocols
        RTX
        UIDefault
        SDL2::SDL2
        Threads::Threads
        codec2  
    )
endfunction()

add_unit_test(m17_golay_test M17_golay.cpp)
add_unit_test(m17_viterbi_test M17_viterbi.cpp)
add_unit_test(m17_rrc_test M17_rrc.cpp)
add_unit_test(cps_test cps.c)
#add_unit_test(linux_inputStream linux_inputStream_test.cpp)
add_unit_test(sine_test play_sine.c)
#add_unit_test(m17_demodulator_test M17_demodulator.cpp)
#add_unit_test(vp_test voice_prompts.c)
