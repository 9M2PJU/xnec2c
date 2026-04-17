#!/usr/bin/env python3
"""Plot all noise temperature models and their historical evolution.

Model families:
  - VE7BQH/DG7YBN: discrete snap at 50, 144, 432 MHz (3 eras)
  - G4CQM: log-log interpolation, 5 anchor points (VK3UM software snapshot)
  - VK3UM documented: minimum quiet sky defaults (6 bands, sky only)
  - ITU-R P.372-16: formula earth (4 environments), galactic sky (eq.13/14)
  - YU1AW: statistical mean sky recommendations (2 bands)

Source references and URLs:

  VE7BQH Era 1 (pre-2018):
    Original VE7BQH estimates, no ITU citation.
    T_sky=1700/200/20, T_earth=9000/1000/350 at 50/144/432.

  VE7BQH Era 2 (~2018-2024):
    VE7BQH Antenna Tables spreadsheet, ITU-R P.372-14.
    Dubus 2/2019 pp.116-117: "Update to Sky and Earth Temperatures for
    VHF/UHF Amateur Radio Bands" (Kluver, H., DG7YBN).
    Interactive mode by UR5EAZ.
    Spreadsheet mirror: https://qsl.net/g6hks/VE7BQH%20Antenna%20Tables.gnumeric
    G4CQM commentary: https://g4cqm.co.uk/VE7BQH%20Antenna%20Tables/index.html

  VE7BQH Era 3 (2025, DG7YBN GT pages):
    DG7YBN took over table maintenance from VE7BQH in October 2025.
    50 MHz (Issue 48): https://dg7ybn.de/GT_Tables/50_MHz_GT.htm
    144 MHz (Issue 127): https://dg7ybn.de/GT_Tables/144_MHz_GT.htm
      Source: ITU-R P.372-16 section 6 pp.98-100
    432 MHz (Issue 28): https://dg7ybn.de/GT_Tables/432_MHz_GT.htm
      Source: U.R.S.I. Radio Science Bulletins No.334, 09.2010
      (Lefering et al, "Man-Made Noise in Our Living Environments",
       Ghent University INTEC / OFCOM)
    White paper: https://dg7ybn.de/GT_Tables/Download/VE7BQH_Tables_White_Paper_2025_09_07.pdf

  DG7YBN Antenna Temperature page:
    https://dg7ybn.de/Ant_Temp/Ant_Temp.htm
    Cites: ITU-R P.372-13 (2016), Ghent/OFCOM 2010, Reich & Reich (1988).
    222 MHz earth from URSI 2010 measured data.
    222 MHz sky from galactic spectral index interpolation (Dubus 2/2019).

  G4CQM (pre-2016):
    https://g4cqm.co.uk/VE7BQH%20Antenna%20Tables/index.html
    Labeled "Corrected T_sky and T_earth (VK3UM's Calc compliance)".
    Earth values match pre-2018 VE7BQH defaults (350 K at 432 MHz).

  VK3UM (Doug McArthur, 1956-2016, SK):
    RPC documentation: https://www.sm2cew.com/rpc.pdf
    Software downloads: http://www.vk5dj.com/doug.html
    States "minimum quiet sky achievable for the frequency selected".
    No temperature table updates since 2016.

  ITU-R P.372-16 (August 2022):
    https://www.itu.int/rec/R-REC-P.372/en
    Table 1 coefficients unchanged since 1994.
    Galactic noise: eq.13 F_am = 52 - 23*log10(f), valid <100 MHz.
    Galactic scaling: eq.14 T_b(fi) = T_b(f0)*(fi/f0)^(-2.75) + 2.7 K.
    Spectral index beta = 2.75 average; varies 2.3-3.0 by direction
    (Reich & Reich, 1988, A&AS 74:7).

  YU1AW (Dragoslav Dobricic):
    "Importance of Antenna Noise Temperature at VHF band"
    https://www.qsl.net/yu1aw/Misc/vhfnoisetemp.pdf
    Recommends T_sky=400 K at 144 MHz (statistical mean, not minimum).
    Also: T_sky=40 K at 432 MHz.

  F5FOD AGTC-JS (no embedded defaults, user-supplied T_sky/T_earth):
    https://f5fod.github.io/AGTC-JS/AGTC-JS_V01-08.html
    https://dg7ybn.de/Ant_soft/Antenna_GT_f5fod.htm

  SM2CEW G/T reference page:
    https://www.sm2cew.com/gt.htm

  W8IO YagiCAD G/T tables (uses G4CQM values at 222/432 MHz):
    https://www.w8io.com/VHF-Ultra-antennas.html
    T_sky/T_earth: 144=200/1000, 222=70/600, 432=20/350
    W8IO uses 200 K sky at 144 MHz — matches no other source (VE7BQH=290,
    G4CQM=250, YU1AW=400). Earth values identical to G4CQM at 222/432.

  NTIA/ITS man-made noise measurements (US Dept of Commerce):
    OT-74-38 (Spaulding & Disney, 1974): 250 kHz - 250 MHz baseline
      https://its.ntia.gov/publications/download/ot-74-38.pdf
      Foundational dataset for ITU-R P.372 Table 1 coefficients.
    TR-98-355 (1998): 136-138 MHz meteorological satellite band
      https://www.its.ntia.gov/publications/download/TR-98-355.pdf
    TR-02-390 (Achatz & Dalke, 2001): 137.5, 402.5, 761.0 MHz
      https://its.ntia.gov/publications/download/tr-02-390.pdf
      Denver/Boulder CO, 1999. Key Fam values:
        137.5 MHz: Business 17.5 dB (16017 K), Residential 3.6 dB (374 K)
        402.5 MHz: Denver Business 8.6 dB (1811 K), Boulder Business 3.1 dB
                   (302 K), Boulder Residential 2.3 dB (202 K)
        761.0 MHz: All sites 2.1-2.8 dB (175-262 K), below thermal floor
      Finding: modern residential 16x quieter than 1970s ITU at VHF;
      all environments converge to thermal floor by 761 MHz.
    TR-11-478 (Wepman & Sanders, 2011): 112.5, 221.5, 401 MHz
      https://its.ntia.gov/publications/download/11-478.pdf
      Boulder/Denver CO. Measured Fam above ITU prediction but within 1 sigma.
    TR-96-330 rev (1996): 1-3 GHz urban impulsive noise characterization
      https://its.ntia.gov/publications/download/TR-96-330_Revised.pdf

  Haslam et al. 408 MHz all-sky survey:
    Original: Haslam C.G.T. et al. 1982, A&AS, 47, 1
    Reprocessed: Remazeilles et al. 2015, MNRAS 451, 4311
    https://lambda.gsfc.nasa.gov/product/foreground/fg_2014_haslam_408_info.html
    Minimum brightness temperature: 11.4 K (galactic pole, includes CMB 2.7 K)
    Synchrotron minimum: 8.7 K (CMB subtracted)
    Zero-level uncertainty: +/- 3 K
    Scale to other frequencies: T_synch(f) = 8.7 * (408/f)^beta + 2.7
    beta = 2.55 average (Lawson 1987); varies 2.3-3.0 by sky region
    (poles tend flatter beta ~2.3-2.5; plane steeper ~2.7-3.0)

  LOFAR LoTSS discrete source contribution at 144 MHz:
    Schwarz et al. 2021, A&A (arXiv:2011.08294)
    https://www.aanda.org/articles/aa/full_html/2021/04/aa38814-20/aa38814-20.html
    Total discrete source contribution: 44.4 +/- 1.7 K (100 uJy to ~100 Jy)
    Additive to diffuse galactic emission; not included in galactic formula.

  Cane 1979 galactic background spectrum:
    Cane, H.V. 1979, MNRAS 189, 465
    Combined ground-based and space-based measurements (20+ observers)
    with half-wave dipoles over ground screen. Provides most reliable
    estimate of galactic background levels at high galactic latitudes
    for 1-100 MHz range. Correction factor ~1.3 for full-sky average
    (Dulk et al. 2001). Includes galactic + extragalactic contributions
    with absorption by galactic disk.

  GLEAM survey (Murchison Widefield Array):
    Wayth et al. 2015; MWA collaboration
    76-222 MHz sky survey at 2 arcmin resolution, declination -90 to +30 deg.
    Directly relevant for 220-222 MHz band sky temperature validation.

  Galactic spectral index surveys:
    Reich & Reich 1988, A&AS 74:7 (408-1420 MHz, beta=2.3-3.0 by region)
    Kogut 2012 (22-1049 MHz, beta=-2.60+/-0.04, curvature c=-0.081+/-0.028)
    Guzmán et al. 2011 (45-408 MHz, beta=-2.5 to -2.6)
    Spinelli et al. 2021 / LEDA (50-87 MHz, beta=-2.56 to -2.50)

  W1GHZ (Paul Wade) microwave noise measurement:
    https://www.w1ghz.org/Preamps/Verifying_Microwave_Receiver_Performance.pdf
    Uses T_sky ~30 K (horn-integrated cold sky) and T_ground = 290 K for
    Y-factor measurement at 1296 MHz. The 30 K reflects wide-beam horn
    integrating warm sky regions; narrow-beam antenna at galactic pole sees
    5-10 K at 1296 MHz.

  Kraus, J.D. 1986, Radio Astronomy, 2nd ed., Cygnus-Quasar Books
    https://archive.org/details/radioastronomy0000krau_ed02
    Standard reference for galactic background temperature vs frequency.
    Figure shows noise sources 0.1-100 GHz: A=man-made, B=galactic avg,
    C=galactic center, D=quiet Sun, E=atmosphere, F=CMB 2.7 K.

  Atmospheric emission (O2/H2O), clear sky, sea level, 45 deg elevation:
    ~2 K at 500 MHz, ~3 K at 900 MHz, ~4 K at 1296 MHz
    Source: JPL IPN Progress Report 42-168/168E; Kraus 1986 ch.8
    Becomes dominant over galactic emission above ~500 MHz.
"""

