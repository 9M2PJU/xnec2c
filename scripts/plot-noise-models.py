#!/usr/bin/env python3
"""Plot proposed xnec2c sky and earth noise temperature model selectors.

Labels match ANT_TEMP_SKY_* and ANT_TEMP_EARTH_* enum indices in measurements.h.

Models:
  Sky:   S0 G4CQM Min Quiet, S1 VK3UM Min Quiet, S2 VE7BQH Base Ref, S3 Galactic P.372 (2022),
         S4 DG7YBN Galactic Avg, S5 Synth Practical Avg, S6 Synth Min Quiet, S7 Custom
  Earth: E0 ITU-R Business (1974), E1 ITU-R Residential (1974), E2 ITU-R Rural (1974),
         E3 ITU-R Quiet Rural (1974), E4 G4CQM Mixed, E5 VE7BQH Base Ref,
         E6 DG7YBN Rural, E7 DG7YBN Residential, E8 DG7YBN City, E9 Custom
"""

import numpy as np
import matplotlib.pyplot as plt

# ======================================================================
# Amateur band edges (ITU Region 2)
# ======================================================================
ham_bands = {
    50.0:   (50.0, 54.0),
    100.0:  (99.0, 101.0),
    112.5:  (111.5, 113.5),
    137.5:  (136.5, 138.5),
    144.0:  (144.0, 148.0),
    220.0:  (220.0, 225.0),
    222.0:  (222.0, 225.0),
    250.0:  (249.0, 251.0),
    401.0:  (400.0, 402.0),
    402.5:  (401.5, 403.5),
    408.0:  (407.0, 409.0),
    432.0:  (420.0, 450.0),
    500.0:  (499.0, 501.0),
    761.0:  (760.0, 762.0),
    900.0:  (902.0, 928.0),
    1000.0: (999.0, 1001.0),
    1296.0: (1240.0, 1300.0),
}


# ======================================================================
# Sky model data
# ======================================================================

# S0: G4CQM Minimum Quiet (pre-2016)
s0_mhz = np.array([50.0, 144.0, 222.0, 432.0, 1296.0])
s0_sky = np.array([2200.0, 250.0, 70.0, 20.0, 10.0])

# S1: VK3UM Documented Minimum (pre-2016)
s1_mhz = np.array([50.0, 144.0, 220.0, 432.0, 900.0, 1296.0])
s1_sky = np.array([2200.0, 250.0, 150.0, 15.0, 10.0, 5.0])

# S2: VE7BQH Base Reference (pre-2018, frozen)
s2_mhz = np.array([50.0, 144.0, 432.0])
s2_sky = np.array([1700.0, 200.0, 20.0])

# S3: Galactic formula (P.372-16 eq.13/14)
# Computed below as function

# S4: DG7YBN Galactic Average (2025-08-29)
s4_mhz = np.array([50.0, 144.0, 432.0])
s4_sky = np.array([5640.0, 290.0, 27.0])

# S5: Synthesized Practical Average (2026-04)
s5_mhz = np.array([50.0, 100.0, 112.5, 137.5, 144.0, 220.0, 222.0, 250.0,
                    401.0, 402.5, 408.0, 432.0, 500.0, 761.0, 900.0,
                    1000.0, 1296.0])
s5_sky = np.array([5640.0, 1139.0, 830.0, 480.0, 290.0, 138.0, 131.0, 97.0,
                   28.0, 27.0, 25.0, 27.0, 22.0, 12.0, 11.0, 11.0, 10.0])

# S6: Synthesized Minimum Quiet (2026-04)
s6_mhz = np.array([50.0, 100.0, 112.5, 137.5, 144.0, 220.0, 222.0, 250.0,
                    401.0, 402.5, 408.0, 432.0, 500.0, 761.0, 900.0,
                    1000.0, 1296.0])
