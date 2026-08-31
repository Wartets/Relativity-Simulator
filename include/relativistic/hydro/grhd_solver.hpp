#pragma once

#include "relativistic/hydro/eos.hpp"
#include "relativistic/hydro/hydro_types.hpp"
#include "relativistic/hydro/con2prim.hpp"
#include "relativistic/hydro/reconstruction.hpp"
#include "relativistic/hydro/riemann_solvers.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include <vector>
#include <span>
#include <cmath>
#include <algorithm>
#include <array>

namespace Relativistic::Hydro {

enum class BoundaryCondition : uint32_t {
	Outflow = 0,
	Periodic = 1,
	Reflecting = 2
};

template <typename EOS = IdealGasEOS<double>, typename Scalar = double>
class RelativisticHydroSolver1D {
public:
	static constexpr size_t GHOST_CELLS = 3;

private:
	size_t nx_{100};
	Scalar x_min_{static_cast<Scalar>(0.0)};
	Scalar x_max_{static_cast<Scalar>(1.0)};
	Scalar dx_{static_cast<Scalar>(0.01)};
	Scalar cfl_{static_cast<Scalar>(0.4)};
	Scalar time_{static_cast<Scalar>(0.0)};

	EOS eos_{};
	Con2PrimSolver<EOS, Scalar> c2p_{eos_};
	HLLCRiemannSolver<EOS, Scalar> riemann_hllc_{eos_};
	HLLRiemannSolver<EOS, Scalar> riemann_hll_{eos_};
	HLLDRiemannSolver<EOS, Scalar> riemann_hlld_{eos_};

	RiemannSolverType solver_type_{RiemannSolverType::HLLC};
	ReconstructionMethod reconstruction_method_{ReconstructionMethod::WENO5_Z};
	BoundaryCondition bc_left_{BoundaryCondition::Outflow};
	BoundaryCondition bc_right_{BoundaryCondition::Outflow};

	std::vector<PrimitiveVariables<Scalar>> prim_{};
	std::vector<ConservedVariables<Scalar>> con_{};
	std::vector<ConservedVariables<Scalar>> u0_{};
	std::vector<ConservedVariables<Scalar>> u1_{};
	std::vector<ConservedVariables<Scalar>> u2_{};
	std::vector<FluxVariables<Scalar>> flux_faces_{};

public:
	RelativisticHydroSolver1D() = default;

	RelativisticHydroSolver1D(
		size_t nx,
		Scalar x_min,
		Scalar x_max,
		const EOS& eos = EOS{},
		ReconstructionMethod rec = ReconstructionMethod::WENO5_Z,
		RiemannSolverType riemann = RiemannSolverType::HLLC,
		Scalar cfl = static_cast<Scalar>(0.4)
	)
		: nx_(nx),
		  x_min_(x_min),
		  x_max_(x_max),
		  dx_((x_max - x_min) / static_cast<Scalar>(nx)),
		  cfl_(cfl),
		  time_(static_cast<Scalar>(0.0)),
		  eos_(eos),
		  c2p_(eos),
		  riemann_hllc_(eos),
		  riemann_hll_(eos),
		  riemann_hlld_(eos),
		  solver_type_(riemann),
		  reconstruction_method_(rec) {
		const size_t total_cells = nx_ + 2 * GHOST_CELLS;
		prim_.assign(total_cells, PrimitiveVariables<Scalar>{});
		con_.assign(total_cells, ConservedVariables<Scalar>{});
		u0_.assign(total_cells, ConservedVariables<Scalar>{});
		u1_.assign(total_cells, ConservedVariables<Scalar>{});
		u2_.assign(total_cells, ConservedVariables<Scalar>{});
		flux_faces_.assign(nx_ + 1, FluxVariables<Scalar>{});
	}

	void set_reconstruction_method(ReconstructionMethod method) noexcept {
		reconstruction_method_ = method;
	}

	void set_riemann_solver(RiemannSolverType solver) noexcept {
		solver_type_ = solver;
	}

	void set_boundary_conditions(BoundaryCondition left, BoundaryCondition right) noexcept {
		bc_left_ = left;
		bc_right_ = right;
	}