import numpy as np
import matplotlib.pyplot as plt
import sys

# ======================================================================
# VE7BQH/DG7YBN — three historical eras
# ======================================================================

# Era 1: Pre-2018 original (single environment, no source citation)
ve7bqh_old_mhz   = np.array([50.0, 144.0, 432.0])
ve7bqh_old_sky    = np.array([1700.0, 200.0, 20.0])
ve7bqh_old_earth  = np.array([9000.0, 1000.0, 350.0])

# Era 2: VE7BQH spreadsheet (ITU-R P.372-14, ~2018-2024)
# Source: G6HKS mirror of VE7BQH Antenna Tables.gnumeric
ve7bqh_mhz = np.array([50.0, 144.0, 432.0])
ve7bqh_sky = np.array([5640.0, 290.0, 27.0])
ve7bqh_earth = {
    'Rural':       np.array([29640.0, 1600.0, 460.0]),
    'Residential': np.array([100600.0, 5400.0, 1800.0]),
    'City':        np.array([271000.0, 14500.0, 7900.0]),
}

# Era 3: DG7YBN GT pages (2025-08-29), URSI Bulletins No.334 at 432 MHz
# Source: dg7ybn.de/GT_Tables/{50,144,432}_MHz_GT.htm
# 50 MHz: ITU-R P.372 / Dubus 2/2019
# 144 MHz: ITU-R P.372-16 section 6 pp.98-100
# 432 MHz: URSI Radio Science Bulletins No.334, 09.2010 (Ghent/OFCOM)
# Note: DG7YBN 432 City published as range "7900-8200 K"; using 7900 (low end)
dg7ybn_mhz = np.array([50.0, 144.0, 432.0])
dg7ybn_sky = np.array([5640.0, 290.0, 27.0])
dg7ybn_earth = {
    'Rural':       np.array([29640.0, 1600.0, 760.0]),
    'Residential': np.array([100600.0, 5400.0, 1800.0]),
    'City':        np.array([271000.0, 14550.0, 7900.0]),
}

# ======================================================================
# G4CQM — frozen pre-2016 snapshot from VK3UM software
# ======================================================================
# Source: g4cqm.co.uk "Corrected T_sky and T_earth (VK3UM's Calc compliance)"
# Earth values match pre-2018 VE7BQH defaults at 432 MHz (350 K)
# 222 MHz earth: from URSI Bulletins No.334 (Ghent/OFCOM, 2010) measured data
# 222 MHz sky: derived from galactic spectral index interpolation (Dubus 2/2019)
# Note: G4CQM uses 222 MHz, VK3UM documentation uses 220 MHz — different bands
g4cqm_mhz   = np.array([50.0, 144.0, 222.0, 432.0, 1296.0])
g4cqm_sky    = np.array([2200.0, 250.0, 70.0, 20.0, 10.0])
g4cqm_earth  = np.array([3000.0, 1000.0, 600.0, 350.0, 290.0])

# ======================================================================
# VK3UM documented defaults — minimum quiet sky achievable
# ======================================================================
# Source: sm2cew.com/rpc.pdf (VK3UM Site Radiation & System Performance Calc)
# Sky only; VK3UM earth values are not independently documented — the G4CQM
# earth array above is the only published earth dataset from the VK3UM lineage.
# VK3UM (Doug McArthur, 1956-2016, SK) — software maintained by VK5DJ, no
# temperature table updates since.
vk3um_mhz = np.array([50.0, 144.0, 220.0, 432.0, 900.0, 1296.0])
vk3um_sky  = np.array([2200.0, 250.0, 150.0, 15.0, 10.0, 5.0])

# ======================================================================
# ITU-R P.372-16 Table 1 (coefficients unchanged since 1994)
# ======================================================================
# F_am = c - d*log10(f_MHz)
# T_earth = 290 * (10^(F_am/10) - 1), clamped to 290 K floor
itu_coeffs = {
    'Business':    (76.8, 27.7),
    'Residential': (72.5, 27.7),
    'Rural':       (67.2, 27.7),
    'Quiet Rural': (53.6, 28.6),
}

# ITU-R P.372-16 galactic noise
# Eq. 13: F_am = 52 - 23*log10(f), valid up to ~100 MHz
# Eq. 14: T_b(fi) = T_b(f0) * (fi/f0)^(-2.75) + 2.7, for scaling above

# ======================================================================
# YU1AW recommendations — statistical mean sky temperatures
# ======================================================================
# Source: "Importance of Antenna Noise Temperature at VHF band" (Dobricic)
# These are recommended all-sky statistical means, not minimum quiet sky
yu1aw_mhz = np.array([144.0, 432.0])
yu1aw_sky  = np.array([400.0, 40.0])

# ======================================================================
# NTIA/ITS man-made noise measurements (Fam in dB above kT0b)
# ======================================================================
# T_earth = 290 * (10^(Fam/10) - 1)
# Source: NTIA TR-02-390 (Achatz & Dalke, 2001), Denver/Boulder CO, 1999
ntia_tr02390 = {
    (137.5, 'Business'):             17.5,  # 16017 K
    (137.5, 'Residential'):           3.6,  #   374 K
    (402.5, 'Business Denver'):       8.6,  #  1811 K
    (402.5, 'Business Boulder'):      3.1,  #   302 K (small city)
    (402.5, 'Residential Boulder'):   2.3,  #   202 K
    (761.0, 'Residential Lakewood'):  2.8,  #   262 K
    (761.0, 'Residential Boulder'):   2.1,  #   180 K
    (761.0, 'Business Boulder'):      2.5,  #   226 K
    (761.0, 'Business Denver'):       2.5,  #   226 K
}
# Source: NTIA TR-11-478 (Wepman & Sanders, 2011), Boulder/Denver CO
# Measured Fam > ITU prediction but within 1 sigma at all three frequencies.
# Exact Fam values require PDF extraction (136 pages, image-heavy).
ntia_tr11478_freqs = [112.5, 221.5, 401.0]  # MHz, business + residential

# ======================================================================
# Haslam 408 MHz all-sky survey (Haslam et al. 1982, Remazeilles 2015)
# ======================================================================
haslam_408_min_total = 11.4     # K, galactic pole (includes CMB 2.7 K)
haslam_408_cmb = 2.7            # K, cosmic microwave background
haslam_408_synch_min = 8.7      # K, galactic pole synchrotron (CMB subtracted)
haslam_408_zero_err = 3.0       # K, zero-level uncertainty (+/-)

# ======================================================================
# W8IO / YagiCAD G/T defaults (independent reference)
# ======================================================================
# Source: w8io.com/VHF-Ultra-antennas.html
w8io_mhz   = np.array([144.0, 222.0, 432.0])
w8io_sky    = np.array([200.0, 70.0, 20.0])
w8io_earth  = np.array([1000.0, 600.0, 350.0])

# ======================================================================
# Atmospheric emission (O2/H2O), sea level, clear sky, 45 deg elevation
# ======================================================================
# Becomes dominant over galactic emission above ~500 MHz
atm_emission = {500: 2.0, 900: 3.0, 1296: 4.0}  # K, approximate

# ======================================================================
# LOFAR LoTSS discrete source contribution at 144 MHz
# ======================================================================
# Schwarz et al. 2021, A&A: 44.4 +/- 1.7 K (100 uJy to ~100 Jy)
# Additive to diffuse galactic emission; not included in galactic formula.
lofar_144_discrete = 44.4  # K

# ======================================================================
# Environmental definitions (from URSI Bulletins No.334, 2010)
# ======================================================================
# Rural:       Open countryside, agricultural, building density <1/ha,
#              no major roads or railways
# Residential: Villages and pure residential; no commercial/industrial;
#              no railways/major roads/high-voltage lines within 1 km
# City:        Dense commercial/industrial buildings, major roads and
#              railways present

# ======================================================================
# Amateur band edges (ITU Region 2)
# ======================================================================
ham_bands = {
    50.0:   (50.0, 54.0),
    144.0:  (144.0, 148.0),
    220.0:  (220.0, 225.0),
    222.0:  (222.0, 225.0),
    432.0:  (420.0, 450.0),
    900.0:  (902.0, 928.0),
    1296.0: (1240.0, 1300.0),
}


def log_interp(freq, tbl_mhz, tbl_val):
    """Log-log interpolation matching ant_temp_log_interp() in measurements.c"""
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


def ve7bqh_snap(freq, sky_arr, earth_arr):
    """Nearest-band snap matching ant_temp_resolve() VE7BQH path."""
    t_sky = np.zeros_like(freq)
    t_earth = np.zeros_like(freq)
    for i, f in enumerate(freq):
        b = np.argmin(np.abs(f - ve7bqh_mhz))
        t_sky[i] = sky_arr[b]
        t_earth[i] = earth_arr[b]
    return t_sky, t_earth


def itu_earth(freq, c, d):
    """ITU-R P.372 man-made noise temperature.
    F_am = c - d*log10(f_MHz)
    T = 290 * (10^(F_am/10) - 1), floored at 290 K.
    """
    fam = c - d * np.log10(freq)
    t = 290.0 * (10.0 ** (fam / 10.0) - 1.0)
    return np.maximum(t, 290.0)