s6_sky = np.array([2200.0, 480.0, 370.0, 280.0, 250.0, 70.0, 70.0, 56.0,
                   22.0, 21.0, 21.0, 20.0, 17.0, 13.0, 12.0, 12.0, 10.0])


# ======================================================================
# Earth model data
# ======================================================================

# E0-E3: ITU formula coefficients
itu_coeffs = {
    'E0 ITU-R Business (1974)':    (76.8, 27.7),
    'E1 ITU-R Residential (1974)': (72.5, 27.7),
    'E2 ITU-R Rural (1974)':       (67.2, 27.7),
    'E3 ITU-R Quiet Rural (1974)': (53.6, 28.6),
}

# E4: G4CQM Mixed (pre-2016)
e4_mhz = np.array([50.0, 144.0, 222.0, 432.0, 1296.0])
e4_earth = np.array([3000.0, 1000.0, 600.0, 350.0, 290.0])

# E5: VE7BQH Base Reference (pre-2018, frozen)
e5_mhz = np.array([50.0, 144.0, 432.0])
e5_earth = np.array([9000.0, 1000.0, 350.0])

# E6: DG7YBN Rural (2025-08-29)
e6_mhz = np.array([50.0, 144.0, 432.0])
e6_earth = np.array([29640.0, 1600.0, 760.0])

# E7: DG7YBN Residential (2025-08-29)
e7_mhz = np.array([50.0, 144.0, 432.0])
e7_earth = np.array([100600.0, 5400.0, 1800.0])

# E8: DG7YBN City (2025-08-29)
e8_mhz = np.array([50.0, 144.0, 432.0])
e8_earth = np.array([271000.0, 14550.0, 8050.0])


# ======================================================================
# Functions
# ======================================================================

def galactic_sky(freq):
    """ITU-R P.372-16 galactic noise: eq.13 (<100 MHz), eq.14 (>100 MHz)."""
    result = np.zeros_like(freq)
    fam_100 = 52.0 - 23.0 * np.log10(100.0)
    t_100 = 290.0 * 10.0 ** (fam_100 / 10.0)
    for i, f in enumerate(freq):
        if f <= 100.0:
            fam = 52.0 - 23.0 * np.log10(f)
            result[i] = 290.0 * 10.0 ** (fam / 10.0)
        else:
            result[i] = (t_100 - 2.7) * (f / 100.0) ** (-2.75) + 2.7
    return result


def itu_earth(freq, c, d):
    """ITU-R P.372-16 man-made noise temperature, 290 K floor."""
    fam = c - d * np.log10(freq)
    t = 290.0 * (10.0 ** (fam / 10.0) - 1.0)
    return np.maximum(t, 290.0)


def log_interp(freq, tbl_mhz, tbl_val):
    """Log-log interpolation with edge clamping."""
    lf = np.log10(freq)
    lt = np.log10(tbl_mhz)
    lv = np.log10(tbl_val)
    result = np.zeros_like(freq)
    for i, f in enumerate(lf):
        if f <= lt[0]:
            result[i] = tbl_val[0]
        elif f >= lt[-1]:
            result[i] = tbl_val[-1]
        else:
            idx = np.searchsorted(lt, f) - 1
            t = (f - lt[idx]) / (lt[idx + 1] - lt[idx])
            result[i] = 10.0 ** (lv[idx] + t * (lv[idx + 1] - lv[idx]))
    return result


def snap_nearest(freq, tbl_mhz, tbl_val):
    """Nearest-band snap."""
    result = np.zeros_like(freq)
    for i, f in enumerate(freq):
        idx = np.argmin(np.abs(f - tbl_mhz))
        result[i] = tbl_val[idx]
    return result


