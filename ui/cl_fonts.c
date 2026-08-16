/*
 * cl_fonts.c
 *
 * The measured leading for each face - see the note in cl_fonts.h for why
 * these exist.  Every value is the number of pixels a label has to move
 * down from the Figma node's y for its ink to land where the frame puts it,
 * as reported by test/compare_to_figma.py.
 */

#include "cl_fonts.h"

const cl_face_t CL_SPEED_136 = { &cl_font_speed_136, 40 };
const cl_face_t CL_RANGE_15  = { &cl_font_range_15,   4 };

const cl_face_t CL_BOLD_22   = { &cl_font_bold_22,    6 };
const cl_face_t CL_BOLD_16   = { &cl_font_bold_16,    3 };
const cl_face_t CL_BOLD_14   = { &cl_font_bold_14,    3 };

const cl_face_t CL_SEMI_24   = { &cl_font_semi_24,    5 };
const cl_face_t CL_SEMI_20   = { &cl_font_semi_20,    3 };
const cl_face_t CL_SEMI_18   = { &cl_font_semi_18,    3 };
const cl_face_t CL_SEMI_16   = { &cl_font_semi_16,    4 };
const cl_face_t CL_SEMI_14   = { &cl_font_semi_14,    2 };
