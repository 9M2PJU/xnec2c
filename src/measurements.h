#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H 1

#include <math.h>
#include <stdio.h>

// The enum, and the struct measurement_t values must be in
// the same order or this will not work!
enum MEASUREMENT_INDEXES
{
	MEAS_MHZ,
	MEAS_ZREAL,
	MEAS_ZIMAG,
	MEAS_ZMAG,
	MEAS_ZPHASE,
	MEAS_VSWR,
	MEAS_S11,
	MEAS_S11_REAL,
	MEAS_S11_IMAG,
	MEAS_S11_ANG,
	MEAS_GAIN_MAX,
	MEAS_GAIN_NET,
	MEAS_GAIN_THETA,
	MEAS_GAIN_PHI,
	MEAS_GAIN_VIEWER,
	MEAS_GAIN_VIEWER_NET,
	MEAS_FB_RATIO,
	MEAS_GAIN_DEV_PX,
	MEAS_GAIN_DEV_NX,
	MEAS_GAIN_DEV_PY,
	MEAS_GAIN_DEV_NY,
	MEAS_GAIN_DEV_PZ,
	MEAS_GAIN_DEV_NZ,
	MEAS_ANT_TEMP,
	MEAS_ANT_TEMP_TOT,
	MEAS_GT,

	MEAS_COUNT
};

// Defined in measurements.c:
extern const char *meas_names[];
extern const char *meas_display_names[];
extern const char *meas_descriptions[];

/**
 * ant_temp_z_world() - world-frame z-component of a pattern cell after
 *     virtual rotation to the desired antenna elevation.
 * @tht:      NEC theta of the cell (radians)
 * @phi:      NEC phi of the cell (radians)
 * @tht_mg:   NEC theta of the max-gain direction (radians)
 * @phi_mg:   NEC phi of the max-gain direction (radians)
 * @elev_rad: desired boresight elevation above horizontal (radians)
 *
 * Rodrigues rotation about the horizontal axis perpendicular to the
 * max-gain azimuth: k = (-sin phi_mg, cos phi_mg, 0).
 * The rotation angle places the boresight at elevation elev_rad.
 *
 * Returns positive when the cell faces sky, negative for earth.
 *
 * Callers compare against ANT_TEMP_Z_EPSILON rather than 0.0 to avoid
 * IEEE754 noise at exact boundary angles (e.g. cos(pi/2) ≈ 6e-17).
 */
#define ANT_TEMP_Z_EPSILON 1e-10
static inline double ant_temp_z_world(
		double tht, double phi,
		double tht_mg, double phi_mg,
		double elev_rad)
{
	double alpha = tht_mg + elev_rad;
	return cos(tht) * sin(alpha)
		- sin(tht) * cos(phi - phi_mg) * cos(alpha);
}

/**
 * ant_temp_rotate_point() - Rodrigues rotation of a pattern cell direction
 * @tht:      NEC theta of the cell (radians)
 * @phi:      NEC phi of the cell (radians)
 * @tht_mg:   NEC theta of the max-gain direction (radians)
 * @phi_mg:   NEC phi of the max-gain direction (radians)
 * @elev_rad: desired boresight elevation above horizontal (radians)
 * @xr:       output rotated x component
 * @yr:       output rotated y component
 * @zr:       output rotated z component
 *
 * Applies the same Rodrigues rotation as ant_temp_z_world() but returns
 * the full rotated Cartesian direction vector.  Used by the noise-mode
 * renderer to place pattern cells at visually rotated positions so the
 * sky/earth boundary appears horizontal while the pattern tilts upward.
 *
 * Rotation axis: k = (-sin phi_mg, cos phi_mg, 0)
 * Rotation angle: elev_rad
 * Formula: v' = v cos E + (k x v) sin E + k (k . v)(1 - cos E)
 */
static inline void ant_temp_rotate_point(
		double tht, double phi,
		double tht_mg, double phi_mg,
		double elev_rad,
		double *xr, double *yr, double *zr)
{
	/* Original Cartesian direction from spherical (θ, φ) */
	double st = sin(tht), ct = cos(tht);
	double sp = sin(phi), cp = cos(phi);
	double vx = st * cp;
	double vy = st * sp;
	double vz = ct;

	/* Rotation axis k = (-sin φ_mg, cos φ_mg, 0) */
	double kx = -sin(phi_mg);
	double ky =  cos(phi_mg);

	/* k × v  (kz = 0) */
	double cx = -ky * vz;
	double cy =  kx * vz;
	double cz =  ky * vx - kx * vy;

	/* k · v  (kz = 0) */
	double dot = kx * vx + ky * vy;

	double ce = cos(elev_rad);
	double se = sin(elev_rad);

	*xr = vx * ce + cx * se + kx * dot * (1.0 - ce);
	*yr = vy * ce + cy * se + ky * dot * (1.0 - ce);
	*zr = vz * ce + cz * se;
}

#define ANT_TEMP_ENV_COUNT 8
extern const char *ant_temp_env_names[ANT_TEMP_ENV_COUNT];
extern const char *noise_env_widget_ids[ANT_TEMP_ENV_COUNT];

typedef struct
{
	union {
		struct
		{
			double mhz;
			double zreal, zimag;
			double zmag, zphase;
			double vswr;
			double s11;
			double s11_real, s11_imag, s11_ang;
			double gain_max, gain_net;
			double gain_max_theta, gain_max_phi;
			double gain_viewer, gain_viewer_net;
			double fb_ratio;
			double gain_dev_px, gain_dev_nx;
			double gain_dev_py, gain_dev_ny;
			double gain_dev_pz, gain_dev_nz;
			double ant_temp, ant_temp_tot;
			double gt;
		};

		double a[MEAS_COUNT];
	};

} measurement_t;


int ant_temp_resolve(double freq_mhz, int env,
	double *t_sky, double *t_earth);
void meas_calc(measurement_t *m, int idx);
int meas_name_idx(char *name, int len);
void meas_format(measurement_t *m, char *format, char *out, int outlen);
int meas_write_format(measurement_t *m, char *format, FILE *fp);

void meas_write_header(FILE *fp, char *delim);
void meas_write_data(FILE *fp, char *delim);

void meas_write_header_enc(FILE *fp, char *delim, char *left, char *right);
void meas_write_data_enc(FILE *fp, char *delim, char *left, char *right);

#endif
