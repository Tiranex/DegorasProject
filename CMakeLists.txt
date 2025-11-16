# ======================================================================================================================
#  Copyright (C) 2025 Degoras Project Team
#
#  Authors:
#
#      Ángel Vera Herrera       <avera@roa.es> | <angelvh.engr@gmail.com>
#      Jesús Relinque Madroñal
#
#  Licensed under the MIT License.
# =================================T=====================================================================================
#   APP SPACEOBJECT MANAGER - CMAKELIST (Versión QT)
# ======================================================================================================================

# ----------------------------------------------------------------------------------------------------------------------
# BASIC PROJECT CONFIGURATION

# Minimum CMake version required.
cmake_minimum_required(VERSION 3.31)

# Project definition.
project(DegorasProject LANGUAGES CXX)

add_subdirectory(DP_Core)
add_subdirectory(Filter_Tool)
add_subdirectory(Bd_Degoras)
