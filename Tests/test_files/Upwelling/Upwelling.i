# ------------------  INPUTS TO MAIN PROGRAM  -------------------
max_step = 10

amrex.fpe_trap_invalid = 1

fabarray.mfiter_tile_size = 1024 1024 1024

# PROBLEM SIZE & GEOMETRY
geometry.prob_lo     =      0.     0.    -150.
geometry.prob_hi     =  41000. 80000.       0.

amr.n_cell           =  41     80      16

# periodic in x to match WRF setup
geometry.is_periodic = 1 1 0
#ylo.type = "SlipWall"
#yhi.type = "SlipWall"
zlo.type = "SlipWall"
zhi.type = "SlipWall"

# TIME STEP CONTROL
romsx.fixed_dt       = 300.0 # Timestep size (seconds)
# NDTFAST  = 30.0 # Number of baratropic steps => 300.0/30.0 = 10.0
romsx.fixed_fast_dt  = 10.0 # Baratropic timestep size (seconds)
# romsx.fixed_fast_dt  = 300.0 # Baratropic timestep size (seconds) testing value
romsx.fixed_ndtfast_ratio  = 30 # Baratropic timestep size (seconds)

# DIAGNOSTICS & VERBOSITY
romsx.sum_interval   = 1       # timesteps between computing mass
romsx.v              = 0       # verbosity in ROMSX.cpp (0: none, 1: print boxes, etc, 2: print values)
amr.v              = 1       # verbosity in Amr.cpp

# REFINEMENT / REGRIDDING
amr.max_level       = 0       # maximum level number allowed

# CHECKPOINT FILES
romsx.check_file      = chk        # root name of checkpoint file
romsx.check_int       = -57600      # number of timesteps between checkpoints

# PLOTFILES
romsx.plot_file_1     = plt        # prefix of plotfile name
romsx.plot_int_1      = 100          # number of timesteps between plotfiles
romsx.plot_vars_1     = omega salt temp x_velocity y_velocity z_velocity
romsx.plotfile_type   = amrex

# SOLVER CHOICE
romsx.use_gravity = true
romsx.use_coriolis = true
romsx.horizontal_advection_scheme = "upstream3" # upstream3 or centered4
romsx.use_rayleigh_damping = false
romsx.spatial_order = 2

romsx.les_type         = "None"
#
# diffusion coefficient from Straka, K = 75 m^2/s
#
romsx.molec_diff_type = "ConstantAlpha"
romsx.rho0_trans = 1.0 # [kg/m^3], used to convert input diffusivities
romsx.dynamicViscosity = 75.0 # [kg/(m-s)] ==> nu = 75.0 m^2/s
romsx.alpha_T = 75.0 # [m^2/s]

# PROBLEM PARAMETERS (optional)
prob.R0 = 1027.0
prob.S0 = 35.0
prob.T0 = 14.0

# PROBLEM PARAMETERS (shear)
prob.rho_0 = 1.0
prob.T_0   = 1.0
prob.A_0   = 1.0
prob.u_0   = 0.0
prob.v_0   = 0.0
prob.rad_0 = 0.25
prob.z0    = 0.1
prob.zRef  = 80.0e-3
prob.uRef  = 8.0e-3

prob.prob_type = 12