def galactic_sky(freq):
    """ITU-R P.372-16 galactic noise temperature.
    Below 100 MHz: eq.13 F_am = 52 - 23*log10(f), T = 290*(10^(F/10))
    Above 100 MHz: eq.14 power-law T_b(fi) = T_b(100)*(fi/100)^(-2.75) + 2.7
    """
    result = np.zeros_like(freq)

    # Reference: T_b at 100 MHz from eq. 13
    fam_100 = 52.0 - 23.0 * np.log10(100.0)
    # fa = 10^(F_am/10), T_a = 290 * fa  (eq. 9: T_a = T_0 * fa)
    t_100 = 290.0 * 10.0 ** (fam_100 / 10.0)

    for i, f in enumerate(freq):
        if f <= 100.0:
            fam = 52.0 - 23.0 * np.log10(f)
            result[i] = 290.0 * 10.0 ** (fam / 10.0)
        else:
            # Eq. 14: power-law extrapolation from 100 MHz reference
            result[i] = (t_100 - 2.7) * (f / 100.0) ** (-2.75) + 2.7
    return result


# Frequency sweeps restricted to each model's defined range
g4cqm_freqs = np.logspace(np.log10(50.0), np.log10(1296.0), 300)
itu_freqs = np.logspace(np.log10(0.3), np.log10(250.0), 300)
galactic_freqs = np.logspace(np.log10(0.3), np.log10(3000.0), 500)

# Compute all models
models_sky = {}
models_earth = {}

# VE7BQH Era 2 (spreadsheet, P.372-14): discrete points
for env_name, earth_arr in ve7bqh_earth.items():
    models_sky[f'VE7BQH {env_name}'] = (ve7bqh_mhz, ve7bqh_sky)
    models_earth[f'VE7BQH {env_name}'] = (ve7bqh_mhz, earth_arr)

# DG7YBN Era 3 (2025 GT pages, URSI 2010): discrete points
for env_name, earth_arr in dg7ybn_earth.items():
    models_sky[f'DG7YBN {env_name}'] = (dg7ybn_mhz, dg7ybn_sky)
    models_earth[f'DG7YBN {env_name}'] = (dg7ybn_mhz, earth_arr)

# VE7BQH Era 1 (pre-2018): single environment
models_sky['Old VE7BQH'] = (ve7bqh_old_mhz, ve7bqh_old_sky)
models_earth['Old VE7BQH'] = (ve7bqh_old_mhz, ve7bqh_old_earth)

# G4CQM: interpolation within anchor range (50-1296 MHz)
models_sky['G4CQM'] = (g4cqm_freqs, log_interp(g4cqm_freqs, g4cqm_mhz, g4cqm_sky))
models_earth['G4CQM'] = (g4cqm_freqs, log_interp(g4cqm_freqs, g4cqm_mhz, g4cqm_earth))

# VK3UM documented defaults (sky only, minimum quiet sky)
models_sky['VK3UM min'] = (vk3um_mhz, vk3um_sky)

# ITU-R: formula earth within 0.3-250 MHz, G4CQM sky anchors for overlap range
itu_sky_mask = (itu_freqs >= 50.0) & (itu_freqs <= 1296.0)
itu_sky_freqs = itu_freqs[itu_sky_mask]
itu_sky_vals = log_interp(itu_sky_freqs, g4cqm_mhz, g4cqm_sky)
for env_name, (c, d) in itu_coeffs.items():
    models_sky[f'ITU {env_name}'] = (itu_sky_freqs, itu_sky_vals)
    models_earth[f'ITU {env_name}'] = (itu_freqs, itu_earth(itu_freqs, c, d))

# Galactic sky (full range where formula is defined)
models_sky['Galactic (P.372)'] = (galactic_freqs, galactic_sky(galactic_freqs))

# YU1AW statistical mean sky
models_sky['YU1AW mean'] = (yu1aw_mhz, yu1aw_sky)

# Print data table at key frequencies
print_freqs = [50.0, 100.0, 144.0, 220.0, 222.0, 250.0, 432.0, 900.0, 1000.0, 1296.0]

# Sky table: focused set of models for readability
sky_display = ['Old VE7BQH', 'VE7BQH Rural', 'G4CQM', 'VK3UM min',
               'YU1AW mean', 'Galactic (P.372)']
print("\n=== T_sky (K) — model comparison ===")
header = f"{'MHz':>8}"
for name in sky_display:
    header += f"  {name:>16}"
print(header)

for f in print_freqs:
    row = f"{f:8.1f}"
    for name in sky_display:
        if name not in models_sky:
            row += f"  {'':>16}"
            continue
        mf, mv = models_sky[name]
        idx = np.argmin(np.abs(mf - f))
        if np.abs(mf[idx] - f) < f * 0.05:
            row += f"  {mv[idx]:16.1f}"
        else:
            row += f"  {'—':>16}"
    print(row)

# Earth table: show VE7BQH era evolution and G4CQM
earth_display_rural = ['Old VE7BQH', 'VE7BQH Rural', 'DG7YBN Rural',
                        'G4CQM', 'ITU Rural', 'ITU Quiet Rural']
print("\n=== T_earth (K) — Rural/baseline comparison ===")
header = f"{'MHz':>8}"
for name in earth_display_rural:
    header += f"  {name:>16}"
print(header)

for f in print_freqs:
    row = f"{f:8.1f}"
    for name in earth_display_rural:
        if name not in models_earth:
            row += f"  {'':>16}"
            continue
        mf, mv = models_earth[name]
        idx = np.argmin(np.abs(mf - f))
        if np.abs(mf[idx] - f) < f * 0.05:
            row += f"  {mv[idx]:16.1f}"
        else:
            row += f"  {'—':>16}"
    print(row)

# VE7BQH era evolution at anchor frequencies — all environments
for env_name in ['Rural', 'Residential', 'City']:
    print(f"\n=== VE7BQH/DG7YBN {env_name} earth evolution ===")
    print(f"{'MHz':>8} {'Era1 pre2018':>14} {'Era2 P372-14':>14} {'Era3 URSI2010':>14} {'Era2->3 delta':>14}")
    for i, f in enumerate(ve7bqh_mhz):
        e1 = ve7bqh_old_earth[i]
        e2 = ve7bqh_earth[env_name][i]
        e3 = dg7ybn_earth[env_name][i]
        delta = f"{(e3 - e2) / e2 * 100:+.0f}%" if e2 != e3 else "same"
        print(f"{f:8.0f} {e1:14.0f} {e2:14.0f} {e3:14.0f} {delta:>14}")

# DG7YBN Era 3 vs VE7BQH Era 2 — all environments
print("\n=== DG7YBN (URSI 2010) vs VE7BQH spreadsheet (P.372-14) — all environments ===")
print(f"{'MHz':>8} {'Env':>14} {'VE7BQH':>10} {'DG7YBN':>10} {'Delta':>10}")
for env_name in ['Rural', 'Residential', 'City']:
    for i, f in enumerate(ve7bqh_mhz):
        e2 = ve7bqh_earth[env_name][i]
        e3 = dg7ybn_earth[env_name][i]
        delta = f"{(e3 - e2) / e2 * 100:+.1f}%" if abs(e2 - e3) > 1.0 else "same"
        print(f"{f:8.0f} {env_name:>14} {e2:10.0f} {e3:10.0f} {delta:>10}")

# ITU sky dependency note
print("\n=== ITU model sky dependency ===")
print("  ITU-R P.372 defines earth/man-made noise only (formula).")
print("  ITU models in xnec2c borrow G4CQM sky anchors for T_sky.")
print("  All four ITU environments share the same sky: G4CQM log-log interpolation.")

# 220 vs 222 MHz note
print("\n=== 220 vs 222 MHz band discrepancy ===")
print("  VK3UM documentation: 220 MHz, T_sky=150 K (ITU Region 2 band: 220-225 MHz)")
print("  G4CQM in xnec2c code: 222 MHz, T_sky=70 K (center of 222-225 MHz allocation)")
print("  These are the same amateur band but different anchor frequencies and values.")
print("  The 222 MHz sky value (70 K) is from galactic spectral index interpolation.")
print("  The 220 MHz sky value (150 K) is from VK3UM EME Calculator defaults.")

# G4CQM vs VK3UM documented defaults
print("\n=== G4CQM (code) vs VK3UM (documented) sky comparison ===")
print(f"{'MHz':>8} {'G4CQM':>10} {'VK3UM':>10} {'Match':>8}")
for f in [50, 144, 220, 222, 432, 900, 1296]:
    g = "—"
    v = "—"
    if f in list(g4cqm_mhz):
        idx = list(g4cqm_mhz).index(f)
        g = f"{g4cqm_sky[idx]:.0f}"
    if f in list(vk3um_mhz):
        idx = list(vk3um_mhz).index(f)
        v = f"{vk3um_sky[idx]:.0f}"
    match = "—"
    if g != "—" and v != "—":
        match = "yes" if g == v else "NO"
    elif g == "—" and v != "—":
        match = "G4CQM n/a"
    elif g != "—" and v == "—":
        match = "VK3UM n/a"
    print(f"{f:>8} {g:>10} {v:>10} {match:>8}")


# --- Spectral index analysis ---
# Power-law exponent alpha where T ~ f^alpha between adjacent anchors

def spectral_index(f1, f2, t1, t2):
    """Slope in log-log space: alpha = log(T2/T1) / log(f2/f1)"""
    if t1 <= 0 or t2 <= 0 or f1 <= 0 or f2 <= 0 or f1 == f2:
        return float('nan')
    return np.log10(t2 / t1) / np.log10(f2 / f1)

# Collect all unique anchor frequencies across models
all_anchors = sorted(set(list(ve7bqh_mhz) + list(g4cqm_mhz)))