def draw_snap_bars(ax, tbl_mhz, tbl_val, color, label, alpha=0.8, lw=3):
    """Draw horizontal bars at ham band edges for snap model."""
    labeled = False
    for freq, temp in zip(tbl_mhz, tbl_val):
        if freq in ham_bands:
            lo, hi = ham_bands[freq]
        else:
            lo, hi = freq * 0.98, freq * 1.02
        lbl = label if not labeled else None
        labeled = True
        ax.hlines(temp, lo, hi, colors=color, linewidths=lw, alpha=alpha,
                  label=lbl)
        ax.plot([lo, hi], [temp, temp], 's', markersize=3, color=color,
                alpha=alpha)


def draw_interp_curve(ax, tbl_mhz, tbl_val, color, label, ls='-', lw=2,
                      alpha=0.9, freq_range=None):
    """Draw log-log interpolated curve between anchors."""
    if freq_range is None:
        freq_range = (tbl_mhz[0], tbl_mhz[-1])
    freqs = np.logspace(np.log10(freq_range[0]),
                        np.log10(freq_range[1]), 500)
    vals = log_interp(freqs, tbl_mhz, tbl_val)
    ax.plot(freqs, vals, ls, linewidth=lw, color=color, alpha=alpha,
            label=label)

    # Anchor dots
    ax.plot(tbl_mhz, tbl_val, 'o', markersize=5, color=color, alpha=alpha,
            markeredgecolor='black', markeredgewidth=0.5)


# ======================================================================
# Frequency grids
# ======================================================================
sweep_full = np.logspace(np.log10(0.3), np.log10(3000.0), 800)
sweep_itu = np.logspace(np.log10(0.3), np.log10(250.0), 400)


# ======================================================================
# Colors
# ======================================================================
sky_colors = {
    'S0': '#0055AA',    # G4CQM min — dark blue
    'S1': '#DD6600',    # VK3UM min — burnt orange
    'S2': '#8B4513',    # Base Ref — saddle brown
    'S3': '#CC0088',    # Galactic formula — magenta
    'S4': '#228B22',    # DG7YBN avg — forest green
    'S5': '#CC0000',    # Synth avg — dark red
    'S6': '#6A0DAD',    # Synth min — deep purple
}

earth_colors = {
    'E0': '#CC0000',    # ITU Business — dark red dashed
    'E1': '#DD6600',    # ITU Residential — burnt orange dashed
    'E2': '#228B22',    # ITU Rural — forest green dashed
    'E3': '#008888',    # ITU Quiet Rural — dark teal
    'E4': '#0055AA',    # G4CQM mixed — dark blue
    'E5': '#8B4513',    # Base Ref — saddle brown
    'E6': '#228B22',    # DG7YBN Rural — forest green
    'E7': '#DD6600',    # DG7YBN Residential — burnt orange
    'E8': '#CC0000',    # DG7YBN City — dark red
}


# ======================================================================
# Figure: 2x2 panels
# ======================================================================
fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(20, 16))

# --- Panel 1: All sky models ---
ax1.set_title('Sky Temperature Models (T_sky)')
ax1.set_xlabel('Frequency (MHz)')
ax1.set_ylabel('Temperature (K)')
ax1.set_xscale('log')
ax1.set_yscale('log')
ax1.grid(True, which='both', alpha=0.3)

# S3: Galactic formula (continuous background)
ax1.plot(sweep_full, galactic_sky(sweep_full), '-', linewidth=1.5,
         color=sky_colors['S3'], alpha=0.6, label='S3 Galactic P.372 (2022)',
         zorder=1)

# S5: Synthesized avg (interp curve)
draw_interp_curve(ax1, s5_mhz, s5_sky, sky_colors['S5'],
                  'S5 Synth Practical Avg (2026)', lw=2.5)

# S6: Synthesized min (interp curve)
draw_interp_curve(ax1, s6_mhz, s6_sky, sky_colors['S6'],
                  'S6 Synth Min Quiet (2026)', lw=2.5)

# S4: DG7YBN avg (snap bars)
draw_snap_bars(ax1, s4_mhz, s4_sky, sky_colors['S4'],
               'S4 DG7YBN Galactic Avg (2025)')