	[[nodiscard]] constexpr size_t nx() const noexcept { return nx_; }
	[[nodiscard]] constexpr Scalar dx() const noexcept { return dx_; }
	[[nodiscard]] constexpr Scalar time() const noexcept { return time_; }

	[[nodiscard]] Scalar cell_center_x(size_t i) const noexcept {
		return x_min_ + (static_cast<Scalar>(i) + static_cast<Scalar>(0.5)) * dx_;
	}

	[[nodiscard]] const PrimitiveVariables<Scalar>& primitive(size_t i) const noexcept {
		return prim_[i + GHOST_CELLS];
	}

	PrimitiveVariables<Scalar>& primitive(size_t i) noexcept {
		return prim_[i + GHOST_CELLS];
	}

	void initialize_sod_shock_tube(
		Scalar rho_l = static_cast<Scalar>(10.0),
		Scalar p_l = static_cast<Scalar>(40.0 / 3.0),
		Scalar rho_r = static_cast<Scalar>(1.0),
		Scalar p_r = static_cast<Scalar>(1e-6),
		Scalar x_diaphragm = static_cast<Scalar>(0.5)
	) noexcept {
		for (size_t i = 0; i < nx_; ++i) {
			const Scalar x = cell_center_x(i);
			if (x < x_diaphragm) {
				primitive(i) = PrimitiveVariables<Scalar>(rho_l, p_l, static_cast<Scalar>(0.0));
			} else {
				primitive(i) = PrimitiveVariables<Scalar>(rho_r, p_r, static_cast<Scalar>(0.0));
			}
			con_[i + GHOST_CELLS] = prim_to_con_flat(primitive(i));
		}
		apply_boundary_conditions();
		time_ = static_cast<Scalar>(0.0);
	}