print("\n=== T_sky spectral index (alpha where T ~ f^alpha) ===")
print(f"  Galactic power law: -2.30 (eq.13, <100 MHz) / -2.75 (eq.14, >100 MHz)")
print(f"{'Segment':>20} {'VE7BQH':>10} {'Old VE7BQH':>12} {'G4CQM':>10} {'VK3UM':>10} {'Galactic':>10}")

for i in range(len(all_anchors) - 1):
    f1, f2 = all_anchors[i], all_anchors[i + 1]
    seg = f"{f1:.0f}-{f2:.0f} MHz"

    v_alpha = "—"
    if f1 in list(ve7bqh_mhz) and f2 in list(ve7bqh_mhz):
        i1 = list(ve7bqh_mhz).index(f1)
        i2 = list(ve7bqh_mhz).index(f2)
        v_alpha = f"{spectral_index(f1, f2, ve7bqh_sky[i1], ve7bqh_sky[i2]):.2f}"

    vo_alpha = "—"
    if f1 in list(ve7bqh_old_mhz) and f2 in list(ve7bqh_old_mhz):
        i1 = list(ve7bqh_old_mhz).index(f1)
        i2 = list(ve7bqh_old_mhz).index(f2)
        vo_alpha = f"{spectral_index(f1, f2, ve7bqh_old_sky[i1], ve7bqh_old_sky[i2]):.2f}"

    g_alpha = "—"
    if f1 in list(g4cqm_mhz) and f2 in list(g4cqm_mhz):
        i1 = list(g4cqm_mhz).index(f1)
        i2 = list(g4cqm_mhz).index(f2)
        g_alpha = f"{spectral_index(f1, f2, g4cqm_sky[i1], g4cqm_sky[i2]):.2f}"

    vk_alpha = "—"
    if f1 in list(vk3um_mhz) and f2 in list(vk3um_mhz):
        i1 = list(vk3um_mhz).index(f1)
        i2 = list(vk3um_mhz).index(f2)
        vk_alpha = f"{spectral_index(f1, f2, vk3um_sky[i1], vk3um_sky[i2]):.2f}"

    gal1 = galactic_sky(np.array([f1]))[0]
    gal2 = galactic_sky(np.array([f2]))[0]
    gal_alpha = f"{spectral_index(f1, f2, gal1, gal2):.2f}"

    print(f"{seg:>20} {v_alpha:>10} {vo_alpha:>12} {g_alpha:>10} {vk_alpha:>10} {gal_alpha:>10}")

print("\n=== T_earth spectral index ===")
print(f"{'Segment':>20} {'VE7BQH Rur':>12} {'G4CQM':>10} {'ITU Rural':>12} {'ITU QR':>10}")

def itu_earth_scalar(f, c, d):
    fam = c - d * np.log10(f)
    t = 290.0 * (10.0 ** (fam / 10.0) - 1.0)
    return max(t, 290.0)

for i in range(len(all_anchors) - 1):
    f1, f2 = all_anchors[i], all_anchors[i + 1]
    seg = f"{f1:.0f}-{f2:.0f} MHz"

    v_alpha = "—"
    if f1 in list(ve7bqh_mhz) and f2 in list(ve7bqh_mhz):
        i1 = list(ve7bqh_mhz).index(f1)
        i2 = list(ve7bqh_mhz).index(f2)
        v_alpha = f"{spectral_index(f1, f2, ve7bqh_earth['Rural'][i1], ve7bqh_earth['Rural'][i2]):.2f}"

    g_alpha = "—"
    if f1 in list(g4cqm_mhz) and f2 in list(g4cqm_mhz):
        i1 = list(g4cqm_mhz).index(f1)
        i2 = list(g4cqm_mhz).index(f2)
        g_alpha = f"{spectral_index(f1, f2, g4cqm_earth[i1], g4cqm_earth[i2]):.2f}"

    itu_r = f"{spectral_index(f1, f2, itu_earth_scalar(f1, 67.2, 27.7), itu_earth_scalar(f2, 67.2, 27.7)):.2f}"
    itu_qr = f"{spectral_index(f1, f2, itu_earth_scalar(f1, 53.6, 28.6), itu_earth_scalar(f2, 53.6, 28.6)):.2f}"

    print(f"{seg:>20} {v_alpha:>12} {g_alpha:>10} {itu_r:>12} {itu_qr:>10}")

# --- Cross-model sky ratios relative to galactic ---
print("\n=== T_sky ratios (model / galactic) ===")
print(f"  ratio=1.0 means model matches galactic formula exactly")
print(f"{'MHz':>8} {'Old VE7BQH':>12} {'VE7BQH':>10} {'G4CQM':>10} {'VK3UM':>10} {'YU1AW':>10}")
ratio_freqs = [50, 144, 220, 222, 432, 900, 1296]
for f in ratio_freqs:
    gal = galactic_sky(np.array([float(f)]))[0]
    vo = "—"
    if f in list(ve7bqh_old_mhz):
        idx = list(ve7bqh_old_mhz).index(f)
        vo = f"{ve7bqh_old_sky[idx] / gal:.3f}"
    v = "—"
    if f in list(ve7bqh_mhz):
        idx = list(ve7bqh_mhz).index(f)
        v = f"{ve7bqh_sky[idx] / gal:.3f}"
    g = "—"
    if f in list(g4cqm_mhz):
        idx = list(g4cqm_mhz).index(f)
        g = f"{g4cqm_sky[idx] / gal:.3f}"
    vk = "—"
    if f in list(vk3um_mhz):
        idx = list(vk3um_mhz).index(f)
        vk = f"{vk3um_sky[idx] / gal:.3f}"
    yu = "—"
    if f in list(yu1aw_mhz):
        idx = list(yu1aw_mhz).index(f)
        yu = f"{yu1aw_sky[idx] / gal:.3f}"
    print(f"{f:>8} {vo:>12} {v:>10} {g:>10} {vk:>10} {yu:>10}")

# --- G4CQM earth position among ITU environments ---
print("\n=== G4CQM earth position among ITU environments ===")
for f in g4cqm_mhz:
    idx = list(g4cqm_mhz).index(f)
    g_val = g4cqm_earth[idx]
    itu_vals = {name: itu_earth_scalar(f, c, d) for name, (c, d) in itu_coeffs.items()}
    ordered = sorted(itu_vals.items(), key=lambda x: x[1])
    pos = "below all"
    for j, (name, val) in enumerate(ordered):
        if g_val < val:
            if j == 0:
                pos = f"below {name} ({val:.0f})"
            else:
                prev_name, prev_val = ordered[j - 1]
                pos = f"between {prev_name} ({prev_val:.0f}) and {name} ({val:.0f})"
            break
    else:
        pos = f"above {ordered[-1][0]} ({ordered[-1][1]:.0f})"
    print(f"  {f:7.0f} MHz: G4CQM={g_val:8.0f}  {pos}")

# --- ITU 290K floor activation ---
print("\n=== ITU 290K floor activation frequency ===")
print(f"  (frequency above which Fa formula yields T < 290 K)")
fa_floor = 10 * np.log10(2)
for name, (c, d) in itu_coeffs.items():
    f_floor = 10 ** ((c - fa_floor) / d)
    print(f"  {name:>12}: {f_floor:.1f} MHz")

# --- VE7BQH vs ITU earth comparison ---
print("\n=== T_earth: VE7BQH Rural vs ITU Rural ===")
for f in ve7bqh_mhz:
    itu_val = itu_earth_scalar(f, 67.2, 27.7)
    idx = list(ve7bqh_mhz).index(f)
    v_val = ve7bqh_earth['Rural'][idx]
    print(f"  {f:.0f} MHz: VE7BQH={v_val:.0f}  ITU={itu_val:.0f}  ratio={v_val/itu_val:.3f}")

# --- NTIA measured vs ITU predicted ---
print("\n=== NTIA measured vs ITU predicted (T_earth, K) ===")
print(f"  Source: NTIA TR-02-390 (Achatz & Dalke, 2001), Denver/Boulder CO")
print(f"{'MHz':>8} {'Environment':>22} {'Fam dB':>8} {'NTIA T':>10} {'ITU T':>10} {'Ratio':>8}")
for (freq, env), fam in sorted(ntia_tr02390.items()):
    ntia_t = 290.0 * (10.0 ** (fam / 10.0) - 1.0)
    if 'Business' in env or 'City' in env:
        c, d = 76.8, 27.7
    else:
        c, d = 72.5, 27.7
    itu_t = max(290.0 * (10.0 ** ((c - d * np.log10(freq)) / 10.0) - 1.0), 290.0)
    ratio = ntia_t / itu_t if itu_t > 0 else 0
    print(f"{freq:8.1f} {env:>22} {fam:8.1f} {ntia_t:10.0f} {itu_t:10.0f} {ratio:8.2f}")
print("  Finding: modern residential 16x quieter than 1970s ITU model at VHF")
print("  Finding: all environments converge to thermal floor by 761 MHz")
print("  Finding: Denver business 3.3x ITU at 402 MHz (large city effect)")

# --- VK3UM 220 MHz contradiction ---
gal_avg_220 = galactic_sky(np.array([220.0]))[0]
print("\n=== VK3UM 220 MHz 'minimum quiet sky' analysis ===")
print(f"  VK3UM claims: 150 K at 220 MHz (minimum quiet sky achievable)")
print(f"  Galactic formula average: {gal_avg_220:.0f} K at 220 MHz (eq.14)")
print(f"  ITU-R P.372: minimum = average - 3 dB = {gal_avg_220/2:.0f} K")
print(f"  G4CQM at 222 MHz: 70 K (spectral index interpolation, Dubus 2/2019)")
print(f"  VK3UM 150 K exceeds galactic average ({gal_avg_220:.0f} K) by {150/gal_avg_220:.1f}x")
print(f"  G4CQM 70 K is consistent with 3 dB rule ({gal_avg_220/2:.0f} K)")
print(f"  Conclusion: VK3UM 220 MHz is mislabeled; it is an average, not minimum")