# S0: G4CQM min (interp curve)
draw_interp_curve(ax1, s0_mhz, s0_sky, sky_colors['S0'],
                  'S0 G4CQM Min Quiet (pre-2016)')

# S1: VK3UM min (interp curve)
draw_interp_curve(ax1, s1_mhz, s1_sky, sky_colors['S1'],
                  'S1 VK3UM Min Quiet (pre-2016)', ls='--')

# S2: Base Ref (snap bars, faded)
draw_snap_bars(ax1, s2_mhz, s2_sky, sky_colors['S2'],
               'S2 VE7BQH Base Ref (pre-2018)', alpha=0.7, lw=2)

ax1.axhline(y=2.7, color='#333333', linestyle='--', alpha=0.6,
            label='CMB 2.7 K')
ax1.axvline(x=100, color='#333333', linestyle=':', alpha=0.5,
            label='eq.13/14 boundary')

ax1.legend(loc='upper right', fontsize=7)
ax1.set_xlim(30, 3000)
ax1.set_ylim(1, 20000)


# --- Panel 2: All earth models ---
ax2.set_title('Earth/Man-made Temperature Models (T_earth)')
ax2.set_xlabel('Frequency (MHz)')
ax2.set_ylabel('Temperature (K)')
ax2.set_xscale('log')
ax2.set_yscale('log')
ax2.grid(True, which='both', alpha=0.3)

# E0-E3: ITU formula curves (background)
for name, (c, d) in itu_coeffs.items():
    key = name[:2]
    ax2.plot(sweep_itu, itu_earth(sweep_itu, c, d), '--', linewidth=1.5,
             color=earth_colors[key], alpha=0.7, label=name)

# E6: DG7YBN Rural (snap bars)
draw_snap_bars(ax2, e6_mhz, e6_earth, earth_colors['E6'],
               'E6 DG7YBN Rural (2025)')

# E7: DG7YBN Residential (snap bars)
draw_snap_bars(ax2, e7_mhz, e7_earth, earth_colors['E7'],
               'E7 DG7YBN Residential (2025)')

# E8: DG7YBN City (snap bars)
draw_snap_bars(ax2, e8_mhz, e8_earth, earth_colors['E8'],
               'E8 DG7YBN City (2025)')

# E4: G4CQM mixed (interp curve)
draw_interp_curve(ax2, e4_mhz, e4_earth, earth_colors['E4'],
                  'E4 G4CQM Mixed (pre-2016)')

# E5: Base Ref (snap bars, faded)
draw_snap_bars(ax2, e5_mhz, e5_earth, earth_colors['E5'],
               'E5 VE7BQH Base Ref (pre-2018)', alpha=0.7, lw=2)

ax2.axhline(y=290, color='#333333', linestyle=':', alpha=0.7, label='290 K floor')

ax2.legend(loc='upper right', fontsize=6)
ax2.set_xlim(30, 3000)
ax2.set_ylim(200, 500000)


# --- Panel 3: Sky interpolation method comparison ---
ax3.set_title('Sky: Snap vs Log-Log Interpolation')
ax3.set_xlabel('Frequency (MHz)')
ax3.set_ylabel('Temperature (K)')
ax3.set_xscale('log')
ax3.set_yscale('log')
ax3.grid(True, which='both', alpha=0.3)

# S3 galactic formula as reference
ax3.plot(sweep_full, galactic_sky(sweep_full), '-', linewidth=1.5,
         color=sky_colors['S3'], alpha=0.6, label='S3 Galactic P.372 (2022)')

# S4: DG7YBN — snap staircase + interp curve
sweep_s4 = np.logspace(np.log10(50), np.log10(432), 300)
ax3.plot(sweep_s4, snap_nearest(sweep_s4, s4_mhz, s4_sky), '-',
         linewidth=2, color=sky_colors['S4'], alpha=0.6,
         label='S4 DG7YBN Galactic Avg snap')
