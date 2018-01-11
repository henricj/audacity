sound_type snd_make_white(time_type t0, rate_type sr, time_type d);
sound_type snd_white(time_type t0, rate_type sr, time_type d);
    /* LISP: (snd-white ANYNUM ANYNUM ANYNUM) */

/* CHANGE LOG
 * --------------------------------------------------------------------
 * 28Apr03 rbd all systems now use rand(), based on DM's modifications
 */

#include <stdlib.h>
#include <math.h>