# --- Haslam 408 MHz scaling ---
print("\n=== Haslam 408 MHz galactic pole minimum, scaled ===")
print(f"  Direct measurement: {haslam_408_min_total} K total, {haslam_408_synch_min} K synchrotron")
print(f"  Zero-level uncertainty: +/- {haslam_408_zero_err} K")
print(f"  Formula: T_min(f) = {haslam_408_synch_min} * (408/f)^beta + {haslam_408_cmb}")
print(f"{'MHz':>8} {'beta=2.40':>10} {'beta=2.55':>10} {'beta=2.75':>10} {'G4CQM':>8} {'VK3UM':>8}")
for f in [50, 144, 220, 222, 432, 900, 1296]:
    vals = []
    for beta in [2.40, 2.55, 2.75]:
        t = haslam_408_synch_min * (408.0 / f) ** beta + haslam_408_cmb
        vals.append(f"{t:10.1f}")
    g = "—"
    v = "—"
    if f in list(g4cqm_mhz):
        g = f"{g4cqm_sky[list(g4cqm_mhz).index(f)]:.0f}"
    if f in list(vk3um_mhz):
        v = f"{vk3um_sky[list(vk3um_mhz).index(f)]:.0f}"
    print(f"{f:8} {vals[0]} {vals[1]} {vals[2]} {g:>8} {v:>8}")
print("  Practical antennas see higher values due to sidelobe integration")
print("  G4CQM/VK3UM values represent practical minimum, not physics floor")

# --- W8IO vs G4CQM ---
print("\n=== W8IO/YagiCAD vs G4CQM defaults ===")
print(f"{'MHz':>8} {'W8IO sky':>10} {'G4CQM sky':>11} {'W8IO earth':>12} {'G4CQM earth':>13}")
for i, f in enumerate(w8io_mhz):
    g_sky = g_earth = "—"
    if f in list(g4cqm_mhz):
        idx = list(g4cqm_mhz).index(f)
        g_sky = f"{g4cqm_sky[idx]:.0f}"
        g_earth = f"{g4cqm_earth[idx]:.0f}"
    print(f"{f:8.0f} {w8io_sky[i]:10.0f} {g_sky:>11} {w8io_earth[i]:12.0f} {g_earth:>13}")
print("  W8IO sky 144=200 K matches no other source (VE7BQH=290, G4CQM=250, YU1AW=400)")
print("  W8IO earth identical to G4CQM at 222/432 MHz")

# --- Atmospheric emission ---
print("\n=== Atmospheric emission (O2/H2O) — sea level, clear, 45 deg ===")
print("  Becomes dominant over galactic emission above ~500 MHz")
for f, t in sorted(atm_emission.items()):
    gal = galactic_sky(np.array([float(f)]))[0]
    print(f"  {f:>6} MHz: atm ~{t:.0f} K, galactic {gal:.1f} K, CMB 2.7 K, total ~{t+gal+2.7:.0f} K")

# --- LOFAR discrete source contribution ---
print(f"\n=== LOFAR LoTSS discrete sources at 144 MHz ===")
print(f"  Contribution: {lofar_144_discrete} K (Schwarz et al. 2021)")
print(f"  Additive to diffuse galactic emission (not in formula)")
print(f"  Galactic formula (avg): {galactic_sky(np.array([144.0]))[0]:.0f} K")
print(f"  G4CQM min quiet sky: {g4cqm_sky[1]:.0f} K")
print(f"  If discrete sources added to G4CQM: {g4cqm_sky[1]+lofar_144_discrete:.0f} K")


# --- Synthesized best values ---
print("\n" + "=" * 78)
print("SYNTHESIZED BEST VALUES — all known frequencies")
print("=" * 78)

print("\n--- Sky: Galactic Average ---")
print(f"{'MHz':>8} {'Best (K)':>10} {'Source':>40}")
sky_avg_best = [
    (50,    5640,  "VE7BQH/DG7YBN (matches P.372 eq.13 -1.5%)"),
    (112.5,  830,  "P.372 eq.14 power law (no anchor)"),
    (137.5,  480,  "P.372 eq.14 power law (no anchor)"),
    (144,    400,  "YU1AW statistical mean"),
    (220,    150,  "VK3UM (Haslam scaled avg ~170 K)"),
    (221.5,  145,  "interpolated 220-222"),
    (222,    140,  "Haslam scale + eq.14 (G4CQM 70 is min)"),
    (401,     28,  "P.372 eq.14 (near 408 Haslam)"),
    (402.5,   27,  "P.372 eq.14"),
    (408,     25,  "Haslam high-lat avg"),
    (432,     27,  "VE7BQH/DG7YBN (includes discrete srcs)"),
    (761,      7,  "eq.14 (atm+CMB dominate)"),
    (900,      9,  "physics: gal<1 + atm~3 + CMB 2.7"),
    (1296,     8,  "physics: gal<0.5 + atm~4 + CMB 2.7"),
]
for f, t, src in sky_avg_best:
    print(f"{f:8.1f} {t:10.0f}  {src}")

print("\n--- Sky: Minimum Quiet (galactic pole, high-gain antenna) ---")
print(f"{'MHz':>8} {'Best (K)':>10} {'Source':>40} {'Haslam lo':>10} {'Haslam hi':>10}")
sky_min_best = [
    (50,    2200, "G4CQM + VK3UM agree"),
    (112.5,  370, "Haslam scaled (no anchor)"),
    (137.5,  280, "Haslam scaled (no anchor)"),
    (144,    250, "G4CQM + VK3UM agree"),
    (220,     70, "G4CQM corrected (VK3UM 150 is avg)"),
    (221.5,   70, "interpolated"),
    (222,     70, "G4CQM Dubus 2019 spectral interp"),
    (401,     13, "Haslam scaled"),
    (402.5,   13, "Haslam scaled"),
    (408,   11.4, "Haslam direct measurement"),
    (432,     15, "VK3UM (Haslam pole ~10 + sidelobe)"),
    (761,      5, "Haslam scaled + atm 2 K"),
    (900,      7, "physics: synch~0 + atm~2 + CMB 2.7"),
    (1296,     5, "VK3UM (high-altitude dry site)"),
]
for f, t, src in sky_min_best:
    h_lo = haslam_408_synch_min * (408.0 / f) ** 2.40 + haslam_408_cmb
    h_hi = haslam_408_synch_min * (408.0 / f) ** 2.75 + haslam_408_cmb
    print(f"{f:8.1f} {t:10.1f}  {src:<40s} {h_lo:10.1f} {h_hi:10.1f}")

print("\n--- Earth: All environments, all frequencies ---")
print(f"{'MHz':>8} {'QR':>8} {'Rural':>8} {'Resid':>8} {'City':>8} {'Source'}")
earth_best = [
    (50,      656, 29640, 100600, 271000, "ITU QR; VE7BQH Era 3"),
    (112.5,   290,  2880,  10470,  28650, "ITU formula"),
    (137.5,   290,  1530,    374,  16020, "ITU QR/Rur; NTIA Res/Bus"),
    (144,     290,  1600,   5400,  14550, "VE7BQH Era 3"),
    (220,     290,   290,   1340,   3490, "ITU formula (Rural at floor)"),
    (221.5,   290,   290,   1310,   3400, "ITU formula"),
    (222,     290,   290,   1290,   3360, "ITU formula"),
    (401,     290,   290,    290,    566, "ITU formula"),
    (402.5,   290,   290,    202,   1811, "NTIA Res Boulder / Bus Denver"),
    (408,     290,   290,    290,    531, "ITU formula"),
    (432,     290,   760,   1800,   7900, "VE7BQH Era 3 (URSI 2010)"),
    (761,     290,   290,    221,    226, "NTIA all sites near floor"),
    (900,     290,   290,    290,    290, "thermal floor; not measured"),
    (1296,    290,   290,    290,    290, "thermal floor; not measured"),
]
for f, qr, rur, res, city, src in earth_best:
    print(f"{f:8.1f} {qr:8.0f} {rur:8.0f} {res:8.0f} {city:8.0f}  {src}")


# --- Compute continuous spectral index curves for plotting ---
# Local slope = d(log T)/d(log f) computed as finite difference on dense grids

def local_spectral_index(freqs, temps):
    """Compute running spectral index from dense frequency/temperature arrays."""
    lf = np.log10(freqs)
    lt = np.log10(np.maximum(temps, 1e-30))
    alpha = np.gradient(lt, lf)
    return alpha

# Continuous spectral index for sky models
sky_alpha_g4cqm = local_spectral_index(g4cqm_freqs,
    log_interp(g4cqm_freqs, g4cqm_mhz, g4cqm_sky))
sky_alpha_galactic = local_spectral_index(galactic_freqs,
    galactic_sky(galactic_freqs))

# Continuous spectral index for earth models
earth_alpha_g4cqm = local_spectral_index(g4cqm_freqs,
    log_interp(g4cqm_freqs, g4cqm_mhz, g4cqm_earth))

# ITU earth slopes (use a common dense grid within ITU range)
itu_dense = np.logspace(np.log10(0.3), np.log10(250.0), 500)
earth_alpha_itu = {}
for env_name, (c, d) in itu_coeffs.items():
    earth_alpha_itu[env_name] = local_spectral_index(itu_dense,
        itu_earth(itu_dense, c, d))