draw_interp_curve(ax3, s4_mhz, s4_sky, sky_colors['S4'],
                  'S4 DG7YBN Galactic Avg interp', ls='--', lw=2.5, alpha=0.9)

# S0: G4CQM — snap staircase + interp curve
sweep_s0 = np.logspace(np.log10(50), np.log10(1296), 300)
ax3.plot(sweep_s0, snap_nearest(sweep_s0, s0_mhz, s0_sky), '-',
         linewidth=2, color=sky_colors['S0'], alpha=0.6,
         label='S0 G4CQM Min Quiet snap')
draw_interp_curve(ax3, s0_mhz, s0_sky, sky_colors['S0'],
                  'S0 G4CQM Min Quiet interp', ls='--', lw=2.5, alpha=0.9)

# S1: VK3UM — snap + interp
sweep_s1 = np.logspace(np.log10(50), np.log10(1296), 300)
ax3.plot(sweep_s1, snap_nearest(sweep_s1, s1_mhz, s1_sky), '-',
         linewidth=2, color=sky_colors['S1'], alpha=0.6,
         label='S1 VK3UM Min Quiet snap')
draw_interp_curve(ax3, s1_mhz, s1_sky, sky_colors['S1'],
                  'S1 VK3UM Min Quiet interp', ls='--', lw=2.5, alpha=0.9)

ax3.legend(loc='upper right', fontsize=7)
ax3.set_xlim(30, 3000)
ax3.set_ylim(1, 20000)


# --- Panel 4: Earth interpolation method comparison ---
ax4.set_title('Earth: Snap vs Log-Log Interpolation')
ax4.set_xlabel('Frequency (MHz)')
ax4.set_ylabel('Temperature (K)')
ax4.set_xscale('log')
ax4.set_yscale('log')
ax4.grid(True, which='both', alpha=0.3)

# ITU Rural as reference
ax4.plot(sweep_itu, itu_earth(sweep_itu, 67.2, 27.7), '--', linewidth=1.5,
         color=earth_colors['E2'], alpha=0.6, label='E2 ITU-R Rural (1974) formula')

# E6: DG7YBN Rural — snap + interp
sweep_e6 = np.logspace(np.log10(50), np.log10(432), 300)
ax4.plot(sweep_e6, snap_nearest(sweep_e6, e6_mhz, e6_earth), '-',
         linewidth=2, color=earth_colors['E6'], alpha=0.6,
         label='E6 DG7YBN Rural snap')
draw_interp_curve(ax4, e6_mhz, e6_earth, earth_colors['E6'],
                  'E6 DG7YBN Rural interp', ls='--', lw=2.5, alpha=0.9)

# E4: G4CQM — snap + interp
sweep_e4 = np.logspace(np.log10(50), np.log10(1296), 300)
ax4.plot(sweep_e4, snap_nearest(sweep_e4, e4_mhz, e4_earth), '-',
         linewidth=2, color=earth_colors['E4'], alpha=0.6,
         label='E4 G4CQM Mixed snap')
draw_interp_curve(ax4, e4_mhz, e4_earth, earth_colors['E4'],
                  'E4 G4CQM Mixed interp', ls='--', lw=2.5, alpha=0.9)

# E8: DG7YBN City — snap + interp
sweep_e8 = np.logspace(np.log10(50), np.log10(432), 300)
ax4.plot(sweep_e8, snap_nearest(sweep_e8, e8_mhz, e8_earth), '-',
         linewidth=2, color=earth_colors['E8'], alpha=0.6,
         label='E8 DG7YBN City snap')
draw_interp_curve(ax4, e8_mhz, e8_earth, earth_colors['E8'],
                  'E8 DG7YBN City interp', ls='--', lw=2.5, alpha=0.9)

ax4.axhline(y=290, color='#333333', linestyle=':', alpha=0.7, label='290 K floor')

