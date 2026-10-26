/*
THIRD_PERSON_CAMERA.C

An optional local over-the-shoulder view on foot (display.third_person).
The director's following camera draws it from the biped's own camera track,
moved to one side; Backspace (or the gamepad's Black button, held) toggles
it, and a weapon's zoom returns to first person while zoomed. The
deterministic camera used for shooting keeps its first-person view, and
nothing the machines send each other changes: the reticle is moved to where
the first-person ray lands (passenger_camera.c's reticle).
*/

#include "cseries.h"
#include "third_person_camera.h"
#include "camera/director.h"
#include "cutscene/cinematics.h"
#include "game/players.h"
#include "objects/objects.h"
#include "physics/collisions.h"
#include "units/units.h"

double config_real(const char *name);
int config_boolean(const char *name);
unsigned long config_changes(void);

static boolean toggled_off[MAXIMUM_LOCAL_PLAYERS];

static struct
{
	boolean initialized;
	unsigned long read_at;
	boolean enabled;
	real side;
	real up;
	real distance;
} settings;

static void read_settings(void)
{
	unsigned long changes = config_changes();

	if (settings.initialized && settings.read_at == changes)
		return;
	settings.initialized = TRUE;
	settings.read_at = changes;
	settings.enabled = config_boolean("display.third_person") != 0;
	settings.side = (real)PIN(config_real("display.third_person_side"), -1.0, 1.0);
	settings.up = (real)PIN(config_real("display.third_person_up"), -0.5, 0.5);
	settings.distance = (real)PIN(config_real("display.third_person_distance"), 0.25, 3.0);
}

boolean third_person_camera_setting(void)
{
	read_settings();
	return settings.enabled;
}

void third_person_camera_toggle(short local_player_index)
{
	if (local_player_index >= 0 && local_player_index < MAXIMUM_LOCAL_PLAYERS)
		toggled_off[local_player_index] = !toggled_off[local_player_index];
}

boolean third_person_camera_wanted(short local_player_index)
{
	long unit_index, player_index;
	struct unit_datum *unit;

	/* (no game_engine_running() test: this fork allows the view in
	multiplayer; upstream's passenger view refuses it there) */
	if (local_player_index < 0 || local_player_index >= MAXIMUM_LOCAL_PLAYERS ||
		!third_person_camera_setting() || toggled_off[local_player_index] ||
		cinematic_in_progress() ||
		(director_camera_scripted && *director_camera_scripted))
		return FALSE;
	player_index = local_player_get_player_index(local_player_index);
	if (player_index == NONE)
		return FALSE;
	unit_index = player_control_get_unit_index(local_player_index);
	if (unit_index == NONE || player_get(player_index)->unit_index != unit_index)
		return FALSE;
	unit = unit_try_and_get(unit_index);
	/* (in a seat, the vehicle's own camera and the passenger view decide) */
	return unit && !TEST_FLAG(unit->object.damage_flags, _object_dead_bit) &&
		unit->object.parent_object_index == NONE;
}

boolean third_person_camera_zoomed(short local_player_index)
{
	return player_control_get_zoom_level(local_player_index) != NONE;
}

boolean third_person_camera_enabled(short local_player_index)
{
	return third_person_camera_wanted(local_player_index) &&
		!third_person_camera_zoomed(local_player_index);
}

real third_person_camera_distance_scale(void)
{
	read_settings();
	return settings.distance;
}

void third_person_camera_shoulder(
	long unit_index,
	real_point3d const *head,
	real_vector3d const *forward,
	real_vector3d *offset)
{
	real_vector3d right, ray;
	real_vector2d heading;
	struct collision_result collision;
	real side;

	read_settings();
	side = settings.side;
	heading.i = forward->i;
	heading.j = forward->j;
	if (normalize2d(&heading) == 0.f)
	{
		heading.i = 1.f;
		heading.j = 0.f;
	}
	/* The observer moves its focus right by +j (observer.c). A wall at the
	shoulder brings the camera in, with 0.15 world units to spare. */
	right.i = heading.j;
	right.j = -heading.i;
	right.k = 0.f;
	if (side != 0.f)
	{
		real reach = (real)fabs(side) + 0.15f;

		scale_vector3d(&right, side < 0.f ? -reach : reach, &ray);
		if (collision_test_vector(_collision_test_environment_flags, head, &ray, unit_index, &collision))
		{
			real_vector3d to_wall;
			real allowed;

			vector_from_points3d(head, &collision.point, &to_wall);
			allowed = MAX(magnitude3d(&to_wall) - 0.15f, 0.f);

			if (allowed < (real)fabs(side))
				side = side < 0.f ? -allowed : allowed;
		}
	}
	offset->i = 0.f;
	offset->j = side;
	offset->k = settings.up;
}