# Sky ratio curves: model / galactic at each frequency
gal_at_g4cqm = galactic_sky(g4cqm_freqs)
sky_ratio_g4cqm = log_interp(g4cqm_freqs, g4cqm_mhz, g4cqm_sky) / gal_at_g4cqm


# Plot: 6 panels (3x2)
fig, ((ax1, ax2), (ax3, ax4), (ax5, ax6)) = plt.subplots(3, 2, figsize=(18, 18))

# Color schemes
ve7bqh_colors = {'Rural': '#2ca02c', 'Residential': '#ff7f0e', 'City': '#d62728'}
itu_colors = {'Business': '#d62728', 'Residential': '#ff7f0e', 'Rural': '#2ca02c', 'Quiet Rural': '#17becf'}

# --- Panel 1: T_sky — all models and history ---
ax1.set_title('Sky Temperature (T_sky) — Models and History')
ax1.set_ylabel('Temperature (K)')
ax1.set_yscale('log')
ax1.set_xscale('log')
ax1.grid(True, which='both', alpha=0.3)

# Galactic formula (continuous reference)
f, v = models_sky['Galactic (P.372)']
ax1.plot(f, v, '-', linewidth=2, color='#e377c2', label='Galactic avg (P.372)', zorder=1)

# Old VE7BQH (Era 1, pre-2018) — faded
for i, (freq, temp) in enumerate(zip(ve7bqh_old_mhz, ve7bqh_old_sky)):
    lo, hi = ham_bands[freq]
    lbl = 'Old VE7BQH (pre-2018)' if i == 0 else None
    ax1.hlines(temp, lo, hi, colors='#2ca02c', linewidths=2, alpha=0.3, linestyles='dashed', label=lbl)

# VE7BQH current (Era 2/3, sky unchanged between eras)
for i, (freq, temp) in enumerate(zip(ve7bqh_mhz, ve7bqh_sky)):
    lo, hi = ham_bands[freq]
    lbl = 'VE7BQH/DG7YBN (2019+)' if i == 0 else None
    ax1.hlines(temp, lo, hi, colors='#2ca02c', linewidths=3, alpha=0.8, label=lbl)
    ax1.plot([lo, hi], [temp, temp], 's', markersize=4, color='#2ca02c', alpha=0.8)

# G4CQM interpolation
f, v = models_sky['G4CQM']
ax1.plot(f, v, '-', linewidth=2, color='#1f77b4', label='G4CQM (pre-2016)', zorder=2)
for freq, temp in zip(g4cqm_mhz, g4cqm_sky):
    lo, hi = ham_bands[freq]
    ax1.hlines(temp, lo, hi, colors='#1f77b4', linewidths=3, alpha=0.6)

# VK3UM documented defaults (diamonds)
for i, (freq, temp) in enumerate(zip(vk3um_mhz, vk3um_sky)):
    lo, hi = ham_bands[freq]
    lbl = 'VK3UM documented min' if i == 0 else None
    ax1.plot(freq, temp, 'D', markersize=7, color='#ff7f0e', alpha=0.9, label=lbl,
             markeredgecolor='black', markeredgewidth=0.5)

# YU1AW statistical mean (star markers)
for i, (freq, temp) in enumerate(zip(yu1aw_mhz, yu1aw_sky)):
    lbl = 'YU1AW stat. mean' if i == 0 else None
    ax1.plot(freq, temp, '*', markersize=12, color='#d62728', alpha=0.9, label=lbl,
             markeredgecolor='black', markeredgewidth=0.5)

ax1.axvline(x=100, color='gray', linestyle=':', alpha=0.5, label='eq.13/14 boundary')
ax1.axhline(y=2.7, color='#888888', linestyle='--', alpha=0.4, label='CMB 2.7 K')

# Haslam 408 MHz galactic pole minimum (direct measurement)
ax1.plot(408, haslam_408_min_total, 'v', markersize=10, color='#9467bd', alpha=0.9,
         markeredgecolor='black', markeredgewidth=0.5, label='Haslam 408 pole min', zorder=5)

# Haslam-scaled minimum curve with beta uncertainty band
haslam_curve_freqs = np.logspace(np.log10(50.0), np.log10(1500.0), 300)
haslam_min_lo = haslam_408_synch_min * (408.0 / haslam_curve_freqs) ** 2.40 + haslam_408_cmb
haslam_min_hi = haslam_408_synch_min * (408.0 / haslam_curve_freqs) ** 2.75 + haslam_408_cmb
ax1.fill_between(haslam_curve_freqs, haslam_min_lo, haslam_min_hi,
                 color='#9467bd', alpha=0.15, label='Haslam pole min (β=2.4–2.75)')
ax1.plot(haslam_curve_freqs, haslam_min_hi, '-', linewidth=0.8, color='#9467bd', alpha=0.4)

# W8IO sky defaults (independent reference)
for i, (freq, temp) in enumerate(zip(w8io_mhz, w8io_sky)):
    lbl = 'W8IO/YagiCAD' if i == 0 else None
    ax1.plot(freq, temp, 'P', markersize=7, color='#8c564b', alpha=0.8,
             markeredgecolor='black', markeredgewidth=0.5, label=lbl, zorder=4)

ax1.legend(loc='upper right', fontsize=7)
ax1.set_xlim(30, 3000)

# --- Panel 2: T_earth — all environments, models and ITU formula ---
ax2.set_title('Earth/Man-made Temperature (T_earth) — All Environments')
ax2.set_ylabel('Temperature (K)')
ax2.set_yscale('log')
ax2.set_xscale('log')
ax2.grid(True, which='both', alpha=0.3)

# Old VE7BQH (single environment, faded)
for i, (freq, temp) in enumerate(zip(ve7bqh_old_mhz, ve7bqh_old_earth)):
    lo, hi = ham_bands[freq]
    lbl = 'Old VE7BQH (pre-2018)' if i == 0 else None
    ax2.hlines(temp, lo, hi, colors='gray', linewidths=2, alpha=0.4, linestyles='dashed', label=lbl)

# VE7BQH Era 2 all environments (current spreadsheet)
for env_name, earth_arr in ve7bqh_earth.items():
    color = ve7bqh_colors[env_name]
    for i, (freq, temp) in enumerate(zip(ve7bqh_mhz, earth_arr)):
        lo, hi = ham_bands[freq]
        lbl = f'VE7BQH {env_name}' if i == 0 else None
        ax2.hlines(temp, lo, hi, colors=color, linewidths=3, alpha=0.8, label=lbl)
        ax2.plot([lo, hi], [temp, temp], 's', markersize=4, color=color, alpha=0.8)

# DG7YBN Era 3 all environments (URSI 2010) — triangle markers where different
for env_name, earth_arr in dg7ybn_earth.items():
    color = ve7bqh_colors[env_name]
    labeled = False
    for i, (freq, temp) in enumerate(zip(dg7ybn_mhz, earth_arr)):
        ve7_temp = ve7bqh_earth[env_name][i]
        if abs(temp - ve7_temp) > 1.0:
            lbl = f'DG7YBN {env_name} (URSI)' if not labeled else None
            labeled = True
            ax2.plot(freq, temp, '^', markersize=8, color=color, alpha=0.9,
                     markeredgecolor='black', markeredgewidth=0.5, label=lbl)

# G4CQM
f, v = models_earth['G4CQM']
ax2.plot(f, v, '-', linewidth=2, color='#1f77b4', label='G4CQM (pre-2016)')
for freq, temp in zip(g4cqm_mhz, g4cqm_earth):
    lo, hi = ham_bands[freq]
    ax2.hlines(temp, lo, hi, colors='#1f77b4', linewidths=3, alpha=0.6)

# ITU formula curves
for env_name, (c, d) in itu_coeffs.items():
    f, v = models_earth[f'ITU {env_name}']
    ax2.plot(f, v, '--', linewidth=1.5, color=itu_colors[env_name],
             label=f'ITU {env_name}')

ax2.axhline(y=290, color='gray', linestyle=':', alpha=0.5, label='290 K floor')

# NTIA TR-02-390 measured earth temperatures
ntia_plot_data = []
for (freq, env), fam in ntia_tr02390.items():
    t = 290.0 * (10.0 ** (fam / 10.0) - 1.0)
    ntia_plot_data.append((freq, env, t))
ntia_bus = [(f, t) for f, e, t in ntia_plot_data if 'Business' in e]
ntia_res = [(f, t) for f, e, t in ntia_plot_data if 'Residential' in e]
for i, (f, t) in enumerate(ntia_bus):
    lbl = 'NTIA Business (1999)' if i == 0 else None
    ax2.plot(f, t, 'x', markersize=8, color='#d62728', alpha=0.8,
             markeredgewidth=2, label=lbl, zorder=5)
for i, (f, t) in enumerate(ntia_res):
    lbl = 'NTIA Residential (1999)' if i == 0 else None
    ax2.plot(f, t, 'x', markersize=8, color='#ff7f0e', alpha=0.8,
             markeredgewidth=2, label=lbl, zorder=5)

# Mark ITU floor activation frequencies
for env_name, (c, d) in itu_coeffs.items():
    f_floor = 10 ** ((c - fa_floor) / d)
    if f_floor <= 250:
        ax2.axvline(x=f_floor, color=itu_colors[env_name], linestyle=':',
                     alpha=0.4, linewidth=1)
        ax2.annotate(f'{env_name}\n{f_floor:.0f} MHz',
                     xy=(f_floor, 290), fontsize=6, ha='center', va='bottom',
                     color=itu_colors[env_name], alpha=0.8)