	void initialize_balsara1_mhd(Scalar x_diaphragm = static_cast<Scalar>(0.5)) noexcept {
		for (size_t i = 0; i < nx_; ++i) {
			const Scalar x = cell_center_x(i);
			if (x < x_diaphragm) {
				primitive(i) = PrimitiveVariables<Scalar>(
					static_cast<Scalar>(1.0), static_cast<Scalar>(1.0),
					static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0),
					static_cast<Scalar>(0.5), static_cast<Scalar>(1.0), static_cast<Scalar>(0.0)
				);
			} else {
				primitive(i) = PrimitiveVariables<Scalar>(
					static_cast<Scalar>(0.125), static_cast<Scalar>(0.1),
					static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0),
					static_cast<Scalar>(0.5), static_cast<Scalar>(-1.0), static_cast<Scalar>(0.0)
				);
			}
			primitive(i).recompute_derived(eos_.gamma());
			con_[i + GHOST_CELLS] = prim_to_con_flat(primitive(i));
		}
		apply_boundary_conditions();
		time_ = static_cast<Scalar>(0.0);
	}

	void apply_boundary_conditions() noexcept {
		for (size_t g = 0; g < GHOST_CELLS; ++g) {
			if (bc_left_ == BoundaryCondition::Periodic) {
				prim_[g] = prim_[nx_ + g];
				con_[g] = con_[nx_ + g];
			} else if (bc_left_ == BoundaryCondition::Reflecting) {
				prim_[g] = prim_[2 * GHOST_CELLS - 1 - g];
				prim_[g].vx = -prim_[g].vx;
				prim_[g].recompute_derived();
				con_[g] = prim_to_con_flat(prim_[g]);
			} else {
				prim_[g] = prim_[GHOST_CELLS];
				con_[g] = con_[GHOST_CELLS];
			}

			if (bc_right_ == BoundaryCondition::Periodic) {
				prim_[nx_ + GHOST_CELLS + g] = prim_[GHOST_CELLS + g];
				con_[nx_ + GHOST_CELLS + g] = con_[GHOST_CELLS + g];
			} else if (bc_right_ == BoundaryCondition::Reflecting) {
				prim_[nx_ + GHOST_CELLS + g] = prim_[nx_ + GHOST_CELLS - 1 - g];
				prim_[nx_ + GHOST_CELLS + g].vx = -prim_[nx_ + GHOST_CELLS + g].vx;
				prim_[nx_ + GHOST_CELLS + g].recompute_derived();
				con_[nx_ + GHOST_CELLS + g] = prim_to_con_flat(prim_[nx_ + GHOST_CELLS + g]);
			} else {
				prim_[nx_ + GHOST_CELLS + g] = prim_[nx_ + GHOST_CELLS - 1];
				con_[nx_ + GHOST_CELLS + g] = con_[nx_ + GHOST_CELLS - 1];
			}
		}
	}

	[[nodiscard]] Scalar compute_time_step() const noexcept {
		Scalar max_speed = static_cast<Scalar>(0.01);
		for (size_t i = 0; i < nx_; ++i) {
			const auto& p = primitive(i);
			const Scalar cs = eos_.sound_speed(p.rho, p.p);
			const Scalar denom = static_cast<Scalar>(1.0) + std::abs(p.vx) * cs;
			const Scalar lambda = (std::abs(p.vx) + cs) / denom;
			if (lambda > max_speed) {
				max_speed = lambda;
			}
		}
		return cfl_ * (dx_ / max_speed);
	}

	void compute_fluxes() noexcept {
		for (size_t i = 0; i <= nx_; ++i) {
			const size_t cell_l = GHOST_CELLS + i - 1;
			const size_t cell_r = GHOST_CELLS + i;

			const auto prim_l = reconstruct_primitive_variable_component(
				prim_[cell_l - 2], prim_[cell_l - 1], prim_[cell_l], prim_[cell_l + 1], prim_[cell_l + 2],
				reconstruction_method_, true
			);

			const auto prim_r = reconstruct_primitive_variable_component(
				prim_[cell_r + 2], prim_[cell_r + 1], prim_[cell_r], prim_[cell_r - 1], prim_[cell_r - 2],
				reconstruction_method_, true
			);

			if (solver_type_ == RiemannSolverType::HLL) {
				flux_faces_[i] = riemann_hll_.solve_1d_x(prim_l, prim_r);
			} else if (solver_type_ == RiemannSolverType::HLLD) {
				flux_faces_[i] = riemann_hlld_.solve_1d_x(prim_l, prim_r);
			} else {
				flux_faces_[i] = riemann_hllc_.solve_1d_x(prim_l, prim_r);
			}
		}
	}

	void step_ssp_rk3(Scalar dt) noexcept {
		u0_ = con_;

		compute_fluxes();
		const Scalar inv_dx = static_cast<Scalar>(1.0) / dx_;
		for (size_t i = 0; i < nx_; ++i) {
			const size_t idx = GHOST_CELLS + i;
			const auto d_flux = (flux_faces_[i + 1] - flux_faces_[i]) * inv_dx;
			u1_[idx].d = u0_[idx].d - dt * d_flux.fd;
			u1_[idx].sx = u0_[idx].sx - dt * d_flux.fsx;
			u1_[idx].sy = u0_[idx].sy - dt * d_flux.fsy;
			u1_[idx].sz = u0_[idx].sz - dt * d_flux.fsz;
			u1_[idx].tau = u0_[idx].tau - dt * d_flux.ftau;
			u1_[idx].bx = u0_[idx].bx - dt * d_flux.fbx;
			u1_[idx].by = u0_[idx].by - dt * d_flux.fby;
			u1_[idx].bz = u0_[idx].bz - dt * d_flux.fbz;
			prim_[idx] = c2p_.solve(u1_[idx]);
		}
		con_ = u1_;
		apply_boundary_conditions();

		compute_fluxes();
		for (size_t i = 0; i < nx_; ++i) {
			const size_t idx = GHOST_CELLS + i;
			const auto d_flux = (flux_faces_[i + 1] - flux_faces_[i]) * inv_dx;
			u2_[idx].d = static_cast<Scalar>(0.75) * u0_[idx].d + static_cast<Scalar>(0.25) * (u1_[idx].d - dt * d_flux.fd);
			u2_[idx].sx = static_cast<Scalar>(0.75) * u0_[idx].sx + static_cast<Scalar>(0.25) * (u1_[idx].sx - dt * d_flux.fsx);
			u2_[idx].sy = static_cast<Scalar>(0.75) * u0_[idx].sy + static_cast<Scalar>(0.25) * (u1_[idx].sy - dt * d_flux.fsy);
			u2_[idx].sz = static_cast<Scalar>(0.75) * u0_[idx].sz + static_cast<Scalar>(0.25) * (u1_[idx].sz - dt * d_flux.fsz);
			u2_[idx].tau = static_cast<Scalar>(0.75) * u0_[idx].tau + static_cast<Scalar>(0.25) * (u1_[idx].tau - dt * d_flux.ftau);
			u2_[idx].bx = static_cast<Scalar>(0.75) * u0_[idx].bx + static_cast<Scalar>(0.25) * (u1_[idx].bx - dt * d_flux.fbx);
			u2_[idx].by = static_cast<Scalar>(0.75) * u0_[idx].by + static_cast<Scalar>(0.25) * (u1_[idx].by - dt * d_flux.fby);
			u2_[idx].bz = static_cast<Scalar>(0.75) * u0_[idx].bz + static_cast<Scalar>(0.25) * (u1_[idx].bz - dt * d_flux.fbz);
			prim_[idx] = c2p_.solve(u2_[idx]);
		}
		con_ = u2_;
		apply_boundary_conditions();

		compute_fluxes();
		for (size_t i = 0; i < nx_; ++i) {
			const size_t idx = GHOST_CELLS + i;
			const auto d_flux = (flux_faces_[i + 1] - flux_faces_[i]) * inv_dx;
			con_[idx].d = (static_cast<Scalar>(1.0) / static_cast<Scalar>(3.0)) * u0_[idx].d + (static_cast<Scalar>(2.0) / static_cast<Scalar>(3.0)) * (u2_[idx].d - dt * d_flux.fd);
			con_[idx].sx = (static_cast<Scalar>(1.0) / static_cast<Scalar>(3.0)) * u0_[idx].sx + (static_cast<Scalar>(2.0) / static_cast<Scalar>(3.0)) * (u2_[idx].sx - dt * d_flux.fsx);
			con_[idx].sy = (static_cast<Scalar>(1.0) / static_cast<Scalar>(3.0)) * u0_[idx].sy + (static_cast<Scalar>(2.0) / static_cast<Scalar>(3.0)) * (u2_[idx].sy - dt * d_flux.fsy);
			con_[idx].sz = (static_cast<Scalar>(1.0) / static_cast<Scalar>(3.0)) * u0_[idx].sz + (static_cast<Scalar>(2.0) / static_cast<Scalar>(3.0)) * (u2_[idx].sz - dt * d_flux.fsz);
			con_[idx].tau = (static_cast<Scalar>(1.0) / static_cast<Scalar>(3.0)) * u0_[idx].tau + (static_cast<Scalar>(2.0) / static_cast<Scalar>(3.0)) * (u2_[idx].tau - dt * d_flux.ftau);
			con_[idx].bx = (static_cast<Scalar>(1.0) / static_cast<Scalar>(3.0)) * u0_[idx].bx + (static_cast<Scalar>(2.0) / static_cast<Scalar>(3.0)) * (u2_[idx].bx - dt * d_flux.fbx);
			con_[idx].by = (static_cast<Scalar>(1.0) / static_cast<Scalar>(3.0)) * u0_[idx].by + (static_cast<Scalar>(2.0) / static_cast<Scalar>(3.0)) * (u2_[idx].by - dt * d_flux.fby);
			con_[idx].bz = (static_cast<Scalar>(1.0) / static_cast<Scalar>(3.0)) * u0_[idx].bz + (static_cast<Scalar>(2.0) / static_cast<Scalar>(3.0)) * (u2_[idx].bz - dt * d_flux.fbz);
			prim_[idx] = c2p_.solve(con_[idx]);
		}
		apply_boundary_conditions();

		time_ += dt;
	}

	void evolve(Scalar target_time) noexcept {
		while (time_ < target_time) {
			Scalar dt = compute_time_step();
			if (time_ + dt > target_time) {
				dt = target_time - time_;
			}
			step_ssp_rk3(dt);
		}
	}
};

}