ax4.legend(loc='upper right', fontsize=7)
ax4.set_xlim(30, 3000)
ax4.set_ylim(200, 500000)


plt.tight_layout()
plt.savefig('debug/proposed_models.png', dpi=150)
print("Plot saved to debug/proposed_models.png")
plt.show()


# ======================================================================
# Text summary: all models with anchor values
# ======================================================================
print("\n=== Sky Models ===")
sky_models = [
    ('S0', 'G4CQM Min Quiet (pre-2016)', s0_mhz, s0_sky, 'interp'),
    ('S1', 'VK3UM Min Quiet (pre-2016)', s1_mhz, s1_sky, 'interp'),
    ('S2', 'VE7BQH Base Ref (pre-2018)', s2_mhz, s2_sky, 'snap'),
    ('S3', 'Galactic P.372 (2022)', None, None, 'formula'),
    ('S4', 'DG7YBN Galactic Avg (2025)', s4_mhz, s4_sky, 'snap'),
    ('S5', 'Synth Practical Avg (2026)', s5_mhz, s5_sky, 'interp'),
    ('S6', 'Synth Min Quiet (2026)', s6_mhz, s6_sky, 'interp'),
]

for sid, name, mhz, vals, method in sky_models:
    print(f"\n  {sid}: {name}  [method: {method}]")
    if mhz is not None:
        for f, v in zip(mhz, vals):
            gal = galactic_sky(np.array([f]))[0]
            ratio = v / gal
            print(f"    {f:8.1f} MHz  {v:8.1f} K  (galactic={gal:.0f}, ratio={ratio:.2f})")
    else:
        print("    Continuous: eq.13 (<100 MHz), eq.14 (>100 MHz)")
        for f in [50, 100, 144, 222, 432, 900, 1296]:
            gal = galactic_sky(np.array([float(f)]))[0]
            print(f"    {f:8.0f} MHz  {gal:8.1f} K")

print("\n=== Earth Models ===")
earth_models = [
    ('E0', 'ITU-R Business (1974)', 76.8, 27.7, 'formula'),
    ('E1', 'ITU-R Residential (1974)', 72.5, 27.7, 'formula'),
    ('E2', 'ITU-R Rural (1974)', 67.2, 27.7, 'formula'),
    ('E3', 'ITU-R Quiet Rural (1974)', 53.6, 28.6, 'formula'),
    ('E4', 'G4CQM Mixed (pre-2016)', e4_mhz, e4_earth, 'interp'),
    ('E5', 'VE7BQH Base Ref (pre-2018)', e5_mhz, e5_earth, 'snap'),
    ('E6', 'DG7YBN Rural (2025)', e6_mhz, e6_earth, 'snap'),
    ('E7', 'DG7YBN Residential (2025)', e7_mhz, e7_earth, 'snap'),
    ('E8', 'DG7YBN City (2025)', e8_mhz, e8_earth, 'snap'),
]

for eid, name, p1, p2, method in earth_models:
    print(f"\n  {eid}: {name}  [method: {method}]")
    if method == 'formula':
        c, d = p1, p2
        print(f"    F_am = {c} - {d}*log10(f)")
        fa_floor = 10.0 * np.log10(2.0)
        f_floor = 10.0 ** ((c - fa_floor) / d)
        print(f"    Floor activation: {f_floor:.1f} MHz")
        for f in [50, 100, 144, 222, 432, 900, 1296]:
            t = max(290.0 * (10.0 ** ((c - d * np.log10(f)) / 10.0) - 1.0),
                    290.0)
            print(f"    {f:8.0f} MHz  {t:10.1f} K"
                  f"{'  (at floor)' if t <= 290.01 else ''}")
    else:
        mhz, vals = p1, p2
        for f, v in zip(mhz, vals):
            print(f"    {f:8.1f} MHz  {v:10.1f} K")

print("\n  S7: Custom  [user-defined constant]")
print("  E9: Custom  [user-defined constant]")