# Annotate discrepancies where DG7YBN differs from VE7BQH spreadsheet
ax2.annotate('432 Rural: 460 vs 760\n144 City: 14500 vs 14550',
             xy=(432, 500), fontsize=6, ha='left', color='#333333',
             xytext=(600, 800),
             arrowprops=dict(arrowstyle='->', color='#333333', lw=0.8))

ax2.legend(loc='upper right', fontsize=6)
ax2.set_xlim(30, 3000)

# --- Panel 3: Spectral index (local slope in log-log space) ---
ax3.set_title('Spectral Index (local slope: d log T / d log f)')
ax3.set_xlabel('Frequency (MHz)')
ax3.set_ylabel('Alpha (T ~ f^alpha)')
ax3.set_xscale('log')
ax3.grid(True, which='both', alpha=0.3)

# Sky slopes
ax3.plot(g4cqm_freqs, sky_alpha_g4cqm, '-', linewidth=2, color='#1f77b4',
         label='G4CQM sky')
ax3.plot(galactic_freqs, sky_alpha_galactic, '-', linewidth=2, color='#e377c2',
         label='Galactic sky')

# VE7BQH sky: discrete slopes between anchors
for i in range(len(ve7bqh_mhz) - 1):
    f1, f2 = ve7bqh_mhz[i], ve7bqh_mhz[i + 1]
    alpha = spectral_index(f1, f2, ve7bqh_sky[i], ve7bqh_sky[i + 1])
    ax3.hlines(alpha, f1, f2, colors='#2ca02c', linewidths=2, alpha=0.8,
               label='VE7BQH sky' if i == 0 else None)

# Old VE7BQH sky: discrete slopes
for i in range(len(ve7bqh_old_mhz) - 1):
    f1, f2 = ve7bqh_old_mhz[i], ve7bqh_old_mhz[i + 1]
    alpha = spectral_index(f1, f2, ve7bqh_old_sky[i], ve7bqh_old_sky[i + 1])
    ax3.hlines(alpha, f1, f2, colors='#2ca02c', linewidths=1.5, alpha=0.4,
               linestyles='dashed', label='Old VE7BQH sky' if i == 0 else None)

# VK3UM sky: discrete slopes between adjacent anchors
for i in range(len(vk3um_mhz) - 1):
    f1, f2 = vk3um_mhz[i], vk3um_mhz[i + 1]
    alpha = spectral_index(f1, f2, vk3um_sky[i], vk3um_sky[i + 1])
    ax3.hlines(alpha, f1, f2, colors='#ff7f0e', linewidths=2, alpha=0.7,
               label='VK3UM sky' if i == 0 else None)

# Earth slopes
ax3.plot(g4cqm_freqs, earth_alpha_g4cqm, '--', linewidth=1.5, color='#1f77b4',
         alpha=0.7, label='G4CQM earth')
ax3.plot(itu_dense, earth_alpha_itu['Rural'], '--', linewidth=1.5, color='#2ca02c',
         alpha=0.7, label='ITU Rural earth')
ax3.plot(itu_dense, earth_alpha_itu['Quiet Rural'], '--', linewidth=1.5,
         color='#17becf', alpha=0.7, label='ITU QR earth')

# Reference lines for known power-law indices
ax3.axhline(y=-2.30, color='#e377c2', linestyle=':', alpha=0.5)
ax3.annotate('eq.13: -2.30', xy=(1.0, -2.30), fontsize=7, color='#e377c2',
             va='bottom')
ax3.axhline(y=-2.75, color='#e377c2', linestyle=':', alpha=0.5)
ax3.annotate('eq.14: -2.75', xy=(1.0, -2.75), fontsize=7, color='#e377c2',
             va='bottom')

ax3.legend(loc='lower right', fontsize=7)
ax3.set_xlim(0.3, 3000)
ax3.set_ylim(-5, 1)

# --- Panel 4: Sky model / galactic ratio ---
ax4.set_title('T_sky ratio (model / galactic)')
ax4.set_xlabel('Frequency (MHz)')
ax4.set_ylabel('Ratio')
ax4.set_xscale('log')
ax4.set_yscale('log')
ax4.grid(True, which='both', alpha=0.3)

ax4.plot(g4cqm_freqs, sky_ratio_g4cqm, '-', linewidth=2, color='#1f77b4',
         label='G4CQM / galactic')

# VE7BQH discrete ratios
ve7bqh_gal = galactic_sky(ve7bqh_mhz)
for i, (freq, ratio) in enumerate(zip(ve7bqh_mhz, ve7bqh_sky / ve7bqh_gal)):
    lo, hi = ham_bands[freq]
    ax4.hlines(ratio, lo, hi, colors='#2ca02c', linewidths=3, alpha=0.8,
               label='VE7BQH / galactic' if i == 0 else None)
    ax4.plot([lo, hi], [ratio, ratio], 's', markersize=4, color='#2ca02c')

# Old VE7BQH
old_gal = galactic_sky(ve7bqh_old_mhz)
for i, (freq, ratio) in enumerate(zip(ve7bqh_old_mhz, ve7bqh_old_sky / old_gal)):
    lo, hi = ham_bands[freq]
    ax4.hlines(ratio, lo, hi, colors='#2ca02c', linewidths=1.5, alpha=0.4,
               linestyles='dashed', label='Old VE7BQH / galactic' if i == 0 else None)

# VK3UM documented
vk3um_gal = galactic_sky(vk3um_mhz)
for i, (freq, ratio) in enumerate(zip(vk3um_mhz, vk3um_sky / vk3um_gal)):
    ax4.plot(freq, ratio, 'D', markersize=6, color='#ff7f0e', alpha=0.9,
             markeredgecolor='black', markeredgewidth=0.5,
             label='VK3UM / galactic' if i == 0 else None)

# YU1AW
yu1aw_gal = galactic_sky(yu1aw_mhz)
for i, (freq, ratio) in enumerate(zip(yu1aw_mhz, yu1aw_sky / yu1aw_gal)):
    ax4.plot(freq, ratio, '*', markersize=12, color='#d62728', alpha=0.9,
             markeredgecolor='black', markeredgewidth=0.5,
             label='YU1AW / galactic' if i == 0 else None)

ax4.axhline(y=1.0, color='gray', linestyle='-', alpha=0.5, label='galactic = model')
ax4.axvline(x=100, color='gray', linestyle=':', alpha=0.3)

ax4.legend(loc='upper left', fontsize=7)
ax4.set_xlim(30, 3000)
ax4.set_ylim(0.05, 10)

# --- Panel 5: VE7BQH/DG7YBN earth evolution (all environments) ---
ax5.set_title('VE7BQH/DG7YBN T_earth Evolution — All Environments')
ax5.set_xlabel('Frequency (MHz)')
ax5.set_ylabel('Temperature (K)')
ax5.set_xscale('log')
ax5.set_yscale('log')
ax5.grid(True, which='both', alpha=0.3)

# Era 1 (single environment, faded)
ax5.plot(ve7bqh_old_mhz, ve7bqh_old_earth, '--', linewidth=2, color='gray',
         marker='s', markersize=5, alpha=0.5, label='Era 1 (pre-2018, single)')

# Era 2 and Era 3 for each environment
for env_name in ['Rural', 'Residential', 'City']:
    color = ve7bqh_colors[env_name]
    ax5.plot(ve7bqh_mhz, ve7bqh_earth[env_name], '-', linewidth=2, color=color,
             marker='s', markersize=5, alpha=0.8, label=f'Era 2 {env_name}')
    ax5.plot(dg7ybn_mhz, dg7ybn_earth[env_name], ':', linewidth=2, color=color,
             marker='^', markersize=6, alpha=0.8, label=f'Era 3 {env_name}')

# G4CQM for reference
ax5.plot(g4cqm_mhz, g4cqm_earth, '-', linewidth=1.5, color='#1f77b4',
         marker='o', markersize=4, alpha=0.7, label='G4CQM (pre-2016)')

# ITU Rural formula
ax5.plot(itu_dense, itu_earth(itu_dense, 67.2, 27.7), '--', linewidth=1.5,
         color='#9467bd', alpha=0.7, label='ITU Rural formula')

ax5.axhline(y=290, color='gray', linestyle=':', alpha=0.5, label='290 K floor')

# Annotate where Era 2 and Era 3 diverge
ax5.annotate('Rural: 460 vs 760\nCity 144: 14500 vs 14550',
             xy=(432, 600), fontsize=6, ha='center', color='#333333')

ax5.legend(loc='upper right', fontsize=6)
ax5.set_xlim(30, 3000)

# --- Panel 6: G4CQM vs VK3UM sky divergence ---
ax6.set_title('G4CQM vs VK3UM Documented Sky Defaults')
ax6.set_xlabel('Frequency (MHz)')
ax6.set_ylabel('Temperature (K)')
ax6.set_xscale('log')
ax6.set_yscale('log')
ax6.grid(True, which='both', alpha=0.3)

# G4CQM curve
f, v = models_sky['G4CQM']
ax6.plot(f, v, '-', linewidth=2, color='#1f77b4', label='G4CQM interp')
for freq, temp in zip(g4cqm_mhz, g4cqm_sky):
    ax6.plot(freq, temp, 'o', markersize=6, color='#1f77b4')

# VK3UM documented
for i, (freq, temp) in enumerate(zip(vk3um_mhz, vk3um_sky)):
    ax6.plot(freq, temp, 'D', markersize=8, color='#ff7f0e', alpha=0.9,
             markeredgecolor='black', markeredgewidth=0.5,
             label='VK3UM documented' if i == 0 else None)

# Galactic for reference
f, v = models_sky['Galactic (P.372)']
ax6.plot(f, v, '-', linewidth=1.5, color='#e377c2', alpha=0.5, label='Galactic (P.372)')

