# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0"

if(USE_NIST_RAND)
  FILE(GLOB HEADERS ${CMAKE_CURRENT_LIST_DIR}/*.h)

  # target_sources(bike-test
  #   PRIVATE
  #     ${CMAKE_CURRENT_LIST_DIR}/FromNIST/rng.c
  #     ${CMAKE_CURRENT_LIST_DIR}/FromNIST/PQCgenKAT_kem.c

  #     ${HEADERS}
  # )

  target_sources(bike-fia-simulation
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/FromNIST/rng.c 
      ${CMAKE_CURRENT_LIST_DIR}/FromNIST/PQCgenKAT_kem.c
      
      ${HEADERS}
  )

  target_sources(gen-hw-s-plot-data
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/FromNIST/rng.c
      ${CMAKE_CURRENT_LIST_DIR}/FromNIST/PQCgenKAT_kem.c

      ${HEADERS}
  )

else()
  # target_sources(bike-test
    # PRIVATE
      # ${CMAKE_CURRENT_LIST_DIR}/main_test.c
  # )

  target_sources(bike-fia-simulation
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/fia_simulation.c
  )

  target_sources(gen-hw-s-plot-data
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/gen_hw_s_plot_data.c
  )


endif()
