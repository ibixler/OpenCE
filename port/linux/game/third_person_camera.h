/* The local over-the-shoulder view on foot (third_person_camera.c). */
#ifndef THIRD_PERSON_CAMERA_H
#define THIRD_PERSON_CAMERA_H

#include "math/real_math.h"

boolean third_person_camera_setting(void);
void third_person_camera_toggle(short local_player_index);
/* on foot, alive, the setting on and not toggled off; zoom not considered */
boolean third_person_camera_wanted(short local_player_index);
boolean third_person_camera_zoomed(short local_player_index);
/* wanted and not zoomed: the view is third person now */
boolean third_person_camera_enabled(short local_player_index);
real third_person_camera_distance_scale(void);
/* the focus offset (forward, right, up) of the shoulder, brought in from walls */
void third_person_camera_shoulder(
	long unit_index,
	real_point3d const *head,
	real_vector3d const *forward,
	real_vector3d *offset);

#endif