# Annotate divergences
divergences = [
    (220, 150, 222, 70, '220: 150 K\n222: 70 K'),
    (432, 15, 432, 20, '15 vs 20 K'),
    (1296, 5, 1296, 10, '5 vs 10 K'),
]
for vk_f, vk_t, g4_f, g4_t, txt in divergences:
    mid_t = (vk_t + g4_t) / 2
    ax6.annotate(txt, xy=(vk_f, mid_t), fontsize=6, ha='left',
                 xytext=(vk_f * 1.3, mid_t * 1.5), color='#333333',
                 arrowprops=dict(arrowstyle='->', color='#333333', lw=0.6))

ax6.legend(loc='upper right', fontsize=7)
ax6.set_xlim(30, 3000)

plt.tight_layout()
plt.savefig('debug/noise_models.png', dpi=150)
print("\nPlot saved to debug/noise_models.png")

# ======================================================================
# Summary: current accepted values per model family
# ======================================================================
print("\n" + "=" * 78)
print("SUMMARY: Current accepted values per model family")
print("(see SYNTHESIZED BEST VALUES above for research-integrated estimates)")
print("=" * 78)

print("\n--- VE7BQH/DG7YBN (active, maintained by DG7YBN since Oct 2025) ---")
print("  Source: DG7YBN GT pages (Issues 28/48/127, dated 2025-08-29)")
print("  Sky unchanged across all environments (galactic average, Dubus 2/2019)")
print(f"  {'MHz':>6} {'T_sky':>8} {'Rural':>10} {'Resid':>10} {'City':>10} {'Earth source'}")
for i, f in enumerate(dg7ybn_mhz):
    src = {50: "ITU-R P.372 / Dubus 2019",
           144: "ITU-R P.372-16 §6 pp.98-100",
           432: "URSI Bulletins No.334 (2010)"}[f]
    print(f"  {f:6.0f} {dg7ybn_sky[i]:8.0f} {dg7ybn_earth['Rural'][i]:10.0f}"
          f" {dg7ybn_earth['Residential'][i]:10.0f}"
          f" {dg7ybn_earth['City'][i]:10.0f} {src}")

print("\n--- G4CQM (frozen pre-2016, VK3UM software snapshot) ---")
print("  Source: g4cqm.co.uk 'VK3UM's Calc compliance'")
print("  VK3UM (Doug McArthur, SK 2016); no updates since.")
print("  Sky = minimum quiet sky (galactic pole pointing)")
print("  Earth = pre-2018 VE7BQH single-environment defaults")
print(f"  {'MHz':>6} {'T_sky':>8} {'T_earth':>10}")
for i, f in enumerate(g4cqm_mhz):
    print(f"  {f:6.0f} {g4cqm_sky[i]:8.0f} {g4cqm_earth[i]:10.0f}")

print("\n--- VK3UM documented (sky only, minimum quiet sky) ---")
print("  Source: sm2cew.com/rpc.pdf")
print("  Differs from G4CQM code at 220/432/900/1296 MHz")
print(f"  {'MHz':>6} {'T_sky':>8}")
for i, f in enumerate(vk3um_mhz):
    print(f"  {f:6.0f} {vk3um_sky[i]:8.0f}")

print("\n--- ITU-R P.372-16 (August 2022, coefficients unchanged since 1994) ---")
print("  Earth formula: T = 290 * (10^((c - d*log10(f))/10) - 1), floor 290 K")
print("  Sky: borrows G4CQM anchors (ITU defines NO sky model)")
print(f"  {'Environment':>14} {'c':>8} {'d':>8} {'Floor (MHz)':>12}")
for name, (c, d) in itu_coeffs.items():
    f_floor = 10 ** ((c - fa_floor) / d)
    print(f"  {name:>14} {c:8.1f} {d:8.1f} {f_floor:12.1f}")

print("\n--- Galactic (P.372-16 eq.13/14, unchanged) ---")
print("  Eq.13: F_am = 52 - 23*log10(f), valid <100 MHz")
print("  Eq.14: T_b(fi) = T_b(100)*(fi/100)^(-2.75) + 2.7 K")
print("  Spectral index beta=2.75 (sky average); 2.3-3.0 by direction")
print("  Source: Reich & Reich (1988, A&AS 74:7); Haslam et al (1982)")
print("  Note: above ~500 MHz, atmospheric O2/H2O emission adds ~2-5 K to")
print("  practical sky temperature; the galactic formula alone (3.7 K at")
print("  1296 MHz) underestimates practical cold sky. CMB floor = 2.7 K.")

print("\n--- YU1AW (published recommendations, statistical mean) ---")
print("  Source: qsl.net/yu1aw/Misc/vhfnoisetemp.pdf")
print("  144 MHz: T_sky=400 K (all-sky mean, not minimum)")
print("  432 MHz: T_sky=40 K")

print("\n--- Environmental definitions (URSI Bulletins No.334, 2010) ---")
print("  Rural:       Open countryside, agricultural, <1 building/ha,")
print("               no major roads or railways")
print("  Residential: Villages, pure residential; no commercial/industrial;")
print("               no railways/major roads/HV lines within 1 km")
print("  City:        Dense commercial/industrial, major roads/railways")

print("\n--- NTIA/ITS measurement anchors (independent of ITU formula) ---")
print("  TR-02-390 (2001): 137.5, 402.5, 761.0 MHz in Denver/Boulder CO")
print("  TR-11-478 (2011): 112.5, 221.5, 401.0 MHz in Boulder/Denver CO")
print("  Key findings:")
print("    137.5 MHz Business: NTIA 16017 K vs ITU 16327 K = 0.98x (validated)")
print("    137.5 MHz Residential: NTIA 374 K vs ITU 5887 K = 0.064x (16x quieter)")
print("    402.5 MHz Business Denver: NTIA 1811 K vs ITU 554 K = 3.27x (large city)")
print("    761 MHz all sites: 175-262 K, below 290 K thermal floor")
print("  Modern residential environments are 6-16x quieter than 1970s models")
print("  ITU Business model validated at VHF; varies with city size at UHF")
print("  Man-made noise negligible above ~500 MHz in residential/rural")

print("\n--- Code discrepancies vs current published values ---")
print("  VE7BQH Rural 432 MHz: code=460 K, DG7YBN=760 K (URSI 2010)")
print("  VE7BQH City 144 MHz:  code=14500 K, DG7YBN=14550 K (rounding)")
print("  G4CQM sky 432 MHz:    code=20 K, VK3UM doc=15 K, DG7YBN=27 K")
print("  G4CQM sky 1296 MHz:   code=10 K, VK3UM doc=5 K")
print("  G4CQM band 222 MHz:   VK3UM doc uses 220 MHz with T_sky=150 K")

print("\n--- VK3UM 220 MHz discrepancy ---")
print("  VK3UM 220 MHz: 150 K (labeled 'minimum quiet sky achievable')")
print("  Galactic formula avg at 220: ~138 K")
print("  ITU 3 dB rule: minimum = avg/2 = ~69 K")
print("  G4CQM at 222 MHz: 70 K (consistent with 3 dB rule)")
print("  VK3UM 150 K exceeds galactic average: mislabeled as minimum")

print("\n--- Atmospheric emission above 500 MHz ---")
print("  Sea level, clear sky, 45 deg elevation:")
print("    500 MHz: ~2 K, 900 MHz: ~3 K, 1296 MHz: ~4 K")
print("  Galactic formula alone underestimates practical sky above 500 MHz")
print("  Practical sky = galactic + atmosphere + CMB (2.7 K)")

print("\n--- Reference URLs ---")
print("  DG7YBN GT tables:  https://dg7ybn.de/GT_Tables/")
print("  DG7YBN Ant Temp:   https://dg7ybn.de/Ant_Temp/Ant_Temp.htm")
print("  G4CQM VE7BQH:     https://g4cqm.co.uk/VE7BQH%20Antenna%20Tables/index.html")
print("  VK3UM RPC doc:     https://www.sm2cew.com/rpc.pdf")
print("  VK3UM downloads:   http://www.vk5dj.com/doug.html")
print("  ITU-R P.372:       https://www.itu.int/rec/R-REC-P.372/en")
print("  YU1AW paper:       https://www.qsl.net/yu1aw/Misc/vhfnoisetemp.pdf")
print("  F5FOD AGTC-JS:     https://f5fod.github.io/AGTC-JS/AGTC-JS_V01-08.html")
print("  SM2CEW G/T:        https://www.sm2cew.com/gt.htm")
print("  W8IO antennas:     https://www.w8io.com/VHF-Ultra-antennas.html")
print("  VE7BQH spreadsheet: https://qsl.net/g6hks/VE7BQH%20Antenna%20Tables.gnumeric")
print("  NTIA TR-02-390:    https://its.ntia.gov/publications/download/tr-02-390.pdf")
print("  NTIA TR-11-478:    https://its.ntia.gov/publications/download/11-478.pdf")
print("  NTIA OT-74-38:     https://its.ntia.gov/publications/download/ot-74-38.pdf")
print("  NTIA TR-96-330:    https://its.ntia.gov/publications/download/TR-96-330_Revised.pdf")
print("  Haslam 408 MHz:    https://lambda.gsfc.nasa.gov/product/foreground/fg_2014_haslam_408_info.html")
print("  LOFAR LoTSS:       https://www.aanda.org/articles/aa/full_html/2021/04/aa38814-20/aa38814-20.html")
print("  W1GHZ preamps:     https://www.w1ghz.org/Preamps/")
print("  Kraus 1986:        https://archive.org/details/radioastronomy0000krau_ed02")

plt.show()
